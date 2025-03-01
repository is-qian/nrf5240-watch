#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>

//pmic npm1300
#define FAST_FLASH_MS   100
#define SLOW_FLASH_MS   500
#define PRESS_SHORT_MS  1000
#define PRESS_MEDIUM_MS 5000

static volatile int flash_time_ms = SLOW_FLASH_MS;
static volatile bool vbus_connected;

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_pmic));
static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_leds));
static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));
static const struct device *ldo1 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo1));
static const struct device *ldo2 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo2));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
        static int press_t;

        if (pins & BIT(NPM1300_EVENT_SHIPHOLD_PRESS)) {
                press_t = k_uptime_get();
        }

        if (pins & BIT(NPM1300_EVENT_SHIPHOLD_RELEASE)) {
                press_t = k_uptime_get() - press_t;

                if (press_t < PRESS_SHORT_MS) {
                        printk("Short press\n");
                        if (!regulator_is_enabled(ldo1)) {
                                regulator_enable(ldo1);
                        }
                        if (!regulator_is_enabled(ldo2)) {
                                regulator_enable(ldo2);
                        }
                        flash_time_ms = FAST_FLASH_MS;
                } else if (press_t < PRESS_MEDIUM_MS) {
                        printk("Medium press\n");
                        if (regulator_is_enabled(ldo1)) {
                                regulator_disable(ldo1);
                        }
                        if (regulator_is_enabled(ldo2)) {
                                regulator_disable(ldo2);
                        }
                        flash_time_ms = SLOW_FLASH_MS;
                } else {
                        printk("Long press\n");
                        if (vbus_connected) {
                                printk("Ship mode entry not possible with USB connected\n");
                        } else {
                                regulator_parent_ship_mode(regulators);
                        }
                }
        }

        if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
                printk("Vbus connected\n");
                vbus_connected = true;
        }

        if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
                printk("Vbus removed\n");
                vbus_connected = false;
        }
}

bool configure_events(void)
{
        if (!device_is_ready(pmic)) {
                printk("Pmic device not ready.\n");
                return false;
        }

        if (!device_is_ready(regulators)) {
                printk("Regulator device not ready.\n");
                return false;
        }

        if (!device_is_ready(ldo1)) {
                printk("Load switch device not ready.\n");
                return false;
        }

        if (!device_is_ready(charger)) {
                printk("Charger device not ready.\n");
                return false;
        }

        static struct gpio_callback event_cb;

        gpio_init_callback(&event_cb, event_callback,
                           BIT(NPM1300_EVENT_SHIPHOLD_PRESS) | BIT(NPM1300_EVENT_SHIPHOLD_RELEASE) |
                                   BIT(NPM1300_EVENT_VBUS_DETECTED) |
                                   BIT(NPM1300_EVENT_VBUS_REMOVED));

        mfd_npm1300_add_callback(pmic, &event_cb);

        /* Initialise vbus detection status */
        struct sensor_value val;
        int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

        if (ret < 0) {
                return false;
        }

        vbus_connected = (val.val1 != 0) || (val.val2 != 0);

        return true;
}

int init_pmic(void)
{
        if (!device_is_ready(pmic)) {
                printk("Pmic device not ready.\n");
                return false;
        }

        if (!device_is_ready(regulators)) {
                printk("Regulator device not ready.\n");
                return false;
        }

        // if (device_is_ready(ldo1)) {
        //         if (!regulator_is_enabled(ldo1)) {
        //                 regulator_enable(ldo1);
        //         }
        // }

        // if (device_is_ready(ldo2)) {
        //         if (!regulator_is_enabled(ldo2)) {
        //                 regulator_enable(ldo2);
        //         }
        // }
        if (!configure_events()) {
                printk("Error: could not configure events\n");
                return 0;
        }
	return 0;
}

static int cmd_test_pmic(const struct shell *shell, size_t argc,
                         char **argv)
{
        ARG_UNUSED(argc);
        ARG_UNUSED(argv);

        if (!device_is_ready(leds)) {
                printk("Error: led device is not ready\n");
                return 0;
        }

        if (!configure_events()) {
                printk("Error: could not configure events\n");
                return 0;
        }

        printk("PMIC test init ok\n");
        printk("long/medium/short press key on PMIC board to test it\n");

        return 0;
}

SHELL_CMD_REGISTER(test_pmic, NULL, "pmic test commands", cmd_test_pmic);
