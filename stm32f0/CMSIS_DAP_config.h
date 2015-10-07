/*
    Dapper Mime - an open-source CMSIS-DAP implementation
    HAL for STM32F0xx2
    this file is used by the mbed CMSIS-DAP routines

    Copyright (C) 2014 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

//**************************************************************************************************
/**
\defgroup DAP_Config_Debug_gr CMSIS-DAP Debug Unit Information
\ingroup DAP_ConfigIO_gr
@{
Provides definitions about:
 - Definition of Cortex-M processor parameters used in CMSIS-DAP Debug Unit.
 - Debug Unit communication packet size.
 - Debug Access Port communication mode (JTAG or SWD).
 - Optional information about a connected Target Device (for Evaluation Boards).
*/

#include <libopencm3/stm32/gpio.h>

// Board configuration options

/// Processor Clock of the Cortex-M MCU used in the Debug Unit.
/// This value is used to calculate the SWD/JTAG clock speed.
#define CPU_CLOCK               48000000        ///< Specifies the CPU Clock in Hz

/// Number of processor cycles for I/O Port write operations.
/// This value is used to calculate the SWD/JTAG clock speed that is generated with I/O
/// Port write operations in the Debug Unit by a Cortex-M MCU. Most Cortex-M processors
/// requrie 2 processor cycles for a I/O Port Write operation.  If the Debug Unit uses
/// a Cortex-M0+ processor with high-speed peripheral I/O only 1 processor cycle might be
/// required.
#define IO_PORT_WRITE_CYCLES    2               ///< I/O Cycles: 2=default, 1=Cortex-M0+ fast I/0

/// Indicate that Serial Wire Debug (SWD) communication mode is available at the Debug Access Port.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define DAP_SWD                 1               ///< SWD Mode:  1 = available, 0 = not available

/// Indicate that JTAG communication mode is available at the Debug Port.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#if defined(CONF_JTAG)
#define DAP_JTAG                1               ///< JTAG Mode: 1 = available
#else
#define DAP_JTAG                0               ///< JTAG Mode: 0 = not available
#endif

/// Configure maximum number of JTAG devices on the scan chain connected to the Debug Access Port.
/// This setting impacts the RAM requirements of the Debug Unit. Valid range is 1 .. 255.
#define DAP_JTAG_DEV_CNT        8               ///< Maximum number of JTAG devices on scan chain

/// Default communication mode on the Debug Access Port.
/// Used for the command \ref DAP_Connect when Port Default mode is selected.
#define DAP_DEFAULT_PORT        1               ///< Default JTAG/SWJ Port Mode: 1 = SWD, 2 = JTAG.

/// Default communication speed on the Debug Access Port for SWD and JTAG mode.
/// Used to initialize the default SWD/JTAG clock frequency.
/// The command \ref DAP_SWJ_Clock can be used to overwrite this default setting.
#define DAP_DEFAULT_SWJ_CLOCK   10000000         ///< Default SWD/JTAG clock frequency in Hz.

/// Maximum Package Size for Command and Response data.
/// This configuration settings is used to optimized the communication performance with the
/// debugger and depends on the USB peripheral. Change setting to 1024 for High-Speed USB.
#define DAP_PACKET_SIZE         64              ///< USB: 64 = Full-Speed, 1024 = High-Speed.

/// Maximum Package Buffers for Command and Response data.
/// This configuration settings is used to optimized the communication performance with the
/// debugger and depends on the USB peripheral. For devices with limited RAM or USB buffer the
/// setting can be reduced (valid range is 1 .. 255). Change setting to 4 for High-Speed USB.
#define DAP_PACKET_COUNT        12              ///< Buffers: 64 = Full-Speed, 4 = High-Speed.

#define DAP_PACKET_QUEUE_SIZE (DAP_PACKET_COUNT+8)

/// Debug Unit is connected to fixed Target Device.
/// The Debug Unit may be part of an evaluation board and always connected to a fixed
/// known device.  In this case a Device Vendor and Device Name string is stored which
/// may be used by the debugger or IDE to configure device parameters.
#define TARGET_DEVICE_FIXED     0               ///< Target Device: 1 = known, 0 = unknown;

#if TARGET_DEVICE_FIXED
#define TARGET_DEVICE_VENDOR    ""              ///< String indicating the Silicon Vendor
#define TARGET_DEVICE_NAME      ""              ///< String indicating the Target Device
#endif

///@}

#define PIN_SWCLK_BITPOS        (6)
#define PIN_SWCLK               (1<<PIN_SWCLK_BITPOS)
#define PIN_SWDIO_BITPOS        (5)
#define PIN_SWDIO               (1<<PIN_SWDIO_BITPOS)
#define PIN_nRESET_BITPOS       (1)
#define PIN_nRESET              (1<<PIN_nRESET_BITPOS)

#define PIN_LED_CON_BITPOS      (0)
#define PIN_LED_CON             (1<<PIN_LED_CON_BITPOS)
#define PIN_LED_RUN_BITPOS      (1)
#define PIN_LED_RUN             (1<<PIN_LED_RUN_BITPOS)

#define PIN_LED_ACT_BITPOS      (4)
#define PIN_LED_ACT             (1<<PIN_LED_ACT_BITPOS)

/*
SWD functionality
*/

static __inline void PORT_SWD_SETUP (void)
{
  GPIOA_BSRR = PIN_SWDIO;
  GPIOB_BSRR = PIN_SWCLK;
  
  GPIOA_MODER   &= ~( (0x3 << (PIN_SWDIO_BITPOS << 1)) );
  GPIOA_MODER   &= ~( (0x3 << (PIN_SWCLK_BITPOS << 1)) );
  
  GPIOA_MODER   |=  ( (0x1 << (PIN_SWDIO_BITPOS << 1)) );
  GPIOA_MODER   |=  ( (0x1 << (PIN_SWCLK_BITPOS << 1)) );

  GPIOA_OTYPER  &= ~( PIN_SWDIO );
  GPIOA_OTYPER  &= ~( PIN_SWCLK );
  
  GPIOA_OSPEEDR &= ~( (0x3 << (PIN_SWDIO_BITPOS << 1)) );
  GPIOA_OSPEEDR &= ~( (0x3 << (PIN_SWCLK_BITPOS << 1)));

  GPIOA_OSPEEDR |=  ( (0x3 << (PIN_SWDIO_BITPOS << 1)) );
  GPIOA_OSPEEDR |=  ( (0x3 << (PIN_SWCLK_BITPOS << 1)) );
}

static __inline void PORT_OFF (void)
{
  GPIOA_BRR = PIN_SWDIO;
  GPIOA_BRR = PIN_SWCLK;
  GPIOA_MODER &= ~( (0x3 << (PIN_SWDIO_BITPOS << 1)) );
  GPIOA_MODER &= ~( (0x3 << (PIN_SWCLK_BITPOS << 1)) );
}

static __inline void PIN_SWCLK_TCK_SET (void)
{
  GPIOA_BSRR = PIN_SWCLK;
}

static __inline void PIN_SWCLK_TCK_CLR (void)
{
  GPIOA_BRR = PIN_SWCLK;
}

static __inline uint32_t PIN_SWDIO_TMS_IN  (void)
{
  return (GPIOA_IDR & PIN_SWDIO) ? 0x1 : 0x0;
}

static __inline void PIN_SWDIO_TMS_SET (void)
{
  GPIOA_BSRR = PIN_SWDIO;
}

static __inline void PIN_SWDIO_TMS_CLR (void)
{
  GPIOA_BRR = PIN_SWDIO;
}

static __inline uint32_t PIN_SWDIO_IN (void)
{
  return (GPIOA_IDR & PIN_SWDIO) ? 0x1 : 0x0;
}

static __inline void PIN_SWDIO_OUT (uint32_t bit)
{
  if (bit & 1)
    GPIOA_BSRR = PIN_SWDIO;
  else
    GPIOA_BRR = PIN_SWDIO;
}

static __inline void     PIN_SWDIO_OUT_ENABLE  (void)
{
  GPIOA_MODER &= ~( (0x3 << (PIN_SWDIO_BITPOS << 1)) );
  GPIOA_MODER |=  ( (0x1 << (PIN_SWDIO_BITPOS << 1)) );
}

static __inline void     PIN_SWDIO_OUT_DISABLE (void)
{
  GPIOA_MODER &= ~( (0x3 << (PIN_SWDIO_BITPOS << 1)) );
}

/*
JTAG-only functionality (not used in this application)
*/

static __inline void PORT_JTAG_SETUP (void) {}

static __inline uint32_t PIN_TDI_IN  (void) {  return 0; }

static __inline void     PIN_TDI_OUT (uint32_t bit) { (void)bit; }

static __inline uint32_t PIN_TDO_IN (void) {  return 0; }

static __inline uint32_t PIN_nTRST_IN (void) {  return 0; }

static __inline void     PIN_nTRST_OUT  (uint32_t bit) { (void)bit; }

/*
other functionality not applicable to this application
*/
static __inline uint32_t PIN_SWCLK_TCK_IN  (void) {
	return (GPIOA_IDR & PIN_SWCLK) ? 0x1 : 0x0;
}

static __inline uint32_t PIN_nRESET_IN  (void) {
	return (GPIOB_IDR & PIN_nRESET) ? 0x1 : 0x0;
}

static __inline void PIN_nRESET_OUT (uint32_t bit) {
	if (bit & 0x1)
	{
	  GPIOB_BSRR = PIN_nRESET;
	} else {
		GPIOB_BRR = PIN_nRESET;
	}
}

static __inline void LED_CONNECTED_OUT (uint32_t bit) {
  if (bit & 0x1) {
    GPIOA_MODER &= ~( (0x3 << (PIN_LED_CON_BITPOS << 1)) );
    GPIOA_MODER |=  ( (0x1 << (PIN_LED_CON_BITPOS << 1)) );
  } else {
    GPIOA_MODER &= ~( (0x3 << (PIN_LED_CON_BITPOS << 1)) );
  }
}

static __inline void LED_RUNNING_OUT (uint32_t bit) {
  if (bit & 0x1) {
    GPIOA_MODER &= ~( (0x3 << (PIN_LED_RUN_BITPOS << 1)) );
    GPIOA_MODER |=  ( (0x1 << (PIN_LED_RUN_BITPOS << 1)) );
  } else {
    GPIOA_MODER &= ~( (0x3 << (PIN_LED_RUN_BITPOS << 1)) );
  }
}

static __inline void LED_ACTIVITY_OUT (uint32_t bit) {
  if (bit & 0x1) {
    GPIOA_MODER &= ~( (0x3 << (PIN_LED_ACT_BITPOS << 1)) );
    GPIOA_MODER |=  ( (0x1 << (PIN_LED_ACT_BITPOS << 1)) );
  } else {
    GPIOA_MODER &= ~( (0x3 << (PIN_LED_ACT_BITPOS << 1)) );
  }
}

static __inline void DAP_SETUP (void) {
	GPIOA_MODER &= ~( (0x3 << (PIN_LED_RUN_BITPOS << 1)) );
	GPIOA_MODER &= ~( (0x3 << (PIN_LED_CON_BITPOS << 1)) );
	GPIOA_BRR = (PIN_LED_RUN | PIN_LED_CON);
}

static __inline uint32_t RESET_TARGET (void) { return 0; }

#endif