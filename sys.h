#ifndef SYS_H_INCLUDED
#define SYS_H_INCLUDED

#include "simplelink.h"
#include "osi.h"

void sys_restart(void);
_i16 sys_queue_read_ptr(OsiMsgQ_t *queue, void **ptr, OsiTime_t timeout);
_i16 sys_queue_write_ptr(OsiMsgQ_t *queue, void *ptr, OsiTime_t timeout);

#endif // SYS_H_INCLUDED
