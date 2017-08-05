/* Hardware Specific Definitions for Data Acquisition - BMS hardware

Definitions for the Selected Microcontroller Board. Port definitions are for
the external perpiherals. Make sure that the settings for internal peripherals
do not clash with these. Check consistency of these with the ChaN FAT board.h
where I/O ports are defined for the SD card.

This is for the BMS hardware using the ET-STAMP-STM board.

The hardware and processor being used are defined in the makefile as
HARDWARE and BOARD.

Initial 19 December 2016

*/

/*
 * Copyright (C) 2016 K. Sarkies <ksarkies@internode.on.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HARDWARE_BMS_H_
#define HARDWARE_BMS_H_

#include <stdbool.h>
#include <stdint.h>

#define RTC_SOURCE RTC

/* Size of communications receive and transmit buffers. */
#define BUFFER_SIZE 128

/* Number of A/D converter channels available (STM32F103) */
#define NUM_INTERFACES  6
#define NUM_CHANNEL     1 + 2*NUM_INTERFACES

/* Timer parameters */
/* register value representing a PWM period of 50 microsec (5 kHz) */
#define PWM_PERIOD      14400

/* USART */
#define BAUDRATE        38400

/* Watchdog Timer Timeout Period in ms */
#define IWDG_TIMEOUT_MS 1500

/* Flash. Largest page size compatible with most families used.
(note only STM32F1xx,  STM32F05x have compatible memory organization). */
#define FLASH_PAGE_SIZE 2048

/* RTC select hardware RTC or software counter */
#define RTC_SOURCE      RTC

/* A/D Converter Channels */

#define ADC_CHANNEL_0            0
#define ADC_CHANNEL_1            1
#define ADC_CHANNEL_2            2
#define ADC_CHANNEL_3            3
#define ADC_CHANNEL_4            4
#define ADC_CHANNEL_5            5
#define ADC_CHANNEL_6            6
#define ADC_CHANNEL_7            7
#define ADC_CHANNEL_8           10
#define ADC_CHANNEL_9           11
#define ADC_CHANNEL_10          12
#define ADC_CHANNEL_11          13
#define ADC_CHANNEL_TEMPERATURE 14
/* GPIO Port Settings */
#define PA_ANALOGUE_INPUTS          GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |\
                                            GPIO5 | GPIO6 | GPIO7
#define PA_DIGITAL_INPUTS           GPIO11 | GPIO12
#define PA_DIGITAL_OUTPUTS          GPIO8 | GPIO13 | GPIO14 | GPIO15
#define PB_DIGITAL_INPUTS           GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |\
                                            GPIO5 | GPIO6 | GPIO7 | GPIO8 |\
                                            GPIO9 | GPIO10 | GPIO11
#define PC_ANALOGUE_INPUTS          GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4
#define PC_DIGITAL_OUTPUTS          GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 |\
                                            GPIO10 | GPIO11 | GPIO12 |\
                                            GPIO13

#endif

