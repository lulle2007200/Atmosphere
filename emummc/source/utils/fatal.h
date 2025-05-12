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

#pragma once
#include "../nx/smc.h"

enum FatalReason
{
    Fatal_InitMMC = 0,
    Fatal_InitSD,
    Fatal_InvalidAccessor,
    Fatal_ReadNoAccessor,
    Fatal_WriteNoAccessor,
    Fatal_IoMappingLegacy,
    Fatal_UnknownVersion,
    Fatal_BadResult,
    Fatal_GetConfig,
    Fatal_OpenAccessor,
    Fatal_CloseAccessor,
    Fatal_IoMapping,
    Fatal_FatfsMount,
    Fatal_FatfsFileOpen,
    Fatal_FatfsMemExhaustion,
    Fatal_InvalidEnum,
    Fatal_InvalidPartition,
    Fatal_PartitionSwitchFail,
    Fatal_OOB,
    Fatal_Max
};

/* Atmosphere reboot-to-fatal-error. */
typedef struct {
    u32 magic;
    u32 error_desc;
    u64 program_id;
    union {
        u64 gprs[32];
        struct {
            u64 _gprs[29];
            u64 fp;
            u64 lr;
            u64 sp;
        };
    };
    u64 pc;
    u64 module_base;
    u32 pstate;
    u32 afsr0;
    u32 afsr1;
    u32 esr;
    u64 far;
    u64 report_identifier; /* Normally just system tick. */
    u64 stack_trace_size;
    u64 stack_dump_size;
    u64 stack_trace[0x20];
    u8  stack_dump[0x100];
    u8  tls[0x100];
}atmosphere_fatal_error_ctx;

/* "AFE2" */
#define ATMOSPHERE_REBOOT_TO_FATAL_MAGIC 0x32454641

#define ATMOSPHERE_IRAM_PAYLOAD_BASE 0x40010000

#define ATMOSPHERE_FATAL_ERROR_ADDR 0x4003E000
#define ATMOSPHERE_FATAL_ERROR_CONTEXT ((volatile atmosphere_fatal_error_ctx *)(ATMOSPHERE_FATAL_ERROR_ADDR))

void __attribute__((noreturn)) fatal_abort(enum FatalReason abortReason);
