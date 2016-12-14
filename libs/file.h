/*  File Management.

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

#ifndef _FILE_H_
#define _FILE_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_OPEN_FILES              2

/*--------------------------------------------------------------------------*/
/* Prototypes */
/*--------------------------------------------------------------------------*/

uint8_t init_file(void);
uint8_t get_free_clusters(uint32_t* freeClusters, uint32_t* clusterSize);
uint8_t read_directory_entry(char* directoryName, char* type, uint32_t* size,
                             char* fileName);
uint8_t open_write_file(char* fileName, uint8_t* writeFileHandle);
uint8_t open_read_file(char* fileName, uint8_t* readFileHandle);
uint8_t delete_file(char* fileName);
uint8_t read_from_file(uint8_t fileHandle, uint8_t* blockLength, uint8_t* data);
uint8_t write_to_file(uint8_t fileHandle, uint8_t* blockLength, uint8_t* data);
uint8_t close_file(uint8_t fileHandle);
bool valid_file_handle(uint8_t fileHandle);
void get_file_name(uint8_t fileHandle, char* fileName);

#endif

