#ifndef PROGRAMMER_H_INCLUDED
#define PROGRAMMER_H_INCLUDED

#include "osi.h"


typedef {

}ProgrammerConfig;


_i16 programmer_start(void);
_i16 programmer_resume(OsiMsgQ_t *in_queue, OsiMsgQ_t *out_queue);
_i16 programmer_pause(void);

#endif // PROGRAMMER_H_INCLUDED
