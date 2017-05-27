#ifndef LOGGING_H
#define LOGGING_H

#include <stdlib.h>
#include <osi.h>

OsiLockObj_t print_lock;

void logging_init(void);


#define SYNCRONIZED_UART_PRINT(fmt, ...)    {\
                                                osi_LockObjLock(&print_lock, 0);\
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
                    osi_TaskDelete(&hdnl);\
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
