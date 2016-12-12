/* File Management.

File management functions are provided.

K. Sarkies, 10 December 2016
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

#include <stdint.h>

#include "ff.h"
#include "file.h"
#include "buffer.h"
#include "comms.h"
#include "hardware.h"

#define  _BV(bit) (1 << (bit))

/*--------------------------------------------------------------------------*/
/* Globals */

/* Local Variables */
/* ChaN FAT */
static FATFS Fatfs[_VOLUMES];
static FATFS* fs;		            /* File system object for logical drive 0 */
static FIL file[MAX_OPEN_FILES];    /* file descriptions, 2 files maximum. */
static FILINFO fileInfo[MAX_OPEN_FILES];
static bool fileUsable;
static uint8_t filemap=0;           /* map of open file handles */
static uint8_t writeFileHandle;
static uint8_t readFileHandle;

/*--------------------------------------------------------------------------*/
/* Local Prototypes */

static uint8_t findFileHandle(void)
static void deleteFileHandle(uint8_t fileHandle)
/*--------------------------------------------------------------------------*/
/* Helpers */
/*--------------------------------------------------------------------------*/
/** @brief File Initialization

The FreeRTOS queue and semaphore are initialised. The file system work area is
initialised.
*/

uint8_t init_file(void)
{
/* initialise the drive working area */
    FRESULT fileStatus = f_mount(&Fatfs[0],"",0);
    fileUsable = (fileStatus == FR_OK);

/* Initialise some global variables */
    writeFileHandle = 0xFF;
    readFileHandle = 0xFF;
    uint8_t i=0;
    for (i=0; i<MAX_OPEN_FILES; i++) fileInfo[i].fname[0] = 0;
    filemap = 0;
    return fileStatus;
}

/*--------------------------------------------------------------------------*/
/** @brief Read the free space on the drive.

@param[out] uint32_t: number of free clusters.
@param[out] uint32_t: cluster size.
@returns uint8_t: status of operation.
*/

uint8_t get_free_clusters(uint32_t* freeClusters, uint32_t* clusterSize)
{
    FRESULT fileStatus = f_getfree("", (DWORD*)freeClusters, &fs);
    *clusterSize = fs->csize;
    return fileStatus;
}

/*--------------------------------------------------------------------------*/
/** @brief Read a directory entry.

@param[in] char*: directory name
@param[out] char*: type of entry (d = directory, f = file, n = error, e = end).
@param[out] uint32_t*: file size.
@param[out] char*: file name
@returns uint8_t: status of operation.
*/

uint8_t read_directory_entry(char* directoryName, char* type, uint32_t* size,
                             char* fileName)
{
    static DIR directory;
    FILINFO fileInfo;
    FRESULT fileStatus = FR_OK;
    if (directoryName[0] != 0) fileStatus = f_opendir(&directory, directoryName);
    if (fileStatus == FR_OK)
        fileStatus = f_readdir(&directory, &fileInfo);
    stringCopy(fileName, fileInfo.fname);
    *size = fileInfo.fsize;
    *type = 'f';
    if (fileInfo.fattrib == AM_DIR) *type = 'd';
    if (fileInfo.fname[0] == 0)
    {
        *type = 'e';
        *size = 0;
    }
    if (fileStatus != FR_OK)
    {
        fileName[0] = 0;
        *type = 'n';
        *size = 0;
    }
    return fileStatus;
}

/*--------------------------------------------------------------------------*/
/** @brief Open a file for Writing.

If the file doesn't exist a valid filename must be provided. The file is
created and a file handle is obtained and returned. If the file name is a null
string then a valid file handler must be provided and the file is opened.

@param[in] char*: file name
@param[out] uint8_t*: file handle.
@returns uint8_t: status of operation.
*/

uint8_t open_write_file(char* fileName, uint8_t* writeFileHandle)
{
    uint8_t fileHandle = 0xFF;
/* Already open */
    if (writeFileHandle < 0xFF)
        fileStatus = FR_DENIED;
    else
    {
        fileHandle = findFileHandle();
/* Unable to be allocated */
        if (fileHandle >= MAX_OPEN_FILES)
            fileStatus = FR_TOO_MANY_OPEN_FILES;
        else
        {
/* Try to open a file write/read, creating it if necessary */
            fileStatus = f_open(&file[fileHandle], fileName, \
                                FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
/* Skip to the end of the file to append. */
            if (fileStatus == FR_OK)
                fileStatus = f_lseek(&file[fileHandle], f_size(file));
            else
            {
                deleteFileHandle(fileHandle);
                fileHandle = 0xFF;
            }
            writeFileHandle = fileHandle;
            if (fileStatus == FR_OK)
                fileStatus = f_stat(fileName, fileInfo+writeFileHandle);
        }
    }
    return fileStatus;
}

/*--------------------------------------------------------------------------*/
/** @brief Find a file handle

The file handle map consists of bits set when a handle has been allocated.
This function searches for a free handle.

@returns uint8_t 0..MAX_OPEN_FILES-1 or 255 if no handle was allocated (too many
open files).
*/

static uint8_t findFileHandle(void)
{
    uint8_t i=0;
    uint8_t fileHandle=0xFF;
    uint8_t mask = 0;
    while ((i<MAX_OPEN_FILES) && (fileHandle == 0xFF))
    {
        mask = (1 << i);
        if ((filemap & mask) == 0) fileHandle = i;
        i++;
    }
    if (fileHandle < 0xFF) filemap |= mask;
    return fileHandle;
}

/*--------------------------------------------------------------------------*/
/** @brief Delete a file handle

The file handle map consists of bits set when a handle has been allocated.
This function deletes a handle. Does nothing if file handle is not valid.

@param fileHandle: uint8_t the handle for the file to be deleted.
*/

static void deleteFileHandle(uint8_t fileHandle)
{
    if (fileHandle < MAX_OPEN_FILES)
        filemap &= ~(1 << fileHandle);
}


