/*
 * Copyright (c) 2018-2024 CTCaer
 * Copyright (c) 2019 Atmosphère-NX
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

#include <bdk.h>
#include "secmon_exo.h"

// Atmosphère reboot-to-fatal-error.
typedef struct _atm_fatal_error_ctx
{
	u32 magic;
	u32 error_desc;
	u64 title_id;
	union
	{
		u64 gprs[32];
		struct
		{
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
	u64 report_identifier; // Normally just system tick.
	u64 stack_trace_size;
	u64 stack_dump_size;
	u64 stack_trace[0x20];
	u8  stack_dump[0x100];
	u8  tls[0x100];
} atm_fatal_error_ctx;



#define ATM_FATAL_ERR_CTX_ADDR 0x4003E000
#define  ATM_FATAL_MAGIC       0x30454641 // AFE0

#define HOS_PID_BOOT2 0x8


static const char *get_error_desc(u32 error_desc)
{
	switch (error_desc)
	{
	case 0x100:
		return "IABRT"; // Instruction Abort.
	case 0x101:
		return "DABRT"; // Data Abort.
	case 0x102:
		return "IUA";   // Instruction Unaligned Access.
	case 0x103:
		return "DUA";   // Data Unaligned Access.
	case 0x104:
		return "UDF";   // Undefined Instruction.
	case 0x106:
		return "SYS";   // System Error.
	case 0x301:
		return "SVC";   // Bad arguments or unimplemented SVC.
	case 0xF00:
		return "KRNL";  // Kernel panic.
	case 0xFFD:
		return "SO";    // Stack Overflow.
	case 0xFFE:
		return "std::abort";
	default:
		return "UNK";
	}
}

void secmon_exo_check_panic()
{
	volatile atm_fatal_error_ctx *rpt = (atm_fatal_error_ctx *)ATM_FATAL_ERR_CTX_ADDR;

	// Mask magic to maintain compatibility with any AFE version, thanks to additive struct members.
	if ((rpt->magic & 0xF0FFFFFF) != ATM_FATAL_MAGIC)
		return;

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	WPRINTF("Panic occurred while running Atmosphere.\n\n");
	WPRINTFARGS("Title ID: %08X%08X", (u32)((u64)rpt->title_id >> 32), (u32)rpt->title_id);
	WPRINTFARGS("Error:    %s (0x%x)\n", get_error_desc(rpt->error_desc), rpt->error_desc);

	// Check if mixed atmosphere sysmodules.
	if ((u32)rpt->title_id == HOS_PID_BOOT2)
		WPRINTF("Mismatched Atmosphere files?\n");


	// Change magic to invalid, to prevent double-display of error/bootlooping.
	rpt->magic = 0;

	display_backlight_brightness(100, 1000);
}
