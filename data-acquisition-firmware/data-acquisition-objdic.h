/* Data Acquisition Object Dictionary

This file defines the CANopen object dictionary variables made available to an
external PC and to other processing modules which may be CANopen devices or
tasks running on the same microcontroller.
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

#ifndef _DATA_ACQUISITION_OBJDIC_H_
#define _DATA_ACQUISITION_OBJDIC_H_

#define FIRMWARE_VERSION    "1.00"

/*--------------------------------------------------------------------------*/
/* Calibration factors to convert A/D measurements to physical entities. */

/* For current the scaling factor gives a value in 1/256 Amp precision.
Subtract this from the measured value and scale by this factor.
Then after averaging scale back by 4096 to give the values used here.
Simply scale back further by 256 to get the actual (floating point)
current. Thus the results here are 256 times the current in amperes. */

#define CURRENT_OFFSET 2028

/* Current scale: amplifier gain 10.4 times ref voltage 3.280 times 256 */

#define CURRENT_SCALE 8373

/* The voltage amplifier scale versions 1 and 2 are obsolete */
/* Voltage amplifier has different parameters for different board versions */
#if (VERSION==1)

/* Voltage offset: 5 (2 times local ref 2.5V) times the gain 1.803
times 256 times 4096 */

#define VOLTAGE_OFFSET 9453071

/* Voltage scale: amplifier gain 1.803 times ref voltage 3.3 times 256 */

#define VOLTAGE_SCALE 1523

#warning "Version 1 Selected"

#elif (VERSION==2)

/* Voltage offset: 5 (2 times local ref 2.5V) times the gain 1.833
times 256 times 4096 */

#define VOLTAGE_OFFSET 9611946

/* Voltage scale: amplifier gain 1.833 times ref voltage 3.3 times 256 */

#define VOLTAGE_SCALE 1548

#warning "Version 2 Selected"

/* This voltage amplifier scale is the latest and only version used */
#elif (VERSION==3)

/* Voltage offset: 5 (2 times local ref 2.5V) times the gain 1.679
times 256 times 4096 */

#define VOLTAGE_OFFSET 10565197

/* Voltage scale: amplifier gain 1.679 times ref voltage 3.3 times 256 */

#define VOLTAGE_SCALE 1418

#else
#error "Version is not defined"
#endif

/* Temperature measurement via LM335 for reference voltage 3.280V.
Scale is 3.28V over 10mV per degree C times 256.
Offset is 2.732V at 0 degrees C over 3.280 times 4096. */

#define TEMPERATURE_SCALE   328*256
#define TEMPERATURE_OFFSET  3412

/*--------------------------------------------------------------------------*/
/****** Object Dictionary Items *******/
/* Configuration items, updated externally, are stored to NVM */
/* Values must be initialized in set_global_defaults(). */

/*--------------------------------------------------------------------------*/
struct Config
{
/* Valid data block indicator */
    uint8_t validBlock;
/* Communications Control Variables */
    bool enableSend;            /* Any communications transmission occurs */
    bool measurementSend;       /* Measurements are transmitted */
    bool debugMessageSend;      /* Debug messages are transmitted */
/* Recording Control Variables */
    bool recording;             /* Recording of performance data */
/* Measurement Variables */
    uint32_t measurementInterval;   /* Time between measurements */
    uint8_t numberConversions;  /* Number of channels to be converted */
    uint8_t numberSamples;      /* Number of samples for averaging */
};

/* Map the configuration data also as a block of words.
Block size equal to a FLASH page (2048 bytes) to avoid erase problems.
Needed for reading and writing from Flash. */
#define CONFIG_BLOCK_SIZE       2048
union ConfigGroup
{
    uint8_t data[CONFIG_BLOCK_SIZE];
    struct Config config;
};

/*--------------------------------------------------------------------------*/
/* Prototypes */
/*--------------------------------------------------------------------------*/

void set_global_defaults(void);
uint32_t write_config_block(void);
bool is_recording(void);
uint16_t get_controls(void);

#endif

