#include "sys.h"

#include "hw_types.h"
#include "prcm.h"
#include "common.h"
#include "logging.h"

#include "simplelink.h"


void sys_restart(void) {
    OSI_COMMON_LOG("Restarting system\r\n");
    sl_Stop(1000);
    PRCMMCUReset(true);
}
