/*  Defines necessary to specify the processor and board used.

K. Sarkies, 24 November 2016
*/

/*
 * Copyright (C) K. Sarkies <ksarkies@internode.on.net>
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

#define HIGH        1
#define LOW         0

#define RTC_SOURCE RTC

/* Size of communications receive and transmit buffers. */
#define BUFFER_SIZE 128

/* Number of A/D converters available. */
#define NUM_CHANNEL 13

/* 72MHz clock rate divided by 8 and 1000 to set 1 ms count period for systick */
#define MS_COUNT    9000

/* Flash. Largest page size compatible with most families used.
(note only STM32F1xx,  STM32F05x have compatible memory organization). */
#define FLASH_PAGE_SIZE 2048

/*--------------------------------------------------------------------------*/
/* Prototypes */
/*--------------------------------------------------------------------------*/

void cli(void);
void sei(void);
void commsEnableTxInterrupt(uint8_t enable);
void flashReadData(uint32_t *flashBlock, uint8_t *dataBlock, uint16_t size);
uint32_t flashWriteData(uint32_t *flashBlock, uint8_t *dataBlock, uint16_t size);
uint32_t getMilliSecondsCount();
uint32_t getSecondsCount();
void setSecondsCount(uint32_t time);
uint32_t getDelayCount();
void setDelayCount(uint32_t time);
uint8_t adcEOC(void);
uint32_t adcValue(uint8_t channel);
void clock_setup(void);
void gpio_setup(void);
void systickSetup(void);
void adc_setup(void);
void dma_adc_setup(void);
void exti_setup(uint32_t exti_enables, uint32_t port);
void rtc_setup(void);
void usart1_setup(void);
void peripheral_enable(void);
void peripheral_disable(void);

#endif

