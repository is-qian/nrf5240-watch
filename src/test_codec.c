#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <nrfx.h>

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

static int cmd_test_codec(const struct shell *shell, size_t argc, char **argv)
{
	init_codec();
	test_codec();
	return 0;
}

SHELL_CMD_REGISTER(test_codec, NULL, "codec test commands", cmd_test_codec);
