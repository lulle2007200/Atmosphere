/*
 * Copyright (c) 2018 naehrwert
 *
 * Copyright (c) 2018-2024 CTCaer
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
#include "hos/secmon_exo.h"

typedef struct _log_ctx 
{
    u32 magic;
    u32 sz;
    u32 start;
    u32 end;
    char buf[];
} log_ctx_t;

#define IRAM_LOG_CTX_ADDR 0x4003C000

static void check_log(){
    volatile log_ctx_t *log_ctx = (log_ctx_t*)IRAM_LOG_CTX_ADDR;

    if(log_ctx->magic == 0xaabbccdd){
    	gfx_printf("\nLogs:\n");
    	gfx_printf((char*)log_ctx->buf);
    }
}

volatile nyx_storage_t *nyx_str = (nyx_storage_t *)NYX_STORAGE_ADDR;

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
	// Do initial HW configuration. This is compatible with consecutive reruns without a reset.
	hw_init();
	// Pivot the stack under IPL. (Only max 4KB is needed).
	pivot_stack(IPL_LOAD_ADDR);

	// Place heap at a place outside of L4T/HOS configuration and binaries.
	heap_init((void *)IPL_HEAP_START);

	// Prep RTC regs for read. Needed for T210B01 R2C.
	max77620_rtc_prep_read();

	// Initialize display.
	display_init();

	u32 *fb = display_init_window_a_pitch();
	gfx_init_ctxt(fb, 720, 1280, 720);
	gfx_con_init();

	// Initialize backlight PWM.
	display_backlight_pwm_init();
	display_backlight_brightness(100, 0);

	// Show AMS errors
	secmon_exo_check_panic();
	check_log();


	gfx_printf("\n\nPress POWER to power off\nPress VOLUME to boot RCM\n");
	msleep(250);


	do{
		u8 btn = btn_read();
		if(btn & BTN_POWER){
			power_set_state(POWER_OFF);
		}
		if(btn & (BTN_VOL_DOWN | BTN_VOL_UP)){
			power_set_state(REBOOT_RCM);
		}
	}while(true);

	// Halt BPMP if we managed to get out of execution.
	while (true)
		bpmp_halt();
}
