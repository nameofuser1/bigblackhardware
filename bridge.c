#include "bridge.h"

#include "hw_types.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "uart.h"
#include "osi.h"

#include "sys.h"
#include "config.h"
#include "logging.h"


#define BRIDGE_UART         UARTA1_BASE
#define BRIDGE_PERIPH       PRCM_UARTA1


static void bridge_write(_u8 *data, _u16 len);
static void bridge_task(void *pvParameters);


_i16 bridge_start(_u32 baudrate) {
    _i16 status = 0;

    MAP_UARTConfigSetExpClk(BRIDGE_UART,
                            MAP_PRCMPeripheralClockGet(BRIDGE_PERIPH),
                            baudrate,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    MAP_UARTEnable(BRIDGE_UART);

    /* Create bridge task */
    status = osi_TaskCreate(bridge_task, BRIDGE_TASK_NAME, BRIDGE_TASK_STACK_SIZE,
                            NULL, BRIDGE_TASK_PRIO, NULL);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


static void bridge_task(void *pvParameters) {
    char *bridge_msg = "Hello from fucking UART1\r\n";
    _u16 bridge_msg_len = strlen(bridge_msg);

    while(1) {
        bridge_write((_u8*)bridge_msg, bridge_msg_len);
        osi_Sleep(2000);
    }
}


static void bridge_write(_u8 *data, _u16 len) {
    for(_i32 i=0; i<len; i++) {
        MAP_UARTCharPut(BRIDGE_UART, data[i]);
    }
}
