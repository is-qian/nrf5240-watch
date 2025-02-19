#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#if defined(CONFIG_CLOCK_CONTROL_NRF2)
#include <hal/nrf_lrcconf.h>
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static void clock_init(void)
{
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr)
	{
		printk("Unable to get the Clock manager\n");
		return;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0)
	{
		printk("Clock request failed: %d\n", err);
		return;
	}

	do
	{
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res)
		{
			printk("Clock could not be started: %d\n", res);
			return;
		}
	} while (err);

	printk("Clock has started\n");
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF2)

static void clock_init(void)
{
	int err;
	int res;
	const struct device *radio_clk_dev =
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));
	struct onoff_client radio_cli;

	/** Keep radio domain powered all the time to reduce latency. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_1, true);

	sys_notify_init_spinwait(&radio_cli.notify);

	err = nrf_clock_control_request(radio_clk_dev, NULL, &radio_cli);

	do
	{
		err = sys_notify_fetch_result(&radio_cli.notify, &res);
		if (!err && res)
		{
			printk("Clock could not be started: %d\n", res);
			return;
		}
	} while (err == -EAGAIN);

	printk("Clock has started\n");
}

#else
BUILD_ASSERT(false, "No Clock Control driver");
#endif /* defined(CONFIG_CLOCK_CONTROL_NRF) */

static int test_init(void)
{
	int ret;

	ret = init_pmic();
	if (ret)
	{
		printk("pmic init failed!\n");
	}
	ret = init_audio();
	if (ret)
	{
		printk("audio init failed!\n");
	}
	ret = init_bmm350();
	if (ret)
	{
		printk("bmm350 init failed!\n");
	}
	ret = init_bmp390();
	if (ret)
	{
		printk("bmp390 init failed!\n");
	}

	bluetooth_shell_init();
	return ret;
}

int main(void)
{
	// clock_init();
	if (test_init())
	{
		printk("test init failed!");
	}
	while (1)
	{
		k_sleep(K_MSEC(100));
	}
	return 0;
}
