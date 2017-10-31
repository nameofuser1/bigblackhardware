#include "packet_manager.h"

#include "sys.h"
#include "osi.h"

#include "programmer_parser.h"
#include "logging.h"


static inline _i16 write_packet(Packet *packet, OsiMsgQ_t *out_queue) {
    _i16 status;

    status = sys_queue_write_ptr(out_queue, packet, 10);
    return status;
}


_i16 send_ack(_i16 status, OsiMsgQ_t *out_queue) {
    _i16 send_status;
    _u8 ack_data[1];
    set_ack_data(ack_data, status);

    send_status = create_send_packet(ACKPacket, ack_data, 1, out_queue);
    OSI_ASSERT_ON_ERROR(send_status);

    return send_status;
}


_i16 send_error(char *msg, OsiMsgQ_t *out_queue) {
    _i16 send_status;

    send_status = create_send_packet(ErrorPacket, (_u8*)msg, strlen(msg), out_queue);
    OSI_ASSERT_ON_ERROR(send_status);

    return send_status;
}


_i16 send_avr_cmd(_u8 *cmd, OsiMsgQ_t *out_queue) {
    _i16 status;

    status = create_send_packet(CMDPacket, cmd, AVR_CMD_SIZE, out_queue);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


_i16 create_send_packet(PacketType type, _u8 *data, _u16 data_len, OsiMsgQ_t *out_queue) {
    _i16 status;
    Packet *packet;

    status = get_packet_from_pool(&packet);
    OSI_ASSERT_ON_ERROR(status);

    status = create_packet(packet, type, COMPRESSION_OFF, SIGN_OFF, ENCRYPTION_OFF,
                data, data_len);
    OSI_ASSERT_ON_ERROR(status);

    write_packet(packet, out_queue);
    return status;
}


_i16 send_packet(Packet *packet, PacketType type, _u16 data_len, OsiMsgQ_t *out_queue) {
    _i16 status;

    /* Will not change packet_data field */
    status = create_packet(packet, type, COMPRESSION_OFF, SIGN_OFF, ENCRYPTION_OFF,
                           NULL, data_len);
    OSI_ASSERT_ON_ERROR(status);

    write_packet(packet, out_queue);
    return status;
}


_i16 read_packet(Packet **packet, OsiMsgQ_t *in_queue, _u8 timeout) {
    _i16 status;

    status = sys_queue_read_ptr(in_queue, (void**)packet, timeout);
    return status;
}


void set_ack_data(_u8 *data, _i16 success) {
    if(success >= 0) {
        data[0] = 1;
    }
    else {
        data = 0;
    }
}
