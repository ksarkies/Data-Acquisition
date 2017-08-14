/* Hardware Initialisation and ISR definition.

Hardware initialization functions and some of the ISRs are provided.
Also provided are some helper functions for the USART communications.

The processor used here is the STM32F103RET6 on the ET-STM32 STAMP board.
The hardware library is libopencm3.

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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include "buffer.h"
#include "hardware.h"
#include "comms.h"
#include "hardware-bms.h"
#include "data-acquisition.h"

#define  _BV(bit) (1 << (bit))

/*--------------------------------------------------------------------------*/
/* Globals */

extern uint32_t __configBlockStart;
extern uint32_t __configBlockEnd;

/* Local Variables */
static uint32_t v[NUM_CHANNEL]; /* Buffer used by DMA to dump A/D conversions */
static bool adceoc;

/* Time variables needed when systick is used as a timer */
static uint32_t secondsCount;
static uint32_t millisecondsCount;
static uint32_t downCount;

/* This is provided in the FAT filesystem library */
extern void disk_timerproc();

/*--------------------------------------------------------------------------*/
/* Local Prototypes */

/*--------------------------------------------------------------------------*/
/* Helpers */
/*--------------------------------------------------------------------------*/
/** @brief Initialise hardware

Basic setup of hardware.
*/

void hardwareInit(void)
{
    clock_setup();
    gpio_setup();
    systickSetup();
    rtc_setup();
    dma_adc_setup();
    adc_setup();
    usart1_setup();
}

/*--------------------------------------------------------------------------*/
/** @brief Setup the ADC channels

Specify the A/D channels to the hardware to tell it where to place conversion
results.

@param[in] adc: uint8_t A/D converter number.
@param[in] numberChannels: uint8_t
@param[in] channelArray: uint8_t* Array to receive the conversion results.
*/

void setAdcChannelSequence(uint8_t adc, uint8_t numberChannels, uint8_t* channelArray)
{
    if (adc == 0)
        adc_set_regular_sequence(ADC1, numberChannels, channelArray);
}

/*--------------------------------------------------------------------------*/
/** @brief Start an A/D Conversion

@param[in] adc: uint8_t A/D converter number.
*/

void startAdcConversion(uint8_t adc)
{
    if (adc == 0)
        adc_start_conversion_regular(ADC1);
}

/*--------------------------------------------------------------------------*/
/** @brief Disable Global interrupts
*/

void cli(void)
{
    cm_disable_interrupts();
}

/*--------------------------------------------------------------------------*/
/** @brief Enable Global interrupts
*/

void sei(void)
{
    cm_enable_interrupts();
}

/*--------------------------------------------------------------------------*/
/** @brief Enable/Disable USART Interrupt

@param[in] enable: uint8_t true to enable the interrupt, false to disable.
*/

void commsEnableTxInterrupt(uint8_t enable)
{
    if (enable) usart_enable_tx_interrupt(USART1);
    else usart_disable_tx_interrupt(USART1);
}

/*--------------------------------------------------------------------------*/
/** @brief Read a data block from Flash memory

Adapted from code by Damian Miller.

@param[in] flashBlock: uint32_t* address of Flash page start
@param[in] dataBlock: uint32_t* pointer to data block to write
@param[in] size: uint16_t length of data block
*/

void flashReadData(uint32_t *flashBlock, uint8_t *dataBlock, uint16_t size)
{
    uint16_t n;
    uint32_t *flashAddress= flashBlock;

    for(n=0; n<size; n += 4)
    {
        *(uint32_t*)dataBlock = *(flashAddress++);
        dataBlock += 4;
    }
}

/*--------------------------------------------------------------------------*/
/** @brief Program a data block to Flash memory

Adapted from code by Damian Miller.

@param[in] flashBlock: uint32_t* address of Flash page start
@param[in] dataBlock: uint32_t* pointer to data block to write
@param[in] size: uint16_t length of data block
@returns uint32_t result code: 0 success, bit 0 address out of range,
bit 2: programming error, bit 4: write protect error, bit 7 compare fail.
*/

uint32_t flashWriteData(uint32_t *flashBlock, uint8_t *dataBlock, uint16_t size)
{
    uint16_t n;

    uint32_t pageStart = (uint32_t)flashBlock;
    uint32_t flashAddress = pageStart;
    uint32_t pageAddress = pageStart;
    uint32_t flashStatus = 0;

    /*check if pageStart is in proper range*/
    if((pageStart < __configBlockStart) || (pageStart >= __configBlockEnd))
        return 1;

    /*calculate current page address*/
    if(pageStart % FLASH_PAGE_SIZE)
        pageAddress -= (pageStart % FLASH_PAGE_SIZE);

    flash_unlock();

    /*Erasing page*/
    flash_erase_page(pageAddress);
    flashStatus = flash_get_status_flags();
    if(flashStatus != FLASH_SR_EOP)
        return flashStatus;

    /*programming flash memory*/
    for(n=0; n<size; n += 4)
    {
        /*programming word data*/
        flash_program_word(flashAddress+n, *((uint32_t*)(dataBlock + n)));
        flashStatus = flash_get_status_flags();
        if(flashStatus != FLASH_SR_EOP)
            return flashStatus;

        /*verify if correct data is programmed*/
        if(*((uint32_t*)(flashAddress+n)) != *((uint32_t*)(dataBlock + n)))
            return 0x80;
    }

    return 0;
}

/*--------------------------------------------------------------------------*/
/** @brief Read the Elapsed Time in Milliseconds

@returns uint32_t Milliseconds counter value.
*/

uint32_t getMilliSecondsCount()
{
    return millisecondsCount;
}

/*--------------------------------------------------------------------------*/
/** @brief Read the Time

@returns uint32_t seconds counter value.
*/

uint32_t getSecondsCount()
{
#if (RTC_SOURCE == RTC)
    return rtc_get_counter_val();
#else
    return secondsCount;
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Set the Time

@param[in] time: uint32_t seconds counter value to set.
*/

void setSecondsCount(uint32_t time)
{
#if (RTC_SOURCE == RTC)
    rtc_set_counter_val(time);
#else
    secondsCount = time;
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Read the Down Counter

@returns uint32_t counter value in milliseconds.
*/

uint32_t getDelayCount()
{
    return downCount;
}

/*--------------------------------------------------------------------------*/
/** @brief Set the Down Counter

@param[in] time: uint32_t seconds counter value in milliseconds to set.
*/

void setDelayCount(uint32_t time)
{
    downCount = time;
}

/*--------------------------------------------------------------------------*/
/** @brief Return and Reset the A/D End of Conversion Flag

This must be retrieved from a global variable set in the ISR as the hardware
EOC doesn't always change at the end of a conversion (when multiple conversions
take place).

@returns uint8_t boolean true if the flag was set; false otherwise.
*/

uint8_t adcEOC(void)
{
    if (adceoc)
    {
        adceoc = false;
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------*/
/** @brief Return the A/D Conversion Results

Note the channel must be less than the number of channels in the A/D converter.

@param[in] channel: uint8_t A/D channel to be retrieved from the DMA buffer.
@returns uint32_t last value measured by the A/D converter.
*/

uint32_t adcValue(uint8_t channel)
{
    if (channel > NUM_CHANNEL) return 0;
    return v[channel];
}

/*--------------------------------------------------------------------------*/
/** @brief Make Switch Settings

The switches are used to connect loads and source to the device under test.
This is set through the GPIO interface. The load or source specified is
set either to the device specified, or is disconnected from all devices.

This function provides a common interface if different hardware is used.

@param[in] device: uint8_t (1-3, 0 = none)
@param[in] setting: uint8_t load (0-1), source 2.
*/

void setSwitch(uint8_t device, uint8_t setting)
{
    uint16_t switchControl = gpio_port_read(SWITCH_CONTROL_PORT);
    uint16_t switchControlBits = ((switchControl >> SWITCH_CONTROL_SHIFT) & 0x3F);
/* Each two-bit field represents load 1 bits 0-1, load 2 bits 2-3, source bits
4-5, and the field represents the device to be connected (no two devices
can be connected to a load/source at the same time). The final bit pattern of
settings go into the switch control port, preserving the lower bits. */
    if ((device <= 3) && (setting <= 2))
    {
        switchControlBits &= (~(0x03 << (setting<<1)));
        switchControlBits |= ((device & 0x03) << (setting<<1));
        switchControl &= ~(0x3F << SWITCH_CONTROL_SHIFT);
        gpio_port_write(SWITCH_CONTROL_PORT,
                    (switchControl | (switchControlBits << SWITCH_CONTROL_SHIFT)));
    }
}

/*--------------------------------------------------------------------------*/
/** @brief Return the Switch Settings

Each two-bit field represents load 1 bits 0-1, load 2 bits 2-3 source bits 4-5,
and the field represents the device (1-3) to be connected. Device 0 = none
connected.

@returns uint8_t: the switch settings from the relevant port.
*/

uint8_t getSwitchControlBits(void)
{
    return ((gpio_port_read(SWITCH_CONTROL_PORT) >> SWITCH_CONTROL_SHIFT) & 0x3F);
}

/*--------------------------------------------------------------------------*/
/** @brief Set the Interface Reset Line

This activates a reset of the interface for whatever reason that may be needed
supported. A call to the release function following a time interval is needed
to complete the interrupt process.

@param[in] interface: uint32_t interface 0-5, being devices 1-3, loads 1-2,
source.
*/

void overCurrentReset(uint32_t interface)
{
    uint32_t port = 0;
    uint16_t bit = 0;
    switch (interface)
    {
        case 0:
            port = RESET_PORT_1;
            bit = RESET_BIT_1;
            break;
        case 1:
            port = RESET_PORT_2;
            bit = RESET_BIT_2;
            break;
        case 2:
            port = RESET_PORT_3;
            bit = RESET_BIT_3;
            break;
        case 3:
            port = RESET_PORT_4;
            bit = RESET_BIT_4;
            break;
        case 4:
            port = RESET_PORT_5;
            bit = RESET_BIT_5;
            break;
        case 5:
            port = RESET_PORT_6;
            bit = RESET_BIT_6;
            break;
    }
    if (interface < 6) gpio_set(port,bit);
}

/*--------------------------------------------------------------------------*/
/** @brief Release the Interface Reset Line

@param[in] interface: uint32_t interface 0-5, being batteries 1-3, loads 1-2, module.
*/

void overCurrentRelease(uint32_t interface)
{
    uint32_t port = 0;
    uint16_t bit = 0;
    switch (interface)
    {
        case 0:
            port = RESET_PORT_1;
            bit = RESET_BIT_1;
            break;
        case 1:
            port = RESET_PORT_2;
            bit = RESET_BIT_2;
            break;
        case 2:
            port = RESET_PORT_3;
            bit = RESET_BIT_3;
            break;
        case 3:
            port = RESET_PORT_4;
            bit = RESET_BIT_4;
            break;
        case 4:
            port = RESET_PORT_5;
            bit = RESET_BIT_5;
            break;
        case 5:
            port = RESET_PORT_6;
            bit = RESET_BIT_6;
            break;
    }
    if (interface < 6) gpio_clear(port,bit);
}

/*--------------------------------------------------------------------------*/
/** @brief Restore Saved Switch Settings

Each two-bit field represents load 1 bits 0-1, load 2 bits 2-3 source bits 4-5,
and the field represents the device (1-3) to be connected. Device 0 = none
connected.

This can be used as a raw switch setting call but is not recommended for normal
use.

@param[in] settings: uint8_t the switch settings from the relevant port.
*/

void setSwitchControlBits(uint8_t settings)
{
    uint16_t switchControl = gpio_port_read(SWITCH_CONTROL_PORT);
    switchControl &= ~(0x3F << SWITCH_CONTROL_SHIFT);
    gpio_port_write(SWITCH_CONTROL_PORT,
                (switchControl | (settings << SWITCH_CONTROL_SHIFT)));
}

/*--------------------------------------------------------------------------*/
/** @brief Clock Enable

The processor system clock is established. */

void clock_setup(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

/*--------------------------------------------------------------------------*/
/** @brief GPIO Setup.

This sets the clocks for the GPIO and AFIO ports. Any digital outputs and
inputs are also set here using definitions for the ports in the board-defs.h
header.
*/

void gpio_setup(void)
{
/* Enable GPIOA, GPIOB and GPIOC clocks and alternate functions. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_AFIO);
#ifndef USE_SWD
/* Disable SWD and JTAG to allow full use of the ports PA13, PA14, PA15 */
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF,0);
#endif

/* PA inputs analogue for currents, voltages and ambient temperature */
#ifdef PA_ANALOGUE_INPUTS
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG,
                PA_ANALOGUE_INPUTS);
#endif
/* PC inputs analogue for currents, voltages and ambient temperature */
#ifdef PC_ANALOGUE_INPUTS
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG,
                PC_ANALOGUE_INPUTS);
#endif
/* PA outputs digital */
#ifdef PA_DIGITAL_OUTPUTS
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                PA_DIGITAL_OUTPUTS);
    gpio_clear(GPIOA, PA_DIGITAL_OUTPUTS);
#endif
/* PB outputs digital */
#ifdef PB_DIGITAL_OUTPUTS
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                PB_DIGITAL_OUTPUTS);
    gpio_clear(GPIOB, PB_DIGITAL_OUTPUTS);
#endif
/* PC outputs digital */
#ifdef PC_DIGITAL_OUTPUTS
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                PC_DIGITAL_OUTPUTS);
    gpio_clear(GPIOC, PC_DIGITAL_OUTPUTS);
#endif
/* PA inputs digital. Set pull up/down configuration to pull up. */
#ifdef PA_DIGITAL_INPUTS
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                PA_DIGITAL_INPUTS);
    gpio_set(GPIOA,PA_DIGITAL_INPUTS);
#endif
/* PB inputs digital. Set pull up/down configuration to pull up. */
#ifdef PB_DIGITAL_INPUTS
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                PB_DIGITAL_INPUTS);
    gpio_set(GPIOB,PB_DIGITAL_INPUTS);
#endif
/* PC inputs digital. Set pull up/down configuration to pull up. */
#ifdef PC_DIGITAL_INPUTS
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                PC_DIGITAL_INPUTS);
    gpio_set(GPIOC,PC_DIGITAL_INPUTS);
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Systick Setup

Setup SysTick Timer for 1 millisecond interrupts, also enables Systick and
Systick-Interrupt.
*/

void systickSetup(void)
{
/* Set clock source to be the CPU clock prescaled by 8.
for STM32F103 this is 72MHz / 8 => 9,000,000 counts per second */
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

/* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
/* SysTick interrupt every N clock pulses: set reload to N-1 */
    systick_set_reload(8999);

    systick_interrupt_enable();

/* Start counting. */
    systick_counter_enable();
}

/*--------------------------------------------------------------------------*/
/** @brief ADC Setup.

ADC1 is turned on and calibrated.

For A/D conversion on the STM32F103RET6 the A/D ports are:
PA 0-7 is ADC 0-7
PB 0-1 is ADC 8-9
PC 0-5 is ADC 10-15

@param[in] adc_inputs: uint32_t bit map of ports to be used (0-15).
*/

void adc_setup(void)
{
/* Enable the ADC1 clock on APB2 */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_ADC1);
/* ADC clock should be maximum 14MHz, so divide by 8 from 72MHz. */
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV8);
	nvic_enable_irq(NVIC_ADC1_2_IRQ);
/* Make sure the ADC doesn't run during config. */
    adc_power_off(ADC1);
/* Configure ADC1 for multiple conversion of channels, stopping after the last one. */
	adc_enable_scan_mode(ADC1);
	adc_set_single_conversion_mode(ADC1);
	adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_SWSTART);
	adc_set_right_aligned(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
	adc_enable_dma(ADC1);
	adc_enable_eoc_interrupt(ADC1);
/* Setup the ADC */
    adc_power_on(ADC1);
    /* Wait for ADC starting up. */
    uint32_t i;
    for (i = 0; i < 800000; i++)    /* Wait a bit. */
        __asm__("nop");
    adc_reset_calibration(ADC1);
    adc_calibrate_async(ADC1);
    while (adc_is_calibrating(ADC1));
    adceoc = false;
}

/*--------------------------------------------------------------------------*/
/** @brief DMA Setup

Enable DMA 1 Channel 1 to take conversion data from ADC 1, and also ADC 2 when
the ADC is used in dual mode. The ADC will dump a burst of data to memory each
time, and we need to grab it before the next conversions start. This must be
called after each transfer to reset the memory buffer to the beginning.
*/

void dma_adc_setup(void)
{
/* Enable DMA1 Clock */
	rcc_periph_clock_enable(RCC_DMA1);
	dma_channel_reset(DMA1,DMA_CHANNEL1);
	dma_set_priority(DMA1,DMA_CHANNEL1,DMA_CCR_PL_LOW);
/* We want all 32 bits from the ADC to include ADC2 data */
	dma_set_memory_size(DMA1,DMA_CHANNEL1,DMA_CCR_MSIZE_32BIT);
	dma_set_peripheral_size(DMA1,DMA_CHANNEL1,DMA_CCR_PSIZE_32BIT);
	dma_enable_memory_increment_mode(DMA1,DMA_CHANNEL1);
	dma_set_read_from_peripheral(DMA1,DMA_CHANNEL1);
/* The register to target is the ADC1 regular data register */
	dma_set_peripheral_address(DMA1,DMA_CHANNEL1,(uint32_t)&ADC_DR(ADC1));
/* The array v[] receives the converted output */
	dma_set_memory_address(DMA1,DMA_CHANNEL1,(uint32_t)v);
	dma_set_number_of_data(DMA1,DMA_CHANNEL1,NUM_CHANNEL);
	dma_enable_channel(DMA1,DMA_CHANNEL1);
}

/*--------------------------------------------------------------------------*/
/** @brief EXTI Setup.

This enables the external interrupts the specified ports.
These will wake the processor and interrupt even in deep sleep mode.

In the STM32F103 the EXTI0-4 have separate interrupt vectors while
EXTI5-9 and EXTI10-15 have shared vectors.

@param[in] exti_enables: uint32_t bit map of ports to be used (0-15,
16 = PVD, 17 = RTC alarm, 18 = USB).
@param[in] port: uint32_t output port (GPIOA, GPIOB, GPIOC)
*/

void exti_setup(uint32_t exti_enables, uint32_t port)
{
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  exti_enables);
    gpio_set(GPIOA,exti_enables);      // Pull up
    exti_select_source(exti_enables, port);
    exti_set_trigger(exti_enables, EXTI_TRIGGER_RISING);
    int i;
    for (i=0; i<16; i++)
    {
        if ((i < 5) && (exti_enables & (1 << i)))
            nvic_enable_irq(NVIC_EXTI0_IRQ+i);
        if ((i >= 5) && (i < 10) && (exti_enables & 0x1F0))
            nvic_enable_irq(NVIC_EXTI9_5_IRQ);
        if ((i >= 10) && (i < 16) && (exti_enables & 0xF600))
            nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    }
    if (exti_enables & (1 << 16))
        nvic_enable_irq(NVIC_PVD_IRQ);
    if (exti_enables & (1 << 17))
        nvic_enable_irq(NVIC_RTC_ALARM_IRQ);
    if (exti_enables & (1 << 18))
        nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
    exti_enable_request(exti_enables);
}

/*--------------------------------------------------------------------------*/
/** @brief RTC Setup.

The RTC is woken up, cleared, set to use the external low frequency LSE clock
(which must be provided on the board used) and set to prescale at 1Hz out.
Thus the counter will contain elapsed time in seconds.

The LSE clock appears to be already running.
*/

void rtc_setup(void)
{
/* Wake up and clear RTC registers using the LSE as clock. */
/* Set prescaler, using value for 1Hz out. */
	rtc_auto_awake(RCC_LSE,0x7FFF);

/* Clear the RTC counter - some counts will occur before prescale is set. */
	rtc_set_counter_val(0);

/* Set the Alarm to trigger in interrupt mode on EXTI17 for wakeup */
	nvic_enable_irq(NVIC_RTC_ALARM_IRQ);
	EXTI_IMR |= EXTI17;
	exti_set_trigger(EXTI17,EXTI_TRIGGER_RISING);
}

/*--------------------------------------------------------------------------*/
/** @brief Initialise USART 1.

USART 1 is configured for 38400 baud, interrupt.and  no flow control.
*/

void usart1_setup(void)
{
/* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_USART1);
/* Enable the USART1 interrupt. */
	nvic_enable_irq(NVIC_USART1_IRQ);
/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
/* Setup GPIO pin GPIO_USART1_RE_RX on GPIO port A for receive. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);
/* Setup UART parameters. */
	usart_set_baudrate(USART1, 38400);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
/* Enable USART1 receive interrupts. */
	usart_enable_rx_interrupt(USART1);
	usart_disable_tx_interrupt(USART1);
/* Finally enable the USART. */
	usart_enable(USART1);
}

/*--------------------------------------------------------------------------*/
/** @brief Peripheral Disables.

This turns off power to all peripherals to reduce power drain during sleep.
RTC and EXTI need to remain on.
*/

void peripheral_disable(void)
{
#ifdef DEEPSLEEP
    rcc_periph_clock_disable(RCC_AFIO);
    rcc_periph_clock_disable(RCC_GPIOA);
    rcc_periph_clock_disable(RCC_USART1);
#endif
    rcc_periph_clock_disable(RCC_GPIOB);
    rcc_periph_clock_disable(RCC_GPIOC);
    rcc_periph_clock_disable(RCC_ADC1);
    adc_power_off(ADC1);
}

/*--------------------------------------------------------------------------*/
/** @brief Peripheral Enables.

This turns on power to all peripherals needed.
*/

void peripheral_enable(void)
{
	clock_setup();
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_ADC1);
    adc_power_on(ADC1);
}

/*--------------------------------------------------------------------------*/
/* Interrupt Service Routines */
/*--------------------------------------------------------------------------*/
/* EXT17/RTC Alarm ISR

The RTC alarm appears as EXTI 17 which must be reset independently of the RTC
alarm flag. Do not reset the latter here as it is needed to guide the main
program to activate regular tasks.
*/

void rtc_alarm_isr(void)
{
	exti_reset_request(EXTI17);
}

/*-----------------------------------------------------------*/
/** @brief Systick Interrupt Handler

This updates the status of any inserted SD card every 10 ms.

Can be used to provide a RTC.
*/
void sys_tick_handler(void)
{
    millisecondsCount++;
/* SD card status update. */
    if ((millisecondsCount % (10)) == 0)
    {
        disk_timerproc();       /* File System hardware checks */
        timer_proc();           /* test run and other checks */
    }

/* updated every second if systick is used for the real-time clock. */
    if ((millisecondsCount % 1000) == 0) secondsCount++;

/* down counter for timing. */
    downCount--;
}

/*--------------------------------------------------------------------------*/
/* USART ISR

Find out what interrupted and get or send data as appropriate.
*/

void usart1_isr(void)
{
	static uint16_t data;

/* Check if we were called because of RXNE. */
	if (usart_get_flag(USART1,USART_SR_RXNE))
	{
/* If buffer full we'll just drop it */
		put_to_receive_buffer((uint8_t) usart_recv(USART1));
	}
/* Check if we were called because of TXE. */
	if (usart_get_flag(USART1,USART_SR_TXE))
	{
/* If buffer empty, disable the tx interrupt */
		data = get_from_send_buffer();
		if ((data & 0xFF00) > 0) usart_disable_tx_interrupt(USART1);
		else usart_send(USART1, (data & 0xFF));
	}
}

/*--------------------------------------------------------------------------*/
/* ADC ISR

Respond to ADC EOC at end of scan.

The EOC status is lost when DMA reads the data register, so use a global
variable.
*/

void adc1_2_isr(void)
{
	gpio_toggle(GPIOB, GPIO9);
    adceoc = true;
/* Clear DMA to restart at beginning of data array */
	dma_adc_setup();
}

/*--------------------------------------------------------------------------*/

