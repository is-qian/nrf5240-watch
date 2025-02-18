#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec button_1 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_1_pin), gpios, {0});
static const struct gpio_dt_spec button_2 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_2_pin), gpios, {0});
static const struct gpio_dt_spec button_3 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_3_pin), gpios, {0});
static const struct gpio_dt_spec button_4 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_4_pin), gpios, {0});

static int cmd_test_button(const struct shell *shell, size_t argc, char **argv)
{
	int button1, button2, button3, button4;
	if(button_1.port)
		gpio_pin_configure_dt(&button_1, GPIO_INPUT | GPIO_PULL_UP);
	if(button_2.port)
		gpio_pin_configure_dt(&button_2, GPIO_INPUT | GPIO_PULL_UP);
	if(button_3.port)
		gpio_pin_configure_dt(&button_3, GPIO_INPUT | GPIO_PULL_UP);
	if(button_4.port)
		gpio_pin_configure_dt(&button_4, GPIO_INPUT | GPIO_PULL_UP);
	button1 = gpio_pin_get(button_1.port, button_1.pin);
	button2 = gpio_pin_get(button_2.port, button_2.pin);
	button3 = gpio_pin_get(button_3.port, button_3.pin);
	button4 = gpio_pin_get(button_4.port, button_4.pin);
	if(button1 < 0)
		printk("failed to read pin: %d\n", button_1.pin);
	if(button2 < 0)
		printk("failed to read pin: %d\n", button_2.pin);
	if(button3 < 0)
		printk("failed to read pin: %d\n", button_3.pin);
	if(button4 < 0)
		printk("failed to read pin: %d\n", button_4.pin);
	printk("button1:%d button2:%d button3:%d button4:%d\n", button1, button2, button3, button4);
	return 0;
}

SHELL_CMD_REGISTER(test_button, NULL, "button test commands", cmd_test_button);