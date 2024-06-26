/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Chuck McManis <cmcmanis@mcmanis.com>
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

/*
 * This is a pretty classic ring buffer for characters
 */
#define BUFLEN 127

static uint16_t start_ndx;
static uint16_t end_ndx;
static char buf[BUFLEN+1];
#define buf_len ((end_ndx - start_ndx) % BUFLEN)
static inline int inc_ndx(int n) { return ((n + 1) % BUFLEN); }
static inline int dec_ndx(int n) { return (((n + BUFLEN) - 1) % BUFLEN); }


static void clock_setup(void)
{
	/* Enable GPIOA, GPIOI clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOI);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for UART4. */
	rcc_periph_clock_enable(RCC_UART4);
}

static void usart_setup(void)
{
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

	/* Setup GPIO pins for UART4 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0);

	/* Setup GPIO pins for UART4 receive. */
	gpio_mode_setup(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_output_options(GPIOI, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO9);

	/* Setup UART4 TX and RX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF8, GPIO0);
	gpio_set_af(GPIOI, GPIO_AF8, GPIO9);
}

int main(void)
{
	int i, j;

	clock_setup();
	gpio_setup();
	usart_setup();
	printf("\nStandard I/O Example.\n");

	/* Blink the LED (PD12) on the board with every transmitted byte. */
	while (1) {
		int delay = 0;
		char local_buf[32];

		gpio_toggle(GPIOI, GPIO8);	/* LED on/off */
		do {
			printf("Enter the delay constant for blink : ");
			fflush(stdout);
			fgets(local_buf, 32, stdin);
			delay = atoi(local_buf);
			if (delay <= 0) {
				printf("Error: expected a delay > 0\n");
			}
		} while (delay <= 0);

		printf("Blinking with a delay of %d\n", delay);
		for (j = 0; j < 1000; j++) {
			gpio_toggle(GPIOI, GPIO8);
			for (i = 0; i < delay; i++) {	/* Wait a bit. */
				__asm__("NOP");
			}
		}
	}
	return 0;
}

/*
 * To implement the STDIO functions you need to create
 * the _read and _write functions and hook them to the
 * USART you are using. This example also has a buffered
 * read function for basic line editing.
 */
int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);
void get_buffered_line(void);

/* back up the cursor one space */
static inline void back_up(void)
{
	end_ndx = dec_ndx(end_ndx);
	usart_send_blocking(UART4, '\010');
	usart_send_blocking(UART4, ' ');
	usart_send_blocking(UART4, '\010');
}

/*
 * A buffered line editing function.
 */
void
get_buffered_line(void) {
	char	c;

	if (start_ndx != end_ndx) {
		return;
	}
	while (1) {
		c = usart_recv_blocking(UART4);
		if (c == '\r') {
			buf[end_ndx] = '\n';
			end_ndx = inc_ndx(end_ndx);
			buf[end_ndx] = '\0';
			usart_send_blocking(UART4, '\r');
			usart_send_blocking(UART4, '\n');
			return;
		}
		/* ^H or DEL erase a character */
		if ((c == '\010') || (c == '\177')) {
			if (buf_len == 0) {
				usart_send_blocking(UART4, '\a');
			} else {
				back_up();
			}
		/* ^W erases a word */
		} else if (c == 0x17) {
			while ((buf_len > 0) &&
					(!(isspace((int) buf[end_ndx])))) {
				back_up();
			}
		/* ^U erases the line */
		} else if (c == 0x15) {
			while (buf_len > 0) {
				back_up();
			}
		/* Non-editing character so insert it */
		} else {
			if (buf_len == (BUFLEN - 1)) {
				usart_send_blocking(UART4, '\a');
			} else {
				buf[end_ndx] = c;
				end_ndx = inc_ndx(end_ndx);
				usart_send_blocking(UART4, c);
			}
		}
	}
}


/*
 * Called by libc stdio fwrite functions
 */
int
_write(int fd, char *ptr, int len)
{
	int i = 0;

	/*
	 * Write "len" of char from "ptr" to file id "fd"
	 * Return number of char written.
	 *
	 * Only work for STDOUT, STDIN, and STDERR
	 */
	if (fd > 2) {
		return -1;
	}
	while (*ptr && (i < len)) {
		usart_send_blocking(UART4, *ptr);
		if (*ptr == '\n') {
			usart_send_blocking(UART4, '\r');
		}
		i++;
		ptr++;
	}
	return i;
}

/*
 * Called by the libc stdio fread fucntions
 *
 * Implements a buffered read with line editing.
 */
int
_read(int fd, char *ptr, int len)
{
	int	my_len;

	if (fd > 2) {
		return -1;
	}

	get_buffered_line();
	my_len = 0;
	while ((buf_len > 0) && (len > 0)) {
		*ptr++ = buf[start_ndx];
		start_ndx = inc_ndx(start_ndx);
		my_len++;
		len--;
	}
	return my_len; /* return the length we got */
}
