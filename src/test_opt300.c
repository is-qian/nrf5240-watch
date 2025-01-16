#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <nrfx.h>

const struct device *const i2c2_opt300 = DEVICE_DT_GET(DT_NODELABEL(light));
static int test_opt300(void)
{
    int ret;
    struct sensor_value light_data;

    if (!device_is_ready(i2c2_opt300)) {
        printk("Device not ready\n");
        return -ENODEV;
    }

    ret = sensor_sample_fetch(i2c2_opt300);
    if (ret) {
        printk("Failed to fetch sample\n");
        return ret;
    }

    ret = sensor_channel_get(i2c2_opt300, SENSOR_CHAN_LIGHT, &light_data);
    if (ret) {
        printk("Failed to get channel data\n");
        return ret;
    }

    printk("light data: %d.%06d\n", light_data.val1, light_data.val2);
    return ret;
}

static int cmd_test_opt300(const struct shell *shell, size_t argc, char **argv)
{
	test_opt300();
	return 0;
}

SHELL_CMD_REGISTER(test_opt300, NULL, "opt300 test commands", cmd_test_opt300);
