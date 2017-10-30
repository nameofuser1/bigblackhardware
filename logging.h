#ifndef LOGGING_H
#define LOGGING_H

#include <stdlib.h>
#include "osi.h"
#include "uart_if.h"
#include "common.h"

OsiLockObj_t print_lock;

void logging_init(void);


#define SYNCRONIZED_UART_PRINT(fmt, ...)    {\
                                                osi_LockObjLock(&print_lock, OSI_WAIT_FOREVER);\
                                                UART_PRINT(fmt, ##__VA_ARGS__);\
                                                osi_LockObjUnlock(&print_lock);\
                                            }

#define SYNCRONIZED_ERR_PRINT(e)    {\
                                        osi_LockObjLock(&print_lock, 0);\
                                        ERR_PRINT(e);\
                                        osi_LockObjUnlock(&print_lock);\
                                    }



#define SERIAL_LOG  SYNCRONIZED_UART_PRINT

#define OSI_COMMON_LOG  SERIAL_LOG
#define OSI_ERROR_LOG   SYNCRONIZED_ERR_PRINT


#define OSI_ARRAY_LOG(fmt, arr, arr_len, ...)   {\
                                                    osi_LockObjLock(&print_lock, OSI_WAIT_FOREVER);\
                                                    if(fmt != NULL) {UART_PRINT(fmt, ##__VA_ARGS__);}\
                                                    for(int i=0; i<arr_len; i++) {UART_PRINT("0x%02x ", arr[i]);}\
                                                    UART_PRINT("\r\n");\
                                                    osi_LockObjUnlock(&print_lock);\
                                                }

#define OSI_ASSERT_ON_ERROR(error_code)\
            {\
                if(error_code < 0) \
                {\
                    SYNCRONIZED_ERR_PRINT(error_code);\
                    return error_code;\
                }\
            }


#define OSI_ASSERT_WITH_EXIT(error_code, hndl)\
            {\
                if(error_code < 0) \
                {\
                    SYNCRONIZED_ERR_PRINT(error_code);\
                    osi_TaskDelete(&hndl);\
                }\
            }


#define OSI_ASSERT_WITHOUT_EXIT(error_code)\
            {\
                if(error_code < 0) \
                {\
                    SYNCRONIZED_ERR_PRINT(error_code);\
                }\
            }


#endif // LOGGING_H
