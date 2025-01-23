#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <nrfx.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>

#include "test_bmm350.h"

#define bxy_sens 14.55f
#define bz_sens 9.0f
#define temp_sens 0.00204f
#define ina_xy_gain_trgt 19.46f
#define ina_z_gain_trgt 31.0f
#define adc_gain (1 / 1.5f)
#define lut_gain 0.714607238769531f
#define power (float)(1000000.0 / 1048576.0)

static const struct device *const i2c2_bmm350 = DEVICE_DT_GET(DT_NODELABEL(i2c1));

static const float sensitivity_xy
	= (power / (bxy_sens * ina_xy_gain_trgt * adc_gain * lut_gain));  // uT/LSB
static const float sensitivity_z
	= (power / (bz_sens * ina_z_gain_trgt * adc_gain * lut_gain));
static const float sensitivity_temp
	= 1 / (temp_sens * adc_gain * lut_gain * 1048576);  // C/LSB

static uint8_t last_odr = 0xff;

static int bmm3_update_odr(float time, float* actual_time) {
	int ODR;
	uint8_t AGGR;
	uint8_t AGGR_AVG;
	uint8_t PMU_CMD;

	if (time <= 0)  // suspend and forced mode both use suspend mode
	{
		PMU_CMD = PMU_CMD_SUS;  // amogus
		ODR = 0;
	} else {
		PMU_CMD = PMU_CMD_NM;
		ODR = 1 / time;
	}

	if (time <= 0) {
		AGGR = 0;
		AGGR_AVG = AGGR_NO_AVG;
		time = 0;  // off
	} else if (ODR > 200)  // TODO: this sucks
	{
		AGGR = AGGR_ODR_400Hz;
		AGGR_AVG = AGGR_NO_AVG;
		time = 1.0 / 400;
	} else if (ODR > 100) {
		AGGR = AGGR_ODR_200Hz;
		AGGR_AVG = AGGR_AVG_2;
		time = 1.0 / 200;
	} else if (ODR > 50) {
		AGGR = AGGR_ODR_100Hz;
		AGGR_AVG = AGGR_AVG_4;
		time = 1.0 / 100;
	} else if (ODR > 25) {
		AGGR = AGGR_ODR_50Hz;
		AGGR_AVG = AGGR_AVG_8;
		time = 1.0 / 50;
	} else if (ODR > 12.5) {
		AGGR = AGGR_ODR_25Hz;
		AGGR_AVG = AGGR_AVG_8;
		time = 1.0 / 25;
	} else if (ODR > 6.25) {
		AGGR = AGGR_ODR_12_5Hz;
		AGGR_AVG = AGGR_AVG_8;
		time = 1.0 / 12.5;
	} else if (ODR > 3.125) {
		AGGR = AGGR_ODR_6_25Hz;
		AGGR_AVG = AGGR_AVG_8;
		time = 1.0 / 6.25;
	} else if (ODR > 1.5625) {
		AGGR = AGGR_ODR_3_125Hz;
		AGGR_AVG = AGGR_AVG_8;
		time = 1.0 / 3.125;
	} else if (ODR > 0) {
		AGGR = AGGR_ODR_1_5625Hz;
		AGGR_AVG = AGGR_AVG_8;
		time = 1.0 / 1.5625;
	} else {
		AGGR = AGGR_ODR_200Hz;
		AGGR_AVG = AGGR_NO_AVG;
		time = 0;
	}

	uint8_t AGGR_SET = AGGR_AVG << 4 | AGGR;
	if (last_odr == AGGR_SET) {
		return 1;
	} else {
		last_odr = AGGR_SET;
	}

	int err = i2c_reg_write_byte(i2c2_bmm350, BMM350_ADDR, BMM350_PMU_CMD_AGGR_SET, AGGR_SET);
	err |= i2c_reg_write_byte(i2c2_bmm350, BMM350_ADDR, BMM350_PMU_CMD, PMU_CMD);
	if (err) {
		printk("I2C error");
	}

	*actual_time = time;
	return err;
}

static int bmm3_init(float time, float* actual_time) {
	int err = i2c_reg_write_byte(i2c2_bmm350, BMM350_ADDR, BMM350_OTP_CMD_REG, 0x80);  // PWR_OFF_OTP
	if (err) {
		printk("I2C error");
	}
	last_odr = 0xff;  // reset last odr
	err |= bmm3_update_odr(time, actual_time);
	return (err < 0 ? err : 0);
}

static void bmm3_shutdown() {
	last_odr = 0xff;  // reset last odr
	int err = i2c_reg_write_byte(i2c2_bmm350, BMM350_ADDR, BMM350_CMD, 0xB6);
	if (err) {
		printk("I2C error");
	}
}

static void bmm3_mag_oneshot() {
	int err = i2c_reg_write_byte(i2c2_bmm350, BMM350_ADDR, BMM350_PMU_CMD, PMU_CMD_FM_FAST);
	if (err) {
		printk("I2C error");
	}
}

static void bmm3_mag_process(uint8_t* raw_m, float m[3]) {
	for (int i = 0; i < 3; i++)  // x, y, z
	{
		m[i] = (int32_t)((((int32_t)raw_m[(i * 3) + 2]) << 16)
						 | (((int32_t)raw_m[(i * 3) + 1]) << 8)
						 | ((int32_t)raw_m[i * 3]));
		m[i] *= i < 2 ? sensitivity_xy : sensitivity_z;
		m[i] /= 100;  // uT to gauss
	}
}

static void bmm3_mag_read(float m[3]) {
	int err = 0;
	// uint8_t status = 1;
    uint8_t reg_addr = BMM350_PMU_CMD_STATUS_0;
	// while ((status & 0x01) == 0x01) {  // wait for forced mode to complete
	// 	err |= i2c_write_read(i2c2_bmm350, BMM350_ADDR, &reg_addr, 1, &status , 1);
	// 	printk("status: %x\n", status);
	// }
	uint8_t rawData[11];
    reg_addr = BMM350_MAG_X_XLSB;
	err |= i2c_write_read(i2c2_bmm350, BMM350_ADDR, &reg_addr, 1, &rawData[0], 11);
	if (err) {
		printk("I2C error");
	}
	bmm3_mag_process(rawData+2, m);
}

static float bmm3_temp_read() {
	int err = 0;
	uint8_t rawTemp[5];
	// uint8_t status = 1;
	uint8_t reg_addr = BMM350_PMU_CMD_STATUS_0;
	// while ((status & 0x01) == 0x01) {  // wait for forced mode to complete
	// 	err |= i2c_write_read(i2c2_bmm350, BMM350_ADDR, &reg_addr, 1, &status , 1);
	// }
	reg_addr = BMM350_TEMP_XLSB;
	err |= i2c_write_read(i2c2_bmm350, BMM350_ADDR, &reg_addr, 1, &rawTemp[0], 5);
	if (err) {
		printk("I2C error");
	}
	float temp = (int32_t)((((int32_t)rawTemp[4]) << 16) | (((int32_t)rawTemp[3]) << 8)
						   | ((int32_t)rawTemp[2]));
	temp *= sensitivity_temp;
	// taken from boschsensortec BMM350_SensorAPI, why is this needed?
	if (temp > 0) {
		temp -= 25.49f;
	} else if (temp < 0) {
		temp += 25.49f;
	}
	return temp;
}

int init_bmm350(void)
{
	int ret;
    float time;
    ret = bmm3_init(0.1, &time);
	return ret;
}

static int test_bmm350(void)
{
	float mag[3];
	float temp;
	bmm3_mag_read(mag);
	temp = bmm3_temp_read();
	printk("mag: %d, %d, %d, temp: %d.%d\n", (int)mag[0], (int)mag[1], (int)mag[2], (int)temp, (int)(temp * 100) % 100);
	return 0;
}

static int cmd_test_codec(const struct shell *shell, size_t argc, char **argv)
{
	test_bmm350();
	return 0;
}

SHELL_CMD_REGISTER(test_bmm350, NULL, "bmm350 test commands", cmd_test_codec);