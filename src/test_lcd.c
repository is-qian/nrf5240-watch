#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <nrfx.h>
#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

typedef void (*fill_buffer)(enum corner corner, uint8_t grey, uint8_t *buf,
			    size_t buf_size);


#ifdef CONFIG_ARCH_POSIX
static void posix_exit_main(int exit_code)
{
#if CONFIG_TEST
	if (exit_code == 0) {
		printk("PROJECT EXECUTION SUCCESSFUL");
	} else {
		printk("PROJECT EXECUTION FAILED");
	}
#endif
	posix_exit(exit_code);
}
#endif

static int test_lcd(int date)
{
	size_t x;
	size_t y;
	size_t rect_w;
	size_t rect_h;
	size_t h_step;
	size_t scale;
	uint8_t bg_color;
	uint8_t *buf;
	const struct device *display_dev;
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size = 0;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		printk("Device %s not found. Aborting sample.",
			display_dev->name);
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	printk("Display sample for %s, display date: 0x%x\n", display_dev->name, date);
	display_get_capabilities(display_dev, &capabilities);

	if (capabilities.screen_info & SCREEN_INFO_MONO_VTILED) {
		rect_w = 16;
		rect_h = 8;
	} else {
		rect_w = 2;
		rect_h = 1;
	}

	if ((capabilities.x_resolution < 3 * rect_w) ||
	    (capabilities.y_resolution < 3 * rect_h) ||
	    (capabilities.x_resolution < 8 * rect_h)) {
		rect_w = capabilities.x_resolution * 40 / 100;
		rect_h = capabilities.y_resolution * 40 / 100;
		h_step = capabilities.y_resolution * 20 / 100;
		scale = 1;
	} else {
		h_step = rect_h;
		scale = (capabilities.x_resolution / 8) / rect_h;
	}

	rect_w *= scale;
	rect_h *= scale;


	buf_size = rect_w * rect_h;

	if (buf_size < (capabilities.x_resolution * h_step)) {
		buf_size = capabilities.x_resolution * h_step;
	}
	switch (capabilities.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		bg_color = 0xFFu;
		buf_size *= 4;
		break;
	case PIXEL_FORMAT_RGB_888:
		bg_color = 0xFFu;
		buf_size *= 3;
		break;
	case PIXEL_FORMAT_RGB_565:
		bg_color = 0xFFu;
		buf_size *= 2;
		break;
	case PIXEL_FORMAT_BGR_565:
		bg_color = 0xFFu;
		buf_size *= 2;
		break;
	case PIXEL_FORMAT_MONO01:
		bg_color = 0xFFu;
		buf_size = DIV_ROUND_UP(DIV_ROUND_UP(
			buf_size, NUM_BITS(uint8_t)), sizeof(uint8_t));
		break;
	case PIXEL_FORMAT_MONO10:
		bg_color = 0x00u;
		buf_size = DIV_ROUND_UP(DIV_ROUND_UP(
			buf_size, NUM_BITS(uint8_t)), sizeof(uint8_t));
		break;
	default:
		printk("Unsupported pixel format. Aborting sample.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	buf = k_malloc(buf_size);
	if (buf == NULL) {
		printk("Could not allocate memory. Aborting sample.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	(void)memset(buf, date, buf_size);

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;
	display_blanking_on(display_dev);
	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		/*
		 * Tweaking the height value not to draw outside of the display.
		 * It is required when using a monochrome display whose vertical
		 * resolution can not be divided by 8.
		 */
		if ((capabilities.y_resolution - idx) < h_step) {
			buf_desc.height = (capabilities.y_resolution - idx);
		}
		display_write(display_dev, 0, idx, &buf_desc, buf);
		k_msleep(10);
	}
	display_blanking_off(display_dev);
	k_free(buf);
}

static int cmd_test_lcd(const struct shell *shell, size_t argc, char **argv)
{
	int date;
	if (argc > 1) {
		date = strtoul(argv[1], NULL, 16);
	} else {
		date = 0x0;
	}
	test_lcd(date);
	return 0;
}

SHELL_CMD_REGISTER(test_lcd, NULL, "lcd test commands", cmd_test_lcd);
