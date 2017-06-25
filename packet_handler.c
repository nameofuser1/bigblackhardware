#include "packet_handler.h"
#include "protocol.h"
#include "packets.h"

#include <stddef.h>
#include "osi.h"

#include "logging.h"
#include "socket.h"

#include "config.h"

/* 10 ms timeout */
#define     SELECT_TIMEOUT_US       10000
#define     QUEUE_READ_TIMEOUT_MS   1

/* Tasks prototypes */
static void vListeningTask(void *pvParameters);
static void vHandlingTask(void *pvParameters);


/* Task handles */
static OsiTaskHandle    handling_task_hndl;
static OsiTaskHandle    listen_task_hndl;

/* Connections are passed through this queue from listening task to reading task */
static OsiMsgQ_t        connections_queue;

/* Static functions prototypes */
static _i16 recv_packet(_i16 sock, Packet *packet);
static _i16 send_packet(_i16 sock, Packet *packet);
static _i16 recv_nbytes(_i16 sock, _u8 *buf, _u16 n);
static _i16 send_nbytes(_i16 sock, _u8 *buf, _u16 n);


_i8 packet_handler_start(void) {
    _i8 status;

    /* Initialize queue for passing connections through */
    status = osi_MsgQCreate(&connections_queue, "ConnectionsQueue",
                            sizeof(ConnectionInfo*), MAX_TCP_CONNECTIONS);
    OSI_ASSERT_ON_ERROR(status);

    /* Create packets pool */
    status = initialize_packets_pool(MSG_POOL_SIZE);
    OSI_ASSERT_ON_ERROR(status);

    /* Start packet handler task */
    status = osi_TaskCreate(vHandlingTask, HANDLING_TASK_NAME, HANDLING_TASK_STACK_SIZE,
                            NULL, HANDLING_TASK_PRIORITY, &handling_task_hndl);
    OSI_ASSERT_ON_ERROR(status);

    /* Start listening for connections */
    status = osi_TaskCreate(vListeningTask, LISTENING_TASK_NAME, LISTENING_TASK_STACK_SIZE,
                            NULL, LISTENING_TASK_PRIORITY, &listen_task_hndl);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


/* Actually we do not need this function. However being presented it
    has to deactivate pool. TODO: add packets pool deletion */
_i8 packet_handler_stop(void) {
    osi_TaskDelete(&handling_task_hndl);

    return SUCCESS;
}


/* *************************************************** *
 * Build read fd_set given current connections.
 *
 * Return maximum socket handle.
 * *************************************************** */
static inline _i16 get_read_fd(ConnectionInfo **conn_info, fd_set *set) {
    _i16 max_fd = 0;
    FD_ZERO(set);

    for(int i=0; i<MAX_TCP_CONNECTIONS; i++) {
        ConnectionInfo *info = conn_info[i];

        if(info != NULL) {
            _i16 hndl = info->hndl;
            FD_SET(hndl, set);

            if(hndl > max_fd) {
                max_fd = hndl;
            }
        }
    }

    return max_fd;
}

/* **************************************************
 *                Listening task                    *
 ****************************************************
 *                                                  *
 *   Configures WLAN module and load all necessary  *
 *      middleware drivers                          *
 *                                                  *
 * ************************************************ */
 static void vListeningTask(void *pvParameters) {
     OSI_COMMON_LOG("Inside listening task\r\n");
    _i8 status;
    _i16 listen_sock;
    sockaddr_in local_addr;
    sockaddr_in remote_addr;
    socklen_t   remote_addr_l;

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(TCP_LISTENING_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    OSI_COMMON_LOG("Created socket\r\n");

    status = bind(listen_sock, (sockaddr*)&local_addr, sizeof local_addr);
    OSI_ASSERT_WITH_EXIT(status, handling_task_hndl);
    OSI_COMMON_LOG("Binded\r\n");

    status = listen(listen_sock, MAX_TCP_CONNECTIONS);
    OSI_ASSERT_WITH_EXIT(status, handling_task_hndl);
    OSI_COMMON_LOG("Started listening\r\n");

    for( ;; ) {
        _i16 conn = accept(listen_sock, (sockaddr*)&remote_addr, &remote_addr_l);
        OSI_COMMON_LOG("Accepted socket %d\r\n", conn);

        SlSockNonblocking_t non_blocking_en;
        status = setsockopt(conn, SOL_SOCKET, SO_NONBLOCKING, (_u8*)&non_blocking_en, sizeof(non_blocking_en));
        OSI_ASSERT_WITHOUT_EXIT(status);

        OSI_COMMON_LOG("Changed socket to non-blocking mode\r\n");

        if(conn > 0) {
            ConnectionInfo *conn_info = mem_Malloc(sizeof *conn_info);
            conn_info->hndl = conn;

            status = osi_MsgQCreate(&conn_info->in_queue, NULL, sizeof(Packet*), MSG_QUEUE_SIZE);
            ASSERT_WITHOUT_EXIT(status);

            status = osi_MsgQCreate(&conn_info->out_queue, NULL, sizeof(Packet*), MSG_QUEUE_SIZE);
            ASSERT_WITHOUT_EXIT(status);

            osi_MsgQWrite(&connections_queue, conn_info, 0);
            OSI_COMMON_LOG("Put connection onto connections queue\r\n");
        }
        else {
            /* Maximum connections are used. Wait till resources will be freed. */
            OSI_ASSERT_WITHOUT_EXIT(conn);
            osi_Sleep(5000);
        }
    }
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
static void vHandlingTask(void *pvParameters)
{
    ConnectionInfo  *conn_info[2] = {NULL, NULL};
    ConnectionInfo  *info;
    _u8             conn_ptr = 0;
    _i16            status;
    Packet          *packet;

    /* Select variables */
    _i16            max_fd;
    fd_set          read_fd;
    timeval         select_time;

    select_time.tv_sec = 0;
    select_time.tv_usec = SELECT_TIMEOUT_US;

    /* Wait for the first connection */
    OSI_COMMON_LOG("Waiting for the first connection\r\n");
    status = osi_MsgQRead(&connections_queue, info, OSI_WAIT_FOREVER);
    ASSERT_WITHOUT_EXIT(status);
    OSI_COMMON_LOG("Got first connection\r\n");

    conn_info[conn_ptr++] = info;

    for( ;; ) {
        max_fd = get_read_fd(conn_info, &read_fd);

        /* MIN 10 MS WAITING */
        status = select(max_fd, &read_fd, NULL, NULL, &select_time);
        OSI_ASSERT_WITHOUT_EXIT(status);

        if(status > 0) {
            OSI_COMMON_LOG("There is data to read in %d sockets\r\n", status);

            for(int i=0; i<MAX_TCP_CONNECTIONS; i++) {
                info = conn_info[i];

                if(info != NULL) {
                    _i16 hndl = info->hndl;

                    /* Read on this socket is available */
                    if(FD_ISSET(hndl, &read_fd)) {
                        OSI_COMMON_LOG("FD set on %d socket\r\n", hndl);

                        status = get_packet_from_pool(&packet);
                        OSI_ASSERT_WITHOUT_EXIT(status);

                        OSI_COMMON_LOG("Receiving packet\r\n");
                        status = recv_packet(hndl, packet);
                        OSI_ASSERT_WITHOUT_EXIT(status);

                        /* TODO: process received packet !*/
                        /* RIGHT HERE */
                        OSI_COMMON_LOG("Printing packet\r\n");
                        print_packet(packet);

                        status = release_packet(packet);
                        OSI_ASSERT_WITHOUT_EXIT(status);
                    }

                    /* Check whether there are packets to send */
                    status = 0;
                    while(status >= 0) {
                        status = osi_MsgQRead(&info->out_queue, packet, QUEUE_READ_TIMEOUT_MS);

                        if(status >= 0) {
                            update_header(packet);
                            send_packet(hndl, packet);

                            release_packet(packet);
                        }
                    }

                }
            }
        }

        /* Check for new connections
        MIN 1 MS WAITING
        TODO: rewrite USING lock objects */
        status = osi_MsgQRead(&connections_queue, info, QUEUE_READ_TIMEOUT_MS);
        if(status >= 0) {
            OSI_COMMON_LOG("New connection found in queue\r\n");

            conn_info[conn_ptr] = info;
            conn_ptr = (conn_ptr == MAX_TCP_CONNECTIONS - 1) ? 0 : conn_ptr+1;
        }
    }
}


static _i16 send_packet(_i16 sock, Packet *packet) {
    _i16 status;
    _u16 data_size = packet->header.data_size;

    status = send_nbytes(sock, packet->raw_header, PACKET_HEADER_SIZE);
    OSI_ASSERT_ON_ERROR(status);

    status = send_nbytes(sock, packet->packet_data, data_size);
    OSI_ASSERT_ON_ERROR(status);

    return SUCCESS;
}


static _i16 recv_packet(_i16 sock, Packet *packet) {
    _i16 status;
    _u16 packet_size;
    /* Trying to receive header */
    OSI_COMMON_LOG("Receiving header\r\n");
    status = recv_nbytes(sock, packet->raw_header, PACKET_HEADER_SIZE);
    OSI_ASSERT_ON_ERROR(status);

    /* Parsing header header */
    status = parse_header(packet->raw_header, &packet->header);
    OSI_ASSERT_ON_ERROR(status);

    /* Receiving body */
    OSI_COMMON_LOG("Receiving body\r\n");
    packet_size = packet->header.data_size;

    if(packet_size > 0) {
        status = recv_nbytes(sock, packet->packet_data, packet_size);
        OSI_ASSERT_ON_ERROR(status);
    }

    return SUCCESS;
}


static _i16 recv_nbytes(_i16 sock, _u8 *buf, _u16 n) {
    _i16 remaining_bytes = n;
    _i16 recv_bytes;

    while(remaining_bytes != 0) {
        OSI_COMMON_LOG("Trying to receive %d bytes on socket %d\r\n", n, sock);
        recv_bytes = recv(sock, buf, remaining_bytes, 0);
        OSI_COMMON_LOG("Received %d bytes. Remaining %d\r\n", recv_bytes, remaining_bytes-recv_bytes);

        if(recv_bytes > 0) {
            remaining_bytes -= recv_bytes;
        }
        else if(recv_bytes != EAGAIN) {
            return FAILURE;
        }
        else {

        }


    }

    return SUCCESS;
}


static _i16 send_nbytes(_i16 sock, _u8 *buf, _u16 n) {
    _i16 status = EAGAIN;

    while(status == EAGAIN) {
        status = send(sock, buf, n, 0);

        if(status < 0 && status != EAGAIN) {
            return status;
        }
    }

    return SUCCESS;
}
