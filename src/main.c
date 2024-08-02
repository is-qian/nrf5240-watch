#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pwm.h>

//micros
#define SPI_FLASH_TEST_REGION_OFFSET 0xff000
#define SPI_FLASH_SECTOR_SIZE        4096

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
//#define UART_DEVICE_NODE DT_CHOSEN(seeed_debug_uart)
#define MSG_SIZE 32
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

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

//pwm backlight ctrl
const struct pwm_dt_spec pwm_led0 =        PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
#define MIN_PERIOD (PWM_SEC(1U) / 128U)
#define MAX_PERIOD (PWM_SEC(1U))

void test_output(void)
{
	static int output_cnt = 0;
	if(lra_en.port)
		gpio_pin_configure_dt(&lra_en, GPIO_OUTPUT);
	if(lcd_bk_en.port)
		gpio_pin_configure_dt(&lcd_bk_en, GPIO_OUTPUT);
	if(lcd_vcom.port)
		gpio_pin_configure_dt(&lcd_vcom, GPIO_OUTPUT);

	output_cnt++;
	if(lra_en.port) 
		output_cnt % 2 ? gpio_pin_set_dt(&lra_en, 1) : gpio_pin_set_dt(&lra_en, 0);
	if(lcd_bk_en.port) 
		output_cnt % 2 ? gpio_pin_set_dt(&lcd_bk_en, 1) : gpio_pin_set_dt(&lcd_bk_en, 0);
	if(lcd_vcom.port) 
		output_cnt % 2 ? gpio_pin_set_dt(&lcd_vcom, 1) : gpio_pin_set_dt(&lcd_vcom, 0);
}

void test_input(void)
{
	int button1, button2, button3, button4;

	if(button_1.port)
		gpio_pin_configure_dt(&button_1, GPIO_INPUT);
	if(button_2.port)
		gpio_pin_configure_dt(&button_2, GPIO_INPUT);
	if(button_3.port)
		gpio_pin_configure_dt(&button_3, GPIO_INPUT);
	if(button_4.port)
		gpio_pin_configure_dt(&button_4, GPIO_INPUT);

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

void test_qspi_flash(const struct device *flash_dev)
{
        const uint8_t expected[] = { 0x55, 0xaa, 0x66, 0x99 };
        const size_t len = sizeof(expected);
        uint8_t buf[sizeof(expected)];
        int rc;

        printf("\nPerform test on single sector");
        /* Write protection needs to be disabled before each write or
         * erase, since the flash component turns on write protection
         * automatically after completion of write and erase
         * operations.
         */
        printf("\nTest 1: Flash erase\n");

        /* Full flash erase if SPI_FLASH_TEST_REGION_OFFSET = 0 and
         * SPI_FLASH_SECTOR_SIZE = flash size
         */
        rc = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET,
                         SPI_FLASH_SECTOR_SIZE);
        if (rc != 0) {
                printf("Flash erase failed! %d\n", rc);
        } else {
                printf("Flash erase succeeded!\n");
        }

        printf("\nTest 2: Flash write\n");

        printf("Attempting to write %zu bytes\n", len);
        rc = flash_write(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, expected, len);
        if (rc != 0) {
                printf("Flash write failed! %d\n", rc);
                return;
        }

        memset(buf, 0, len);
        rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
        if (rc != 0) {
                printf("Flash read failed! %d\n", rc);
                return;
        }

        if (memcmp(expected, buf, len) == 0) {
                printf("Data read matches data written. Good!!\n");
        } else {
                const uint8_t *wp = expected;
                const uint8_t *rp = buf;
                const uint8_t *rpe = rp + len;

                printf("Data read does not match data written!!\n");
                while (rp < rpe) {
                        printf("%08x wrote %02x read %02x %s\n",
                               (uint32_t)(SPI_FLASH_TEST_REGION_OFFSET + (rp - buf)),
                               *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
                        ++rp;
                        ++wp;
                }
        }
}

static void serial_cb(const struct device *dev, void *user_data)
{
        uint8_t c;

        if (!uart_irq_update(uart_dev)) {
                return;
        }

        if (!uart_irq_rx_ready(uart_dev)) {
                return;
        }

        /* read until FIFO empty */
        while (uart_fifo_read(uart_dev, &c, 1) == 1) {
                if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
                        /* terminate string */
                        rx_buf[rx_buf_pos] = '\0';

                        /* if queue is full, message is silently dropped */
                        k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

                        /* reset the buffer (it was copied to the msgq) */
                        rx_buf_pos = 0;
                } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
                        rx_buf[rx_buf_pos++] = c;
                }
                /* else: characters beyond buffer size are dropped */
        }
}

static void print_uart(char *buf)
{
        int msg_len = strlen(buf);

        for (int i = 0; i < msg_len; i++) {
                uart_poll_out(uart_dev, buf[i]);
        }
}

void test_uart(void)
{
        if (!device_is_ready(uart_dev)) {
                printk("UART device not found!");
        }
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
        if (ret < 0) {
                if (ret == -ENOTSUP) {
                        printk("Interrupt-driven UART API support not enabled\n");
                } else if (ret == -ENOSYS) {
                        printk("UART device does not support interrupt-driven API\n");
                } else {
                        printk("Error setting UART callback: %d\n", ret);
                }
        }
        uart_irq_rx_enable(uart_dev);

        print_uart("Hello! I'm your echo bot.\r\n");
        print_uart("Tell me something and press enter:\r\n");
}

void test_pwm(void)
{
	uint32_t max_period = 0;
	uint32_t period;

        if (!pwm_is_ready_dt(&pwm_led0)) {
                printk("Error: PWM device %s is not ready\n",
                       pwm_led0.dev->name);
        }
	pwm_set_dt(&pwm_led0, MAX_PERIOD, MAX_PERIOD / 2U);
        printk("Calibrating for channel %d...\n", pwm_led0.channel);
        max_period = MAX_PERIOD;
        while (pwm_set_dt(&pwm_led0, max_period, max_period / 2U)) {
                max_period /= 2U;
                if (max_period < (4U * MIN_PERIOD)) {
                        printk("Error: PWM device "
                               "does not support a period at least %lu\n",
                               4U * MIN_PERIOD);
                }
        }

        printk("Done calibrating; maximum/minimum periods %u/%lu nsec\n",
               max_period, MIN_PERIOD);
	period = max_period;
	period = 15625000;


	//setup pwm output
	printk("pwm:%d %d\n", pwm_set_dt(&pwm_led0, period, period / 2U), period);
}

int main(void)
{
	int cnt = 0;

        const struct device *flash_dev = DEVICE_DT_GET(DT_ALIAS(spi_flash0));

        char tx_buf[MSG_SIZE];

        if (!device_is_ready(flash_dev)) {
                printk("%s: device not ready.\n", flash_dev->name);
        }


	test_qspi_flash(flash_dev);
	test_uart();
	test_pwm();

	while(1) {
		printk("hello cnt:%d\n", cnt++);
		k_msleep(1000);
		test_output();
		test_input();
		if(k_msgq_get(&uart_msgq, &tx_buf, K_NO_WAIT) == 0) {
	        	print_uart("Echo: ");
	        	print_uart(tx_buf);
	        	print_uart("\r\n");
		}
	}
	return 0;
}
