#ifndef PACKET_HANDLER_H_INCLUDED
#define PACKET_HANDLER_H_INCLUDED


typedef enum {CONN_UART, CONN_PROGRAMMER} ConnectionType;

typedef struct _connection_info {

    _i16            hndl;
    ConnectionType  type;

} ConnectionInfo;


#endif // PACKET_HANDLER_H_INCLUDED
