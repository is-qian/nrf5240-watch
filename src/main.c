#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	int cnt = 0;
	while(1) {
		printk("hello cnt:%d\n", cnt++);
		k_msleep(1000);
	}
	return 0;
}
