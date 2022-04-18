/*
 *    COPYRIGHT NOTICE
 *   Copyright 2019 Horizon Robotics, Inc.
 *    All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <mmc.h>
#include <spi.h>
#include <spi_flash.h>
#include <veeprom.h>
#include <mtd.h>
#include <ubi_uboot.h>

#include <hb_info.h>

#define SECTOR_SIZE (512)
#ifdef CONFIG_HB_BOOT_FROM_NAND
#define BUFFER_SIZE 2048
#else
#define BUFFER_SIZE SECTOR_SIZE
#endif
#define FLAG_RW 1
#define FLAG_RO 0

static unsigned int start_sector;
static unsigned int end_sector;
static char buffer[BUFFER_SIZE];
static int curr_device = -1;

#ifdef CONFIG_CMD_SF
extern struct spi_flash *flash;
#endif
struct mmc *emmc = NULL;

#ifdef CONFIG_HB_BOOT_FROM_NOR
/* init nor flash device  */
static void init_nor_device(void)
{
	char *s;

	if (!flash) {
		s = "sf probe";
		run_command_list(s, -1, 0);
		return;
	}
}
#endif

struct mmc *init_mmc_device(int dev, bool force_init)
{
	struct mmc *mmc;
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return NULL;
	}

	if (force_init)
		mmc->has_init = 0;
	if (mmc_init(mmc))
		return NULL;
	return mmc;
}

static int dw_init(int flag)
{
	int ret = 0;

#if defined CONFIG_HB_BOOT_FROM_NOR
		init_nor_device();
		if (!flash) {
			printf("Failed to initialize SPI flash\n");
			ret = -1;
		}
#elif defined CONFIG_HB_BOOT_FROM_NAND
		start_sector = NOR_VEEPROM_START_SECTOR;
		end_sector = NOR_VEEPROM_END_SECTOR;
#elif defined CONFIG_HB_BOOT_FROM_MMC
		/* check the status of device */
		emmc = init_mmc_device(curr_device, false);
		if (!emmc) {
			printf("Error: no mmc device at slot %d\n", curr_device);
			ret = -1;
		}

		if (flag == FLAG_RW) {
			if (mmc_getwp(emmc) == 1) {
				printf("Error: card is write protected!\n");
				ret = -1;
			}
		}
#endif

	return ret;
}

static int dw_read(unsigned int cur_sector)
{
	int ret = 0;

#if defined CONFIG_HB_BOOT_FROM_NOR
		if (!flash)
			return -1;

		ret = spi_flash_read(flash, cur_sector * 512, SECTOR_SIZE, buffer);
		if (ret != 0) {
			printf("Error: read nor flash fail\n");
			return -1;
		}
#elif defined CONFIG_HB_BOOT_FROM_NAND
		return 0;
#elif defined CONFIG_HB_BOOT_FROM_MMC
		ret = blk_dread(mmc_get_blk_desc(emmc), cur_sector, 1, buffer);
		if (ret != 1) {
			printf("Error: read sector %d fail\n", cur_sector);
			return -1;
		}
#endif

	return ret;
}

static int dw_write(unsigned int cur_sector)
{
	int ret = 0;

#if defined CONFIG_HB_BOOT_FROM_NOR
		if (!flash)
			return -1;

		ret = spi_flash_erase(flash, start_sector, 64 * 1024);
		if (ret != 0) {
			printf("Error: erase nor flash fail\n");
			return -1;
		}

		ret = spi_flash_write(flash, cur_sector * 512, SECTOR_SIZE, buffer);
		if (ret != 0) {
			printf("Error: read nor flash fail\n");
			return -1;
		}
#elif defined CONFIG_HB_BOOT_FROM_NAND
		return ret;
#elif defined CONFIG_HB_BOOT_FROM_MMC
		ret = blk_dwrite(mmc_get_blk_desc(emmc), cur_sector, 1, buffer);
		if (ret != 1) {
			printf("Error: write sector %d fail\n", cur_sector);
			return -1;
		}
#endif

	return ret;
}

/* check veeprom read/write offset and length */
static int is_parameter_valid(int offset, int size)
{
	int offset_left = 0;
	int offset_right = (end_sector - start_sector + 1) * SECTOR_SIZE;

	if (offset < offset_left || offset > offset_right - 1 || offset + size > offset_right)
		return 0;

	return 1;
}

/* init veeprom mmc blocks */
int veeprom_init(void)
{
#if defined CONFIG_HB_BOOT_FROM_NOR
		start_sector = NOR_VEEPROM_START_SECTOR;
		end_sector = NOR_VEEPROM_END_SECTOR;
		init_nor_device();
		if (!flash) {
			printf("Failed to initialize SPI flash\n");
			return -1;
		}
		mtd_probe_devices();
#elif defined CONFIG_HB_BOOT_FROM_NAND
		start_sector = NOR_VEEPROM_START_SECTOR;
		end_sector = NOR_VEEPROM_END_SECTOR;
		mtd_probe_devices();
		if (ubi_part("boot", NULL)) {
			DEBUG_LOG("system ubi image load failed!\n");
			env_set("bootdelay", "-1");
			return 1;
		}
#elif defined CONFIG_HB_BOOT_FROM_MMC
		/* set veeprom raw sectors */
		start_sector = VEEPROM_START_SECTOR;
		end_sector = VEEPROM_END_SECTOR;

		/* set current mmc device number */
		if (curr_device < 0) {
			if (get_mmc_num() > 0) {
				curr_device = 0;
			} else {
				printf("Error: No MMC device available\n");
				return -1;
			}
		}
#endif

	return 0;
}

void veeprom_exit(void)
{

#if defined CONFIG_HB_BOOT_FROM_NOR
		spi_flash_free(flash);
#elif defined CONFIG_HB_BOOT_FROM_NAND
		return;
#elif defined CONFIG_HB_BOOT_FROM_MMC
		curr_device = -1;
#endif
}

/* format veeprom mmc blocks, memset(0) */
int veeprom_format(void)
{
	int ret = 0;
	int flag = FLAG_RW;
	unsigned int cur_sector = 0;

	ret = dw_init(flag);
	if (ret < 0) {
		printf("Failed to initialize veeporm\n");
		return ret;
	}

	/* format raw sectors */
	memset(buffer, 0, sizeof(buffer));
	for (cur_sector = start_sector; cur_sector <= end_sector; ++cur_sector) {
		ret = dw_write(cur_sector);
		if (ret < 0) {
			printf("Error: write veeporm faild\n");
			return ret;
		}
	}

	return ret;
}


int veeprom_read(int offset, char *buf, int size)
{
	int ret = 0;
	int flag = FLAG_RO;

	ret = dw_init(flag);
	if (ret < 0) {
		printf("Failed to initialize veeporm\n");
		return -1;
	}

	if (!is_parameter_valid(offset, size)) {
		printf("Error: parameters invalid\n");
		return -1;
	}
#ifdef CONFIG_HB_BOOT_FROM_NAND
	if (ubi_part(CONFIG_ENV_UBI_PART, NULL)) {
		printf("\n** Cannot find mtd partition \"%s\"\n",
			   CONFIG_ENV_UBI_PART);
		return 1;
	}

	memset(buffer, 0, sizeof(buffer));
	ret = ubi_volume_read("veeprom", buffer, sizeof(buffer));
	flush_cache((ulong)buffer, sizeof(buffer));
	memcpy(buf, buffer + offset, size);
#else
	int sector_left = 0;
	int sector_right = 0;
	int cur_sector = 0;
	int offset_inner = 0;
	int remain_inner = 0;
	sector_left = start_sector + (offset / SECTOR_SIZE);
	sector_right = start_sector + ((offset + size - 1) / SECTOR_SIZE);

	for (cur_sector = sector_left; cur_sector <= sector_right; ++cur_sector) {
		int operate_count = 0;
		memset(buffer, 0, sizeof(buffer));

		ret = dw_read(cur_sector);
		flush_cache((ulong)buffer, 512);
		if (ret < 0) {
			printf("Error: read veeporm faild\n");
			return ret;
		}

		offset_inner = offset - (cur_sector - start_sector) * SECTOR_SIZE;
		remain_inner = SECTOR_SIZE - offset_inner;
		operate_count = (remain_inner >= size ? size : remain_inner);
		size -= operate_count;
		offset += operate_count;
		memcpy(buf, buffer + offset_inner, operate_count);
		buf += operate_count;
	}
#endif /*CONFIG_HB_BOOT_FROM_NAND*/

	return ret;
}

int veeprom_write(int offset, const char *buf, int size)
{
	int ret = 0;
	int flag = FLAG_RW;

	ret = dw_init(flag);
	if (ret < 0) {
		printf("Failed to initialize veeporm\n");
		return ret;
	}

	if (!is_parameter_valid(offset, size)) {
		printf("Error: parameters invalid\n");
		return -1;
	}
#ifdef CONFIG_HB_BOOT_FROM_NAND
	printf("In nand veeprom write!\n");
	memset(buffer, 0, sizeof(buffer));
	printf("In nand veeprom write buffer set success!\n");
	ubi_volume_read("veeprom", buffer, sizeof(buffer));
	flush_cache((ulong)buffer, sizeof(buffer));
	memcpy(buffer + offset, buf, size);
	printf("In nand veeprom write buffer cpy success!\n");
	ubi_volume_write("veeprom", buffer, sizeof(buffer));
#else
	int sector_left = 0;
	int sector_right = 0;
	int cur_sector = 0;
	int offset_inner = 0;
	int remain_inner = 0;
	sector_left = start_sector + (offset / SECTOR_SIZE);
	sector_right = start_sector + ((offset + size - 1) / SECTOR_SIZE);

	for (cur_sector = sector_left; cur_sector <= sector_right; ++cur_sector) {
		int operate_count = 0;
		memset(buffer, 0, sizeof(buffer));

		ret = dw_read(cur_sector);
		flush_cache((ulong)buffer, 512);
		if (ret < 0) {
			printf("Error: read veeporm faild\n");
			return ret;
		}

		offset_inner = offset - (cur_sector - start_sector) * SECTOR_SIZE;
		remain_inner = SECTOR_SIZE - offset_inner;
		operate_count = (remain_inner >= size ? size : remain_inner);
		size -= operate_count;
		offset += operate_count;

		memcpy(buffer + offset_inner, buf, operate_count);
		buf += operate_count;

		ret = dw_write(cur_sector);
		if (ret < 0) {
			printf("Error: write veeporm faild\n");
			return ret;
		}
	}
#endif /*CONFIG_HB_BOOT_FROM_NAND*/
	return ret;
}

int veeprom_clear(int offset, int size)
{
	int ret = 0;
	int flag = FLAG_RW;

	ret = dw_init(flag);
	if (ret < 0) {
		printf("Failed to initialize veeporm\n");
		return -1;
	}

	if (!is_parameter_valid(offset, size)) {
		printf("Error: parameters invalid\n");
		return -1;
	}
#ifdef CONFIG_HB_BOOT_FROM_NAND
	memset(buffer, 0, sizeof(buffer));
	ret = ubi_volume_write("veeprom", buffer, sizeof(buffer));
#else
	int sector_left = 0;
	int sector_right = 0;
	int cur_sector = 0;
	int offset_inner = 0;
	int remain_inner = 0;
	sector_left = start_sector + (offset / SECTOR_SIZE);
	sector_right = start_sector + ((offset + size - 1) / SECTOR_SIZE);
	printf("sector_left = %d\n", sector_left);
	printf("sector_right = %d\n", sector_right);

	for(cur_sector = sector_left; cur_sector <= sector_right; ++cur_sector) {
		int operate_count = 0;
		memset(buffer, 0, sizeof(buffer));

		ret = dw_read(cur_sector);
		flush_cache((ulong)buffer, 512);
		if (ret < 0) {
			printf("Error: read veeporm faild\n");
			return ret;
		}

		offset_inner = offset - (cur_sector - start_sector) * SECTOR_SIZE;
		remain_inner = SECTOR_SIZE - offset_inner;
		operate_count = (remain_inner >= size ? size : remain_inner);
		size -= operate_count;
		offset += operate_count;
		printf("offset_inner = %d\n", offset_inner);
		printf("operate_count = %d\n", operate_count);

		memset(buffer + offset_inner, 0, operate_count);

		ret = dw_write(cur_sector);
		if (ret < 0) {
			printf("Error: write veeporm faild\n");
			return ret;
		}
	}
#endif /*CONFIG_HB_BOOT_FROM_NAND*/
	return ret;
}

int veeprom_dump(void)
{
	int cur_sector = 0;
	int i = 0;
	int ret = 0;
	int flag = FLAG_RO;

	ret = dw_init(flag);
	if (ret < 0) {
		printf("Failed to initialize veeporm\n");
		return -1;
	}

	for (cur_sector = start_sector; cur_sector <= end_sector; ++cur_sector) {
		printf("sector: %d\n", cur_sector);
		memset(buffer, 0, sizeof(buffer));

		ret = dw_read(cur_sector);
		flush_cache((ulong)buffer, VEEPROM_MAX_SIZE);
		if (ret < 0) {
			printf("Error: read veeporm faild\n");
			return ret;
		}

		for (i = 0; i < SECTOR_SIZE; ++i) {
			printf("%02x  ", buffer[i]);
			if (!((i + 1) % 16))
				printf("\n");
		}
	}
	return 0;
}
