#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#define PL_CONTROL_PACKETS_MSK         0x10
#define PL_PROGRAMMER_PACKETS_MSK      0x20
#define PL_UART_PACKETS_MSK            0x30

/* CONTROL PACKETS */
#define PL_PROGRAMMER_INIT             0x10
#define PL_PROGRAMMER_STOP             0x11
#define PL_UART_INIT                   0x12
#define PL_UART_STOP                   0x13
#define PL_RESET                       0x14
#define PL_ACK_PACKET                  0x15
#define PL_CLOSE_CONNECTION            0x16
#define PL_NETWORK_CONFIGURATION       0x17
#define PL_SET_OBSERVER_KEY            0x18
#define PL_SET_ENCRYPTION_KEYS         0x19
#define PL_SET_SIGN_KEYS               0x1A
#define PL_ENABLE_ENCRYPTION           0x1B
#define PL_ENABLE_SIGN                 0x1C

/* PROGRAMMER PACKETS */
#define PL_LOAD_MCU_INFO               0x20
#define PL_PROGRAM_MEMORY              0x21
#define PL_READ_MEMORY                 0x22
#define PL_MEMORY                      0x23
#define PL_CMD                         0x24

/* UART PACKETS */
#define PL_UART_CONFIGURATION          0x30
#define PL_UART_DATA                   0x31

/* Header structure */
#define PL_START_FRAME_BYTE         0x1B

#define PL_FLAGS_FIELD_OFFSET       1
#define PL_FLAGS_FIELD_SIZE         1

#define PL_TYPE_FIELD_OFFSET		2
#define PL_TYPE_FIELD_SIZE          1

#define PL_SIZE_FIELD_OFFSET		3
#define PL_SIZE_FIELD_SIZE			2

#define PL_PACKET_HEADER_SIZE		(PL_SIZE_FIELD_SIZE + PL_TYPE_FIELD_SIZE + PL_FLAGS_FIELD_SIZE + 1)
#define PL_PACKET_RESERVED_BYTES	(PL_SIZE_FIELD_SIZE + PL_TYPE_FIELD_SIZE + PL_FLAGS_FIELD_SIZE + 1)

/* Flags byte */
#define PL_FLAG_COMPRESSION_BIT     0
#define PL_FLAG_ENCRYPTION_BIT      1
#define PL_FLAG_SIGN_BIT            2

/* Maximum data field length of packet */
#define PL_MAX_DATA_LENGTH	        1029

/* ACK packet */
#define PL_ACK_BYTE_OFFSET          0
#define PL_ACK_SUCCESS              1
#define PL_ACK_FAILURE              0

/* Reset packet */
#define PL_RESET_BYTE_OFFSET	    0
#define PL_RESET_ENABLE 		    1
#define PL_RESET_DISABLE		    0

/* Set observer key packet */
#define PL_OBSERVER_KEY_OFFSET      0
#define PL_OBSERVER_KEY_SIZE        32

/* Network packet  */
#define PL_NETWORK_MODE_OFFSET      0
#define PL_NETWORK_MODE_STA         0
#define PL_NETWORK_MODE_AP          1

#define PL_NETWORK_PHY_OFFSET       1
#define PL_NETWORK_PHY_B            0
#define PL_NETWORK_PHY_G            1
#define PL_NETWORK_PHY_N            2

#define PL_NETWORK_CHANNEL_OFFSET   2
#define PL_NETWORK_SSID_LEN_OFFSET  3

/* USART packet */
#define PL_USART_BAUDRATE_BYTE_OFFSET		(0)
#define PL_USART_PARITY_BYTE_OFFSET		    (USART_BAUDRATE_BYTE_OFFSET+1)
#define PL_USART_DATA_BITS_BYTE_OFFSET		(USART_PARITY_BYTE_OFFSET+1)
#define PL_USART_STOP_BITS_BYTE_OFFSET		(USART_DATA_BITS_BYTE_OFFSET+1)

/* USART configuration bytes */
#define PL_USART_PARITY_EVEN		0xC0
#define PL_USART_PARITY_ODD		    0xC1
#define PL_USART_PARITY_NONE		0xC2
#define PL_USART_DATA_BITS_8		0xC3
#define PL_USART_DATA_BITS_9		0xC4
#define PL_USART_STOP_BITS_1		0xC5
#define PL_USART_STOP_BITS_2		0xC6
#define PL_USART_BAUDRATE_9600		0xC7
#define PL_USART_BAUDRATE_19200	    0xC8
#define PL_USART_BAUDRATE_34800     0xC9
#define PL_USART_BAUDRATE_57600	    0xCA
#define PL_USART_BAUDRATE_74880     0xCB
#define PL_USART_BAUDRATE_115200	0xCC

/* USART errors */
#define USART_PARITY_ERROR_BYTE 	0xE0
#define USART_FRAME_ERROR_BYTE		0xE1
#define USART_DATA_BITS_ERROR_BYTE	0xE2
#define USART_STOP_BITS_ERROR_BYTE	0xE3
#define USART_BAUDRATE_ERROR_BYTE	0xE4

// #pragma GCC diagnostic ignored "-Wunused-function"


#endif // PROTOCOL_H_INCLUDED
