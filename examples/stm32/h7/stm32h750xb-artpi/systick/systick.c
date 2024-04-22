/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

static void gpio_setup(void)
{
	/* Enable GPIOB clock. */
	rcc_periph_clock_enable(RCC_GPIOI);

	/* Set GPIO8 (in GPIO port I) to 'output push-pull' for the LEDs. */
	gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
}

void sys_tick_handler(void)
{
	static uint32_t temp32 = 0;
	temp32++;

	/* We call this handler every 1ms so 1000ms = 1s on/off. */
	if (temp32 >= 1000) {
		gpio_toggle(GPIOI, GPIO8); /* LED2 on/off */
		temp32 = 0;
	}
}

struct rcc_pll_config pll_config = {
	.sysclock_source = RCC_PLL,
	.pll_source = RCC_PLLCKSELR_PLLSRC_HSE,
	.hse_frequency = 25000000,
	.pll1 = {5, 192, 2, 2, 2},			/*p:480M, q:480M, r:480M*/
	.pll2 = {2,  64, 2, 2, 4},			/*p:400M, q:400M, r:200M*/
	.pll3 = {5, 160, 8, 8, 24}, 		/*p:100M, q:100M, r:33.3M*/
	.core_pre = RCC_D1CFGR_D1CPRE_BYP, 	/*480Mhz max*/
	.hpre = RCC_D1CFGR_D1HPRE_DIV2, 	/*240Mhz max*/
	.ppre1 = RCC_D2CFGR_D2PPRE_DIV2, 	/*120Mhz max*/
	.ppre2 = RCC_D2CFGR_D2PPRE_DIV2, 	/*120Mhz max*/
	.ppre3 = RCC_D1CFGR_D1PPRE_DIV2,	/*120Mhz max*/
	.ppre4 = RCC_D3CFGR_D3PPRE_DIV2,	/*120Mhz max*/
	.flash_waitstates = FLASH_ACR_LATENCY_4WS,
	.voltage_scale = PWR_VOS_SCALE_0,
	.power_mode = PWR_SYS_LDO,
	.smps_level = PWR_CR3_SMPSLEVEL_VOS
};

int main(void)
{
	rcc_clock_setup_pll(&pll_config);
	gpio_setup();

	/* 480Mhz => 480000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);

	/* 480000000/480000 = 1000 overflows per second - every 1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(479999);

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();

	while (1){ };
}
