/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Damian Miller <damian.m.miller@gmail.com>
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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>

#define USART_ECHO_EN				1
#define SEND_BUFFER_SIZE			256

#define FLASH_BANK_SIZE				0x80000
#define FLASH_SECTOR_SIZE			0x20000
#define FLASH_OPERATION_ADDRESS		(FLASH_BASE + FLASH_BANK_SIZE)
#define FLASH_WRONG_ERASE			0x01
#define FLASH_WRONG_PROGRAM			0x02
#define FLASH_WRONG_DATA_WRITTEN	0x80
#define RESULT_OK					0

/*hardware initialization*/
static void init_system(void);
static void init_uart(void);
/*usart operations*/
static void usart_send_string(uint32_t usart, char *string, uint16_t str_size);
static void usart_get_string(uint32_t usart, char *string, uint16_t str_max_size);
/*flash operations*/
static uint32_t flash_program_data(uint32_t start_address, uint8_t *input_data,
									uint16_t num_elements);
static void flash_read_data(uint32_t start_address, uint16_t num_elements, uint8_t *output_data);
/*local functions to work with strings*/
static void local_ltoa_hex(uint32_t value, uint8_t *out_string);

int main(void)
{
	uint32_t result = 0;
	char str_send[SEND_BUFFER_SIZE], str_verify[SEND_BUFFER_SIZE];

	init_system();

	while (1) {
		usart_send_string(UART4, "Please enter string to write into Flash memory:\n\r",
							SEND_BUFFER_SIZE);
		usart_get_string(UART4, str_send, SEND_BUFFER_SIZE);
		result = flash_program_data(FLASH_OPERATION_ADDRESS, (uint8_t *)str_send,
									SEND_BUFFER_SIZE);

		switch (result) {
		case RESULT_OK: /*everything ok*/
			usart_send_string(UART4, "Verification of written data: ",
								SEND_BUFFER_SIZE);
			flash_read_data(FLASH_OPERATION_ADDRESS, SEND_BUFFER_SIZE,
								(uint8_t *)str_verify);
			usart_send_string(UART4, str_verify, SEND_BUFFER_SIZE);
			break;
		case FLASH_WRONG_DATA_WRITTEN:
			/*data read from Flash is different than written data*/
			usart_send_string(UART4, "Wrong data written into flash memory",
								SEND_BUFFER_SIZE);
			break;
		default: /*wrong flags' values in Flash Status Register (FLASH_SR)*/
			usart_send_string(UART4, "Wrong value of FLASH_SR: ", SEND_BUFFER_SIZE);
			local_ltoa_hex(result, (uint8_t *)str_send);
			usart_send_string(UART4, str_send, SEND_BUFFER_SIZE);
			break;
		}
		/*send end_of_line*/
		usart_send_string(UART4, "\r\n", 3);
	}
	return 0;
}

static struct rcc_pll_config pll_config = {
	.sysclock_source = RCC_PLL1,
	.pll_source = RCC_PLLCKSELR_PLLSRC_HSE,
	.hse_frequency = 25000000,
	.pll1 = {5, 192, 2, 2, 2},			/*p:480M, q:480M, r:480M*/
	.pll2 = {2,  64, 2, 2, 4},			/*p:400M, q:400M, r:200M*/
	.pll3 = {5, 160, 8, 8, 24},			/*p:100M, q:100M, r:33.3M*/
	.core_pre = RCC_D1CFGR_D1CPRE_BYP,	/*480Mhz max*/
	.hpre = RCC_D1CFGR_D1HPRE_DIV2,		/*240Mhz max*/
	.ppre1 = RCC_D2CFGR_D2PPRE_DIV2,	/*120Mhz max*/
	.ppre2 = RCC_D2CFGR_D2PPRE_DIV2,	/*120Mhz max*/
	.ppre3 = RCC_D1CFGR_D1PPRE_DIV2,	/*120Mhz max*/
	.ppre4 = RCC_D3CFGR_D3PPRE_DIV2,	/*120Mhz max*/
	.flash_waitstates = FLASH_ACR_LATENCY_4WS,
	.voltage_scale = PWR_VOS_SCALE_0,
	.power_mode = PWR_SYS_LDO,
	.smps_level = PWR_CR3_SMPSLEVEL_VOS
};

static void init_system(void)
{
	/* setup SYSCLK to work with 64Mhz HSI */
	rcc_clock_setup_pll(&pll_config);
	init_uart();
}

static void init_uart(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOI);
	rcc_periph_clock_enable(RCC_UART4);

	/* Setup GPIO pins for UART4 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0);

	/* Setup GPIO pins for UART4 receive. */
	gpio_mode_setup(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);

	gpio_set_output_options(GPIOI, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO9);

	/* Setup UART4 TX and RX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF8, GPIO0);
	gpio_set_af(GPIOI, GPIO_AF8, GPIO9);

	/* Setup UART4 parameters. */
	usart_set_baudrate(UART4, 115200);
	usart_set_databits(UART4, 8);
	usart_set_stopbits(UART4, USART_STOPBITS_1);
	usart_set_mode(UART4, USART_MODE_TX_RX);
	usart_set_parity(UART4, USART_PARITY_NONE);
	usart_set_flow_control(UART4, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(UART4);
}

static void usart_send_string(uint32_t usart, char *string, uint16_t str_size)
{
	uint16_t iter = 0;
	do {
		usart_send_blocking(usart, string[iter++]);
	} while (string[iter] != 0 && iter < str_size);
}

static void usart_get_string(uint32_t usart, char *out_string, uint16_t str_max_size)
{
	uint8_t sign = 0;
	uint16_t iter = 0;

	while (iter < str_max_size) {
		sign = usart_recv_blocking(usart);

#if USART_ECHO_EN == 1
		usart_send_blocking(usart, sign);
#endif

		if (sign != '\n' && sign != '\r') {
			out_string[iter++] = sign;
		} else {
			out_string[iter] = 0;
			usart_send_string(UART4, "\r\n", 3);
			break;
		}
	}
}

static uint32_t flash_program_data(uint32_t start_address, uint8_t *input_data,
									uint16_t num_elements)
{
	uint16_t iter;
	uint8_t sector;
	enum flash_bank bank;

	/*check if start_address is in proper range*/
	if ((start_address - FLASH_BASE) >= (FLASH_BANK_SIZE * 2)) {
		return 1;
	}

	/* Same as stm32h743xx, STM32H750XB has 2 banks actually */
	if (start_address - FLASH_BASE > FLASH_BANK_SIZE) {
		sector = (start_address - FLASH_BASE - FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
		bank = FLASH_BANK_2;
	} else {
		sector = (start_address - FLASH_BASE) / FLASH_SECTOR_SIZE;
		bank = FLASH_BANK_1;
	}

	flash_unlock(bank);

	/*Erasing sector*/
	flash_erase_sector(bank, sector, FLASH_CR_PROGRAM_X8);
	if (flash_wait_for_last_operation(bank) == false) {
		return FLASH_WRONG_ERASE;
	}

	/*programming flash memory*/
	if (false == flash_program(start_address, input_data, num_elements)) {
		return FLASH_WRONG_PROGRAM;
	}

	for (iter = 0; iter < num_elements; iter += 4) {
		/*verify if correct data is programmed*/
		if (*((uint32_t *)(start_address+iter)) != *((uint32_t *)(input_data + iter))) {
			return FLASH_WRONG_DATA_WRITTEN;
		}
	}

	return 0;
}

static void flash_read_data(uint32_t start_address, uint16_t num_elements, uint8_t *output_data)
{
	uint16_t iter;
	uint32_t *memory_ptr = (uint32_t *)start_address;

	for (iter = 0; iter < num_elements/4; iter++) {
		*(uint32_t *)output_data = *(memory_ptr + iter);
		output_data += 4;
	}
}

static void local_ltoa_hex(uint32_t value, uint8_t *out_string)
{
	uint8_t iter;

	/*end of string*/
	out_string += 8;
	*(out_string--) = 0;

	for (iter = 0; iter < 8; iter++) {
		*(out_string--) = (((value&0xf) > 0x9) ?
			(0x40 + ((value&0xf) - 0x9)) : (0x30 | (value&0xf)));
		value >>= 4;
	}
}
