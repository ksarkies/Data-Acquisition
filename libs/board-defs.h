/* Board Definitions for Data Acquisition

Definitions for the Selected Microcontroller Board. Port definitions are for
the external perpiherals. Make sure that the settings for internal peripherals
do not clash with these. Check consistency of these with the ChaN FAT board.h
where I/O ports are defined for the SD card.

Either the ET-STM32F103 or the ET-STAMP-STM board can be used.

Initial 19 December 2016

*/

/*
 * Copyright 2016 K. Sarkies <ksarkies@internode.on.net>
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

#ifndef BOARD_DEFS_H_
#define BOARD_DEFS_H_

/* A/D Converter Channels */

#if defined USE_ET_STM32F103

#define ADC_CHANNEL_0            0
#define ADC_CHANNEL_1            1
#define ADC_CHANNEL_2            2
#define ADC_CHANNEL_3            3
#define ADC_CHANNEL_4           10
#define ADC_CHANNEL_5           11
#define ADC_CHANNEL_6            4
#define ADC_CHANNEL_7            5
#define ADC_CHANNEL_8           12
#define ADC_CHANNEL_9           13
#define ADC_CHANNEL_10          14
#define ADC_CHANNEL_11          15
#define ADC_CHANNEL_TEMPERATURE  6

#elif defined USE_ET_STAMP_STM32

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

#else
#error "unsupported board"
#endif

/* GPIO Port Settings */
/* The board being used is defined in the makefile */

#if defined USE_ET_STM32F103

#define PB_DIGITAL_OUTPUTS          GPIO8  | GPIO9 | GPIO10 | GPIO11 |\
                                    GPIO12 | GPIO13 | GPIO14 | GPIO15

#elif defined USE_ET_STAMP_STM32

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

#else
#error "unsupported board"
#endif

#endif

