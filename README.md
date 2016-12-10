STM32F103 Data Acquisition Module
---------------------------------

This module is intended initially for the measurement of discharging battery
parameters with a longer term aim at providing a framework for more general
data acquisition purposes as the need arises.

Circuits for current (bidirectional) and voltage (offset by a minimum voltage)
are initially provided by the Battery Management System hardware interface,
with the BMS processor module being used for overall control.

The program reads the two channels continuously and computes average and
variance. The results are transmitted via USART along with the count, as well
as being stored on the on-board SD card.

A command-response interface is provided for control and setting of parameters.

K. Sarkies 10/12/2016

