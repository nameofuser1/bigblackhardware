#include "packet_handler.h"

#define BODY_RETRIES    3

/* Should pass it to initializer */
static OsiMsgQ_t    connections_queue;


/* Returns either number of bytes in the packet or FAILURE if header can not be parsed */
static _i16 parse_header(_u8 *buf);

static void parse_packet(_u8 *buf);


/* ******************************************************************
 *                              Reading task                        *
 ********************************************************************
 *                                                                  *
 *   This task reads socket in non-blocking mode                    *
 *                                                                  *
 *   Since exceptions in select are not supported we do not use it  *
 *                                                                  *
 *                                                                  *
 * ******************************************************************/
static void vReadingTask(void *pvParameters)
{
    ConnectionInfo  *conn_info[TCP_CONNECTION_NUM] = {NULL, NULL};
    ConnectionInfo  *info;
    _u8             data_buf[DATA_BUFFER_SIZE];
    _i16            status;

    osi_MsgQRead(&connections_queue, info, 0);
    conn_info[conn_ptr++] = info;

    for( ;; ) {
        for(int i=0; i<TCP_CONNECTION_NUM; i++) {
            info = conn_info[i];

            if(info[i] != NULL) {
                /* Trying to receive header */
                _i16 hndl = info->hndl;
                status = recv(hndl, data_buf, PACKET_HEADER_SIZE, 0);

                if(status < 0) {
                    /* Check for errors */
                    if(status != EAGAIN && status != SL_POOL_IS_EMPTY) {
                        OSI_ASSERT_WITHOUT_EXIT(status);
                    }
                }
                else if(status == 0) {
                    /* Client was disconnected */
                    OSI_COMMON_LOG("Client disconnected\r\n");

                    mem_Free(info);
                    conn_info[i] = NULL;
                }
                else {
                    /* Received header */
                    _i16 packet_size = parse_header(data_buf);

                    if(packet_size <= 0) {
                        OSI_COMMON_LOG("Header can not be parsed\r\n");
                    }
                    else {
                        /* Successfully parsed header => Trying to read body */
                        for(int j=0; j<BODY_RETRIES; j++) {
                            /* Assume that after header being received it is impossible not to get the rest */
                            status = recv(hndl, data_buf+PACKET_HEADER_SIZE, packet_size, 0);

                            if(status > 0) {
                                parse_packet(data_buf);
                                break;
                            }
                        }
                    }
                }
            }
        }

    }
}
