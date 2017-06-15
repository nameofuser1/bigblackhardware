#include "packets.h"

static _u8          get_type_from_header(_u8 *header);
static _u16         get_size_from_header(_u8 *header);
static PacketType   get_control_packet_type(_u8 _type);
static PacketType   get_programmer_packet_type(_u8 _type);
static PacketType   get_uart_packet_type(_u8 _type);
static inline _u8   get_flag_bit(_u8 *header, _u8 bit);



_i8 create_packet(Packet *packet, _u16 data_size) {
    packet->packet_data = mem_Malloc(data_size);

    if(packet->packet_data == NULL) {
        return -1;
    }

    return 0;
}


_i8 parse_header(_u8 *header, PacketHeader *packet_h) {
    _u16 size;
    _u8 _type;
    PacketType type;

    if(header[0] != PL_START_FRAME_BYTE) {
        return PACKET_WRONG_START_BYTE;
    }

    size = get_size_from_header(header);
    if(size > PL_MAX_DATA_LENGTH) {
        return PACKET_WRONG_SIZE;
    }

    _type = get_type_from_header(header);

    if(_type & PL_CONTROL_PACKETS_MSK) {
        type = get_control_packet_type(_type);
    }
    else if(_type & PL_PROGRAMMER_PACKETS_MSK) {
        type = get_programmer_packet_type(_type);
    }
    else if(_type & PL_UART_PACKETS_MSK) {
        type = get_uart_packet_type(_type);
    }
    else {
        return PACKET_WRONG_TYPE;
    }

    if(type < 0) {
        return PACKET_WRONG_TYPE;
    }

    packet_h->type = type;
    packet_h->data_size = size;
    packet_h->compression = get_flag_bit(header, PL_FLAG_COMPRESSION_BIT);
    packet_h->encryption = get_flag_bit(header, PL_FLAG_ENCRYPTION_BIT);
    packet_h->sign = get_flag_bit(header, PL_FLAG_SIGN_BIT);

    return PACKET_SUCCESS;
}


static _u16 get_size_from_header(_u8 *header) {
	_u16 size = 0;

	for(_u8 i=0; i<PL_SIZE_FIELD_SIZE; i++)
	{
		size |= (header[PL_SIZE_FIELD_OFFSET + i] << 8*(PL_SIZE_FIELD_SIZE - 1 - i));
	}

	return size;
}


static _u8 get_type_from_header(_u8 *header) {
	return header[PL_TYPE_FIELD_OFFSET];
}


static inline _u8 get_flag_bit(_u8 *header, _u8 bit) {
    return (header[PL_FLAGS_FIELD_OFFSET] >> bit) & 0xFF;
}



/* *********************** MAPPINGS FROM PL TO PacketType ****************************/


static PacketType get_uart_packet_type(_u8 _type) {
    PacketType type;

    switch(_type) {

        case PL_UART_CONFIGURATION:
            type = UartConfigurationPacket;
            break;

        case PL_UART_DATA:
            type = UartDataPacket;
            break;

        default:
            type = -1;
    }

    return type;
}


static PacketType get_programmer_packet_type(_u8 _type) {
    PacketType type;

    switch(_type) {

        case PL_LOAD_MCU_INFO:
            type = LoadMCUInfoPacket;
            break;

        case PL_PROGRAM_MEMORY:
            type = ProgramMemoryPacket;
            break;

        case PL_READ_MEMORY:
            type = ReadMemoryPacket;
            break;

        case PL_MEMORY:
            type = MemoryPacket;
            break;

        case PL_CMD:
            type = CMDPacket;
            break;

        default:
            type = -1;
    }

    return type;
}


static PacketType get_control_packet_type(_u8 _type) {
    PacketType type;

    switch(_type) {
        case PL_PROGRAMMER_INIT:
            type = ProgrammerInitPacket;
            break;

        case PL_PROGRAMMER_STOP:
            type = ProgrammerStopPacket;
            break;

        case PL_UART_INIT:
            type = UartInitPacket;
            break;

        case PL_UART_STOP:
            type = UartStopPacket;
            break;

        case PL_RESET:
            type = ResetPacket;
            break;

        case PL_ACK_PACKET:
            type = ACKPacket;
            break;

        case PL_CLOSE_CONNECTION:
            type = CloseConnectionPacket;
            break;

        case PL_NETWORK_CONFIGURATION:
            type = NetworkConfigurationPacket;
            break;

        case PL_ENABLE_ENCRYPTION:
            type = EnableEncryptionPacket;
            break;

        case PL_ENABLE_SIGN:
            type = EnableSignPacket;
            break;

        case PL_SET_ENCRYPTION_KEYS:
            type = EncryptionConfigPacket;
            break;

        case PL_SET_SIGN_KEYS:
            type = SignConfigPacket;
            break;

        case PL_SET_OBSERVER_KEY:
            type = ObserverKeyPacket;
            break;

        default:
            type = -1;
    }

    return type;
}
