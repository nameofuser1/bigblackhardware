#ifndef WLAN_CONFIG_H
#define WLAN_CONFIG_H

#define MAX_SSID_LENGTH     32
#define MAX_PWD_LENGTH      64

#include <simplelink.h>
#include <stdbool.h>

typedef enum {WLAN_CONFIG_AP=0, WLAN_CONFIG_STA} WlanMode;
typedef enum {WLAN_CONFIG_OPEN=0, WLAN_CONFIG_WEP, WLAN_CONFIG_WPA_WPA2} WlanSecurityType;


typedef struct _wlan_config {
    WlanMode            mode;
    WlanSecurityType    sec;
    _u8                 channel;

    _u8                 ssid[MAX_SSID_LENGTH];
    _u8                 ssid_len;

    _u8                 pwd[MAX_PWD_LENGTH];
    _u8                 pwd_len;

} WlanConfig;


typedef enum {WLAN_READY = 0, WLAN_CONNECTING, WLAN_CONNECTED} WlanStatus;


_i8         wlan_init(void);
_i8         wlan_start(WlanConfig *config);
_i8         wlan_stop(void);
_i8         wlan_save_config(WlanConfig *config);
_i8         wlan_load_config(WlanConfig *config);
void        wlan_print_config(WlanConfig *config);
WlanStatus  wlan_status(void);


#endif // WLAN_CONFIG_H
