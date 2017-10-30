#ifndef PACKET_MANAGER_H_INCLUDED
#define PACKET_MANAGER_H_INCLUDED

#include <simplelink.h>
#include <osi.h>

#include "packets.h"


_i16 send_ack(_i16 status, OsiMsgQ_t *out_queue);
_i16 create_send_packet(PacketType type, _u8 *data, _u16 data_len, OsiMsgQ_t *out_queue);
_i16 send_packet(Packet *packet, PacketType type, _u16 data_len, OsiMsgQ_t *out_queue);
_i16 read_packet(Packet **packet, OsiMsgQ_t *out_queue, _u8 timeout);
void set_ack_data(_u8 *data, _i16 success);

#endif // PACKET_MANAGER_H_INCLUDED
