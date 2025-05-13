/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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

#ifndef __EMUMMC_CTX_H__
#define __EMUMMC_CTX_H__

#include "../utils/types.h"
#include "../FS/FS_versions.h"

#define EMUMMC_STORAGE_MAGIC  0x30534645 /* EFS0, EmuFS0 */

// 0x80 for path from config + 0xf for drive prefix
#define EMUMMC_MAX_DIR_LENGTH 0x8F

enum EmummcType {
        EmummcType_None           = 0,
        EmummcType_Partition_Sd   = 1,
        EmummcType_File_Sd        = 2,
        EmummcType_Partition_Emmc = 3,
        EmummcType_File_Emmc      = 4,
};

typedef struct _emuMMC_ctx_t
{
    u32 magic;
    u32 id;
    enum FS_VER fs_ver;
    enum EmummcType EMMC_Type;
    enum EmummcType SD_Type;

    /* Partition based */
    u64 EMMC_StoragePartitionOffset;
    u64 SD_StoragePartitionOffset;

    /* File-Based */
    char storagePath[EMUMMC_MAX_DIR_LENGTH + 1];
} emuMMC_ctx_t, *PemuMMC_ctx_t;

#endif /* __EMUMMC_CTX_H__ */
