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
#include "hardware.h"

#define  _BV(bit) (1 << (bit))

/*--------------------------------------------------------------------------*/
/* Globals */

/* Local Variables */
/* ChaN FAT */
static FATFS Fatfs[_VOLUMES];
static FATFS *fs;		            /* File system object for logical drive 0 */
static FIL file[MAX_OPEN_FILES];    /* file descriptions, 2 files maximum. */
static FILINFO fileInfo[MAX_OPEN_FILES];
static bool fileUsable;
static uint8_t filemap=0;           /* map of open file handles */
static uint8_t writeFileHandle;
static uint8_t readFileHandle;

/*--------------------------------------------------------------------------*/
/* Local Prototypes */

/*--------------------------------------------------------------------------*/
/* Helpers */
/*--------------------------------------------------------------------------*/
/** @brief File Initialization

The FreeRTOS queue and semaphore are initialised. The file system work area is
initialised.
*/

void init_file(void)
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
}

/*--------------------------------------------------------------------------*/
/** @brief Read the free space on the drive.

@param[out] uint32_t number of free clusters.
@param[out] uint32_t cluster size.
@returns uint8_t status of operation.
*/

uint8_t get_free_clusters(uint32_t* freeClusters, uint32_t* sectorCluster)
{
    FRESULT fileStatus = f_getfree("", (DWORD*)freeClusters, &fs);
    *sectorCluster = fs->csize;
    return fileStatus;
}


