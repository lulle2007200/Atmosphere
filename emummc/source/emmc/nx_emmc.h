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

#ifndef _NX_EMMC_H_
#define _NX_EMMC_H_

#include "../utils/types.h"
#include "sdmmc.h"

enum
{
	EMMC_INIT_FAIL = 0,
	EMMC_1BIT_HS52 = 1,
	EMMC_8BIT_HS52 = 2,
	EMMC_MMC_HS200 = 3,
	EMMC_MMC_HS400 = 4,
};

typedef struct _gpt_entry_t
{
	u8 type_guid[0x10];
	u8 part_guid[0x10];
	u64 lba_start;
	u64 lba_end;
	u64 attrs;
	u16 name[36];
} gpt_entry_t;

typedef struct _gpt_header_t
{
	u64 signature;
	u32 revision;
	u32 size;
	u32 crc32;
	u32 res1;
	u64 my_lba;
	u64 alt_lba;
	u64 first_use_lba;
	u64 last_use_lba;
	u8 disk_guid[0x10];
	u64 part_ent_lba;
	u32 num_part_ents;
	u32 part_ent_size;
	u32 part_ents_crc32;
	u8 res2[420];
} gpt_header_t;

#define NX_GPT_FIRST_LBA 1
#define NX_GPT_NUM_BLOCKS 33
#define NX_EMMC_BLOCKSIZE 512

typedef struct _emmc_part_t
{
	u32 lba_start;
	u32 lba_end;
	u64 attrs;
	s8 name[37];
} emmc_part_t;

extern sdmmc_storage_t emmc_storage;
extern sdmmc_t emmc_sdmmc;

int nx_emmc_part_read(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf);
int nx_emmc_part_write(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf);

int  nx_emmc_init_retry(bool power_cycle);
bool nx_emmc_initialize(bool power_cycle);
int  nx_emmc_set_partition(u32 partition);
void nx_emmc_end();

#endif
