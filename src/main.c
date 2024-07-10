#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>

//gpio output
static const struct gpio_dt_spec lra_en =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lra_en_pin), gpios, {0});
static const struct gpio_dt_spec lcd_bk_en =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lcd_bk_en_pin), gpios, {0});
static const struct gpio_dt_spec lcd_vcom =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lcd_vcom_pin), gpios, {0});

//gpio input
static const struct gpio_dt_spec button_1 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_1_pin), gpios, {0});
static const struct gpio_dt_spec button_2 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_2_pin), gpios, {0});
static const struct gpio_dt_spec button_3 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_3_pin), gpios, {0});
static const struct gpio_dt_spec button_4 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_4_pin), gpios, {0});

int main(void)
{
	int cnt = 0;
	int button1, button2, button3, button4;

	if(lra_en.port)
		gpio_pin_configure_dt(&lra_en, GPIO_OUTPUT);
	if(lcd_bk_en.port)
		gpio_pin_configure_dt(&lcd_bk_en, GPIO_OUTPUT);
	if(lcd_vcom.port)
		gpio_pin_configure_dt(&lcd_vcom, GPIO_OUTPUT);

	if(button_1.port)
		gpio_pin_configure_dt(&button_1, GPIO_INPUT);
	if(button_2.port)
		gpio_pin_configure_dt(&button_2, GPIO_INPUT);
	if(button_3.port)
		gpio_pin_configure_dt(&button_3, GPIO_INPUT);
	if(button_4.port)
		gpio_pin_configure_dt(&button_4, GPIO_INPUT);

	while(1) {
		printk("hello cnt:%d\n", cnt++);
		k_msleep(1000);
		if(lra_en.port) 
			cnt % 2 ? gpio_pin_set_dt(&lra_en, 1) : gpio_pin_set_dt(&lra_en, 0);
		if(lcd_bk_en.port) 
			cnt % 2 ? gpio_pin_set_dt(&lcd_bk_en, 1) : gpio_pin_set_dt(&lcd_bk_en, 0);
		if(lcd_vcom.port) 
			cnt % 2 ? gpio_pin_set_dt(&lcd_vcom, 1) : gpio_pin_set_dt(&lcd_vcom, 0);

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
	}
	return 0;
}
