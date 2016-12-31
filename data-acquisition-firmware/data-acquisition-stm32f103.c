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
#include "data-acquisition-stm32f103.h"
#include "data-acquisition-objdic.h"

#include <stdbool.h>

/* Local Prototypes */
static void parseCommand(uint8_t* line);

/* Globals */
static uint8_t writeFileHandle;
static uint8_t readFileHandle;

/* These configuration variables are part of the Object Dictionary. */
/* This is defined in power-management-objdic and is updated in response to
received messages. */
extern union ConfigGroup configData;

/*--------------------------------------------------------------------------*/

int main(void)
{
//	uint8_t channel_array[16];
//	uint16_t i = 0;

    setGlobalDefaults();
	clock_setup();
	gpio_setup();
    systickSetup();
    rtc_setup();
    setTimeFromString("2014-04-17T12:33:48");
//	adc_setup();
	dma_adc_setup();
	usart1_setup();
    init_comms_buffers();
    init_file();
    writeFileHandle = 0xFF;
    readFileHandle = 0xFF;

/* Main event loop */
	while (1)
	{
/* Command Line Interface receiver interpretation. */
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
/* Z Create a standard file system on the memory volume */
            case 'Z':
            {
                sendString("D","Creating Filesystem");
                uint8_t fileStatus = make_filesystem();
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
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
                    sendResponse("fW",writeFileHandle);
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
                    sendResponse("fR",readFileHandle);
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
/* s Return the name and size of open files.
Each open file is reported with a separate message. */
            case 'S':
            {
                uint8_t fileHandle;
                for (fileHandle=0; fileHandle<8; fileHandle++)
                {
                    if (valid_file_handle(fileHandle))
                    {
                        char fileName[20];
                        get_file_name(fileHandle, fileName);
                        sendString("fS",fileName);
                    }
                }
                break;
            }
/* Cf Close File specified by f=file handle. */
            case 'C':
            {
                uint8_t fileHandle = asciiToInt((char*)line+2);
                if (valid_file_handle(fileHandle))
                {
                    uint8_t fileStatus = close_file(fileHandle);
/* Check if it is one of the currently opened files and disable it. */
                    if (fileStatus == 0)
                    {
                        if (writeFileHandle == fileHandle) writeFileHandle = 0xFF;
                        else if (readFileHandle == fileHandle) readFileHandle = 0xFF;
                    }
                    sendResponse("fE",(uint8_t)fileStatus);
                }
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
                uint8_t fileStatus = init_file();
                sendResponse("fE",(uint8_t)fileStatus);
                break;
            }
        }
    }
}

/*--------------------------------------------------------------------------*/
/* ADC ISR

Respond to ADC EOC at end of scan and send data block.
Print the result in decimal and separate with an ASCII dash.
*/

void adc1_2_isr(void)
{
}

/*--------------------------------------------------------------------------*/

