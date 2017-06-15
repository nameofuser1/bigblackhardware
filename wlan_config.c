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

/* Filename to save WLAN configuration into */
#define CONFIG_FNAME            "wlan_config.bin"
#define CONFIG_FILE_MSIZE       256

//SimpleLink WLAN connection Status
extern unsigned long  g_ulStatus;

//If error occurred while executing task
static bool wlan_error = false;

/* Handles for start and stop task */
static OsiTaskHandle start_handle;
static OsiTaskHandle stop_handle;

//Lock object to prevent simultaneous connecting/disconnecting
static OsiLockObj_t start_stop_lock;

//Status
static WlanStatus wl_status = WLAN_READY;

/* Configuration security mappings */
static _u8 sl_sec_types[3] = {[WLAN_CONFIG_OPEN] = SL_SEC_TYPE_OPEN,
                              [WLAN_CONFIG_WEP] = SL_SEC_TYPE_WEP,
                              [WLAN_CONFIG_WPA_WPA2] = SL_SEC_TYPE_WPA_WPA2};


/* Currently used configuration */
static WlanConfig current_config;


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

    if(config->sec == WLAN_CONFIG_OPEN) {
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
    WlanConfig *wlan_config = config;
    OSI_COMMON_LOG("Starting WLAN\r\n");

    mem_copy(&current_config, config, sizeof *config);
    sl_Stop(SL_STOP_TIMEOUT);   // Q: Do we really need it ?
    status = sl_Start(NULL, NULL, 0); //Q: And this?

    if(status < 0) {
        OSI_ASSERT_WITHOUT_EXIT(status);
        wl_status = WLAN_READY;

        goto start_exit;
    }

    if(wlan_config->mode == WLAN_CONFIG_AP) {
        OSI_COMMON_LOG("AP mode\r\n");
        status = configure_ap(config);
    }
    else {
        OSI_COMMON_LOG("Station mode\r\n");
        status = configure_sta(config);
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


_i8 wlan_init(void) {
    int status;

    status = osi_LockObjCreate(&start_stop_lock);
    OSI_ASSERT_ON_ERROR(status);

    sl_Start(NULL, NULL, 0);

    return SUCCESS;
}


_i8 wlan_start(WlanConfig *config) {
    int status;

    OSI_COMMON_LOG("wlan_start\r\n");
    /* Lock to avoid simultaneous start and stop of SIMPLELINK device */
    /* Will be unlocked at the end of wlan_start_task */
    osi_LockObjLock(&start_stop_lock, 0);

    wl_status = WLAN_CONNECTING;

    status = osi_TaskCreate(wlan_start_task, "wlan_start_task", WLAN_STACK_SIZE, config, 1, &start_handle);
    OSI_COMMON_LOG("Created task\r\n");
    OSI_ASSERT_ON_ERROR(status);

    return SUCCESS;
}


_i8 wlan_stop(void) {
    int status;

    /* Lock to avoid simultaneous start and stop of SIMPLELINK device */
    /* Will be unlocked at the end of wlan_stop_task */
    osi_LockObjLock(&start_stop_lock, 0);

    status = osi_TaskCreate(wlan_stop_task, "wlan_stop_task", WLAN_STACK_SIZE, NULL, 1, &stop_handle);
    OSI_ASSERT_ON_ERROR(status);

    return SUCCESS;
}


_i8 wlan_save_config(WlanConfig *config) {
    _i8 status;
    long file_hdnl;

    status = sl_FsOpen(CONFIG_FNAME,
                       FS_MODE_OPEN_CREATE(CONFIG_FILE_MSIZE,
                                           _FS_FILE_OPEN_FLAG_COMMIT |
                                           _FS_FILE_OPEN_FLAG_NO_SIGNATURE_TEST |
                                           _FS_FILE_PUBLIC_READ |
                                           _FS_FILE_PUBLIC_WRITE),
                        NULL, &file_hdnl);

    if(status < 0) {
        goto exit;
    }

    status = sl_FsWrite(file_hdnl, 0, (_u8 *)config, sizeof *config);

    exit:
        sl_FsClose(file_hdnl, NULL, NULL, 0);
        OSI_ASSERT_ON_ERROR(status);

        return SUCCESS;
}


_i8 wlan_load_config(WlanConfig *config) {
    _i8 status;
    long file_hdnl;

    status = sl_FsOpen(CONFIG_FNAME, FS_MODE_OPEN_READ, NULL, &file_hdnl);
    if(status < 0) {
        goto exit;
    }

    status = sl_FsRead(file_hdnl, 0, (_u8*)config, sizeof *config);

    exit:
        sl_FsClose(file_hdnl, NULL, NULL, 0);
        OSI_ASSERT_ON_ERROR(status);

        return SUCCESS;
}


void wlan_print_config(WlanConfig *config) {
    OSI_COMMON_LOG("******************WLAN CONFIG******************\r\n")

    if(config->mode == WLAN_CONFIG_AP) {
        OSI_COMMON_LOG("Mode: AP\r\n");
    }
    else {
        OSI_COMMON_LOG("Mode: STATION\r\n");
    }

    if(config->sec == WLAN_CONFIG_WPA_WPA2) {
        OSI_COMMON_LOG("Security: WPA-WPA2\r\n");
    }
    else if(config->sec == WLAN_CONFIG_OPEN) {
        OSI_COMMON_LOG("Security: OPEN\r\n");
    }

    OSI_COMMON_LOG("Channel: %d\r\n", config->channel);

    OSI_COMMON_LOG("SSID: %s\r\n", config->ssid);
    OSI_COMMON_LOG("SSID length: %d\r\n", config->ssid_len);

    OSI_COMMON_LOG("PWD: %s\r\n", config->pwd);
    OSI_COMMON_LOG("PWD length: %d\r\n", config->pwd_len);
}


WlanStatus wlan_status(void) {
    return wl_status;
}
