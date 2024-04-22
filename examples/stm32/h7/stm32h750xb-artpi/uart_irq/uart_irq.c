/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2013-2015 Piotr Esden-Tempski <piotr@esden.net>
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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

static void clock_setup(void)
{
	/* Enable GPIOA, GPIOI clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOI);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for UART4. */
	rcc_periph_clock_enable(RCC_UART4);
}

static void uart_setup(void)
{
	/* Enable the UART4 interrupt. */
	nvic_enable_irq(NVIC_UART4_IRQ);

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

	/* Enable UART4 Receive interrupt. */
	usart_enable_rx_interrupt(UART4);

	/* Finally enable the USART. */
	usart_enable(UART4);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO8 on GPIO port I for LED. */
	gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
}

int main(void)
{
	clock_setup();
	gpio_setup();
	uart_setup();

	while (1) {
		__asm__("DSB");
		__asm__("WFI");
	}

	return 0;
}

void uart4_isr(void)
{
	static uint8_t data = 'A';

	/* Check if we were called because of RXNE. */
	if (usart_get_flag(UART4, USART_ISR_RXNE) == true) {

		/* Indicate that we got data. */
		gpio_toggle(GPIOI, GPIO8);

		/* Retrieve the data from the peripheral. */
		data = usart_recv(UART4);

		/* Enable transmit interrupt so it sends back the data. */
		usart_enable_tx_interrupt(UART4);
	}

	/* Check if we were called because of TXE. */
	if (usart_get_flag(UART4, USART_ISR_TXE) ==true) {

		/* Put data into the transmit register. */
		usart_send(UART4, data);

		/* Disable the TXE interrupt as we don't need it anymore. */
		usart_disable_tx_interrupt(UART4);
	}
}
