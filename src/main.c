#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec lra_en =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lra_en_pin), gpios, {0});

int main(void)
{
	int cnt = 0;

	if(lra_en.port)
		gpio_pin_configure_dt(&lra_en, GPIO_OUTPUT);

	while(1) {
		printk("hello cnt:%d\n", cnt++);
		k_msleep(1000);
		if(lra_en.port) {
			cnt % 2 ? gpio_pin_set_dt(&lra_en, 1) : gpio_pin_set_dt(&lra_en, 0);
		}
	}
	return 0;
}
