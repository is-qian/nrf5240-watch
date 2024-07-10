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
static const struct gpio_dt_spec button_2 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_2_pin), gpios, {0});

int main(void)
{
	int cnt = 0;
	int button2 = 0;

	if(lra_en.port)
		gpio_pin_configure_dt(&lra_en, GPIO_OUTPUT);
	if(lcd_bk_en.port)
		gpio_pin_configure_dt(&lcd_bk_en, GPIO_OUTPUT);
	if(lcd_vcom.port)
		gpio_pin_configure_dt(&lcd_vcom, GPIO_OUTPUT);

	if(button_2.port)
		gpio_pin_configure_dt(&button_2, GPIO_INPUT);

	while(1) {
		printk("hello cnt:%d\n", cnt++);
		k_msleep(1000);
		if(lra_en.port) 
			cnt % 2 ? gpio_pin_set_dt(&lra_en, 1) : gpio_pin_set_dt(&lra_en, 0);
		if(lcd_bk_en.port) 
			cnt % 2 ? gpio_pin_set_dt(&lcd_bk_en, 1) : gpio_pin_set_dt(&lcd_bk_en, 0);
		if(lcd_vcom.port) 
			cnt % 2 ? gpio_pin_set_dt(&lcd_vcom, 1) : gpio_pin_set_dt(&lcd_vcom, 0);

		button2 = gpio_pin_get(button_2.port, button_2.pin);
		if(button2 < 0)
			printk("failed to read pin: %d\n", button_2.pin);
		else
			printk("pin %d value: %d\n", button_2.pin, button2);
	}
	return 0;
}
