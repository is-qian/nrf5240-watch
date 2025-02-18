#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>

#include <inttypes.h>
#include <stdio.h>

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_2_pin), gpios, {0});

static int cmd_test_systemoff(const struct shell *shell, size_t argc, char **argv)
{
    int rc;
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printk("%s: device not ready.\n", cons->name);
		return 0;
	}

	printk("\n%s system off demo\n", CONFIG_BOARD);

	/* configure button1 as input, interrupt as level active to allow wake-up */
	rc = gpio_pin_configure_dt(&button1, GPIO_INPUT | GPIO_PULL_UP);
	if (rc < 0) {
		printk("Could not configure button1 GPIO (%d)\n", rc);
		return 0;
	}

	rc = gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_LEVEL_INACTIVE);
	if (rc < 0) {
		printk("Could not configure button1 GPIO interrupt (%d)\n", rc);
		return 0;
	}

	printk("Entering system off; press button1 to restart\n");

	rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (rc < 0) {
		printk("Could not suspend console (%d)\n", rc);
		return 0;
	}

	sys_poweroff();

	return 0;
}

SHELL_CMD_REGISTER(test_systemoff, NULL, "system off test commands", cmd_test_systemoff);