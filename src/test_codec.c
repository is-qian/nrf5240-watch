#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <nrfx.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>

#include "test_codec.h"
#include "audio_file.h"

static const struct device *const i2c2_codec = DEVICE_DT_GET(DT_NODELABEL(i2c1));

const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);

static int init_i2c(void)
{
	int ret;
	uint8_t reg_addr = 0x06;
	uint8_t out_data;
	uint16_t slave_addr = 0x001a;
	uint32_t sysClock_Hz, sampleRate_Hz = SAMPLE_FREQUENCY;
	uint8_t indiv = 0, inputDiv = 0;
	uint64_t PllValue = 0;
	uint32_t PllFractional;
	uint8_t PllInteger;
	uint8_t PllFracTop;
	uint8_t PllFracBottom;
	uint32_t masterClock_Hz = MASTER_CLOCK_FREQ;
	static const uint8_t init[][2] = {
		// setup DIG_ROUTING_DAI to DAI
		{0x21, 0x32},
		// word freq to 44.1khz
		{0x22, 0x0a},
		// enalbe master bias
		{0x23, 0x08},
		// reset pll fb div data
		{0X24, 0x00},
		{0X25, 0x00},
		{0X26, 0x20},
		// Sets the input clock range for the PLL 2-10MHz
		{0X27, 0x80},
		// codec in master mode, 32 BCLK per WCLK
		{0x28, 0x80},
		// enable DAI, 16bit per channel
		{0x29, 0x80},
		// setup DIG_ROUTING_DAC to mono
		{0x2a, 0xba},
		// setup MIXIN_R_GAIN to 0dB
		{0x35, 0x03},
		// setup DAC_L_GAIN to 0dB
		{0x45, 0x6f},
		// setup DAC_R_GAIN to 0dB
		{0x46, 0x6f},
		// setup DIALOG7212_CP_CTRL
		{0X47, 0x3d},
		// setup LINE_AMP_GAIN to 0db
		{0x4a, 0x30},//0x3f
		// setup MIXOUT_R_SELECT to DAC_R selected
		{0X4c, 0x08},
		// setup SYSTEM_MODES_OUTPUT to use DAC_R,DAC_L and LINE
		{0X51, 0x08},
		// enable MIXIN_R
		{0x66, 0x80},
		// enable DAC_L
		{0x69, 0x88},
		// enable DAC_R
		{0x6a, 0xa0},
		// setup DIALOG7212_HP_R_CTRL
		{0X6C, 0x40},
		// enable LINE amplifier
		{0x6d, 0xa0},
		// setup DIALOG7212_MIXOUT_R_CTRL
		{0X6F, 0x98},
		// using the PLL with a 2 MHz-5 MHz MCLK
		{0Xf0, 0x8b},
		{0Xf1, 0x03},
		{0Xf0, 0x00},
	};

	// software reset
	reg_addr = 0x1d;
	ret = i2c_reg_write_byte(i2c2_codec, slave_addr, reg_addr, 0x80);
	k_msleep(10);

	// read example
	reg_addr = 0x06;
	ret = i2c_write_read(i2c2_codec, slave_addr, &reg_addr, 1, &out_data, 1);
	if (ret == 0)
	{
		printk("I2C device found at address 0x%02x\n", slave_addr);
	}
	printk("codec i2c read:0x%02x\n", out_data);

	// read chip status
	reg_addr = 0xfd;
	ret = i2c_write_read(i2c2_codec, slave_addr, &reg_addr, 1, &out_data, 1);
	if (ret)
	{
		printk("read codec status failed, err:%d\n", ret);
	}
	printk("chip status:0x%02x\n", out_data);

	for (int i = 0; i < ARRAY_SIZE(init); ++i)
	{
		const uint8_t *entry = init[i];

		ret = i2c_reg_write_byte(i2c2_codec, slave_addr,
								 entry[0], entry[1]);
		printk("set reg 0x%02x to 0x%02x\n", entry[0], entry[1]);
		if (ret < 0)
		{
			printk("Initialization step %d failed\n", i);
			return false;
		}
	}

	if ((sampleRate_Hz == 8000) || (sampleRate_Hz == 16000) || (sampleRate_Hz == 24000) || (sampleRate_Hz == 32000) ||
		(sampleRate_Hz == 48000) || (sampleRate_Hz == 96000))
	{
		sysClock_Hz = 12288000;
	}
	else
	{
		sysClock_Hz = 11289600;
	}
	/* Compute the PLL_INDIV and DIV value for sysClock */
	if ((masterClock_Hz > 2000000) && (masterClock_Hz <= 10000000))
	{
		indiv = 0;
		inputDiv = 2;
	}
	else if ((masterClock_Hz > 10000000) && (masterClock_Hz <= 20000000))
	{
		indiv = 4;
		inputDiv = 4;
	}
	else if ((masterClock_Hz > 20000000) && (masterClock_Hz <= 40000000))
	{
		indiv = 8;
		inputDiv = 8;
	}
	else
	{
		indiv = 12;
		inputDiv = 16;
	}
	/* PLL feedback divider is a Q13 value */
	PllValue = (uint64_t)(((uint64_t)((((uint64_t)sysClock_Hz * SAMPLE_BIT_WIDTH) * inputDiv) << 13)) / (masterClock_Hz));

	/* extract integer and fractional */
	PllInteger = PllValue >> 13;
	PllFractional = (PllValue - (PllInteger << 13));
	PllFracTop = (PllFractional >> 8);
	PllFracBottom = (PllFractional & 0xFF);
	// set pll
	i2c_reg_write_byte(i2c2_codec, slave_addr, 0x24, PllFracTop);
	i2c_reg_write_byte(i2c2_codec, slave_addr, 0x25, PllFracBottom);
	i2c_reg_write_byte(i2c2_codec, slave_addr, 0x26, PllInteger);
	i2c_reg_write_byte(i2c2_codec, slave_addr, 0x27, 0x80 | indiv);
	return 0;
}

static int init_codec(void)
{
	int ret;
	struct i2s_config config;
	if (!device_is_ready(i2s_dev_tx))
	{
		printk("%s is not ready\n", i2s_dev_tx->name);
		return 0;
	}
	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;
	ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, &config);
	if (ret < 0)
	{
		printk("Failed to configure TX stream: %d\n", ret);
		return -1;
	}
	return 0;
}

static int test_codec(void)
{
	int ret;
	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	// enable MCLK and set to 4MHz
	*(volatile uint32_t *)0x40025510 = 0x00000001;
	*(volatile uint32_t *)0x40025514 = 0x20000000;
	if (ret < 0)
	{
		printk("Failed to trigger i2s start: %d\n", ret);
		return -2;
	}
	ret = i2s_write(i2s_dev_tx, i2s_test_data, BLOCK_SIZE);
	if (ret < 0)
	{
		printk("Failed to write data: %d\n", ret);
		return -1;
	}
	k_msleep(100);
	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0)
	{
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
	if (ret)
	{
		printk("init i2c failed\n");
		return -1;
	}
	ret = init_codec();
	if (ret)
	{
		printk("init codec failed\n");
		return -1;
	}
	return 0;
}

static int cmd_test_codec(const struct shell *shell, size_t argc, char **argv)
{
	for (int i = 0; i < 10; i++)
	{
		test_codec();
	}
	return 0;
}

SHELL_CMD_REGISTER(test_codec, NULL, "codec test commands", cmd_test_codec);
