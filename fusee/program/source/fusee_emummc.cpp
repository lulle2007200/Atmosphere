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
#include <exosphere.hpp>
#include <exosphere/secmon/secmon_emummc_context.hpp>
#include "fusee_emummc.hpp"
#include "fusee_ini.hpp"
#include "fusee_mmc.hpp"
#include "fusee_fatal.hpp"
#include "fusee_malloc.hpp"
#include "fs/fusee_fs_api.hpp"
#include "fs/fusee_fs_storage.hpp"
#include "fusee_sd_card.hpp"
#include "fusee_util.hpp"

namespace ams::nxboot {

    namespace {

        using MmcBoot0Storage = fs::MmcPartitionStorage<sdmmc::MmcPartition_BootPartition1>;
        using MmcUserStorage  = fs::MmcPartitionStorage<sdmmc::MmcPartition_UserData>;
        using EmummcFileStorage = fs::MultiFileStorage;

        constinit secmon::EmummcConfiguration g_emummc_cfg = {};
        constinit fs::SdCardStorage g_sd_card_storage;
        constinit MmcBoot0Storage g_mmc_boot0_storage;
        constinit MmcUserStorage g_mmc_user_storage;

        constinit fs::IStorage *g_boot0_storage = nullptr;
        constinit fs::IStorage *g_user_storage  = nullptr;

        constinit fs::SubStorage *g_package2_storage = nullptr;

        struct Guid {
            u32 data1;
            u16 data2;
            u16 data3;
            u8 data4[8];
        };
        static_assert(sizeof(Guid) == 0x10);

        struct GptHeader {
            char signature[8];
            u32 revision;
            u32 header_size;
            u32 header_crc32;
            u32 reserved0;
            u64 my_lba;
            u64 alt_lba;
            u64 first_usable_lba;
            u64 last_usable_lba;
            Guid disk_guid;
            u64 partition_entry_lba;
            u32 number_of_partition_entries;
            u32 size_of_partition_entry;
            u32 partition_entry_array_crc32;
            u32 reserved1;
        };
        static_assert(sizeof(GptHeader) == 0x60);

        struct GptPartitionEntry {
            Guid partition_type_guid;
            Guid unique_partition_guid;
            u64 starting_lba;
            u64 ending_lba;
            u64 attributes;
            char partition_name[0x48];
        };
        static_assert(sizeof(GptPartitionEntry) == 0x80);

        struct Gpt {
            GptHeader header;
            u8 padding[0x1A0];
            GptPartitionEntry entries[128];
        };
        static_assert(sizeof(Gpt) == 16_KB + 0x200);

        constexpr const u16 Package2PartitionName[] = {
            'B', 'C', 'P', 'K', 'G', '2', '-', '1', '-', 'N', 'o', 'r', 'm', 'a', 'l', '-', 'M', 'a', 'i', 'n', 0
        };

        bool IsDirectoryExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && entry_type == fs::DirectoryEntryType_Directory;
        }

    }

    void InitializeEmummc(bool emummc_enabled, const secmon::EmummcEmmcConfiguration &emummc_cfg) {
        Result result;
        if (emummc_enabled) {
            if(emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_Partition_Emmc || emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_Partition_Sd) {
                /* Partition based emummc */
                if(emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_Partition_Emmc) {
                    /* When emmc based, init eMMC */
                    if (R_FAILED((result = InitializeMmc()))) {
                        ShowFatalError("Failed to initialize mmc: 0x%08" PRIx32 "\n", result.GetValue());
                    }
                } else {
                    if (R_FAILED((result = InitializeSdCard ()))) {
                        ShowFatalError("Failed to initialize sd: 0x%08" PRIx32 "\n", result.GetValue());
                    }
                }

                /* Get SD or eMMC storage */
                fs::IStorage &storage = emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_Partition_Sd ? static_cast<fs::IStorage&>(g_sd_card_storage) : static_cast<fs::IStorage&>(g_mmc_user_storage);

                /* Get total storage size */
                s64 storage_size;
                if (R_FAILED((result = storage.GetSize(std::addressof(storage_size))))) {
                    ShowFatalError("Failed to get storage size: 0x%08" PRIx32 "!\n", result.GetValue());
                }

                const s64 partition_start = emummc_cfg.partition_cfg.start_sector * sdmmc::SectorSize;
                g_boot0_storage = AllocateObject<fs::SubStorage>(storage, partition_start, 4_MB);
                g_user_storage = AllocateObject<fs::SubStorage>(storage, partition_start + 8_MB, storage_size - (partition_start + 8_MB));
            } else if (emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_File_Emmc || emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_File_Sd) {
                /* File based emummc */
                char path[0x300];

                if(emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_File_Emmc) {
                    /* When emmc based, init eMMC */
                    if (R_FAILED((result = InitializeMmc()))) {
                        ShowFatalError("Failed to initialize mmc: 0x%08" PRIx32 "\n", result.GetValue());
                    } 

                    if (!fs::MountSys()) {
                        ShowFatalError("Failed to mount mmc!\n");
                    }
                } else {
                    /* When sd based, init sd */
                    if (R_FAILED((result = InitializeSdCard ()))) {
                        ShowFatalError("Failed to initialize sd: 0x%08" PRIx32 "\n", result.GetValue());
                    } 

                    if (!fs::MountSdCard()) {
                        ShowFatalError("Failed to mount sd!\n");
                    }
                }

                 /* Set drive to read from */
                if (emummc_cfg.base_cfg.type == secmon::EmummcEmmcType_File_Sd) {
                    std::strcpy(path, "sdmc:");
                } else {
                    std::strcpy(path, "sys:");
                }


                std::memcpy(path + std::strlen(path), emummc_cfg.file_cfg.path.str, sizeof(emummc_cfg.file_cfg.path.str));
                std::strcat(path, "/eMMC");

                auto len = std::strlen(path);

                /* Open boot0 file */
                fs::FileHandle boot0_file;
                std::strcat(path, "/boot0");
                if(R_FAILED((result = fs::OpenFile(std::addressof(boot0_file), path, fs::OpenMode_Read)))) {
                    ShowFatalError("Failed to open emummc boot0 file: 0x%08" PRIx32 " %s!\n", result.GetValue(), path);
                }

                /* Check if boot1 file exists */
                std::strcpy(path + len, "/boot1");
                {
                    fs::DirectoryEntryType entry_type;
                    bool is_archive;
                    if (R_FAILED((result = fs::GetEntryType(std::addressof(entry_type), std::addressof(is_archive), path)))){
                        ShowFatalError("Failed to find emummc boot1 file: 0x%08" PRIx32 "!\n", result.GetValue());
                    }

                    if (entry_type != fs::DirectoryEntryType_File) {
                        ShowFatalError("emummc boot1 file is not a file!\n");
                    }
                }

                /* Open userdata */
                std::strcpy(path + len, "/");

                /* Create partition */
                /* TODO: construct boot0 storage from path instead */
                g_boot0_storage = AllocateObject<fs::FileHandleStorage>(boot0_file);
                g_user_storage  = AllocateObject<EmummcFileStorage>(path);
            } else {
                ShowFatalError("Unknown emummc type %d\n", static_cast<int>(emummc_cfg.base_cfg.type));
            }
        } else {
            /* Initialize access to mmc. */
            {
                const Result result = InitializeMmc();
                if (R_FAILED(result)) {
                    ShowFatalError("Failed to initialize mmc: 0x%08" PRIx32 "\n", result.GetValue());
                }
            }

            /* Create storages. */
            g_boot0_storage = std::addressof(g_mmc_boot0_storage);
            g_user_storage  = std::addressof(g_mmc_user_storage);
        }

        if (g_boot0_storage == nullptr) {
            ShowFatalError("Failed to initialize BOOT0\n");
        }
        if (g_user_storage == nullptr) {
            ShowFatalError("Failed to initialize Raw EMMC\n");
        }

        /* Read the GPT. */
        Gpt *gpt = static_cast<Gpt *>(AllocateAligned(sizeof(Gpt), 0x200));
        {
            const Result result = g_user_storage->Read(0x200, gpt, sizeof(*gpt));
            if (R_FAILED(result)) {
                ShowFatalError("Failed to read GPT: 0x%08" PRIx32 "\n", result.GetValue());
            }
        }

        /* Check the GPT. */
        if (std::memcmp(gpt->header.signature, "EFI PART", 8) != 0) {
            ShowFatalError("Invalid GPT signature\n");
        }
        if (gpt->header.number_of_partition_entries > util::size(gpt->entries)) {
            ShowFatalError("Too many GPT entries\n");
        }

        /* Create system storage. */
        for (u32 i = 0; i < gpt->header.number_of_partition_entries; ++i) {
            if (gpt->entries[i].starting_lba < gpt->header.first_usable_lba) {
                continue;
            }

            const s64 offset =  INT64_C(0x200) * gpt->entries[i].starting_lba;
            const u64 size   = UINT64_C(0x200) * (gpt->entries[i].ending_lba + 1 - gpt->entries[i].starting_lba);

            if (std::memcmp(gpt->entries[i].partition_name, Package2PartitionName, sizeof(Package2PartitionName)) == 0) {
                g_package2_storage = AllocateObject<fs::SubStorage>(*g_user_storage, offset, size);
            }
        }

        /* Check that we created package2 storage. */
        if (g_package2_storage == nullptr) {
            ShowFatalError("Failed to initialize Package2\n");
        }
    }

    Result ReadBoot0(s64 offset, void *dst, size_t size) {
        R_RETURN(g_boot0_storage->Read(offset, dst, size));
    }

    Result ReadPackage2(s64 offset, void *dst, size_t size) {
        R_RETURN(g_package2_storage->Read(offset, dst, size));
    }

    Result ReadEmummcConfig() {
        /* Set magic. */
        auto &sd_cfg     = g_emummc_cfg.sd_cfg;
        auto &emmc_cfg = g_emummc_cfg.emmc_cfg;

        emmc_cfg.base_cfg.magic = secmon::EmummcEmmcBaseConfiguration::Magic;
        sd_cfg.base_cfg.magic   = secmon::EmummcSdBaseConfiguration::Magic;

        /* Parse emummc ini. */
        u32 enabled = 0;
        u32 id = 0;
        u32 sector = 0;
        const char *path = "";
        const char *n_path = "";

        {
            IniSectionList sections;
            if (ParseIniSafe(sections, "emummc/emummc.ini")) {
                for (const auto &section : sections){
                    /* Skip non-emummc sections */
                    if (std::strcmp(section.name, "emummc")) {
                        continue;
                    }

                    for (const auto &entry : section.kv_list) {
                        if(std::strcmp(entry.key, "enabled") == 0) {
                            enabled = ParseDecimalInteger(entry.value);
                        } else if (std::strcmp(entry.key, "id") == 0) {
                            id = ParseHexInteger(entry.value);
                        } else if (std::strcmp(entry.key, "sector") == 0) {
                            sector = ParseHexInteger(entry.value);
                        } else if (std::strcmp(entry.key, "path") == 0) {
                            path = entry.value;
                        } else if (std::strcmp(entry.key, "nintendo_path") == 0) {
                            n_path = entry.value;
                        }
                    }
                }
            }
        }

        /* Set parsed values to config */
        constexpr const char *emummc_err_str = "Invalid emummc setting!\n";

        emmc_cfg.base_cfg.id = id;
        std::strncpy(emmc_cfg.emu_dir_path.str, n_path, sizeof(emmc_cfg.emu_dir_path.str));
        emmc_cfg.emu_dir_path.str[sizeof(emmc_cfg.emu_dir_path.str) - 1] = '\x00';

        if (enabled == 1) {
            /* SD based */
            if (sector > 0) {
                emmc_cfg.base_cfg.type = secmon::EmummcEmmcType::EmummcEmmcType_Partition_Sd;
                emmc_cfg.partition_cfg.start_sector = sector;
            } else if (path[0] != '\x00' && IsDirectoryExist(path)) {
                emmc_cfg.base_cfg.type = secmon::EmummcEmmcType::EmummcEmmcType_File_Sd;
                std::strncpy(emmc_cfg.file_cfg.path.str, path, sizeof(emmc_cfg.file_cfg.path.str));
                emmc_cfg.file_cfg.path.str[sizeof(emmc_cfg.file_cfg.path.str) - 1] = '\x00';
            } else {
                ShowFatalError(emummc_err_str);
            }
        } else if (enabled == 4){
            /* eMMC based */
            if (sector > 0) {
                emmc_cfg.base_cfg.type = secmon::EmummcEmmcType::EmummcEmmcType_Partition_Emmc;
                emmc_cfg.partition_cfg.start_sector = sector;
            } else if (path[0] != '\x00' /* && IsDirectoryExist(path) */) {
                /* TODO: Should check if directory exist on *eMMC* fat partition instead of SD */
                emmc_cfg.base_cfg.type = secmon::EmummcEmmcType::EmummcEmmcType_File_Emmc;
                std::strncpy(emmc_cfg.file_cfg.path.str, path, sizeof(emmc_cfg.file_cfg.path.str));
                emmc_cfg.file_cfg.path.str[sizeof(emmc_cfg.file_cfg.path.str) - 1] = '\x00';
            } else {
                ShowFatalError(emummc_err_str);
            }
        } else if (enabled == 0) {
            emmc_cfg.base_cfg.type = secmon::EmummcEmmcType::EmummcEmmcType_None;
        } else {
            ShowFatalError(emummc_err_str);
        }

        /* Parse emusd ini. */
        constexpr const char *emusd_err_str = "Invalid emusd setting!\n";

        u32 sd_enabled = 0;
        u32 sd_sector = 0;

        {
            IniSectionList sections;
            if (ParseIniSafe(sections, "emusd/emusd.ini")) {
                for (const auto &section : sections){
                    /* Skip non-emummc sections */
                    if (std::strcmp(section.name, "emusd")) {
                        continue;
                    }

                    for (const auto &entry : section.kv_list) {
                        if(std::strcmp(entry.key, "enabled") == 0) {
                            sd_enabled = ParseDecimalInteger(entry.value);
                        } else if (std::strcmp(entry.key, "sector") == 0) {
                            sd_sector = ParseHexInteger(entry.value);
                        }
                    }
                }
            }
        }

        /* Set parsed values to config */
        if (sd_enabled == 4) {
            if (sd_sector > 0) {
                sd_cfg.partition_cfg.start_sector = sd_sector;
                sd_cfg.base_cfg.type = secmon::EmummcSdType::EmummcSdType_Partition_Emmc;
            } else {
                ShowFatalError(emusd_err_str);
            }
        } else if (sd_enabled == 0) {
            sd_cfg.base_cfg.type = secmon::EmummcSdType::EmummcSdType_None;
        } else {
            ShowFatalError(emusd_err_str);
        }

        R_SUCCEED();
    }

    const secmon::EmummcConfiguration &GetEmummcConfig() {
        return g_emummc_cfg;
    }

}
