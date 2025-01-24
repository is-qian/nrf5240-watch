#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/shell/shell.h>

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
static int output_cnt = 0;
static const struct gpio_dt_spec lcd_bk_en =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lcd_bk_en_pin), gpios, {0});
static const struct gpio_dt_spec lcd_vcom =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lcd_vcom_pin), gpios, {0});
static const struct gpio_dt_spec i2s_mclk =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(i2s_mclk_pin), gpios, {0});
static const struct gpio_dt_spec lsm6d_int1 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lsm6d_int1_pin), gpios, {0});
static const struct gpio_dt_spec lsm6d_int2 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(lsm6d_int2_pin), gpios, {0});
static const struct gpio_dt_spec press_int =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(press_int_pin), gpios, {0});

//gpio input
static const struct gpio_dt_spec button_1 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_1_pin), gpios, {0});
static const struct gpio_dt_spec button_2 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_2_pin), gpios, {0});
static const struct gpio_dt_spec button_3 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_3_pin), gpios, {0});
static const struct gpio_dt_spec button_4 =
   GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_4_pin), gpios, {0});

//adc
const struct device *adc_dev;

#include <hal/nrf_saadc.h>
#define ADC_DEVICE_NAME DT_ADC_0_NAME
#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID 0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN0

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

#define BUFFER_SIZE 1 
static int16_t m_sample_buffer[BUFFER_SIZE];

//lcd spi
#define SPI4_NODE      DT_NODELABEL(spi2)
const struct device *const spi_dev = DEVICE_DT_GET(SPI4_NODE);
struct spi_cs_control cs_ctrl = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET(SPI4_NODE, cs_gpios),
        .delay = 0u,
};

//sensor i2c
#define I2C2_DEV_NAME DT_NODELABEL(i2c1)
const struct device *const i2c2_dev = DEVICE_DT_GET(I2C2_DEV_NAME);

static int test_output_pin(const struct gpio_dt_spec *test_pin)
{
	int ret = 0;
	if(test_pin->port)
		gpio_pin_configure_dt(test_pin, GPIO_OUTPUT);

	if(test_pin->port) 
		output_cnt % 2 ? gpio_pin_set_dt(test_pin, 1) : gpio_pin_set_dt(test_pin, 0);

	return ret;
}

void test_output(void)
{
	output_cnt++;
	test_output_pin(&lcd_bk_en);
	test_output_pin(&lcd_vcom);
	test_output_pin(&i2s_mclk);
	test_output_pin(&lsm6d_int1);
	test_output_pin(&lsm6d_int2);
	test_output_pin(&press_int);
}

void test_input(void)
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

void init_adc(void)
{
	int err;
	printk("nRF53 SAADC sampling AIN0 (P0.04)\n");

	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (!adc_dev) {
		printk("device_get_binding ADC_0 failed\n");
	}
	err = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	if (err) {
		printk("Error in adc setup: %d\n", err);
	}

	NRF_SAADC->TASKS_CALIBRATEOFFSET = 1; 
}

static int test_adc(void)
{
	int ret;

	const struct adc_sequence sequence = {
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	if (!adc_dev) {
		return -1;
	}

	ret = adc_read(adc_dev, &sequence);
	if(ret)
		printk("ADC read err: %d\n", ret);

	printf("P0.04 adc: ");
	for (int i = 0; i < BUFFER_SIZE; i++) {
		printk("%d ", m_sample_buffer[i]);
	}
	printk("\n");

	return ret;
}

/*
 * writes 5 9bit words, you can check the output with a logic analyzer
 */
void test_basic_write_9bit_words(const struct device *dev,
                                 struct spi_cs_control *cs)
{
        struct spi_config config;

        config.frequency = 125000;
        config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(9);
        config.slave = 0;
        config.cs = *cs;

        uint16_t buff[5] = { 0x0101, 0x00ff, 0x00a5, 0x0000, 0x0102};
        int len = 5 * sizeof(buff[0]);

        struct spi_buf tx_buf = { .buf = buff, .len = len };
        struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

        int ret = spi_write(dev, &config, &tx_bufs);

        printf("basic_write_9bit_words; ret: %d\n", ret);
        printf(" wrote %04x %04x %04x %04x %04x\n",
                buff[0], buff[1], buff[2], buff[3], buff[4]);
}

/*
 * A more complicated xfer, sends two words, then sends and receives another
 * 3 words. Connect MOSI to MISO to test read
 */
void test_9bit_loopback_partial(const struct device *dev,
                                struct spi_cs_control *cs)
{
        struct spi_config config;

        config.frequency = 125000;
        config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(9);
        config.slave = 0;
        config.cs = *cs;

        enum { datacount = 5 };
        uint16_t buff[datacount] = { 0x0101, 0x0102, 0x0003, 0x0004, 0x0105};
        uint16_t rxdata[3];

        const int stride = sizeof(buff[0]);

        struct spi_buf tx_buf[2] = {
                {.buf = buff, .len = (2) * stride},
                {.buf = buff + (2), .len = (datacount - 2)*stride},
        };
        struct spi_buf rx_buf[2] = {
                {.buf = 0, .len = (2) * stride},
                {.buf = rxdata, .len = (datacount - 2) * stride},
        };

        struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 2 };
        struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 2 };

        int ret = spi_transceive(dev, &config, &tx_set, &rx_set);

        printf("9bit_loopback_partial; ret: %d\n", ret);
        printf(" tx (i)  : %04x %04x\n", buff[0], buff[1]);
        printf(" tx (ii) : %04x %04x %04x\n", buff[2], buff[3], buff[4]);
        printf(" rx (ii) : %04x %04x %04x\n", rxdata[0], rxdata[1], rxdata[2]);
}

static int init_lcd_spi(void)
{

        if (!device_is_ready(spi_dev)) {
                printk("%s: device not ready.\n", spi_dev->name);
                return 0;
        }

	return 0;
}

static int init_sensor_i2c(void)
{
        if (!device_is_ready(i2c2_dev)) {
                printk("%s: device not ready.\n", i2c2_dev->name);
                return 0;
        }

	return 0;
}

static int test_sensor_i2c(void)
{
	uint8_t i2c_addr;
	uint8_t data = 0;
	int ret;

	for (i2c_addr = 0x03; i2c_addr < 0x78; i2c_addr++) {
		ret = i2c_read(i2c2_dev, &data, 1, i2c_addr);
		if (ret == 0) {
			printk("I2C device found at address 0x%02x\n", i2c_addr);
		}
	}

	return 0;
}

static int test_init(void)
{
        int ret;

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
