/* Hardware Access functions.

Defines necessary to specify the processor and board used.

This file provides a means to access functions specific to particular underlying
processor and hardware. The functions are intended to be general in form but
refer to more specific functions provided elsewhere.

K. Sarkies, 5 August 2017
*/

/*
 * Copyright (C) 2017 K. Sarkies <ksarkies@internode.on.net>
 *
 * This project is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdint.h>
#include <stdbool.h>

#if (BOARD==BMS)
#include "hardware-bms.h"

#else
#error "unsupported hardware"
#endif

#define HIGH        1
#define LOW         0

/*--------------------------------------------------------------------------*/
/* Prototypes */
/*--------------------------------------------------------------------------*/

void hardware_init(void);
void set_adc_channel_sequence(uint8_t adc, uint8_t numberChannels, uint8_t* channelArray);
void start_adc_conversion(uint8_t adc);
void cli(void);
void sei(void);
void comms_enable_tx_interrupt(uint8_t enable);
void flash_read_data(uint32_t *flashBlock, uint8_t *dataBlock, uint16_t size);
uint32_t flash_write_data(uint32_t *flashBlock, uint8_t *dataBlock, uint16_t size);
uint32_t get_milliseconds_count();
uint32_t get_seconds_count();
void set_seconds_count(uint32_t time);
uint32_t get_delay_count();
void set_delay_count(uint32_t time);
bool adc_eoc_is_set(void);
uint32_t adc_value(uint8_t channel);
void clock_setup(void);
void gpio_setup(void);
void systick_setup(void);
void adc_setup(void);
void dma_adc_setup(void);
void exti_setup(uint32_t exti_enables, uint32_t port);
void rtc_setup(void);
void usart1_setup(void);
void peripheral_enable(void);
void peripheral_disable(void);
void set_switch(uint8_t device, uint8_t setting);
uint8_t get_switch_control_bits(void);
void set_switch_control_bits(uint8_t settings);
void overcurrent_reset(uint32_t interface);
void overcurrent_release(uint32_t interface);

#endif

