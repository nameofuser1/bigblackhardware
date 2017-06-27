#ifndef PACKET_HANDLER_H_INCLUDED
#define PACKET_HANDLER_H_INCLUDED

#include "simplelink.h"
#include "osi.h"
#include "packets.h"


typedef struct _connection_info {

    _i16            hndl;
    PacketGroup     group;

    OsiMsgQ_t       in_queue;
    OsiMsgQ_t       out_queue;

} ConnectionInfo;


_i8 packet_handler_start(void);
_i8 packet_handler_stop(void);


#endif // PACKET_HANDLER_H_INCLUDED
