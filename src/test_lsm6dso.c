#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <nrfx.h>

static const struct device *const i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static const struct device *const i2c2_lsm6dso = DEVICE_DT_GET(DT_NODELABEL(lsm6dso));
static const uint8_t slave_addr = DT_PROP(DT_NODELABEL(lsm6dso), reg);
static int lsm6dso_powerdown(void)
{
    int ret;
    uint16_t reg_data[] = {0x10, 0x11};
    for (int i = 0; i < sizeof(reg_data) / sizeof(reg_data[0]); i++)
    {
        ret = i2c_reg_write_byte(i2c, slave_addr, reg_data[i], 0x00);
        if (ret)
        {
            printk("powerdown failed, err:%d\n", ret);
            return ret;
        }
    }
    return ret;
}

static int test_lsm6dso(void)
{
    int ret;
    struct sensor_value accel_data[3];
    struct sensor_value gyro_data[3];
	struct sensor_value odr_attr;
	/* set accel/gyro sampling frequency to 12.5 Hz */
	odr_attr.val1 = 12.5;
	odr_attr.val2 = 0;

    if (!device_is_ready(i2c2_lsm6dso)) {
        printk("Device not ready\n");
        return -ENODEV;
    }

    ret = sensor_attr_set(i2c2_lsm6dso, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
    if (ret) {
        printk("Failed to set accel sampling frequency\n");
        return ret;
    }

    ret = sensor_attr_set(i2c2_lsm6dso, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
    if (ret) {
        printk("Failed to set gyro sampling frequency\n");
        return ret;
    }
    
    ret = sensor_sample_fetch(i2c2_lsm6dso);
    if (ret) {
        printk("Failed to fetch sample\n");
        return ret;
    }

    ret = sensor_channel_get(i2c2_lsm6dso, SENSOR_CHAN_ACCEL_XYZ, accel_data);
    if (ret) {
        printk("Failed to get accel data\n");
        return ret;
    }

    ret = sensor_channel_get(i2c2_lsm6dso, SENSOR_CHAN_GYRO_XYZ, gyro_data);
    if (ret) {
        printk("Failed to get gyro data\n");
        return ret;
    }

    printk("accel data: %d.%06d, %d.%06d, %d.%06d\n", accel_data[0].val1, accel_data[0].val2, accel_data[1].val1, accel_data[1].val2, accel_data[2].val1, accel_data[2].val2);
    printk("gyro data: %d.%06d, %d.%06d, %d.%06d\n", gyro_data[0].val1, gyro_data[0].val2, gyro_data[1].val1, gyro_data[1].val2, gyro_data[2].val1, gyro_data[2].val2);
    return ret;
}

static int cmd_test_lsm6dso(const struct shell *shell, size_t argc, char **argv)
{
	test_lsm6dso();
	return 0;
}

SHELL_CMD_REGISTER(test_lsm6dso, NULL, "lsm6dso test commands", cmd_test_lsm6dso);
