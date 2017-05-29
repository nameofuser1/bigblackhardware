#include "wlan_config.h"

#include <stdlib.h>
#include <string.h>

#include <rom_map.h>
#include <utils.h>

#include <common.h>
#include <gpio_if.h>
#include <uart_if.h>

#include <osi.h>
#include "logging.h"


#define WLAN_STACK_SIZE         1024

/* Time to connect to a wireless network in seconds */
#define MAX_CONNECTING_TIME     10


//SimpleLink Status
extern unsigned long  g_ulStatus;

//If error occurred while executing task
static bool wlan_error = false;

static OsiTaskHandle start_handle;
static OsiTaskHandle stop_handle;

//Lock object to prevent simultaneous connecting/disconnecting
static OsiLockObj_t start_stop_lock;

//Status
static WlanStatus wl_status = WLAN_READY;


static _u8 sl_sec_types[3] = {[CONFIG_OPEN] = SL_SEC_TYPE_OPEN,
                              [CONFIG_WEP] = SL_SEC_TYPE_WEP,
                              [CONFIG_WPA_WPA2] = SL_SEC_TYPE_WPA_WPA2};


static bool configure_ap(WlanConfig *config) {
    int status;

    status = sl_WlanSetMode(ROLE_AP);
    OSI_ASSERT_ON_ERROR(status);

    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, config->ssid_len, config->ssid);
    OSI_ASSERT_ON_ERROR(status);

    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, (_u8*)&sl_sec_types[config->sec]);
    OSI_ASSERT_ON_ERROR(status);

    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD, config->pwd_len, config->pwd);
    OSI_ASSERT_ON_ERROR(status);

    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1, (_u8*)&config->channel);
    OSI_ASSERT_ON_ERROR(status);

    return (sl_Start(NULL, NULL, NULL) == ROLE_AP);
}


static int configure_sta(WlanConfig *config) {
    int status;
    int connecting_time;
    SlSecParams_t sec_params;
    void *sec_params_ptr;

    status = sl_WlanSetMode(ROLE_STA);
    OSI_ASSERT_ON_ERROR(status);

    status = sl_Stop(SL_STOP_TIMEOUT);
    OSI_ASSERT_ON_ERROR(status);
    sl_Start(0, 0, 0);

    if(config->sec == CONFIG_OPEN) {
        sec_params_ptr = NULL;
    }
    else {
        sec_params.Key = (signed char*)config->pwd;
        sec_params.KeyLen = config->pwd_len;
        sec_params.Type = sl_sec_types[config->sec];

        sec_params_ptr = &sec_params;
    }

    OSI_COMMON_LOG("WLAN CONNECT CALL\r\n");
    status = sl_WlanConnect((signed char*)config->ssid, config->ssid_len, 0, sec_params_ptr, 0);
    OSI_ASSERT_ON_ERROR(status);

    // Wait for WLAN Event
    connecting_time = 0;
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
    {
        // Toggle LEDs to Indicate Connection Progress
        GPIO_IF_LedOff(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOn(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(8000000);

        if(++connecting_time == MAX_CONNECTING_TIME) {
            sl_WlanDisconnect();
            sl_Stop(SL_STOP_TIMEOUT);

            OSI_COMMON_LOG("WLAN connection timeout has expired\r\n");
            return FAILURE;
        }
    }

    return SUCCESS;
}


static void wlan_start_task(void *config) {
    int status;
    WlanConfig *wlan_cfg = config;

    OSI_COMMON_LOG("Starting WLAN\r\n");

    status = sl_Start(NULL, NULL, NULL);

    if(status < 0) {
        OSI_ASSERT_WITHOUT_EXIT(status);
        wl_status = WLAN_READY;

        goto start_exit;
    }

    if(wlan_cfg->mode == CONFIG_AP) {
        OSI_COMMON_LOG("AP mode\r\n");
        status = configure_ap(wlan_cfg);
    }
    else {
        OSI_COMMON_LOG("Station mode\r\n");
        status = configure_sta(wlan_cfg);
    }

    if(status < 0) {
        wl_status = WLAN_READY;
        OSI_ASSERT_WITHOUT_EXIT(status);
    }
    else {
        wl_status = WLAN_CONNECTED;
    }

start_exit:
    // Unlock mutex
    osi_LockObjUnlock(&start_stop_lock);
    osi_TaskDelete(&start_handle);
}


static void wlan_stop_task(void *params) {
    (void)params;
    int status;

    OSI_COMMON_LOG("Stopping WLAN\r\n");

    status = sl_Stop(SL_STOP_TIMEOUT);
    OSI_ASSERT_WITHOUT_EXIT(status);

    /* Update WLAN status and unlock mutex*/
    wl_status = WLAN_READY;
    osi_LockObjUnlock(&start_stop_lock);

    osi_TaskDelete(&stop_handle);
}


_u8 wlan_init(void) {
    int status;

    status = osi_LockObjCreate(&start_stop_lock);
    OSI_ASSERT_ON_ERROR(status);

    return SUCCESS;
}


_u8 wlan_start(WlanConfig *config) {
    int status;

    OSI_COMMON_LOG("wlan_start\r\n");
    /* Lock to avoid simultaneous start and stop of SIMPLELINK device */
    /* Will be unlocked at the end of wlan_start_task */
    osi_LockObjLock(&start_stop_lock, 0);
    OSI_COMMON_LOG("Locked object\r\n");

    wl_status = WLAN_CONNECTING;

    status = osi_TaskCreate(wlan_start_task, "wlan_start_task", WLAN_STACK_SIZE, config, 1, &start_handle);
    OSI_COMMON_LOG("Created task\r\n");
    OSI_ASSERT_ON_ERROR(status);

    return SUCCESS;
}


_u8 wlan_stop(void) {
    int status;

    /* Lock to avoid simultaneous start and stop of SIMPLELINK device */
    /* Will be unlocked at the end of wlan_stop_task */
    osi_LockObjLock(&start_stop_lock, 0);

    status = osi_TaskCreate(wlan_stop_task, "wlan_stop_task", WLAN_STACK_SIZE, NULL, 1, &stop_handle);
    OSI_ASSERT_ON_ERROR(status);

    return SUCCESS;
}


WlanStatus wlan_status(void) {
    return wl_status;
}
