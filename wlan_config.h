#ifndef WLAN_CONFIG_H
#define WLAN_CONFIG_H

#define MAX_SSID_LENGTH     32
#define MAX_PWD_LENGTH      64

#include <simplelink.h>
#include <stdbool.h>

typedef enum {CONFIG_AP=0, CONFIG_STA} WlanMode;
typedef enum {CONFIG_OPEN=0, CONFIG_WEP, CONFIG_WPA_WPA2} WlanSecurityType;


typedef struct _wlan_config {
    WlanMode            mode;
    WlanSecurityType    sec;
    _u8                 channel;

    _u8                ssid[MAX_SSID_LENGTH];
    _u8                 ssid_len;

    _u8                pwd[MAX_PWD_LENGTH];
    _u8                 pwd_len;

} WlanConfig;


_u8 wlan_init(void);
_u8 wlan_start(WlanConfig *config);
_u8 wlan_stop(void);
bool wlan_connected(void);


#endif // WLAN_CONFIG_H
