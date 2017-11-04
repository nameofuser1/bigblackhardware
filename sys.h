#ifndef SYS_H_INCLUDED
#define SYS_H_INCLUDED

#include "simplelink.h"
#include "osi.h"
#include "logging.h"


void sys_restart(void);
_i16 sys_queue_read_ptr(OsiMsgQ_t *queue, void **ptr, OsiTime_t timeout);
_i16 sys_queue_write_ptr(OsiMsgQ_t *queue, void *ptr, OsiTime_t timeout);


#define SYS_ASSERT_CRITICAL(status)     OSI_ASSERT_WITHOUT_EXIT(status);\
                                        if(status < 0) {\
                                            OSI_COMMON_LOG("CRITICAL ERROR. RESTARTING\r\n");\
                                            sys_restart();\
                                        }

#define MCU_RESET_ON    0
#define MCU_RESET_OFF   1

void sys_reset_mcu(_u8 status);

#endif // SYS_H_INCLUDED
