#ifndef PACKET_HANDLER_H_INCLUDED
#define PACKET_HANDLER_H_INCLUDED

#include "simplelink.h"
#include "osi.h"

typedef enum {CONN_UART, CONN_PROGRAMMER} ConnectionType;

typedef struct _connection_info {

    _i16            hndl;
    ConnectionType  type;

    OsiMsgQ_t       in_queue;
    OsiMsgQ_t       out_queue;

} ConnectionInfo;


_i8 packet_handler_start(void);
_i8 packet_handler_stop(void);


#endif // PACKET_HANDLER_H_INCLUDED
