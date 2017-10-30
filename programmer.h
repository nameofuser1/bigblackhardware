#ifndef PROGRAMMER_H_INCLUDED
#define PROGRAMMER_H_INCLUDED

#include "hw_types.h"
#include "simplelink.h"
#include "osi.h"

_i16 programmer_start(void);
_i16 programmer_resume(void);
_i16 programmer_pause(void);
_i16 programmer_set_queues(OsiMsgQ_t *in_queue, OsiMsgQ_t *out_queue);

#endif // PROGRAMMER_H_INCLUDED
