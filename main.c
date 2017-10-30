//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Free-RTOS Demo
// Application Overview - The objective of this application is to showcasing the
//                        FreeRTOS feature like Multiple task creation, Inter
//                        task communication using queues. Two tasks and one
//                        queue is created. one of the task sends a constant
//                        message into the queue and the other task receives the
//                        same from the queue. after receiving every message, it
//                        displays that message over UART.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_FreeRTOS_Application
// or
// docs\examples\CC32xx_FreeRTOS_Application.pdf
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup freertos_demo
//! @{
//
//*****************************************************************************

//*****************************************************************************
// Same application can be used to demonstrate TI-RTOS demo with a small change
// in CCS project proerties:
//      a. add ti_rtos_config in workspace and add as dependencies in
//         project setting Build->Dependecies->add
//      b. Define USE_TIRTOS in Properties->Build->Advanced Options->
//         Predefined Symbols instead of USE_FREERTOS
//      c. add ti_rtos.a in linker files search option
//*****************************************************************************

// Standard includes.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "osi.h"

// Driverlib includes
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "prcm.h"
#include "utils.h"

// Common interface includes
#include "simplelink.h"
#include "socket.h"
#include "common.h"
#include "uart_if.h"

#include "pinmux.h"

#include "wlan_config.h"
#include "udp_resolver.h"
#include "wired_configurator.h"
#include "logging.h"
#include "packet_handler.h"
#include "packets.h"

#include "programmer.h"
#include "bridge.h"

#include "config.h"
#include "sys.h"



//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#ifndef USE_TIRTOS
/* in case of TI-RTOS don't include startup_*.c in app project */
#if defined(gcc) || defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//*****************************************************************************
static void BoardInit();


#ifdef USE_FREERTOS
//*****************************************************************************
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//*****************************************************************************

//*****************************************************************************
//
//! \brief Application defined hook (or callback) function - assert
//!
//! \param[in]  pcFile - Pointer to the File Name
//! \param[in]  ulLine - Line Number
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    //Handle Assert here
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void)
{
    //Handle Idle Hook for Profiling, Power Management etc
}

//*****************************************************************************
//
//! \brief Application defined malloc failed hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationMallocFailedHook()
{
    //Handle Memory Allocation Errors
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined stack overflow hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationStackOverflowHook( OsiTaskHandle *pxTask,
                                   signed char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
    while(1)
    {
    }
}
#endif //USE_FREERTOS


/* Global variables for initialization task*/
static OsiTaskHandle init_handle;


/* Setting up default WLAN config in the absence of saved one */
static void get_default_config(WlanConfig *config) {
    config->mode = WLAN_CONFIG_STA;

    config->ssid_len = strlen(SSID_NAME);
    mem_copy(config->ssid, SSID_NAME, config->ssid_len);

    config->pwd_len = strlen(SSID_PWD);
    mem_copy(config->pwd, SSID_PWD, config->pwd_len);

    config->sec = WLAN_CONFIG_WPA_WPA2;
}


/* **************************************************
 *                 Initialization task              *
 ****************************************************
 *                                                  *
 *   Configures WLAN module and load all necessary  *
 *      middleware drivers                          *
 *                                                  *
 * ************************************************ */
static void vInitializationTask(void *pvParameters) {
    int status;
    WlanConfig   config;
    int wlan_connecting_time;

    /*** MCU RESET ***/
    sys_reset_mcu(MCU_RESET_OFF);

    /* Initializing WLAN module first in order to enable simplelink */
    status = wlan_init();
    OSI_ASSERT_WITH_EXIT(status, init_handle);

    /* Starting WIRED configuration listener */
    wired_conf_start();

    /* Set up WLAN configuration */
    mem_set(&config, 0, sizeof config);
    if(wlan_load_config(&config) != SUCCESS) {
        OSI_COMMON_LOG("Failed to load config. Initializing with default one.\r\n");
        get_default_config(&config);
    }

    wlan_print_config(&config);

    status = wlan_start(&config);
    OSI_ASSERT_WITH_EXIT(status, init_handle);

    /* Waiting for connection */
    WlanStatus wl_status;
    while((wl_status = wlan_status()) != WLAN_CONNECTED) {
        /* Check whether error occurred whilst connecting */
        if(wl_status != WLAN_CONNECTING) {
            OSI_COMMON_LOG("Failed to initialize WLAN module\r\n");
            goto init_exit;
        }

        osi_Sleep(1000);
    }

    /*** STARTING PROGRAMMER ***/
    OSI_COMMON_LOG("Starting programming module...\r\n");
    programmer_start();

    /*** STARTING BRIDGE ***/
    OSI_COMMON_LOG("Starting bridge module... \r\n");
    bridge_start(115200);

    /*** STARTING UDP RESOLVER ***/
    OSI_COMMON_LOG("UDP resolver initialization\r\n");
    UdpResolverCfg cfg;
    cfg.key = DEFAULT_DEVICE_KEY;
    cfg.port = UDP_RESOLVER_PORT;

    status = udp_resolver_start(&cfg);
    OSI_ASSERT_WITH_EXIT(status, init_handle);

    /*** STARTING LISTENING AND PACKET HANDLING  ***/
    packet_handler_start();

init_exit:
    OSI_COMMON_LOG("Out of init task\r\n");
    osi_TaskDelete(&init_handle);
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t    CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
  //
  // Enable Processor
  //
  MAP_IntMasterEnable();
  MAP_IntEnable(FAULT_SYSTICK);

  PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//!  main function handling the freertos_demo.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
int main( void )
{
    //
    // Initialize the board
    //
    BoardInit();

    PinMuxConfig();

    //
    // Initializing the terminal
    //
    InitTerm();

    //
    // Clearing the terminal
    //
    ClearTerm();

    //
    // Display Banner
    //
    DisplayBanner(APP_NAME);

    /* Initialize logging module */
    logging_init();

    VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    osi_TaskCreate(vInitializationTask, INIT_TASK_NAME, INIT_TASK_STACK_SIZE,
                   NULL, INIT_TASK_PRIORITY, &init_handle);

    osi_start();

    return 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
