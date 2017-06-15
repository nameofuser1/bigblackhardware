#ifndef PACKETS_H_INCLUDED
#define PACKETS_H_INCLUDED

#include "simplelink.h"
#include "protocol.h"

#define PACKET_SUCCESS              0
#define PACKET_WRONG_START_BYTE    -1
#define PACKET_WRONG_SIZE          -2
#define PACKET_WRONG_TYPE          -3


typedef enum {
    /* Control packets */
    ProgrammerInitPacket = 0,
    ProgrammerStopPacket,
    UartInitPacket,
    UartStopPacket,
    ResetPacket,
    ACKPacket,
    CloseConnectionPacket,
    NetworkConfigurationPacket,
    EnableEncryptionPacket,
    EnableSignPacket,
    EncryptionConfigPacket,
    SignConfigPacket,
    ObserverKeyPacket,

    /* Programmer packets */
    LoadMCUInfoPacket,
    ProgramMemoryPacket,
    ReadMemoryPacket,
    MemoryPacket,
    CMDPacket,

    /* UART packets */
    UartConfigurationPacket,
    UartDataPacket
} PacketType;


typedef struct packet_header {

    PacketType  type;
    _u16        data_size;

    /* Flags */
    _u8         compression;
    _u8         encryption;
    _u8         sign;

} PacketHeader;


typedef struct packet {

    PacketHeader        header;
    _u8                 *packet_data;

} Packet;


_i8 create_packet(Packet *packet, _u16 data_size);
_i8 parse_header(_u8 *header, PacketHeader *packet_h);


#endif // PACKETS_H_INCLUDED
