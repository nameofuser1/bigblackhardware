#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include <simplelink.h>
#include "packets.h"
#include "osi.h"


_i16 process_packet(OsiMsgQ_t *out_queue, Packet *packet);



#endif // CONTROLLER_H_INCLUDED
