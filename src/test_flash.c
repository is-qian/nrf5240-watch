#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/flash.h>

#define SPI_FLASH_TEST_REGION_OFFSET 0xff000
#define SPI_FLASH_SECTOR_SIZE        4096

const struct device *flash_dev = DEVICE_DT_GET_ONE(nordic_qspi_nor);

static int cmd_test_flash(const struct shell *shell, size_t argc, char **argv)
{

    const uint8_t expected[] = {0x55, 0xaa, 0x66, 0x99};
    const size_t len = sizeof(expected);
    uint8_t buf[sizeof(expected)];
    int rc;

	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready.\n", flash_dev->name);
		return 0;
	}

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
    if (rc != 0)
    {
        printf("Flash erase failed! %d\n", rc);
    }
    else
    {
        printf("Flash erase succeeded!\n");
    }
    printf("\nTest 2: Flash write\n");
    printf("Attempting to write %zu bytes\n", len);
    rc = flash_write(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, expected, len);
    if (rc != 0)
    {
        printf("Flash write failed! %d\n", rc);
        return -1;
    }
    memset(buf, 0, len);
    rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
    if (rc != 0)
    {
        printf("Flash read failed! %d\n", rc);
        return -2;
    }
    if (memcmp(expected, buf, len) == 0)
    {
        printf("Data read matches data written. Good!!\n");
    }
    else
    {
        const uint8_t *wp = expected;
        const uint8_t *rp = buf;
        const uint8_t *rpe = rp + len;
        printf("Data read does not match data written!!\n");
        while (rp < rpe)
        {
            printf("%08x wrote %02x read %02x %s\n",
                   (uint32_t)(SPI_FLASH_TEST_REGION_OFFSET + (rp - buf)),
                   *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
            ++rp;
            ++wp;
        }
    }
    return 0;
}

SHELL_CMD_REGISTER(test_flash, NULL, "flash test commands", cmd_test_flash);