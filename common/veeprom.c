#include <common.h>
#include <command.h>
#include <mmc.h>
#include <veeprom.h>

#define SECTOR_SIZE (512)
#define BUFFER_SIZE SECTOR_SIZE

static unsigned int start_sector;
static unsigned int end_sector;
static char buffer[BUFFER_SIZE];
static int curr_device = -1;

/* init mmc device and return device pointer */
struct mmc *init_mmc_device(int dev, bool force_init)
{
	struct mmc *mmc = NULL;

	mmc = find_mmc_device(dev);
	if (!mmc)
		return NULL;

	if (force_init)
		mmc->has_init = 0;
	if (mmc_init(mmc))
		return NULL;

	return mmc;
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

	return 0;
}

void veeprom_exit(void)
{
	curr_device = -1;
}

/* format veeprom mmc blocks, memset(0) */
int veeprom_format(void)
{
	struct mmc *mmc = NULL;
	unsigned int cur_sector = 0;
	unsigned int n = 0;

	/* check the status of device */
	mmc = init_mmc_device(curr_device, false);
	if (!mmc) {
		printf("Error: no mmc device at slot %d\n", curr_device);
		return -1;
	}

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	
	/* format raw sectors */
	memset(buffer, 0, sizeof(buffer));
	for (cur_sector = start_sector; cur_sector <= end_sector; ++cur_sector) {
		n = blk_dwrite(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		if (n != 1) {
			printf("Error: write sector %d fail\n", cur_sector);
			return -1;
		}
	}
	return 0;

}

int veeprom_read(int offset, char *buf, int size)
{
	struct mmc *mmc = NULL;
	int sector_left = 0;
	int sector_right = 0;
	int cur_sector = 0;
	int offset_inner = 0;
	int remain_inner = 0;
	unsigned int n = 0;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc) {
		printf("Error: no mmc device at slot %d\n", curr_device);
		return -1;
	}

	if (!is_parameter_valid(offset, size)) {
		printf("Error: parameters invalid\n");
		return -1;
	}

	sector_left = start_sector + (offset / SECTOR_SIZE);
	sector_right = start_sector + ((offset + size - 1) / SECTOR_SIZE);

	for (cur_sector = sector_left; cur_sector <= sector_right; ++cur_sector) {
		int operate_count = 0;
		memset(buffer, 0, sizeof(buffer));

		n = blk_dwrite(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		flush_cache((ulong)buffer, 512);
		if (n != 1) {
			printf("Error: read sector %d fail\n", cur_sector);
			return -1;
		}

		offset_inner = offset - (cur_sector - start_sector) * SECTOR_SIZE;
		remain_inner = SECTOR_SIZE - offset_inner;
		operate_count = (remain_inner >= size ? size : remain_inner);
		size -= operate_count;
		offset += operate_count;
		memcpy(buf, buffer + offset_inner, operate_count);
		buf += operate_count;
	}

	return 0;
}

int veeprom_write(int offset, const char *buf, int size)
{
	struct mmc *mmc = NULL;
	int sector_left = 0;
	int sector_right = 0;
	int cur_sector = 0;
	int offset_inner = 0;
	int remain_inner = 0;
	unsigned int n = 0;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc) {
		printf("Error: no mmc device at slot %d\n", curr_device);
		return -1;
	}

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	if (!is_parameter_valid(offset, size)) {
		printf("Error: parameters invalid\n");
		return -1;
	}

	sector_left = start_sector + (offset / SECTOR_SIZE);
	sector_right = start_sector + ((offset + size - 1) / SECTOR_SIZE);

	for (cur_sector = sector_left; cur_sector <= sector_right; ++cur_sector) {
		int operate_count = 0;
		memset(buffer, 0, sizeof(buffer));

		n = blk_dread(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		flush_cache((ulong)buffer, 512);
		if (n != 1) {
			printf("Error: read sector %d fail\n", cur_sector);
			return -1;
		}

		offset_inner = offset - (cur_sector - start_sector) * SECTOR_SIZE;
		remain_inner = SECTOR_SIZE - offset_inner;
		operate_count = (remain_inner >= size ? size : remain_inner);
		size -= operate_count;
		offset += operate_count;

		memcpy(buffer + offset_inner, buf, operate_count);
		buf += operate_count;

		n = blk_dwrite(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		if (n != 1) {
			printf("Error: write sector %d fail\n", cur_sector);
			return -1;
		}
	}
	
	return 0;
}

int veeprom_clear(int offset, int size)
{
	struct mmc *mmc = NULL;
	int sector_left = 0;
	int sector_right = 0;
	int cur_sector = 0;
	int offset_inner = 0;
	int remain_inner = 0;
	unsigned int n = 0;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc) {
		printf("Error: no mmc device at slot %d\n", curr_device);
		return -1;
	}

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	if (!is_parameter_valid(offset, size)) {
		printf("Error: parameters invalid\n");
		return -1;
	}

	sector_left = start_sector + (offset / SECTOR_SIZE);
	sector_right = start_sector + ((offset + size - 1) / SECTOR_SIZE);
	printf("sector_left = %d\n", sector_left);
	printf("sector_right = %d\n", sector_right);

	for(cur_sector = sector_left; cur_sector <= sector_right; ++cur_sector) {
		int operate_count = 0;
		memset(buffer, 0, sizeof(buffer));

		n = blk_dread(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		flush_cache((ulong)buffer, 512);
		if (n != 1) {
			printf("read sector %d fail\n", cur_sector);
			return -1;
		}

		offset_inner = offset - (cur_sector - start_sector) * SECTOR_SIZE;
		remain_inner = SECTOR_SIZE - offset_inner;
		operate_count = (remain_inner >= size ? size : remain_inner);
		size -= operate_count;
		offset += operate_count;
		printf("offset_inner = %d\n", offset_inner);
		printf("operate_count = %d\n", operate_count);

		memset(buffer + offset_inner, 0, operate_count);

		n = blk_dwrite(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		if (n != 1) {
			printf("write sector %d fail\n", cur_sector);
			return -1;
		}
	}
	return 0;
}

int veeprom_dump(void)
{
	struct mmc *mmc = NULL;
	int cur_sector = 0;
	unsigned int n = 0;
	int i = 0;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc) {
		printf("Error: no mmc device at slot %d\n", curr_device);
		return -1;
	}

	for (cur_sector = start_sector; cur_sector <= end_sector; ++cur_sector) {
		printf("sector: %d\n", cur_sector);
		memset(buffer, 0, sizeof(buffer));

		n = blk_dread(mmc_get_blk_desc(mmc), cur_sector, 1, buffer);
		flush_cache((ulong)buffer, VEEPROM_MAX_SIZE);
		if (n != 1) {
			printf("read sector %d fail\n", cur_sector);
			return -1;
		}

		for (i = 0; i < SECTOR_SIZE; ++i) {
			printf("%02x  ", buffer[i]);
			if (!((i + 1) % 16))
				printf("\n");
		}
	}
	return 0;
}
