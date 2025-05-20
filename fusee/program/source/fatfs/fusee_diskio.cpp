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
#include "diskio_cpp.h"
#include "../fusee_sd_card.hpp"
#include "../fusee_mmc.hpp"
#include "../fusee_emusd.hpp"

bool diskio_read_sd_card(void *dst, size_t size, size_t sector_index, size_t sector_count) {
    return R_SUCCEEDED(::ams::nxboot::ReadSdCard(dst, size, sector_index, sector_count));
}

bool diskio_write_sd_card(size_t sector_index, size_t sector_count, const void *src, size_t size) {
    return R_SUCCEEDED(::ams::nxboot::WriteSdCard(sector_index, sector_count, src, size));
}

bool diskio_read_system(void *dst, size_t size, size_t sector_index, size_t sector_count) {
    return R_SUCCEEDED(::ams::nxboot::ReadMmc(dst, size, ::ams::sdmmc::MmcPartition::MmcPartition_UserData, sector_index, sector_count));
}

bool diskio_write_system(size_t sector_index, size_t sector_count, const void *src, size_t size) {
    return R_SUCCEEDED(::ams::nxboot::WriteMmc(::ams::sdmmc::MmcPartition::MmcPartition_UserData, sector_index, sector_count, src, size));
}

bool diskio_read_boot1(void *dst, size_t size, size_t sector_index, size_t sector_count) {
    return R_SUCCEEDED(::ams::nxboot::ReadMmc(dst, size, ::ams::sdmmc::MmcPartition::MmcPartition_BootPartition2, sector_index, sector_count));
}

bool diskio_write_boot1(size_t sector_index, size_t sector_count, const void *src, size_t size) {
    /* Don't allow writes to eMMC BOOT1 */
    return false;
}

bool diskio_read_emusd(void *dst, size_t size, size_t sector_index, size_t sector_count) {
    return R_SUCCEEDED(::ams::nxboot::ReadEmuSd(dst, size, sector_index, sector_count));
}

bool diskio_write_emusd(size_t sector_index, size_t sector_count, const void *src, size_t size) {
    return R_SUCCEEDED(::ams::nxboot::WriteEmuSd(sector_index, sector_count, src, size));
}
