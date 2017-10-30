#ifndef PACKETS_H_INCLUDED
#define PACKETS_H_INCLUDED

#include "simplelink.h"
#include "protocol.h"

#define PACKET_SUCCESS              0
#define PACKET_WRONG_START_BYTE    -1
#define PACKET_WRONG_SIZE          -2
#define PACKET_WRONG_TYPE          -3

#define PACKET_HEADER_SIZE          PL_PACKET_HEADER_SIZE


#define COMPRESSION_ON  1
#define COMPRESSION_OFF 0

#define SIGN_ON         1
#define SIGN_OFF        0

#define ENCRYPTION_ON   1
#define ENCRYPTION_OFF  0


typedef enum {
    ControlGroup = 0,
    ProgrammerGroup,
    UartGroup,
    PACKETS_GROUPS_NUM

} PacketGroup;


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
    UartDataPacket,

    PacketsNumber
} PacketType;


#define CONTROL_PACKETS_NUM         (ObserverKeyPacket+1)
#define PROGRAMMER_PACKETS_NUM      (CMDPacket - LoadMCUInfoPacket + 1)
#define UART_PACKETS_NUM            (UartDataPacket - UartConfigurationPacket + 1)

#define CONTROL_PACKETS_SHIFT       (0)
#define PROGRAMMER_PACKETS_SHIFT    (CONTROL_PACKETS_NUM)
#define UART_PACKETS_SHIFT          (CONTROL_PACKETS_NUM + PROGRAMMER_PACKETS_NUM)

typedef struct packet_header {

    PacketType  type;
    PacketGroup group;
    _u16        data_size;

    /* Flags */
    _u8         compression;
    _u8         encryption;
    _u8         sign;

} PacketHeader;


typedef struct packet {

    PacketHeader        header;
    _u8                 raw_header[PL_PACKET_HEADER_SIZE];
    _u8                 packet_data[1024];

} Packet;


_i16    create_packet(Packet *packet, PacketType type, _u8 comp,
                  _u8 sign, _u8 enc, _u8 *data, _u16 data_size);
_i16     parse_header(_u8 *header, PacketHeader *packet_h);
_i16     update_header(Packet *packet);
_i16     initialize_packets_pool(_u8 num);
_i16     get_packet_from_pool(Packet **packet);
_i16     release_packet(Packet *packet);

PacketGroup     get_type_group(PacketType type);
void            print_packet(Packet *packet);


#endif // PACKETS_H_INCLUDED
