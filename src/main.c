#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>

const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(DT_NODELABEL(lcd_bk));
uint32_t freq = DT_PROP(DT_NODELABEL(lcd_bk), freq);
static const struct gpio_dt_spec button_2 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_2_pin), gpios, {0});
static const struct gpio_dt_spec button_3 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_3_pin), gpios, {0});
static const struct gpio_dt_spec button_4 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_4_pin), gpios, {0});
 
int main(void)
{
	int button1, button2, button3, button4;
	uint32_t period, pulse_width, duty;
	if(button_2.port)
		gpio_pin_configure_dt(&button_2, GPIO_INPUT | GPIO_PULL_UP);
	if(button_3.port)
		gpio_pin_configure_dt(&button_3, GPIO_INPUT | GPIO_PULL_UP);
	if(button_4.port)
		gpio_pin_configure_dt(&button_4, GPIO_INPUT | GPIO_PULL_UP);

	while (1)
	{
		button2 = gpio_pin_get(button_2.port, button_2.pin);
		button3 = gpio_pin_get(button_3.port, button_3.pin);
		button4 = gpio_pin_get(button_4.port, button_4.pin);
		if(button2 == 0){
			k_msleep(10);
			if(button2 == 0)
				duty += 10;
		}
		if(button3 == 0){
			k_msleep(10);
			if(button3 == 0)
				duty = 10;
		}
		if(button4 == 0){
			k_msleep(10);
			if(button4 == 0)
				duty -= 10;
		}
		if(duty > 100)
			duty = 100;
		if(duty < 0)
			duty = 0;
	
		period = NSEC_PER_SEC / freq;
		pulse_width = (period * duty) / 100;
		pwm_set_dt(&backlight, period, pulse_width);/* code */
	}
	return 0;
}
