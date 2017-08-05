/* Hardware Initialisation and ISR definition.

This file refers to hardware specific code modules defined to support the
underlying hardware and processor.

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

#include <stdint.h>

/*--------------------------------------------------------------------------*/
/* BMS board includes the processor STM32F103 */

#if (BOARD==BMS)
#include "hardware-bms.c"

#else
#error "unsupported hardware"
#endif

/*--------------------------------------------------------------------------*/

