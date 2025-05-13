/*
 * Copyright (c) 2018 naehrwert
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "../utils/types.h"
#include "nx_emmc.h"

static u32 emmc_mode = EMMC_MMC_HS400;

int nx_emmc_part_read(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;
	return sdmmc_storage_read(storage, part->lba_start + sector_off, num_sectors, buf);
}

int nx_emmc_part_write(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;
	return sdmmc_storage_write(storage, part->lba_start + sector_off, num_sectors, buf);
}

int emmc_init_retry(bool power_cycle)
{
	u32 bus_width = SDMMC_BUS_WIDTH_8;
	u32 type = SDHCI_TIMING_MMC_HS400;

	// Power cycle SD eMMC.
	if (power_cycle)
	{
		emmc_mode--;
		nx_emmc_end();
	}

	// Get init parameters.
	switch (emmc_mode)
	{
	case EMMC_INIT_FAIL: // Reset to max.
		return 0;
	case EMMC_1BIT_HS52:
		bus_width = SDMMC_BUS_WIDTH_1;
		type = SDHCI_TIMING_MMC_HS52;
		break;
	case EMMC_8BIT_HS52:
		type = SDHCI_TIMING_MMC_HS52;
		break;
	case EMMC_MMC_HS200:
		type = SDHCI_TIMING_MMC_HS200;
		break;
	case EMMC_MMC_HS400:
		type = SDHCI_TIMING_MMC_HS400;
		break;
	default:
		emmc_mode = EMMC_MMC_HS400;
	}

	return sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, bus_width, type);
}

void nx_emmc_end() { sdmmc_storage_end(&emmc_storage); }

bool nx_emmc_initialize(bool power_cycle)
{
	// Reset mode in case of previous failure.
	if (emmc_mode == EMMC_INIT_FAIL)
		emmc_mode = EMMC_MMC_HS400;

	if (power_cycle)
		nx_emmc_end();

	int res = !emmc_init_retry(false);

	while (true)
	{
		if (!res)
			return true;
		else
		{
			if (emmc_mode == EMMC_INIT_FAIL)
				break;
			else
				res = !emmc_init_retry(true);
		}
	}

	nx_emmc_end();

	return false;
}

int nx_emmc_set_partition(u32 partition) { return sdmmc_storage_set_mmc_partition(&emmc_storage, partition); }
