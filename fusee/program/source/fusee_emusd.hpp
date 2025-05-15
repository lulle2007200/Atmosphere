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

#include "fusee_sd_card.hpp"
#include <exosphere.hpp>
#include <exosphere/secmon/secmon_emummc_context.hpp>

namespace ams::nxboot {
    void InitializeEmuSd(const secmon::EmummcSdConfiguration &emusd_cfg);
    Result ReadEmuSd(void *dst, size_t size, size_t sector_index, size_t sector_count);
    Result WriteEmuSd(size_t sector_index, size_t sector_count, const void *src, size_t size);

}