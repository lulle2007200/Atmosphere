/*
 * Copyright (c) Atmosphère-NX
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
#include "fusee_emusd.hpp"
#include "fs/fusee_fs_storage.hpp"
#include "fusee_fatal.hpp"
#include "fusee_malloc.hpp"
#include "fusee_mmc.hpp"
#include <exosphere.hpp>
#include <exosphere/secmon/secmon_emummc_context.hpp>

namespace ams::nxboot {

    namespace {
        constinit fs::IStorage *g_emusd_base_storage = nullptr;
        constinit fs::IStorage *g_emusd_storage      = nullptr;

        struct MbrPartitionEntry {
            u8 boot_flag;
            u8 chs_start[3];
            u8 type;
            u8 chs_end[3];
            u32 sct_start;
            u32 sct_size;
        };

        struct __attribute__((packed)) Mbr {
            u8 bootloader[440];
            u32 signature;
            u8 padding[2];
            MbrPartitionEntry entries[4];
            u16 magic;
        };
    }

    void InitializeEmuSd(const secmon::EmummcSdConfiguration &emusd_cfg) {
        if (emusd_cfg.IsActive()) {
            if (emusd_cfg.base_cfg.type == secmon::EmummcSdType_Partition_Emmc || emusd_cfg.base_cfg.type == secmon::EmummcSdType_Partition_Sd) {
                /* Partition based */
                Result r;
                if (emusd_cfg.base_cfg.type == secmon::EmummcSdType_Partition_Sd) {
                    if (R_FAILED(r = InitializeSdCard())) {
                        ShowFatalError("Failed to initialize SD Card: %" PRIx32 "!\n", r.GetValue());
                    }
                    g_emusd_base_storage = AllocateObject<fs::SdCardStorage>();
                } else {
                    if (R_FAILED(r = InitializeMmc())) {
                        ShowFatalError("Failed to initialize eMMC: %" PRIx32 "!\n", r.GetValue());
                    }
                    g_emusd_base_storage = AllocateObject<fs::MmcPartitionStorage<sdmmc::MmcPartition_UserData>>();
                }

                /* Get base storage size */
                s64 size;
                if(R_FAILED(g_emusd_base_storage->GetSize(std::addressof(size)))) {
                    ShowFatalError("Failed to get size!");
                }

                /* Read emuSD size from mbr */
                Mbr *mbr = static_cast<Mbr *>(AllocateAligned(sizeof(mbr), 0x200));
                const s64 offset = INT64_C(0x200) * emusd_cfg.partition_cfg.start_sector;
                r = g_emusd_base_storage->Read(offset, mbr, sizeof(*mbr));
                if (R_FAILED(r)) {
                    ShowFatalError("Failed to read MBR: 0x%08" PRIx32 "\n", r.GetValue());
                }
                

                /* Fatal if emuSD too big */
                const s64 emusd_size = INT64_C(0x200) * (mbr->entries[0].sct_size + mbr->entries[0].sct_start);
                if (offset + emusd_size > static_cast<s64>(size)) {
                    ShowFatalError("Invalid emusd!");
                }

                g_emusd_storage = AllocateObject<fs::SubStorage>(*g_emusd_base_storage, offset, emusd_size);
            } else if(emusd_cfg.base_cfg.type == secmon::EmummcSdType_File_Emmc || emusd_cfg.base_cfg.type == secmon::EmummcSdType_File_Sd) {
                /* File based */
                Result r;
                if(emusd_cfg.base_cfg.type == secmon::EmummcSdType_File_Emmc) {
                    /* When emmc based, init eMMC */
                    if (R_FAILED((r = InitializeMmc()))) {
                        ShowFatalError("Failed to initialize mmc: 0x%08" PRIx32 "\n", r.GetValue());
                    } 

                    if (!fs::MountSys()) {
                        ShowFatalError("Failed to mount mmc!\n");
                    }
                } else {
                    /* When sd based, init sd */
                    if (R_FAILED((r = InitializeSdCard ()))) {
                        ShowFatalError("Failed to initialize sd: 0x%08" PRIx32 "\n", r.GetValue());
                    } 

                    if (!fs::MountSdCard()) {
                        ShowFatalError("Failed to mount sd!\n");
                    }
                }

                char path[0xa0];
                if (emusd_cfg.base_cfg.type == secmon::EmummcSdType_File_Emmc) {
                    memcpy(path, "sys:", 5);
                } else {
                    memcpy(path, "sdmc:", 6);
                }
                memcpy(path + strlen(path), emusd_cfg.file_cfg.path.str, sizeof(emusd_cfg.file_cfg.path.str));
                memcpy(path + strlen(path), "/SD/", 5);
                path[sizeof(path) - 1] = '\x00';

                g_emusd_storage = AllocateObject<fs::MultiFileStorage>(path, fs::OpenMode_ReadWrite);
            } else {
                ShowFatalError("Invalid emuSD config!\n");
            }
        } else {
            const Result r = InitializeSdCard();
            if (R_FAILED(r)) {
                ShowFatalError("Failed to initialize SD card: 0x%08" PRIx32 "!\n", r.GetValue());
            }
            g_emusd_storage = AllocateObject<fs::SdCardStorage>();
        }
    }

    Result ReadEmuSd(void *dst, size_t size, size_t sector_index, size_t sector_count) {
        R_UNLESS(g_emusd_storage, fs::ResultNotInitialized());
        R_RETURN(g_emusd_storage->Read(INT64_C(0x200) * sector_index, dst, INT64_C(0x200) * sector_count));
    }

    Result WriteEmuSd(size_t sector_index, size_t sector_count, const void *src, size_t size) {
        R_UNLESS(g_emusd_storage, fs::ResultNotInitialized());
        R_RETURN(g_emusd_storage->Write(INT64_C(0x200) * sector_index, src, INT64_C(0x200) * sector_count));
    }

}
