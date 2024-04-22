/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
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

/* Linker script for rt-thread ARTPI(STM32H750XBH6, 128K flash, 512K RAM). */

/* Define memory regions. */
MEMORY
{
	ram (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 128K
	ram2 (rwx) : ORIGIN = 0x30000000, LENGTH = 128K
	ram3 (rwx) : ORIGIN = 0x30020000, LENGTH = 128K
	ram4 (rwx) : ORIGIN = 0x30040000, LENGTH = 32K
	ram5 (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
	ccm (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Include the common ld script. */
INCLUDE cortex-m-generic.ld

