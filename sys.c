#include "sys.h"

#include "hw_types.h"
#include "hw_memmap.h"

#include "prcm.h"
#include "common.h"

#include "pin.h"
#include "hw_gpio.h"
#include "gpio.h"
#include "gpio_if.h"


typedef struct _queue_ptr_wrapper {
    void *ptr;
} QueuePtrWrapper;


/*******************************************************
    Restarts device
********************************************************/
void sys_restart(void) {
    sl_Stop(1000);
    PRCMMCUReset(true);
}


/*******************************************************
    Since Queues send messages by copy we need wrapper
    for pointers in order to pass big data chunks
    without copying
 *******************************************************/
_i16 sys_queue_read_ptr(OsiMsgQ_t *queue, void **ptr, OsiTime_t timeout) {
    _i16 status;
    QueuePtrWrapper temp;

    status = osi_MsgQRead(queue, &temp, timeout);
    *ptr = temp.ptr;

    return status;
}


/*******************************************************
    Since Queues send messages by copy we need wrapper
    for pointers in order to pass big data chunks
    without copying
 *******************************************************/
_i16 sys_queue_write_ptr(OsiMsgQ_t *queue, void *ptr, OsiTime_t timeout) {
    _i16 status;
    QueuePtrWrapper temp = {.ptr = ptr};

    status = osi_MsgQWrite(queue, &temp, timeout);
    return status;
}


/*********************************************************
    Pulls connected MCU reset line down if status is 1
    otherwise pulls it up
**********************************************************/
void sys_reset_mcu(_u8 status) {
    GPIO_IF_Set(6, GPIOA0_BASE, GPIO_PIN_6, status);
}
