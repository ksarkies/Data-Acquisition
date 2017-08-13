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

void hardwareInit(void);
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
void setSwitch(uint8_t device, uint8_t setting);
uint8_t getSwitchControlBits(void);
void setSwitchControlBits(uint8_t settings);
void overCurrentReset(uint32_t interface);
void overCurrentRelease(uint32_t interface);

#endif

