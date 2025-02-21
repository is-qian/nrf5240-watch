#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

static const struct device *const i2c2_motor = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static const struct gpio_dt_spec lra_en =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lra_en_pin), gpios, {0});

static int cmd_test_motor(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t reg_data = 0x00;
	uint8_t out_data;
	uint16_t slave_addr = 0x0058;

	//assert the EN pin
        if(lra_en.port)
                gpio_pin_configure_dt(&lra_en, GPIO_OUTPUT);
	gpio_pin_set_dt(&lra_en, 1);	

        if (!device_is_ready(i2c2_motor)) {
                printk("%s: device not ready.\n", i2c2_motor->name);
                return 0;
        }

	//read device ID
	ret = i2c_write_read(i2c2_motor, slave_addr, &reg_data, 1, &out_data, 1);
	if(ret == 0) {
		printk("I2C device found at address 0x%02x\n", slave_addr);
	}
	printk("MOTOR ID:0x%02x\n", out_data);

	//exit the STANDBY mode
	ret = i2c_reg_write_byte(i2c2_motor, slave_addr, 0x01, 0x00);	
	if(ret) {
		printk("motor exit STANDBY mode failed, err:%d\n", ret);
	}

	k_msleep(10);

	//set mode to RTP(real time playback) mode
	ret = i2c_reg_update_byte(i2c2_motor, slave_addr, 0x01, 0x07, 0x05);
	if(ret) {
		printk("motor enter RTP mode failed, err:%d\n", ret);
	}

	//set LRA mode
	ret = i2c_reg_update_byte(i2c2_motor, slave_addr, 0x1a, 0x80, 0x80);

	//setup amplitude to max for motor
	ret = i2c_reg_write_byte(i2c2_motor, slave_addr, 0x02, 0x7f);	
	if(ret) {
		printk("motor setup amplitude to max failed, err:%d\n", ret);
	}

	k_msleep(2000);

	//close motor
	ret = i2c_reg_write_byte(i2c2_motor, slave_addr, 0x02, 0x00);	
	if(ret) {
		printk("motor stop failed, err:%d\n", ret);
	}

	printk("motor test finished\n");

	return 0;
}

SHELL_CMD_REGISTER(test_motor, NULL, "motor test commands", cmd_test_motor);
