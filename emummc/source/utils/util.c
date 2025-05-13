/*
* Copyright (c) 2018 naehrwert
* Copyright (C) 2018 CTCaer
* Copyright (C) 2019 M4xw
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

#include "util.h"
#include "fatal.h"
#include "types.h"
#include "../nx/counter.h"
// #include "../nx/svc.h"
#include "../soc/t210.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <switch/kernel/svc.h>

_Alignas(4096) u8 working_buf[4096];

typedef struct _log_ctx 
{
	u32 magic;
	u32 sz;
	u32 start;
	u32 end;
	char buf[];
} log_ctx_t;

typedef struct _io_mapping_t
{
	u64 phys;
	u64 virt;
	u64 size;
} io_mapping_t;
static io_mapping_t io_mapping_list[10] = {0}; // Max 10 Mappings
#define IO_MAPPING_COUNT (sizeof(io_mapping_list) / sizeof(io_mapping_t))

static inline uintptr_t _GetIoMapping(u64 io_addr, u64 io_size)
{
    u64 vaddr;
    u64 aligned_addr = (io_addr & ~0xFFFul);
    u64 aligned_size = io_size + (io_addr - aligned_addr);

    if (emuMMC_ctx.fs_ver >= FS_VER_10_0_0) {
        u64 out_size;
        if (svcQueryMemoryMapping(&vaddr, &out_size, aligned_addr, aligned_size) != 0) {
            fatal_abort(Fatal_IoMapping);
        }
    } else {
        if (svcLegacyQueryIoMapping(&vaddr, aligned_addr, aligned_size) != 0) {
            fatal_abort(Fatal_IoMappingLegacy);
        }
    }

    return (uintptr_t)(vaddr + (io_addr - aligned_addr));
}

intptr_t QueryIoMapping(u64 addr, u64 size)
{
	for (int i = 0; i < IO_MAPPING_COUNT; i++)
	{
		if (io_mapping_list[i].phys == addr && io_mapping_list[i].size == size)
		{
			return io_mapping_list[i].virt;
		}
	}

	u64 ioMap = _GetIoMapping(addr, size);

	for (int i = 0; i < IO_MAPPING_COUNT; i++)
	{
		if (io_mapping_list[i].phys == 0 && io_mapping_list[i].virt == 0 && io_mapping_list[i].size == 0) // First empty
		{
			io_mapping_list[i].virt = ioMap;
			io_mapping_list[i].phys = addr;
			io_mapping_list[i].size = size;
			break;
		}
	}

	return (intptr_t)ioMap;
}

u64 get_tmr_s()
{
	return armTicksToNs(armGetSystemTick()) / 1e+9;
}

u64 get_tmr_ms()
{
	return armTicksToNs(armGetSystemTick()) / 1000000;
}

u64 get_tmr_us()
{
	return armTicksToNs(armGetSystemTick()) / 1000;
}

// TODO: Figure if Sleep or Busy loop
void msleep(u64 milliseconds)
{
	u64 now = get_tmr_ms();
	while (((u64)get_tmr_ms() - now) < milliseconds)
		;
	//svcSleepThread(1000000 * milliseconds);
}

// TODO: Figure if Sleep or Busy loop
void usleep(u64 microseconds)
{
	u64 now = get_tmr_us();
	while (((u64)get_tmr_us() - now) < microseconds)
		;
	//svcSleepThread(1000 * microseconds);
}

void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops)
{
	for (u32 i = 0; i < num_ops; i++)
		base[ops[i].off] = ops[i].val;
}

#define IRAM_LOG_CTX_ADDR 0x4003C000
#define IRAM_LOG_MAX_SZ 4096

void log_iram(const char* fmt, ...) {
	static const u32 max_log_sz = sizeof(working_buf) - sizeof(log_ctx_t);
	static bool init_done = false;

	log_ctx_t *log_ctx = (log_ctx_t*)working_buf;

	smcCopyFromIram(working_buf, IRAM_LOG_CTX_ADDR, sizeof(working_buf));

	if(!init_done){
		init_done = true;
		log_ctx->buf[0] = '\0';
		log_ctx->magic = 0xaabbccdd;
		log_ctx->start = 0;
		log_ctx->end   = 0;
	}

	va_list args;
	va_start(args, fmt);
	int res = vsnprintf(log_ctx->buf + log_ctx->end, sizeof(working_buf) - sizeof(log_ctx_t) - log_ctx->end, fmt, args);
	va_end(args);

	if(res < 0 || log_ctx->start + res + 1 > max_log_sz) {
		return;
	}

	log_ctx->end += res;
	smcCopyToIram(IRAM_LOG_CTX_ADDR, working_buf, sizeof(working_buf));

	// static const u32 max_log_sz = IRAM_LOG_MAX_SZ - sizeof(log_ctx_t);
	// static u32 cur_log_offset = 0;
	// static u32 start = 0;
	// static bool init_done = false;

	// if(!init_done){
	// 	init_done = true;
	// 	__attribute__((aligned(0x20))) log_ctx_t log_ctx;
	// 	log_ctx.magic = 0xaabbccdd;
	// 	log_ctx.sz = max_log_sz;
	// 	log_ctx.start = 0;
	// 	log_ctx.end = 0;

	// 	smcCopyToIram(IRAM_LOG_CTX_ADDR, &log_ctx, sizeof(log_ctx_t));
	// }


	// if(working_buf[0] == '\x00') {
	// 	return;
	// }

	// u32 len = strlen((char*)working_buf);
	// u32 bytes_left = len + 1;


	// if(cur_log_offset % 4) {
	// 	char __attribute__((aligned(4))) prev[4] = {0};
	// 	smcCopyFromIram(&prev, IRAM_LOG_CTX_ADDR + sizeof(log_ctx_t) + cur_log_offset, 4);

	// 	uintptr_t target_addr = (uintptr_t)IRAM_LOG_CTX_ADDR + sizeof(log_ctx_t) + (cur_log_offset & ~3);

	// 	u32 prev_len  = cur_log_offset % 4;
	// 	u32 prev_free = 4 - prev_len;
	// 	u32 bytes_to_cpy = MIN(prev_free, bytes_left);

	// 	memcpy(prev + prev_len, working_buf, bytes_to_cpy);

	// 	bytes_left -= bytes_to_cpy;

	// 	smcCopyToIram(target_addr, prev, 4);

	// 	memmove(working_buf, working_buf + bytes_to_cpy, bytes_left);

	// 	cur_log_offset += bytes_to_cpy;
	// }

	// if(cur_log_offset >= max_log_sz) {
	// 	cur_log_offset = 0;
	// 	start = cur_log_offset + 1;
	// }

	// char *cur = (char*)working_buf;
	// while(bytes_left) {
	// 	// cur_log_offset is now 4 byte aligned
	// 	uintptr_t target_addr = (uintptr_t)IRAM_LOG_CTX_ADDR + sizeof(log_ctx_t) + cur_log_offset;

	// 	u32 bytes_to_cpy = MIN(4096, bytes_left);
	// 	bytes_to_cpy = MIN(bytes_to_cpy, 4096 - (target_addr % 4096));
	// 	bytes_to_cpy = MIN(bytes_to_cpy, 4096 - (((uintptr_t)cur) % 4096));

	// 	smcCopyToIram(target_addr, cur, bytes_to_cpy);

	// 	bytes_left -= bytes_to_cpy;

	// 	if(cur_log_offset >= max_log_sz) {
	// 		cur_log_offset = 0;
	// 		start = cur_log_offset + 1;
	// 	}
	// }

	// __attribute__((aligned(0x20))) log_ctx_t log_ctx;
	// log_ctx.magic = 0xaabbccdd;
	// log_ctx.sz = max_log_sz;
	// log_ctx.end = cur_log_offset;
	// log_ctx.start = start;
	// smcCopyToIram(IRAM_LOG_CTX_ADDR, &log_ctx, sizeof(log_ctx_t));

	// if(cur_log_offset != 0) {
	// 	cur_log_offset--;
	// } else {
	// 	cur_log_offset = max_log_sz - 1;
	// }

}
