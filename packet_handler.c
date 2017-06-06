#include "packet_handler.h"

#define     MAX_CLIENTS_NUM         2

/* 10 ms timeout */
#define     SELECT_TIMEOUT_US       10000

#define     QUEUE_READ_TIMEOUT_MS   1

/* Should pass it to initializer */
static OsiMsgQ_t    connections_queue;


/* Returns either number of bytes in the packet or FAILURE if header can not be parsed */
static _i16 parse_header(_u8 *buf);
static void parse_packet(_u8 *buf);
static _i16 recv_nbytes(_i16 sock, _u8 *buf, _u16 n);


/* *************************************************** *
 * Build read fd_set given current connections.
 *
 * Return maximum socket handle.
 * *************************************************** */
static inline _i16 get_read_fd(ConnectionInfo *conn_info, fd_set *set) {
    _i16 max_fd = 0;
    FD_ZERO(set);

    for(int i=0; i<TCP_CONNECTION_NUM; i++) {
        ConnectionInfo *info = conn_info[i];

        if(info != NULL) {
            _i16 hdnl = info->hndl;
            FD_SET(hndl, set);

            if(hdnl > max_fd) {
                max_fd = hndl;
            }
        }
    }

    return max_fd;
}

/* ******************************************************************
 *                              Reading task                        *
 ********************************************************************
 *                                                                  *
 *   This task reads socket in non-blocking mode                    *
 *                                                                  *
 *   Since exceptions in select are not supported CLIENT MUST SENT  *
 *      close connection request                                    *
 *                                                                  *
 *                                                                  *
 * ******************************************************************/
static void vReadingTask(void *pvParameters)
{
    ConnectionInfo  *conn_info[TCP_CONNECTION_NUM] = {NULL, NULL};
    ConnectionInfo  *info;
    _u8             conn_ptr = 0;
    _u8             data_buf[DATA_BUFFER_SIZE];
    _i16            status;

    /* Select variables */
    _i16            max_fd;
    fd_set          read_fd;
    timeval         select_time;

    select_time.tv_sec = 0;
    select_time.tv_usec = SELECT_TIMEOUT_US;

    /* Wait for the first connection */
    osi_MsgQRead(&connections_queue, info, 0);
    conn_info[conn_ptr++] = info;

    for( ;; ) {
        max_fd = get_read_fd(conn_info, &read_fd);

        status = select(max_fd, &read_fd, NULL, NULL, &select_time);
        OSI_ASSERT_WITHOUT_EXIT(status);

        for(int i=0; i<TCP_CONNECTION_NUM; i++) {
            info = conn_info[i];

            if(info != NULL) {
                _i16 hdnl = info->hndl;

                /* Read on this socket is available */
                if(FD_ISSET(hndl, &read_fd)) {
                    /* Trying to receive header */
                    status = recv_nbytes(hndl, data_buf, PACKET_HEADER_SIZE);
                    OSI_ASSERT_WITHOUT_EXIT(status);

                    /* Received header */
                    _i16 packet_size = parse_header(data_buf);
                    if(packet_size < 0) {
                        OSI_ASSERT_WITHOUT_EXIT(packet_size);
                    }
                    else {
                        status = recv_nbytes(hndl, data_buf+PACKET_HEADER_SIZE, packet_size, 0);
                        OSI_ASSERT_WITHOUT_EXIT(status);

                        parse_packet(data_buf);

                        /* Here comes control for base packets like closing connection and etc */
                    }
                }

                status = 0;
                while(status >= 0) {
                    /* Has to pass DATA STRUCTURE TO INDENTIFY NUMBER OF BYTES */
                    status = osi_MsgQRead(&info->out_queue, data_buf, QUEUE_READ_TIMEOUT_MS);

                    status = EAGAIN;
                    while(status == EAGAIN) {
                        /* FIX IT. Just a DUMMY send */
                        status = send(hndl, data_buf, 1000, 0);

                        if(status < 0 && status != EAGAIN) {
                            /* Client has to ALWAYS inform us about disconnection */
                            OSI_ASSERT_WITHOUT_EXIT(status);
                        }
                    }
                }

            }
        }

        /* Check for new connections */
        status = osi_MsgQRead(&connections_queue, info, QUEUE_READ_TIMEOUT_MS);
        if(status > 0) {
            conn_info[conn_ptr] = info;
            conn_ptr = (conn_ptr == MAX_CLIENTS_NUM - 1) ? 0 : conn_ptr+1;
        }
    }
}
