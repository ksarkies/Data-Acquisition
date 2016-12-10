This is an adaptation of ChaN's FatFS FAT filesystem library based on v0.12.

Common files are ff.c, ff.h, ffconf.h, integer.h, diskio.h. These should be
updated when a new version of ChaN FAT library is released. Ensure that any
changes to ffconf.h are reflected in the new version.

The remaining files adapt the library to the hardware being used, to FreeRTOS
and to libopencm3. Some changes to these may be necessary if the ChaN FAT API
changes.

Currently the makefile calls on sd_spi_loc3_stm32_freertos.c

More information is available on [Jiggerjuice](http://www.jiggerjuice.info/electronics/projects/solarbms/solarbms-software.html).

(c) K. Sarkies 22/11/2016

