#ifndef UDP_RESOLVER_H
#define UDP_RESOLVER_H

#include "simplelink.h"


typedef struct {

    char            *key;
    unsigned int    port;

} UdpResolverCfg;


_u8 udp_resolver_start(UdpResolverCfg *_cfg);
void udp_resolver_stop(void);


#endif // UDP_RESOLVER_H
