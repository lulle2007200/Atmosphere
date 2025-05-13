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

#include <string.h>
#include <switch/sf/service.h>
#include "fatal.h"
#include "../utils/util.h"

#if EMUMMC_HAS_FATAL_PAYLOAD 
#include "fatal_handler_bin.h"
#endif

void __attribute__((noreturn)) fatal_abort(enum FatalReason abortReason)
{
    atmosphere_fatal_error_ctx error_ctx;
    memset(&error_ctx, 0, sizeof(atmosphere_fatal_error_ctx));

    // Basic error storage for Atmosphere
    error_ctx.magic = ATMOSPHERE_REBOOT_TO_FATAL_MAGIC;
    error_ctx.program_id = 0x0100000000000000; // FS
    error_ctx.error_desc = abortReason;

    // Try using bpc:ams to show fatal error
    Handle h;
    Service s;
    Result rc = svcConnectToNamedPort(&h, "bpc:ams");
    u32 retry_cnt = 20;
    while (R_VALUE(rc) == KERNELRESULT(NotFound) && retry_cnt != 0) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&h, "bpc:ams");
        retry_cnt--;
    }

    if (R_SUCCEEDED(rc)){
        serviceCreate(&s, h);
        serviceDispatch(&s, 65000,
                        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias | SfBufferAttr_FixedSize },
                        .buffers = { { &error_ctx, 0x450 } }
                        );
    }

    // bpc:ams not available yet
    // Copy error context to iram and reboot to fatal payload

    memcpy(&working_buf, &error_ctx, sizeof(error_ctx));
    smcCopyToIram(ATMOSPHERE_FATAL_ERROR_ADDR, &working_buf, sizeof(error_ctx));

    #if EMUMMC_HAS_FATAL_PAYLOAD
    for (size_t ofs = 0; ofs < fatal_handler_bin_size; ofs += 4096) {
        memcpy(&working_buf, fatal_handler_bin + ofs, MIN(fatal_handler_bin_size - ofs, 4096));
        smcCopyToIram(ATMOSPHERE_IRAM_PAYLOAD_BASE + ofs, &working_buf, MIN(fatal_handler_bin_size - ofs, 4096));
    }
    smcRebootToIramPayload();
    #else
    smcRebootToRcm();
    #endif

    while(true){}
}
