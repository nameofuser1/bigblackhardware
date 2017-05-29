#include "udp_resolver.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "osi.h"
#include "socket.h"
#include "logging.h"

#include "common.h"


/* OSI task stack size */
#define UDP_STACK_SIZE  1024

/* Income packet parameters */
#define HEADER_OFFSET           0
#define HEADER_SIZE             1
#define HEADER_BYTE             0xAD

#define KEY_OFFSET              (HEADER_OFFSET + HEADER_SIZE)
#define KEY_SIZE                32

#define PORT_OFFSET             (KEY_OFFSET + KEY_SIZE)
#define PORT_SIZE               4

#define RESOLVE_PACKET_SIZE     (HEADER_SIZE + KEY_SIZE + PORT_SIZE)
#define ANSWER_PACKET_SIZE      1
#define ANSWER_BYTE             0xFF

#define UDP_BUFFER_SIZE         RESOLVE_PACKET_SIZE


/* Static variables */
static OsiTaskHandle        udp_handle;
static _i16                 listen_sock = -1;

static UdpResolverCfg cfg = {
    .key = NULL,
    .port = 0
};


static inline bool check_for_resolve(_u8 *buf, _u16 size) {
    if(size != RESOLVE_PACKET_SIZE || buf[HEADER_OFFSET] != HEADER_BYTE) {
        return false;
    }

    return true;
}


static inline bool cmp_keys(_u8 *buf, _u8 *key) {
    return (strncmp(buf+KEY_OFFSET, key, KEY_SIZE) == 0);
}


static inline _u32 get_port(_u8 *buf) {
    _u32 port = 0;

    for(int i=0; i<PORT_SIZE; i++) {
        port |= (buf[PORT_OFFSET+i] & 0xFF) << ((PORT_SIZE-i-1)*8);
    }

    return port;
}


static void udp_resolver_task(void *pvParams) {
    int status;
    _u8 buffer[UDP_BUFFER_SIZE];
    _u8 answer[ANSWER_PACKET_SIZE];
    sockaddr_in sock_local_addr;
    sockaddr_in sock_remote_addr;
    int sock_addr_size;
    UdpResolverCfg *cfg = pvParams;

    sock_local_addr.sin_family = AF_INET;
    sock_local_addr.sin_addr.s_addr = 0x00;
    sock_local_addr.sin_port = sl_Htons((_u16)cfg->port);

    sock_addr_size = sizeof(SlSockAddrIn_t);

    /* Create listen sock an UDP socket */
    listen_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    OSI_ASSERT_WITH_EXIT(listen_sock, udp_handle);

    status = bind(listen_sock, (sockaddr*)&sock_local_addr, sock_addr_size);

    if(status < 0) {
        goto err_exit;
    }

    for( ;; ) {
        status = recvfrom(listen_sock, buffer, RESOLVE_PACKET_SIZE, 0,
                          (sockaddr*)&sock_remote_addr, (SlSocklen_t*)&sock_addr_size);

        if(status <= 0) {
            goto err_exit;
        }

        /* Check packet for being resolving one and sending answer */
        /* Status contains number of bytes written to the buffer */
        if(check_for_resolve(buffer, status)) {
            if(cmp_keys(buffer, cfg->key)) {
                answer[0] = ANSWER_BYTE;
                _u16 port = (_u16)get_port(buffer);

                /* Update port information */
                sockaddr_in *rmt_addr = (sockaddr_in*)&sock_remote_addr;
                rmt_addr->sin_port = htons((_u16)port);

                /* Send answer */
                status = sendto(listen_sock, answer, ANSWER_PACKET_SIZE, 0,
                                (sockaddr*)rmt_addr, sock_addr_size);

                if(status < 0) {
                    goto err_exit;
                }
            }
            else {
                OSI_COMMON_LOG("Keys does not match\r\n");
            }
        }
        else {
            OSI_COMMON_LOG("Not an observer packet\r\n");
        }
    }

err_exit:
    close(listen_sock);
    OSI_ASSERT_WITH_EXIT(status, udp_handle);
}



_u8 udp_resolver_start(UdpResolverCfg *_cfg) {
    int status;
    cfg.key = mem_Malloc(KEY_SIZE + 1);

    strcpy(cfg.key, _cfg->key);
    cfg.port = _cfg->port;

    status = osi_TaskCreate(udp_resolver_task, "udp_resolver_task", UDP_STACK_SIZE,
                            &cfg, 1, &udp_handle);
    OSI_ASSERT_ON_ERROR(status);
}


void udp_resolver_stop(void) {
    if(listen_sock >= 0) {
        mem_Free(cfg.key);

        close(listen_sock);
        osi_TaskDelete(&udp_handle);
    }
}
