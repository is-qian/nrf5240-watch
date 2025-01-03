#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/i2c.h>

static const struct device *const i2c2_motor = DEVICE_DT_GET(DT_NODELABEL(i2c2));

static int cmd_test_motor(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t reg_data, out_data;
	uint16_t slave_addr = 0x0000;
        if (!device_is_ready(i2c2_motor)) {
                printk("%s: device not ready.\n", i2c2_motor->name);
                return 0;
        }

	ret = i2c_write_read(i2c2_motor, slave_addr, &reg_data, 1, &out_data, 1);
	if(ret == 0) {
		printk("I2C device found at address 0x%02x\n", i2c2_motor);
	}
	printk("reg data:0x%02x\n", out_data);

	return 0;
}

SHELL_CMD_REGISTER(test_motor, NULL, "motor test commands", cmd_test_motor);
