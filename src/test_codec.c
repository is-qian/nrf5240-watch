#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <nrfx.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>

#include "test_codec.h"

static const struct device *const i2c2_codec = DEVICE_DT_GET(DT_NODELABEL(i2c1));

const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);

static int init_i2c(void)
{
	int ret;
	uint8_t reg_addr = 0x06;
	uint8_t out_data;
	uint16_t slave_addr = 0x001a;
	static const uint8_t init[][2] = {
		//word freq to 44.1khz
		{ 0x22, 0x0a },
		//codec in slave mode, 32 BCLK per WCLK
		{ 0x28, 0x00 },
		//enable DAC_L
		{ 0x69, 0x88 },
		//setup LINE_AMP_GAIN to 15db
		{ 0x4a, 0x3f },
		//enable LINE amplifier
		{ 0x6d, 0x80 },
		//enable DAC_R
		{ 0x6a, 0x80 },
		//setup MIXIN_R_GAIN to 18dB
		{ 0x35, 0x0f },
		//enable MIXIN_R
		{ 0x66, 0x80 },
		//setup DIG_ROUTING_DAI to DAI
		{ 0x21, 0x32 },
		//setup DIG_ROUTING_DAC to mono
		{ 0x2a, 0xba },
		//setup DAC_L_GAIN to 12dB
		{ 0x45, 0x7f },
		//setup DAC_R_GAIN to 12dB
		{ 0x46, 0x7f },
		//enable DAI, 16bit per channel
		{ 0x29, 0x80 },
		//setup SYSTEM_MODES_OUTPUT to use DAC_R,DAC_L and LINE
		{ 0X51, 0x00 },
		//setup Master bias enable
		{ 0X23, 0x08 },
		//Sets the input clock range for the PLL 40-80MHz
		{ 0X27, 0x00 },
		//setup MIXOUT_R_SELECT to DAC_R selected
		{ 0X4C, 0x08 },
		//setup MIXOUT_R_CTRL to MIXOUT_R mixer amp enable and MIXOUT R mixer enable
		{ 0X6F, 0x98 },
	};

	//software reset
	reg_addr = 0x1d;
	ret = i2c_reg_write_byte(i2c2_codec, slave_addr, reg_addr, 0x80);
	if(ret) {
		printk("software reset failed, err:%d\n", ret);
	}
	k_msleep(10);

	//read example
	reg_addr = 0x06;
	ret = i2c_write_read(i2c2_codec, slave_addr, &reg_addr, 1, &out_data, 1);
	if(ret == 0) {
		printk("I2C device found at address 0x%02x\n", slave_addr);
	}
	printk("codec i2c read:0x%02x\n", out_data);

	//read chip status 
	reg_addr = 0xfd;
	ret = i2c_write_read(i2c2_codec, slave_addr, &reg_addr, 1, &out_data, 1);
	if(ret) {
		printk("read codec status failed, err:%d\n", ret);
	}
	printk("chip status:0x%02x\n", out_data);


        for (int i = 0; i < ARRAY_SIZE(init); ++i) {
                const uint8_t *entry = init[i];

                ret = i2c_reg_write_byte(i2c2_codec, slave_addr,
                                         entry[0], entry[1]);
		printk("set reg 0x%02x to 0x%02x\n", entry[0], entry[1]);
                if (ret < 0) {
                        printk("Initialization step %d failed\n", i);
                        return false;
                }
        }

	return 0;
}

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

int init_audio(void)
{
	int ret;
	ret = init_i2c();
	if(ret) {
		printk("init i2c failed\n");
		return -1;
	}
	ret = init_codec();
	if(ret) {
		printk("init codec failed\n");
		return -1;
	}
	return 0;
}

static int cmd_test_codec(const struct shell *shell, size_t argc, char **argv)
{
	for(int i = 0; i < 10; i++) {
		test_codec();
	}
	return 0;
}

SHELL_CMD_REGISTER(test_codec, NULL, "codec test commands", cmd_test_codec);
