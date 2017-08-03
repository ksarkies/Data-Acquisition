STM32F103 Data Acquisition Module
---------------------------------

This module is intended initially for the measurement of battery parameters
with a longer term aim at providing a framework for more general data
acquisition purposes as the need arises.

Circuits for current (bidirectional) and voltage (offset by a minimum voltage)
are initially provided by the Battery Management System hardware interfaces,
with the BMS processor module being used for overall control and the BMS switch
used for generating electronics subsystem power and switching of loads. The
processor is the ARM Cortex M3 STM32F103 which has a number of 12 bit A/D
converter channels.

The program reads temperature and two channels (current and voltage)
continuously and computes average and variance. The results are transmitted via
USART, as well as being stored in the on-board SD card.

A command-response interface is provided for control and setting of parameters.

A GUI provides real-time display and windows for configuration and file system
control.

K. Sarkies 02/08/2017

