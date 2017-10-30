#include "packets.h"
#include "pool.h"
#include "common.h"
#include "logging.h"

/* Local functions prototypes */
static _u8          get_type_from_header(_u8 *header);
static _u16         get_size_from_header(_u8 *header);
static PacketType   get_control_packet_type(_u8 _type);
static PacketType   get_programmer_packet_type(_u8 _type);
static PacketType   get_uart_packet_type(_u8 _type);
static inline _u8   get_flag_bit(_u8 *header, _u8 bit);
static inline void  set_flag_bit(_u8 *header, _u8 bit);

/* Pool for packets */
static pool_t       packets_pool;


_i16 create_packet(Packet *packet, PacketType type, _u8 comp,
                  _u8 sign, _u8 enc, _u8 *data, _u16 data_size) {
    _i16 status;

    PacketHeader *header = &packet->header;
    header->compression = comp;
    header->sign = sign;
    header->encryption = enc;
    header->data_size = data_size;

    status = update_header(packet);
    OSI_ASSERT_ON_ERROR(status);

    if(data != NULL && data_size != 0) {
        memcpy(packet->packet_data, data, data_size);
    }

    return status;
}


_i16 initialize_packets_pool(_u8 num) {
    pool_create(&packets_pool, NULL, sizeof(Packet), num);

    return 0;
}


_i16 get_packet_from_pool(Packet **packet) {
    return pool_get(&packets_pool, (void**)packet);
}


_i16 release_packet(Packet *packet) {
    return pool_release(&packets_pool, packet);
}


_i16 parse_header(_u8 *header, PacketHeader *packet_h) {
    _u16 size;
    _i16 _type;

    if(header[0] != PL_START_FRAME_BYTE) {
        return PACKET_WRONG_START_BYTE;
    }

    size = get_size_from_header(header);
    if(size > PL_MAX_DATA_LENGTH) {
        return PACKET_WRONG_SIZE;
    }

    _type = get_type_from_header(header);

    if(_type & PL_CONTROL_PACKETS_MSK) {
        OSI_COMMON_LOG("Trying to get control type\r\n");
        packet_h->group = ControlGroup;
        _type = get_control_packet_type(_type);
    }
    else if(_type & PL_PROGRAMMER_PACKETS_MSK) {
        OSI_COMMON_LOG("Trying to get programmer type\r\n");
        packet_h->group = ProgrammerGroup;
        _type = get_programmer_packet_type(_type);
    }
    else if(_type & PL_UART_PACKETS_MSK) {
        OSI_COMMON_LOG("Trying to get UART type\r\n");
        packet_h->group = UartGroup;
        _type = get_uart_packet_type(_type);
    }
    else {
        return PACKET_WRONG_TYPE;
    }

    if(_type < 0) {
        return PACKET_WRONG_TYPE;
    }

    packet_h->type = _type;
    packet_h->data_size = size;
    packet_h->compression = get_flag_bit(header, PL_FLAG_COMPRESSION_BIT);
    packet_h->encryption = get_flag_bit(header, PL_FLAG_ENCRYPTION_BIT);
    packet_h->sign = get_flag_bit(header, PL_FLAG_SIGN_BIT);

    return PACKET_SUCCESS;
}


_i16 update_header(Packet *packet) {
    PacketHeader header = packet->header;
    _u16 data_size = header.data_size;
    _u8 *raw_header = packet->raw_header;

    if(data_size > PL_MAX_DATA_LENGTH) {
        return PACKET_WRONG_SIZE;
    }

    raw_header[0] = PL_START_FRAME_BYTE;

    for(int i=0; i<PL_SIZE_FIELD_SIZE; i++) {
        raw_header[PL_SIZE_FIELD_OFFSET+i] = (data_size >> 8*(PL_SIZE_FIELD_SIZE-i-1)) & 0xFF;
    }

    raw_header[PL_FLAGS_FIELD_OFFSET] = 0x00;

    if(header.compression) {
        set_flag_bit(raw_header, PL_FLAG_COMPRESSION_BIT);
    }

    if(header.encryption) {
        set_flag_bit(raw_header, PL_FLAG_ENCRYPTION_BIT);
    }

    if(header.sign) {
        set_flag_bit(raw_header, PL_FLAG_SIGN_BIT);
    }

    return PACKET_SUCCESS;
}


static char* packets_types[PacketsNumber] = {
    [ProgrammerInitPacket] = "Init programmer packet",
    [ProgrammerStopPacket] = "Stop programmer packet",
    [UartInitPacket] = "Init UART packet",
    [UartStopPacket] = "Stop UART packet",
    [ResetPacket] = "Reset Packet",
    [ACKPacket] = "ACK packet",
    [CloseConnectionPacket] = "Close connection packet",
    [NetworkConfigurationPacket] = "Network configuration packet",
    [EnableEncryptionPacket] = "Enable encryption packet",
    [EnableSignPacket] = "Enable sign packet",
    [EncryptionConfigPacket] = "Encryption config packet",
    [SignConfigPacket] = "Sign config packet",
    [ObserverKeyPacket] = "Observer key packet",

    /* Programmer packets */
    [LoadMCUInfoPacket] = "Load MCU packet",
    [ProgramMemoryPacket] = "Program memory packet",
    [ReadMemoryPacket] = "Read memory packet",
    [MemoryPacket] = "Memory packet",
    [CMDPacket] = "CMD packet",

    /* UART packets */
    [UartConfigurationPacket] = "UART config packet",
    [UartDataPacket] = "Uart data packet"
};


static char* flag_status[2] = {
    "Disabled",
    "Enabled"
};


void print_packet(Packet *packet) {
    PacketHeader header = packet->header;

    OSI_COMMON_LOG("\r\n*********** PACKET *************\r\n");
    OSI_COMMON_LOG("Type int: %d\r\n", header.type);
    OSI_COMMON_LOG("Type: %s\r\n", packets_types[header.type]);
    OSI_COMMON_LOG("Encryption: %s\r\n", flag_status[header.encryption]);
    OSI_COMMON_LOG("Sign: %s\r\n", flag_status[header.sign]);
    OSI_COMMON_LOG("Compression: %s\r\n", flag_status[header.compression]);
    OSI_COMMON_LOG("Data size: %d\r\n", header.data_size);

    int print_max = (10 > header.data_size) ? header.data_size : 10;
    for(int i=0; i<print_max; i++) {
        OSI_COMMON_LOG("0x%02x ", packet->packet_data[i]);
    }

    if (print_max < header.data_size) {
        OSI_COMMON_LOG("...\r\n");
    }
    else {
        OSI_COMMON_LOG("\r\n");
    }
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
    return (header[PL_FLAGS_FIELD_OFFSET] >> bit) & 0x01;
}


static inline void set_flag_bit(_u8 *header, _u8 bit) {
    header[PL_FLAGS_FIELD_OFFSET] |= (1 << bit);
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
