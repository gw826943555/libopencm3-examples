/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Damjan Marion <damjan.marion@gmail.com>
 * Copyright (C) 2011 Mark Panajotovic <marko@electrontube.org>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
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
#include <libopencm3/stm32/gpio.h>

static void clock_setup(void)
{
	/* Enable GPIOC GPIOI clock. */
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOI);
}

static void gpio_setup(void)
{
	/* Set GPIOI_8, GPIOC_15 to 'output'. */
	/* Using API functions: */
	gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
}

int main(void)
{
	clock_setup();
	gpio_setup();

	/* Set two LEDs for wigwag effect when toggling. */
	gpio_clear(GPIOC, GPIO15);
	gpio_set(GPIOI, GPIO8);


	/* Blink the LEDs (GPIOI_8 and GPIOC_15) on the board. */
	while (1) {
		/* Toggle LEDs. */
		gpio_toggle(GPIOC, GPIO15);
		gpio_toggle(GPIOI, GPIO8);
		for (int i = 0; i < 3200000; i++) { /* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
