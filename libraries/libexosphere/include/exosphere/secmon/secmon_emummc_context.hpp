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
#pragma once
#include <vapours.hpp>

namespace ams::secmon {

    enum EmummcEmmcType : u32 {
        EmummcEmmcType_None           = 0,
        EmummcEmmcType_Partition_Sd   = 1,
        EmummcEmmcType_File_Sd        = 2,
        EmummcEmmcType_Partition_Emmc = 3,
        EmummcEmmcType_File_Emmc      = 4,
    };

    enum EmummcSdType : u32 {
        EmummcSdType_None           = 0,
        EmummcSdType_Partition_Emmc = 3,
        // Not (currently) supported
        // EmummcSdType_Partition_Sd   = 1,
        // EmummcSdType_File_Sd        = 2,
        // EmummcSdType_File_Emmc      = 4,
    };

    enum EmummcMmc {
        EmummcMmc_Nand = 0,
        EmummcMmc_Sd   = 1,
        EmummcMmc_Gc   = 2,
    };

    constexpr inline size_t EmummcFilePathLengthMax = 0x80;

    struct EmummcFilePath {
        char str[EmummcFilePathLengthMax];
    };
    static_assert(util::is_pod<EmummcFilePath>::value);
    static_assert(sizeof(EmummcFilePath) == EmummcFilePathLengthMax);


    struct EmummcPartitionConfiguration {
        u64 start_sector;
    };
    static_assert(util::is_pod<EmummcPartitionConfiguration>::value);

    struct EmummcFileConfiguration {
        EmummcFilePath path;
    };
    static_assert(util::is_pod<EmummcFileConfiguration>::value);

    struct EmummcEmmcBaseConfiguration {
        static constexpr u32 Magic = util::FourCC<'E','F','S','0'>::Code;
        u32 magic;
        EmummcEmmcType type;
        u32 id;
        u32 fs_version;

        constexpr bool IsValid() const {
            return this->magic == Magic;
        }

        constexpr bool IsActive() const {
            return this->IsValid() && this->type != EmummcEmmcType::EmummcEmmcType_None;
        }
    };
    static_assert(util::is_pod<EmummcEmmcBaseConfiguration>::value);
    static_assert(sizeof(EmummcEmmcBaseConfiguration) == 0x10);


    struct EmummcEmmcConfiguration {
        EmummcEmmcBaseConfiguration base_cfg;
        union {
            EmummcPartitionConfiguration partition_cfg;
            EmummcFileConfiguration file_cfg;
        };
        EmummcFilePath emu_dir_path;

        constexpr bool IsValid() const {
            return this->base_cfg.IsValid();
        }

        constexpr bool IsActive() const {
            return this->base_cfg.IsActive();
        }
    };
    static_assert(util::is_pod<EmummcEmmcConfiguration>::value);
    static_assert(sizeof(EmummcEmmcConfiguration) == 0x110);

    struct EmummcSdBaseConfiguration {
        static constexpr u32 Magic = util::FourCC<'E','F','S','0'>::Code;
        u32 magic;
        EmummcSdType type;
        /* id currently unused */
        u32 id;
        u32 fs_version;

        constexpr bool IsValid() const {
            return this->magic == Magic;
        }

        constexpr bool IsActive() const {
            return this->IsValid() && this->type != EmummcSdType::EmummcSdType_None;
        }
    };
    static_assert(util::is_pod<EmummcSdBaseConfiguration>::value);
    static_assert(sizeof(EmummcSdBaseConfiguration) == 0x10);

    struct EmummcSdConfiguration {
        EmummcSdBaseConfiguration base_cfg;
        union {
            EmummcPartitionConfiguration partition_cfg;
            /* File based currently not supported */
            /* EmummcFileConfiguration file_cfg */
        };

        constexpr bool IsValid() const {
            return this->base_cfg.IsValid();
        }

        constexpr bool IsActive() const {
            return this->base_cfg.IsActive();
        }
    };
    static_assert(util::is_pod<EmummcSdConfiguration>::value);
    static_assert(sizeof(EmummcSdConfiguration) == 0x18);

    struct EmummcConfiguration {
        EmummcEmmcConfiguration emmc_cfg;
        EmummcSdConfiguration sd_cfg;
    };
    static_assert(util::is_pod<EmummcConfiguration>::value);
    static_assert(sizeof(EmummcConfiguration) <= 0x200);

}