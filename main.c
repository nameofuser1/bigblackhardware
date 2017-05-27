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
#include <common.h>
#include "uart_if.h"

#include "pinmux.h"

#include "wlan_config.h"
#include "logging.h"
//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION     "1.1.1"
#define UART_PRINT              Report
#define SPAWN_TASK_PRIORITY     9
#define OSI_STACK_SIZE          2048
#define APP_NAME                "FreeRTOS Demo"
#define MAX_MSG_LENGTH			16

#ifdef SSID_NAME
#undef SSID_NAME
#endif // SSID_NAME

#define SSID_NAME               "ChtoZaSet"
#define SSID_PWD                "GrEs4242SeRg"

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
static void vTestTask1( void *pvParameters );
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

//******************************************************************************
//
//! First test task
//!
//! \param pvParameters is the parameter passed to the task while creating it.
//!
//!    This Function
//!        1. Receive message from the Queue and display it on the terminal.
//!
//! \return none
//
//******************************************************************************
void vTestTask1( void *pvParameters )
{
    static int stopped = 0;
    for( ;; )
    {
		OSI_COMMON_LOG("Hey you\r\n");

		if(wlan_connected() && !stopped) {
            OSI_COMMON_LOG("Connected to WLAN. Now Stopping.\r\n");
            stopped = 1;
            wlan_stop();
		}
		osi_Sleep(500);
    }
}


static WlanConfig *config;
static OsiTaskHandle init_handle;

/* **************************************************
 *                 Initialization task              *
 ****************************************************
 *                                                  *
 *   Configures WLAN module and load all necessary  *
 *      middleware drivers                          *
 *                                                  *
 * ************************************************ */
 static void vInitializationTask(void *pvParameters) {
    /* Set WLAN configuration */
    config = mem_Malloc(sizeof *config);
    mem_set(config, '0', sizeof *config);

    config->mode = CONFIG_STA;

    config->ssid_len = strlen(SSID_NAME);
    mem_copy(config->ssid, SSID_NAME, config->ssid_len);

    config->pwd_len = strlen(SSID_PWD);
    mem_copy(config->pwd, SSID_PWD, config->pwd_len);

    config->sec = CONFIG_WPA_WPA2;

    OSI_COMMON_LOG("WLAN INIT\r\n");

    /* Starting WLAN */
    wlan_init();
    wlan_start(config);

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

    osi_TaskCreate( vTestTask1, "TASK1",\
    							OSI_STACK_SIZE, NULL, 1, NULL );

    osi_TaskCreate(vInitializationTask, "InitTask", OSI_STACK_SIZE, NULL, 2, &init_handle);

    osi_start();

    return 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
