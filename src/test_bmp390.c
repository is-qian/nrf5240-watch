#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <nrfx.h>
// 需要修改zephyr/drivers/sensor/bosch/bmp388/bmp388.h 91行 0x50改为0x60
const struct device *const i2c2_bmp390 = DEVICE_DT_GET(DT_NODELABEL(bmp388));
static int test_bmp390(void)
{
    int ret;
    struct sensor_value pressure_data;
    struct sensor_value temp_data;

    if (!device_is_ready(i2c2_bmp390)) {
        printk("Device not ready\n");
        return -ENODEV;
    }

    ret = sensor_sample_fetch(i2c2_bmp390);
    if (ret) {
        printk("Failed to fetch sample\n");
        return ret;
    }

    ret = sensor_channel_get(i2c2_bmp390, SENSOR_CHAN_PRESS, &pressure_data);
    if (ret) {
        printk("Failed to get pressure data\n");
        return ret;
    }

    ret = sensor_channel_get(i2c2_bmp390, SENSOR_CHAN_AMBIENT_TEMP, &temp_data);
    if (ret) {
        printk("Failed to get temp data\n");
        return ret;
    }

    printk("pressure data: %d.%06d, temp data: %d.%06d\n", pressure_data.val1, pressure_data.val2, temp_data.val1, temp_data.val2);
}

static int cmd_test_bmp390(const struct shell *shell, size_t argc, char **argv)
{
	test_bmp390();
	return 0;
}

SHELL_CMD_REGISTER(test_bmp390, NULL, "bmp390 test commands", cmd_test_bmp390);
