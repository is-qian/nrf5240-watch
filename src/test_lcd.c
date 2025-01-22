#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <nrfx.h>

const struct device *const lcd =  DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const uint32_t display_width = DT_PROP(DT_CHOSEN(zephyr_display), width);
static const uint32_t display_height = DT_PROP(DT_CHOSEN(zephyr_display), height);
static struct display_capabilities cfg;
static uint8_t bpp;
static uint8_t disp_buffer[DT_PROP(DT_CHOSEN(zephyr_display), width) *
			   DT_PROP(DT_CHOSEN(zephyr_display), height) * 4];

static inline uint8_t bytes_per_pixel(enum display_pixel_format pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 4;
	case PIXEL_FORMAT_RGB_888:
		return 3;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_BGR_565:
		return 2;
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
	default:
		return 1;
	}

	return 0;
}

static int test_lcd(void)
{
    int ret;
    display_get_capabilities(lcd, &cfg);
    bpp = bytes_per_pixel(cfg.current_pixel_format);
    printk("bpp: %d\n", bpp);

	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width * bpp,
	};

	memset(disp_buffer, 0, sizeof(disp_buffer));
    display_blanking_off(lcd);
    for (int i = 0; i < display_height; i++) {
        for (int j = 0; j < display_width; j++) {
            uint8_t *pixel = &disp_buffer[(i * display_width + j) * bpp];
            pixel[0] = 0xf0;
            if (bpp > 1) {
                pixel[1] = 0xff;
                pixel[2] = 0xff;
            }
            if (bpp == 4) {
                pixel[3] = 0xff;
            }
        }
    }
	display_write(lcd, 0, 0, &desc, disp_buffer);
    display_blanking_on(lcd);
    return ret;
}

static int cmd_test_lcd(const struct shell *shell, size_t argc, char **argv)
{
	test_lcd();
	return 0;
}

SHELL_CMD_REGISTER(test_lcd, NULL, "lcd test commands", cmd_test_lcd);
