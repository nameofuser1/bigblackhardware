#include "logging.h"


void logging_init(void) {
    osi_LockObjCreate(&print_lock);
}
