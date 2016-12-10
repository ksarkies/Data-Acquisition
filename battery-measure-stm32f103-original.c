/* STM32F1 Test of SLA battery discharge

Circuits for current (bidirectional) and voltage (offset by minimum voltage).

Read the two channels continuously and compute average and variance.
Transmit via USART along with the count.
When a response is received, restart the averaging process.

The board used is the ET-STM32F103 but the test should work on a variety of
STM32F103 based hardware with access to ADC channels 0-7.
(On this board, ADC1 channels 2 and 3 (PA2, PA3) are shared with USART2 and two
jumpers J13,J14 need to be moved to access the analogue ports).

10 May 2013
*/

/*
 * This file is part of the Battery Management System project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) K. Sarkies <ksarkies@internode.on.net>
 *
 * This project is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f1/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/dispatch/nvic.h>
#include "buffer.h"

/* Local Prototypes */

void usart_print_int(int value);
void usart_print_hex(u16 value);
void usart_print_string(char *ch);
void timer_setup(void);
void adc_setup(void);
void dma_setup(void);
void gpio_setup(void);
void usart_setup(void);
void clock_setup(void);

#define BUFFER_SIZE 128
#define N_CONV 2

/* Globals */
u8 adc_complete = 0;
u32 v[N_CONV];
u16 n_sample = 0;
u32 avg[N_CONV];
u64 var[N_CONV];
u8 send_buffer[BUFFER_SIZE+3];
u8 receive_buffer[BUFFER_SIZE+3];

/*--------------------------------------------------------------------------*/

int main(void)
{
	u8 channel_array[16];
	u16 i = 0;

	clock_setup();
	gpio_setup();
	usart_setup();
	dma_setup();
	adc_setup();
	timer_setup();	
	buffer_init(send_buffer,BUFFER_SIZE);
	buffer_init(receive_buffer,BUFFER_SIZE);
	usart_enable_tx_interrupt(USART1);

/* Clear stats */
    n_sample = 0;
	for (i = 0; i < N_CONV; i++)
	{
        avg[i] = 0;
        var[i] = 0;
	}
/* Setup array of selected channels for conversion */
	for (i = 0; i < N_CONV; i++) channel_array[i] = i;
	adc_set_regular_sequence(ADC1, N_CONV, channel_array);
/* Start conversion and wait for conversion end. DMA into v[] */
	while (1)
	{
        adc_complete = 0;
		gpio_toggle(GPIOB, GPIO8);
		adc_start_conversion_regular(ADC1);
        while (! dma_get_interrupt_flag(DMA1,DMA_CHANNEL1, DMA_TCIF));
        gpio_toggle(GPIOB, GPIO11);
		/* Hang around until the next timer tick */
        for (i = 0; i < 50; i++)
        {
		    while (!timer_get_flag(TIM2, TIM_SR_CC1IF));
		    timer_clear_flag(TIM2, TIM_SR_CC1IF);
        }
	}

	return 0;
}

/*--------------------------------------------------------------------------*/

/* The processor system clock is established and the necessary peripheral
clocks are turned on */

void clock_setup(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

/*--------------------------------------------------------------------------*/

/* USART 1 is configured for 115200 baud, no flow control and interrupt */

void usart_setup(void)
{
	/* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN |
				    RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN);
	/* Enable the USART1 interrupt. */
	nvic_enable_irq(NVIC_USART1_IRQ);
	/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	/* Setup GPIO pin GPIO_USART1_RE_RX on GPIO port A for receive. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);
	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	/* Enable USART1 receive interrupts. */
	usart_enable_rx_interrupt(USART1);
	usart_disable_tx_interrupt(USART1);
	/* Finally enable the USART. */
	usart_enable(USART1);
}

/*--------------------------------------------------------------------------*/

/* GPIO Port B bits 8-15 setup for LED indicator outputs */

void gpio_setup(void)
{
	/* Enable GPIOB clock (for LED GPIOs). */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL,
			  GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_clear(GPIOB, GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
}

/*--------------------------------------------------------------------------*/

/* Enable DMA 1 Channel 1 to take conversion data from ADC 1, and also ADC 2 when the
ADC is used in dual mode. The ADC will dump a burst of data to memory each time, and we
need to grab it before the next conversions start. This must be called after each transfer
to reset the memory buffer to the beginning. */

void dma_setup(void)
{
	/* Enable DMA1 Clock */
	rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_DMA1EN);
	dma_channel_reset(DMA1,DMA_CHANNEL1);
	dma_set_priority(DMA1,DMA_CHANNEL1,DMA_CCR_PL_LOW);
/* We want all 32 bits from the ADC to include ADC2 data */
	dma_set_memory_size(DMA1,DMA_CHANNEL1,DMA_CCR_MSIZE_32BIT);
	dma_set_peripheral_size(DMA1,DMA_CHANNEL1,DMA_CCR_PSIZE_32BIT);
	dma_enable_memory_increment_mode(DMA1,DMA_CHANNEL1);
	dma_set_read_from_peripheral(DMA1,DMA_CHANNEL1);
/* The register to target is the ADC1 regular data register */
	dma_set_peripheral_address(DMA1,DMA_CHANNEL1,(u32) &ADC1_DR);
/* The array v[] receives the converted output */
	dma_set_memory_address(DMA1,DMA_CHANNEL1,(u32) v);
	dma_set_number_of_data(DMA1,DMA_CHANNEL1,N_CONV);
	dma_enable_channel(DMA1,DMA_CHANNEL1);
}

/*--------------------------------------------------------------------------*/

/* ADC1 is setup for scan mode. Single conversion does all selected
channels once through then stops. DMA enabled to collect data. */

void adc_setup(void)
{
	/* Enable clocks for ADCs */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN |
				    RCC_APB2ENR_AFIOEN | RCC_APB2ENR_ADC1EN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN |
				    RCC_APB2ENR_AFIOEN | RCC_APB2ENR_ADC2EN);
	nvic_enable_irq(NVIC_ADC1_2_IRQ);
/* Set port PA bits 0-7 for ADC1 to analogue input. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_ANALOG, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO7);
	/* Make sure the ADC doesn't run during config. */
	adc_off(ADC1);
	adc_off(ADC2);
	/* Configure ADC1 for multiple conversion of channels, stopping after the last one. */
	adc_enable_scan_mode(ADC1);
	adc_set_single_conversion_mode(ADC1);
	adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_SWSTART);
	adc_set_right_aligned(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
	adc_enable_dma(ADC1);
	adc_enable_eoc_interrupt(ADC1);
/* Power on and calibrate */
	adc_power_on(ADC1);
	/* Wait for ADC starting up. */
	u32 i;
	for (i = 0; i < 800000; i++)    /* Wait a bit. */
		__asm__("nop");
	adc_reset_calibration(ADC1);
	adc_calibration(ADC1);
}

/*--------------------------------------------------------------------------*/

/* Setup timer 2 to run through a period and to set a compare flag when the count
reaches a preset value based on the output compare channel 1. */
void timer_setup(void)
{
	/* Enable TIM2 clock. */
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);
	timer_reset(TIM2);
/* Timer global mode: - Divider 4, Alignment edge, Direction up */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT_MUL_4,
		       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_continuous_mode(TIM2);
	timer_set_period(TIM2, 0xFFFF);
	timer_enable_oc_output(TIM2, TIM_OC1);
	timer_disable_oc_clear(TIM2, TIM_OC1);
	timer_disable_oc_preload(TIM2, TIM_OC1);
	timer_set_oc_slow_mode(TIM2, TIM_OC1);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_TOGGLE);
	timer_set_oc_value(TIM2, TIM_OC1, 0x8FFF);
	timer_disable_preload(TIM2);
	timer_enable_counter(TIM2);
}

/*--------------------------------------------------------------------------*/

/* Print out a value in ASCII decimal form (ack Thomas Otto). */
void usart_print_int(int value)
{
	u8 i;
	u8 nr_digits = 0;
	char buffer[25];

	if (value < 0)
	{
		buffer_put(send_buffer, '-');
		value = value * -1;
	}
	if (value == 0) buffer[nr_digits++] = '0';
	else while (value > 0)
	{
		buffer[nr_digits++] = "0123456789"[value % 10];
		value /= 10;
	}
	for (i = nr_digits; i > 0; i--)
	{
		buffer_put(send_buffer, buffer[i-1]);
	}
	buffer_put(send_buffer, ' ');
}

/*--------------------------------------------------------------------------*/

/* Print out a value in ASCII hex form. */
void usart_print_hex(u16 value)
{
	u8 i;
	char buffer[25];

	for (i = 0; i < 4; i++)
	{
		buffer[i] = "0123456789ABCDEF"[value & 0xF];
		value >>= 4;
	}
	for (i = 4; i > 0; i--)
	{
		buffer_put(send_buffer, buffer[i-1]);
	}
	buffer_put(send_buffer, ' ');
}

/*--------------------------------------------------------------------------*/

/* Print a String. */
void usart_print_string(char *ch)
{
  	while(*ch)
	{
     	buffer_put(send_buffer, *ch);
     	ch++;
  	}
}

/*--------------------------------------------------------------------------*/

/* Respond to ADC EOC at end of scan and send data block.
Print the result in decimal and separate with an ASCII dash.*/
void adc1_2_isr(void)
{
	static u8 i = 0;
	gpio_toggle(GPIOB, GPIO9);
/* Build statistics */
    for (i = 0; i < N_CONV; i++)
    {
        avg[i] += v[i];
        var[i] += v[i]*v[i];
        n_sample++;
    }
    if ((n_sample > 256) || (buffer_get(receive_buffer) != 0x100))
    {
		gpio_toggle(GPIOB, GPIO10);
/* Enable USART to send */
/* Send off accumulated data as a single line, and reset all stats */
        for (i = 0; i < N_CONV; i++)
        {
            usart_print_hex(n_sample);
            usart_print_hex(avg[i]);
            usart_print_hex(var[i]);
        }
        usart_print_string("\r\n");
        usart_enable_tx_interrupt(USART1);
/* Clear stats */
        n_sample = 0;
        for (i = 0; i < N_CONV; i++)
        {
            avg[i] = 0;
            var[i] = 0;
        }
    }
/* Clear DMA to restart at beginning of data array */
	dma_setup();
    adc_complete = 1;
}

/*--------------------------------------------------------------------------*/

/* Find out what interrupted and get or send data as appropriate */
void usart1_isr(void)
{
	static u16 data;

	/* Check if we were called because of RXNE. */
	if (usart_get_flag(USART1,USART_SR_RXNE))
	{
		/* If buffer full we'll just drop it */
		buffer_put(receive_buffer, (u8) usart_recv(USART1));
	}
	/* Check if we were called because of TXE. */
	if (usart_get_flag(USART1,USART_SR_TXE))
	{
		/* If buffer empty, disable the tx interrupt */
		data = buffer_get(send_buffer);
		if ((data & 0xFF00) > 0) usart_disable_tx_interrupt(USART1);
		else usart_send(USART1, (data & 0xFF));
	}
}

