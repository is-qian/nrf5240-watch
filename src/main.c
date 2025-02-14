#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>

static int test_init(void)
{
        int ret;

        ret = init_pmic();
        if (ret) {
                printk("pmic init failed!\n");
        }
        ret = init_audio();
        if (ret) {
                printk("audio init failed!\n");
        }
        ret = init_bmm350();
        if (ret) {
                printk("bmm350 init failed!\n");
        }
        return ret;
}

int main(void)
{
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
        if (!device_is_ready(dev)) {
		printk("UART device not found!");
                return 0;
        }
        if (test_init()) {
		printk("test init failed!");
        }
	while(1) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
	return 0;
}
