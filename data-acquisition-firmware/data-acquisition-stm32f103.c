/**
@mainpage STM32F1 Data Acquisition
@version 1.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 9 December 2016

@brief Firmware for data acquisition framework and applications.

Firmware for data acquisition with the intention of providing a base for
building a variety of such applications.

Used initially for measurement of Lead Acid battery discharge characteristics.

Measure current (bidirectional) and voltage (offset by minimum voltage).
Read the two channels continuously and compute average and variance.
Transmit via USART along with the count, and record on the SD card.

Initially uses the Battery Management System hardware components.

libopencm3 is used for hardware interface to the STM32F103 CM3 microcontroller
ChaN FAT is used for the SD Card recording.

Initial 23 November 2016

09/12/2016: libopencm3

09/12/2016: ChaN FatFS R0.12b
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include "../libs/buffer.h"
#include "../libs/hardware.h"
#include "../libs/comms.h"
#include "../libs/file.h"
#include "../libs/timelib.h"
#include "../libs/board-defs.h"
#include "data-acquisition-stm32f103.h"
#include "data-acquisition-objdic.h"

#include <stdbool.h>

/* Local Prototypes */
static void parseCommand(uint8_t* line);

/* Globals */
static uint8_t writeFileHandle;
static uint8_t readFileHandle;
static char writeFileName[12];
static char readFileName[12];

/* These configuration variables are part of the Object Dictionary. */
/* This is defined in power-management-objdic and is updated in response to
received messages. */
extern union ConfigGroup configData;

/*--------------------------------------------------------------------------*/

int main(void)
{
    setGlobalDefaults();
    clock_setup();
    gpio_setup();
    systickSetup();
    rtc_setup();

    dma_adc_setup();
    adc_setup();
    uint8_t channel_array[NUM_CHANNEL];
    channel_array[0] = ADC_CHANNEL_0;
    channel_array[1] = ADC_CHANNEL_1;
    channel_array[2] = ADC_CHANNEL_2;
    channel_array[3] = ADC_CHANNEL_3;
    channel_array[4] = ADC_CHANNEL_4;
    channel_array[5] = ADC_CHANNEL_5;
    channel_array[6] = ADC_CHANNEL_6;
    channel_array[7] = ADC_CHANNEL_7;
    channel_array[8] = ADC_CHANNEL_8;
    channel_array[9] = ADC_CHANNEL_9;
    channel_array[10] = ADC_CHANNEL_10;
    channel_array[11] = ADC_CHANNEL_11;
    channel_array[12] = ADC_CHANNEL_TEMPERATURE;
    adc_set_regular_sequence(ADC1, NUM_CHANNEL, channel_array);

    uint16_t i = 0;
    uint32_t avg[NUM_CHANNEL];
//    uint64_t var[NUM_CHANNEL];
    uint32_t current[NUM_INTERFACES];
    uint64_t voltage[NUM_INTERFACES];

    setDelayCount(configData.config.measurementInterval);

    usart1_setup();
    init_comms_buffers();

    init_file_system();
    writeFileHandle = 0xFF;
    readFileHandle = 0xFF;
    writeFileName[0] = 0;
    readFileName[0] = 0;

/* Main event loop */
	while (1)
	{
/* -------- CLI ------------*/
/* Command Line Interface receiver interpretation.
 Process incoming commands as they appear on the serial input.*/
        static uint8_t line[80];
        static uint8_t characterPosition = 0;
        if (receive_data_available())
        {
            uint8_t character = get_from_receive_buffer();
            if ((character == 0x0D) || (character == 0x0A) || (characterPosition > 78))
            {
                line[characterPosition] = 0;
                characterPosition = 0;
                parseCommand(line);
            }
            else line[characterPosition++] = character;
        }
        if (getDelayCount() > 0xFFFFFF) setDelayCount(configData.config.measurementInterval);

/* -------- Measurements --------- */
        if (getDelayCount() == 0)
        {

/* Clear stats and setup array of selected channels for conversion */
            uint8_t i = 0;
	        for (i = 0; i < NUM_CHANNEL; i++)
	        {
                avg[i] = 0;
//                var[i] = 0;
	        }
/* Run a burst of samples and average */
            uint8_t sample = 0;
            uint8_t numSamples = configData.config.numberSamples;
            if (numSamples < 1) numSamples = 1;
            for (sample = 0; sample < numSamples; sample++)
            {
/* Start conversion and wait for conversion end. */
	            adc_start_conversion_regular(ADC1);
                while (! adcEOC());
	            for (i = 0; i < NUM_CHANNEL; i++)
	            {
                    avg[i] += adcValue(i);
//                    var[i] += avg[i]*avg[i];
	            }
            }
            int16_t temperature = ((avg[12]/numSamples-TEMPERATURE_OFFSET)
                                        *TEMPERATURE_SCALE)/4096;
            uint8_t numInterfaces = configData.config.numberConversions-1;
            if (numInterfaces > NUM_INTERFACES) numInterfaces = NUM_INTERFACES;
            for (i=0; i < numInterfaces; i++)
            {
                uint8_t k = i+i;
                current[i] = ((avg[k]/numSamples-CURRENT_OFFSET)*CURRENT_SCALE)/4096;
                voltage[i] = (avg[k+1]/numSamples*VOLTAGE_SCALE+VOLTAGE_OFFSET)/4096;
            }
/* ------------- Transmit and save to file -----------*/
/* Send out a time string */
            char timeString[20];
            putTimeToString(timeString);
            sendString("pH",timeString);
            recordString("pH",timeString,writeFileHandle);
/* Send out temperature measurement. */
            sendResponse("dT",temperature);
            recordSingle("dT",temperature,writeFileHandle);
/* Send off accumulated data */
            char id[4];
            id[0] = 'd';
            id[1] = 'B';
            id[3] = 0;
            for (i=0; i < numInterfaces; i++)
            {
                id[2] = '1'+i;
                dataMessageSend(id, current[i], voltage[i]);
                recordDual(id, current[i], voltage[i], writeFileHandle);
            }
        }
	}

	return 0;
}

/*--------------------------------------------------------------------------*/
/** @brief Parse a command line and act on it.

Commands are single line ASCII text strings consisting of a category character
(a=action, d=data request, p=parameter, f=file) followed by command character
and an arbitrary length set of parameters (limited to 80 character line length
in total).

Unrecognizable messages are just discarded.

@param[in] line: uint8_t* pointer to the command line in ASCII
*/

void parseCommand(uint8_t* line)
{
/* ======================== Action commands ========================  */
/**
Action Commands */
    if (line[0] == 'a')
    {
/**
 */
        switch (line[1])
        {
/* Write the current configuration block to FLASH */
        case 'W':
            {
                writeConfigBlock();
                break;
            }
/* Request identification string with version sent back.  */
        case 'E':
            {
                char ident[35] = "Data Acquisition System,";
                stringAppend(ident,FIRMWARE_VERSION);
                sendString("dE",ident);
                break;
            }
        }
    }
/* ======================== Data request commands ================  */
/**
Data Request Commands */
    else if (line[0] == 'd')
    {
        switch (line[1])
        {
/**
Return the internal time.
 */
        case 'H':
            {
                char timeString[20];
                putTimeToString(timeString);
                sendString("pH",timeString);
            }
        }
    }
/* ======================== Parameter commands ================  */
/**
Parameter Setting Commands */
    else if (line[0] == 'p')
    {
        switch (line[1])
        {
/* c-, c+ Turn communications sending on or off */
        case 'c':
            {
                if (line[2] == '-') configData.config.enableSend = false;
                else if (line[2] == '+') configData.config.enableSend = true;
                break;
            }
/* d-, d+ Turn on debug messages */
        case 'd':
            {
                if (line[2] == '+') configData.config.debugMessageSend = true;
                if (line[2] == '-') configData.config.debugMessageSend = false;
                break;
            }
/* Hxxxx Set time from an ISO 8601 formatted string. */
        case 'H':
            {
                setTimeFromString((char*)line+2);
                break;
            }
/* M-, M+ Turn on/off data messaging (mainly for debug) */
        case 'M':
            {
                if (line[2] == '-') configData.config.measurementSend = false;
                else if (line[2] == '+') configData.config.measurementSend = true;
                break;
            }
/* r-, r+ Turn recording on or off */
        case 'r':
            {
                if (line[2] == '-') configData.config.recording = false;
                else if ((line[2] == '+') && (writeFileHandle < 0x0FF))
                    configData.config.recording = true;
                break;
            }
        }
    }

/* ======================== File commands ================ */
/*
F           - get free clusters
Wfilename   - Open file for read/write. Filename is 8.3 string style. Returns handle.
Rfilename   - Open file read only. Filename is 8.3 string style. Returns handle.
Xfilename   - Delete the file. Filename is 8.3 string style.
Cxx         - Close file. x is the file handle.
Gxx         - Read a record from read or write file.
Ddirname    - Get a directory listing. Directory name is 8.3 string style.
d[dirname]  - Get the first (if dirname present) or next entry in directory.
s           - Get status of open files and configData.config.recording flag
M           - Mount the SD card.
All commands return an error status byte at the end.
Only one file for writing and a second for reading is possible.
Data is not written to the file externally. */

/* File Commands */
    else if (line[0] == 'f')
    {
        switch (line[1])
        {
/* F Return number of free clusters followed by the cluster size in sectors. */
            case 'F':
            {
                uint32_t freeClusters = 0;
                uint32_t sectorsPerCluster = 0;
                uint8_t fileStatus = get_free_clusters(&freeClusters, &sectorsPerCluster);
                dataMessageSend("fF",freeClusters,sectorsPerCluster);
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
/* d[d] Directory listing, d is the d=directory name. Get the first (if d
present) or next entry in the directory. If the name has a zero in the first
position, return the next entry in the directory listing. Returns the type,
size and name preceded by a comma. If there are no further entries found in the
directory, then size and name are not sent back. The type character can be:
 f = file, d = directory, n = error e = end */
            case 'd':
            {
                char fileName[20];
                char type;
                uint32_t size;
                uint8_t fileStatus =
                    read_directory_entry((char*)line+2, &type, &size, fileName);
                char dirInfo[20];
                dirInfo[0] = type;
                dirInfo[1] = 0;
                if (type != 'e')
                {
                    char fileSize[5];
                    hexToAscii((size >> 16) & 0xFFFF,fileSize);
                    stringAppend(dirInfo,fileSize);
                    hexToAscii(size & 0xFFFF,fileSize);
                    stringAppend(dirInfo,fileSize);
                    stringAppend(dirInfo,fileName);
                }
                sendString("fd",dirInfo);
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
/* Wf Open a file f=filename for writing less than 12 characters.
Parameter is a filename, 8 character plus dot plus 3 character extension.
Returns a file handle. On error, file handle is 0xFF. */
            case 'W':
            {
                if (stringLength((char*)line+2) < 12)
                {
                    uint8_t fileStatus =
                        open_write_file((char*)line+2, &writeFileHandle);
                    if (fileStatus == 0)
                    {
                        stringCopy(writeFileName,(char*)line+2);
                        sendResponse("fW",writeFileHandle);
                    }
                    sendResponse("fE",(uint8_t)fileStatus);
                }
                break;
            }
/* Rf Open a file f=filename for reading less than 12 characters.
Parameter is a filename, 8 character plus dot plus 3 character extension.
Returns a file handle. On error, file handle is 0xFF. */
            case 'R':
            {
                if (stringLength((char*)line+2) < 12)
                {
                    uint8_t fileStatus = 
                        open_read_file((char*)line+2, &readFileHandle);
                    if (fileStatus == 0)
                    {
                        stringCopy(readFileName,(char*)line+2);
                         sendResponse("fR",readFileHandle);
                    }
                    sendResponse("fE",(uint8_t)fileStatus);
                }
                break;
            }
/* Gf Read a record from the file specified by f=file handle. Return as a comma
separated list. */
            case 'G':
            {
                uint8_t fileHandle = asciiToInt((char*)line+2);
                if (valid_file_handle(fileHandle))
                {
                    char string[80];
                    uint8_t fileStatus = 
                        read_line_from_file(fileHandle,string);
                    sendString("fG",string);
                    sendResponse("fE",(uint8_t)fileStatus);
                }
                break;
            }
/* s Send a status message containing: software switches and names of open
files, with open write filehandle and filename first followed by read filehandle
and filename, or blank if any file is not open. */
            case 's':
            {
                commsPrintString("fs,");
                commsPrintInt((int)getControls());
                commsPrintString(",");
                uint8_t writeStatus;
                commsPrintInt(writeFileHandle);
                commsPrintString(",");
                if (writeFileHandle < 0xFF)
                {
                    commsPrintString(writeFileName);
                    commsPrintString(",");
                }
                commsPrintInt(readFileHandle);
                if (readFileHandle < 0xFF)
                {
                    commsPrintString(",");
                    commsPrintString(readFileName);
                }
                commsPrintString("\r\n");
                break;
            }
/* Cf Close File specified by f=file handle. */
            case 'C':
            {
                uint8_t fileHandle = asciiToInt((char*)line+2);
                uint8_t fileStatus = close_file(&fileHandle);
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
/* X Delete File. */
            case 'X':
            {
                uint8_t fileStatus = delete_file((char*)line+2);
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
/* M Reinitialize the memory card. */
            case 'M':
            {
                uint8_t fileStatus = init_file_system();
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
/* Z Create a standard file system on the memory volume */
            case 'Z':
            {
                sendString("D","Creating Filesystem");
                uint8_t fileStatus = make_filesystem();
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
        }
    }
}

/*--------------------------------------------------------------------------*/

