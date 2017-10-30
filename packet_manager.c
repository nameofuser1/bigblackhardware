#include "packets.h"
#include "osi.h"
Packet* PacketManager_create_packet(PacketType type, _u8 *data,
                                    _u8 data_size, _u8 comp, _u8 sign, _u8 enc) {
    _i16 status;
    Packet *packet;

    status = get_packet_from_pool(&packet);
    ASSERT_ON_ERROR(status);

    PacketHeader *header = &(packet->header);

    header->type = type;
    header->group = get_type_group(type);
    header->compression = comp;
    header->sign = sign;
    header->encryption = enc;
    header->data_size = data_size;

    mem_copy(packet->packet_data, data, data_size);
    update_header(Packet);

    return Packet;
}
