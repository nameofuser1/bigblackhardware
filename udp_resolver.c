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

#define KEY_SIZE                32
#define RESOLVE_PACKET_SIZE     10
#define ANSWER_PACKET_SIZE      10

#define UDP_BUFFER_SIZE         RESOLVE_PACKET_SIZE


/* Static variables */
static OsiTaskHandle    udp_handle;
static _i16              listen_sock = -1;


static bool check_for_resolve(_u8 *buf, _u16 size) {
    return true;
}


static bool cmp_keys(_u8 *buf, _u8 *key) {
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

        if(status < 0) {
            goto err_exit;
        }

        /* Check packet for being resolving one and sending answer */
        if(check_for_resolve(buffer, RESOLVE_PACKET_SIZE)) {
            if(cmp_keys(buffer, cfg->key)) {
                status = sendto(listen_sock, answer, ANSWER_PACKET_SIZE, 0,
                                (sockaddr*)&sock_remote_addr, sock_addr_size);

                if(status < 0) {
                    goto err_exit;
                }
            }
        }
    }

err_exit:
    close(listen_sock);
    OSI_ASSERT_WITH_EXIT(status, udp_handle);
}



_u8 udp_resolver_start(UdpResolverCfg _cfg) {
    static UdpResolverCfg cfg;
    int status;

    strcpy(cfg.key, _cfg.key);
    cfg.port = _cfg.port;

    status = osi_TaskCreate(udp_resolver_task, "udp_resolver_task", UDP_STACK_SIZE,
                            &cfg, 1, &udp_handle);
    OSI_ASSERT_ON_ERROR(status);
}


void udp_resolver_stop(void) {
    if(listen_sock >= 0) {
        close(listen_sock);
        osi_TaskDelete(&udp_handle);
    }
}
