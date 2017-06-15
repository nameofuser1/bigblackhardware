#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"
#include "hw_uart.h"
#include "uart.h"
#include "uart_if.h"


#include "wired_configurator.h"
#include "wlan_config.h"
#include "protocol.h"
#include "sys.h"
#include "logging.h"

#include "packets.h"

#include "udma.h"
#include "udma_if.h"

#include "osi.h"
#include "prcm.h"

#include "common.h"

#define WLAN_UPDATER_STACK_SIZE 1024

/*  */
#define WIRED_UART              UARTA0_BASE
#define WIRED_UART_PRCM         PRCM_UARTA0
#define WIRED_UART_RX_DMA       UART_DMA_RX
#define WIRED_MAX_CONFIG_SIZE   256

#define WIRED_UART_BAUDRATE     115200

typedef enum {StateRecvHeader=0, StateRecvBody} WiredState;

/* Current state */
static WiredState state = StateRecvHeader;

/* Buffer to save income packets */
static Packet   cfg_packet;


static void wired_recv(_u8 n_bytes, _u8 offset) {
    UDMASetupTransfer(UDMA_CH8_UARTA0_RX,
                      UDMA_MODE_BASIC,
                      n_bytes,
                      UDMA_SIZE_8,
                      UDMA_ARB_2,
                      (void*)(WIRED_UART + UART_O_DR),
                      UDMA_SRC_INC_NONE,
                      cfg_packet.packet_data+offset,
                      UDMA_DST_INC_8);

    MAP_UARTDMAEnable(WIRED_UART, UART_DMA_RX);
}


/* TODO: Need to add validations on lengths and etc. */
static _i8 get_wlan_config_from_packet(_u8 *packet, WlanConfig *config) {
    _u8 channel = packet[PL_NETWORK_CHANNEL_OFFSET];
    _u8 mode = packet[PL_NETWORK_MODE_OFFSET];
    _u8 phy = packet[PL_NETWORK_PHY_OFFSET];
    _u8 ssid_len = packet[PL_NETWORK_SSID_LEN_OFFSET];
    _u8 pwd_len = packet[PL_NETWORK_SSID_LEN_OFFSET+ssid_len+1];
    _u8 pwd_off = PL_NETWORK_SSID_LEN_OFFSET+ssid_len+2;

    /* Save SSID name */
    config->ssid_len = ssid_len;
    mem_copy(config->ssid, packet+PL_NETWORK_SSID_LEN_OFFSET+1, ssid_len);

    /* Save password */
    config->pwd_len = pwd_len;
    mem_copy(config->pwd, packet+pwd_off, pwd_len);

    /* Configuring mode */
    if(mode == PL_NETWORK_MODE_AP) {
        config->mode = WLAN_CONFIG_AP;
    }
    else if(mode == PL_NETWORK_MODE_STA) {
        config->mode = WLAN_CONFIG_STA;
    }
    else {
        OSI_COMMON_LOG("Unknown WLAN mode\r\n");
        goto err_exit;
    }

    if(pwd_len != 0) {
        config->sec = WLAN_CONFIG_WPA_WPA2;
    }
    else {
        config->sec = WLAN_CONFIG_OPEN;
    }

    config->channel = channel;

    exit:
        return SUCCESS;

    err_exit:
        return FAILURE;
}


/* Updater task handle */
static OsiTaskHandle updater_hdnl;


static void updater_task(void *pvParameters) {
    _i8 status;

    switch(cfg_packet.header.type) {

        case NetworkConfigurationPacket:
            (void)pvParameters;
            WlanConfig cfg;
            mem_set(&cfg, 0, sizeof cfg);

            /* Trying to parse network packet */
            status = get_wlan_config_from_packet(cfg_packet.packet_data, &cfg);
            OSI_ASSERT_WITH_EXIT(status, updater_hdnl);

            OSI_COMMON_LOG("Got config\r\n");
            wlan_print_config(&cfg);

            /* Trying to save configuration */
            status = wlan_save_config(&cfg);
            OSI_ASSERT_WITH_EXIT(status, updater_hdnl);

            /* Restarting system to apply changes */
            sys_restart();

            break;

        case ObserverKeyPacket:
            break;

        case EncryptionConfigPacket:
            break;

        case SignConfigPacket:
            break;
    }

    exit:
        osi_TaskDelete(&updater_hdnl);
}


static void uart_irq_hdnl(void) {
    _i16 status;

    /* Disable DMA */
    MAP_UARTDMADisable(WIRED_UART, WIRED_UART_RX_DMA);

    if(state == StateRecvHeader) {
        status = parse_header(cfg_packet.packet_data, &cfg_packet.header);

        if(status < 0) {
            OSI_ERROR_LOG(status);
            return;
        }

        UART_PRINT("%d\r\n", cfg_packet.header.data_size);

        /* Receive the rest */
        state = StateRecvBody;
        wired_recv(cfg_packet.header.data_size, 0);
    }
    else {
        UART_PRINT("Received body:\r\n\t");
        _i16 size = cfg_packet.header.data_size;

        for(int i=0; i<size; i++) {
            UART_PRINT("0x%02x ", cfg_packet.packet_data[PL_PACKET_HEADER_SIZE + i]);
        }
        UART_PRINT("\r\n");

        status = osi_TaskCreate(updater_task, "updater_task", WLAN_UPDATER_STACK_SIZE,
                                NULL, 2, &updater_hdnl);

        if(status < 0) {
            OSI_ERROR_LOG(status);
            return;
        }

        /* Receive header again */
        status = StateRecvHeader;
        wired_recv(PL_PACKET_HEADER_SIZE, 0);
    }

    /* Clear interrupt */
    MAP_UARTIntClear(WIRED_UART, UART_INT_DMARX);
}


void wired_conf_start(void) {
    create_packet(&cfg_packet, WIRED_MAX_CONFIG_SIZE);
    UDMAInit();

    MAP_UARTIntRegister(WIRED_UART, uart_irq_hdnl);
    MAP_UARTIntEnable(WIRED_UART, UART_INT_DMARX);


    MAP_UARTConfigSetExpClk(WIRED_UART, MAP_PRCMPeripheralClockGet(WIRED_UART_PRCM),
                            WIRED_UART_BAUDRATE,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));

    /* UART(NOT DMA) RX/TX interrupts are triggered each 2 bytes. */
    UARTFIFOLevelSet(WIRED_UART, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    state = StateRecvHeader;
    wired_recv(PL_PACKET_HEADER_SIZE, 0);
}

























