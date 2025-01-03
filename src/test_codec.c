#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <nrfx.h>
#include <nrfx_dppi.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_gpiote.h>
#include <nrfx_timer.h>

//codec
#define I2S_RX_NODE  DT_NODELABEL(i2s_rxtx)
#define I2S_TX_NODE  I2S_RX_NODE
#define SAMPLE_FREQUENCY    44100
#define SAMPLE_BIT_WIDTH    16
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#define NUMBER_OF_CHANNELS  2
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      2
#define TIMEOUT             1000
#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 2)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);
const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);
static int16_t i2s_test_data[BLOCK_SIZE] = {
          0x1122,   0x3344,   0x5566,  0x7788,
};

static int init_codec(void)
{
	int ret;
	struct i2s_config config;
        if (!device_is_ready(i2s_dev_tx)) {
                printk("%s is not ready\n", i2s_dev_tx->name);
                return 0;
        }
        config.word_size = SAMPLE_BIT_WIDTH;
        config.channels = NUMBER_OF_CHANNELS;
        config.format = I2S_FMT_DATA_FORMAT_I2S;
        config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
        config.frame_clk_freq = SAMPLE_FREQUENCY;
        config.mem_slab = &mem_slab;
        config.block_size = BLOCK_SIZE;
        config.timeout = TIMEOUT;
        ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, &config);
        if (ret < 0) {
                printk("Failed to configure TX stream: %d\n", ret);
                return -1;
        }
	return 0;
}
static int test_codec(void)
{
	int ret;
	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	if(ret < 0) {
		printk("Failed to trigger i2s start: %d\n", ret);
		return -2;
	}
	ret = i2s_write(i2s_dev_tx, i2s_test_data, BLOCK_SIZE);
	if(ret < 0) {
		printk("Failed to write data: %d\n", ret);
		return -1;
	}
	k_msleep(100);
	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if(ret < 0) {
		printk("Failed to trigger i2s drop: %d\n", ret);
		return -2;
	}
	printk("Streams test OK\n");
	return 0;
}

#define GPIOTE_INTERRUPT_PRIORITY 0
#define GPIOTE_MCLK_SYNC_PIN_OUT 0
#define DIVIDER_COUNT 20
#define ERROR_STATUS_CHECK(error) if(error != NRFX_SUCCESS){ printk("error code not 0! IS = %d", error);}


#define PIN 58 

nrfx_timer_t m_timer = NRFX_TIMER_INSTANCE(0);
static int init_mclk(void)
{
	nrfx_gpiote_init(GPIOTE_INTERRUPT_PRIORITY, 0);

	nrfx_err_t err;
	nrfx_gpiote_output_config_t out_config;
	err = nrfx_gpiote_out_init(PIN, &out_config);
	if( err != NRFX_SUCCESS )
	{
		printk("err_code not 0 for nrf_drv_gpiote_in_init");
	}

	nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG(8000000);
	timer_cfg.mode = NRF_TIMER_MODE_TIMER;
	timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
	err = nrfx_timer_init(&m_timer, &timer_cfg, NULL);
	ERROR_STATUS_CHECK(err)

        nrfx_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, DIVIDER_COUNT, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);    

        uint8_t channel;
	const nrfx_gpiote_t *gpiote = 1;
        err =  nrfx_dppi_channel_alloc(&channel);
        nrfx_gpiote_pin_t gpiote_mclk_sync_pin_out = PIN;

 	uint32_t gpiote_mclk_toggle_task_addr = nrfx_gpiote_out_task_address_get(gpiote, gpiote_mclk_sync_pin_out); 
	uint32_t timer_event_address = nrfx_timer_event_address_get(&m_timer, NRF_TIMER_EVENT_COMPARE0);
	printk("%x %x\n", gpiote_mclk_sync_pin_out, timer_event_address);

	nrfx_gppi_channel_endpoints_setup(channel, timer_event_address, gpiote_mclk_toggle_task_addr );
	nrfx_gppi_channels_enable(BIT(channel));
	nrfx_gpiote_out_task_enable(gpiote, PIN);
	nrfx_timer_enable(&m_timer);

	return 0;
}

static int cmd_test_codec(const struct shell *shell, size_t argc, char **argv)
{
	init_codec();
	test_codec();
	return 0;
}

SHELL_CMD_REGISTER(test_codec, NULL, "codec test commands", cmd_test_codec);
