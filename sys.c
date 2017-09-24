#include "sys.h"

#include "hw_types.h"
#include "prcm.h"
#include "common.h"


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
