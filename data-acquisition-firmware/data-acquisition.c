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

#include "../libs/buffer.h"
#include "../libs/hardware.h"
#include "../libs/comms.h"
#include "../libs/stringlib.h"
#include "../libs/file.h"
#include "../libs/timelib.h"
#include "data-acquisition.h"
#include "data-acquisition-objdic.h"

#include <stdbool.h>

/* Local Prototypes */
static void parseCommand(uint8_t* line);

/* Globals */
static uint8_t writeFileHandle;
static uint8_t readFileHandle;
static char writeFileName[12];
static char readFileName[12];
static uint16_t interface;         /* Interface to be reset */
static uint8_t resetTimer;
static uint8_t testType;
static uint32_t voltageLimit;
static uint32_t timeLimit;
static uint32_t timeElapsed;
static uint32_t secondsElapsed;
static uint8_t device;
static uint8_t setting;
static bool testRunning;
static bool testStarted;
static int32_t current[NUM_INTERFACES];
static uint64_t voltage[NUM_INTERFACES];

/* These configuration variables are part of the Object Dictionary. */
/* This is defined in data-acquisition-objdic and is updated in response to
received messages. */
extern union ConfigGroup configData;

/*--------------------------------------------------------------------------*/

int main(void)
{
    set_global_defaults();
    hardware_init();
    init_comms_buffers();

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
    set_adc_channel_sequence(0, NUM_CHANNEL, channel_array);

    uint16_t i = 0;
    uint32_t avg[NUM_CHANNEL];

    set_delay_count(configData.config.measurementInterval);

    init_file_system();
    writeFileHandle = 0xFF;
    readFileHandle = 0xFF;
    writeFileName[0] = 0;
    readFileName[0] = 0;

    interface = 0;
    resetTimer = 0;

    testType = 0;
    voltageLimit = 0;
    timeLimit = 0;
    timeElapsed = 0;
    secondsElapsed = 0;
    device = 0;
    setting = 0;
    testRunning = false;
    testStarted = false;

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
        if (get_delay_count() > 0xFFFFFF) set_delay_count(configData.config.measurementInterval);

/* -------- Measurements --------- */
        if (get_delay_count() == 0)
        {

/* Clear stats and setup array of selected channels for conversion */
            uint8_t i = 0;
	        for (i = 0; i < NUM_CHANNEL; i++)
	        {
                avg[i] = 0;
	        }
/* Run a burst of samples and average */
            uint8_t sample = 0;
            uint8_t numSamples = configData.config.numberSamples;
            if (numSamples < 1) numSamples = 1;
            for (sample = 0; sample < numSamples; sample++)
            {
/* Start conversion and wait for conversion end. */
                start_adc_conversion(0);
                while (! adc_eoc_is_set()) {}
                for (i = 0; i < NUM_CHANNEL; i++)
	            {
                    avg[i] += adc_value(i);
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
            put_time_to_string(timeString);
            send_string("pH",timeString);
            if (is_recording()) record_string("pH",timeString,writeFileHandle);
/* Send out temperature measurement. */
            send_response("dT",temperature);
            if (is_recording()) record_single("dT",temperature,writeFileHandle);
/* Send off accumulated data as dBx where x is 0-5 for devices 1-3, loads 1-2,
source. */
            char id[4];
            id[0] = 'd';
            id[1] = 'B';
            id[3] = 0;
            for (i=0; i < numInterfaces; i++)
            {
                id[2] = '1'+i;
                data_message_send(id, current[i], voltage[i]);
                if (is_recording()) record_dual(id, current[i], voltage[i], writeFileHandle);
            }
/* Send out switch status */
            send_response("ds",(int)get_switch_control_bits());
            if (is_recording()) record_single("ds",(int)get_switch_control_bits(),writeFileHandle);
/* Send out running test information. This is always sent during a test
run even if no time limit has been set to indicate an active test run. */
            if (testRunning) send_response("dR",timeElapsed);
            if (testStarted) send_response("dr",secondsElapsed);
            send_response("dX",testRunning);
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
        switch (line[1])
        {
/* Snm Manually set switch. Follow by device n (1-3, 0 = none) and
load m (0-1)/source (2). Each two-bit field represents a load or panel, and the
setting is the battery to be connected (no two batteries can be connected to a
load/panel). */
        case 'S':
            {
                device = line[2]-'0';
                setting = line[3]-'0'-1;
                if ((device > 0) && (device <= NUM_DEVICES) && (setting < 3))
                    set_switch(device, setting);
                break;
            }
/* Request preset test parameters to be sent back. */
        case 'P':
            {
                data_message_send("dP",testType,timeLimit);
                send_response("dV",voltageLimit);
                break;
            }
/* This starts a test run if the testType has been correctly set. */
        case 'G':
            {
                if ((testType > 0) && (testType < 4) && (voltageLimit > 0))
                {
                    testStarted = true;
                    secondsElapsed = 0;
                    testRunning = true;
                }
                break;
            }
/* X Test run - Manual Stop. Turn off all load/source interfaces. */
        case 'X':
            {
                uint8_t setting = 0;
                for (setting=0; setting<NUM_LOADS+NUM_SOURCES; setting++)
                {
                    set_switch(0, setting);
                }
                testRunning = false;
                timeElapsed = 0;
                break;
            }
/* Rn Reset an interface. Set a timer to expire after 250ms at which time the
reset line is released. The command is followed by an interface number n=0-5
being device 1-3, loads 1-2 and source. */
        case 'R':
            {
                interface = line[2]-'0';
                if (interface > NUM_INTERFACES-1) break;
                if (interface > 0)
                {
                    overcurrent_reset(interface);
                    resetTimer = 25;        /* Set to 250 ms */
                }
                break;
            }
/* W Write the current configuration block to FLASH */
        case 'W':
            {
                write_config_block();
                break;
            }
/* Request identification string with version sent back.  */
        case 'E':
            {
                char ident[35] = "Data Acquisition System,";
                string_append(ident,FIRMWARE_VERSION);
                send_string("dE",ident);
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
                put_time_to_string(timeString);
                send_string("pH",timeString);
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
                set_time_from_string((char*)line+2);
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
/* Tn Test run - Set Time limit n in seconds */
        case 'T':
            {
                timeLimit = ascii_to_int((char*)line+2);
                break;
            }
/* Vn Test run - Voltage limit n in volts times 256 */
        case 'V':
            {
                voltageLimit = ascii_to_int((char*)line+2);
                break;
            }
/* Rn Test run - Start test with test type specified n = 1-3. */
        case 'R':
            {
                testType = line[2]-'0';
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
                data_message_send("fF",freeClusters,sectorsPerCluster);
                send_response("fE",(uint8_t)fileStatus);
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
                if (! file_system_usable()) break;
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
                    hex_to_ascii((size >> 16) & 0xFFFF,fileSize);
                    string_append(dirInfo,fileSize);
                    hex_to_ascii(size & 0xFFFF,fileSize);
                    string_append(dirInfo,fileSize);
                    string_append(dirInfo,fileName);
                }
                send_string("fd",dirInfo);
                send_response("fE",(uint8_t)fileStatus);
                break;
            }
/* Wf Open a file f=filename for writing less than 12 characters.
Parameter is a filename, 8 character plus dot plus 3 character extension.
Returns a file handle. On error, file handle is 0xFF. */
            case 'W':
            {
                if (! file_system_usable()) break;
                if (string_length((char*)line+2) < 12)
                {
                    uint8_t fileStatus =
                        open_write_file((char*)line+2, &writeFileHandle);
                    if (fileStatus == 0)
                    {
                        string_copy(writeFileName,(char*)line+2);
                        send_response("fW",writeFileHandle);
                    }
                    send_response("fE",(uint8_t)fileStatus);
                }
                break;
            }
/* Rf Open a file f=filename for reading less than 12 characters.
Parameter is a filename, 8 character plus dot plus 3 character extension.
Returns a file handle. On error, file handle is 0xFF. */
            case 'R':
            {
                if (! file_system_usable()) break;
                if (string_length((char*)line+2) < 12)
                {
                    uint8_t fileStatus = 
                        open_read_file((char*)line+2, &readFileHandle);
                    if (fileStatus == 0)
                    {
                        string_copy(readFileName,(char*)line+2);
                        send_response("fR",readFileHandle);
                    }
                    send_response("fE",(uint8_t)fileStatus);
                }
                break;
            }
/* Gf Read a record from the file specified by f=file handle. Return as a comma
separated list. */
            case 'G':
            {
                if (! file_system_usable()) break;
                uint8_t fileHandle = ascii_to_int((char*)line+2);
                if (valid_file_handle(fileHandle))
                {
                    char string[80];
                    uint8_t fileStatus = 
                        read_line_from_file(fileHandle,string);
                    send_string("fG",string);
                    send_response("fE",(uint8_t)fileStatus);
                }
                break;
            }
/* s Send a status message containing: software switches and names of open
files, with open write filehandle and filename first followed by read filehandle
and filename, or blank if any file is not open. */
            case 's':
            {
                comms_print_string("fs,");
                comms_print_int((int)get_controls());
                comms_print_string(",");
                uint8_t writeStatus;
                comms_print_int(writeFileHandle);
                comms_print_string(",");
                if (writeFileHandle < 0xFF)
                {
                    comms_print_string(writeFileName);
                    comms_print_string(",");
                }
                comms_print_int(readFileHandle);
                if (readFileHandle < 0xFF)
                {
                    comms_print_string(",");
                    comms_print_string(readFileName);
                }
                comms_print_string("\r\n");
                break;
            }
/* Cf Close File specified by f=file handle. */
            case 'C':
            {
                if (! file_system_usable()) break;
                uint8_t fileHandle = ascii_to_int((char*)line+2);
                uint8_t fileStatus = close_file(&fileHandle);
                if (fileStatus == 0) writeFileHandle = 0xFF;
                send_response("fE",(uint8_t)fileStatus);
                break;
            }
/* X Delete File. */
            case 'X':
            {
                if (! file_system_usable()) break;
                uint8_t fileStatus = delete_file((char*)line+2);
                send_response("fE",(uint8_t)fileStatus);
                break;
            }
/* M Reinitialize the memory card. */
            case 'M':
            {
                uint8_t fileStatus = init_file_system();
                send_response("fE",(uint8_t)fileStatus);
                break;
            }
/* Z Create a standard file system on the memory volume */
            case 'Z':
            {
                send_string("D","Creating Filesystem");
                uint8_t fileStatus = make_filesystem();
                send_response("fE",(uint8_t)fileStatus);
                break;
            }
        }
    }
}

/*--------------------------------------------------------------------------*/
/** @brief Take certain timed actions.

These are hardware reset, and timed test runs, both of which are one-shot
timers.

This is called every 10ms from the hardware timer ISR.
*/

void timer_proc(void)
{
    static uint32_t secondsTimer = 0;
/* Handle timer for over current reset release */
    if ((interface > 0) && (resetTimer-- == 0))
    {
        overcurrent_release(interface);
        interface = 0;
    }
/* handle timer and voltage checks for test runs every second */
    if (secondsTimer++ > 100)
    {
        secondsTimer = 0;
        if (testStarted)
            secondsElapsed++;
        if (testRunning)
        {
            timeElapsed++;
            if (((testType == 2) && (timeElapsed > timeLimit))
                || (voltage[device-1] < voltageLimit))
            {
                testRunning = false;
                timeElapsed = 0;
                set_switch(0, setting);
            }
        }
    }
}


