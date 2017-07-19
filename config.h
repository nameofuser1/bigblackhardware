#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "BigBlackProgrammer"

/* Simplelink task priority */
#define SPAWN_TASK_PRIORITY         9

/* Initialization task */
#define INIT_TASK_STACK_SIZE        1024
#define INIT_TASK_NAME              "InitializationTask"
#define INIT_TASK_PRIORITY          3

/* LISTENING TASK */
#define LISTENING_TASK_STACK_SIZE   1024
#define LISTENING_TASK_NAME         "ListeningTask"
#define LISTENING_TASK_PRIORITY     2

/* Packet handling task */
#define HANDLING_TASK_STACK_SIZE    2048
#define HANDLING_TASK_NAME          "ControllerTask"
#define HANDLING_TASK_PRIORITY      2

/* Wired Configurator Task */
#define WIRED_CONF_TASK_STACK_SIZE  1024
#define WIRED_CONF_TASK_NAME        "WiredConfigurator"
#define WIRED_CONF_TASK_PRIO        2

#define WIRED_UART                  UARTA0_BASE
#define WIRED_UART_PRCM             PRCM_UARTA0
#define WIRED_UART_RX_DMA           UART_DMA_RX
#define WIRED_UART_BAUDRATE         115200

#define WIRED_MAX_CONFIG_SIZE       256

/* UART task. Do I need a task? */
#define UART_TASK_STACK_SIZE        1024
#define UART_TASK_NAME              "UartTask"
#define UART_TASK_PRIO              2

/* Programmer task */
#define PROGRAMMER_TASK_STACK_SIZE  2048
#define PROGRAMMER_TASK_NAME        "ProgrammerTask"
#define PROGRAMMER_TASK_PRIO        2

/* Network parameters */
#define SSID_NAME                   "ChtoZaSet"
#define SSID_PWD                    "GrEs4242SeRg"

/* UDP resolver parameters */
#define UDP_RESOLVER_PORT           1094
#define DEFAULT_DEVICE_KEY          "00000000000000000000000000000000"

/* TCP parameters */
#define MAX_TCP_CONNECTIONS         2
#define TCP_LISTENING_PORT          1000
#define TCP_LISTENING_ADDR          0x00

/* Maximum in/out messages in queue */
#define MSG_QUEUE_SIZE              5
#define MSG_POOL_SIZE               (2*MSG_QUEUE_SIZE)

/* Maximum time that can be taken to write into queue */
#define MSG_QUEUE_WRITE_WAIT_MS     5

#define PROG_SPI_BASE               GSPI_BASE
#define PROG_SPI_DEFAULT_FREQ       100000

#endif // CONFIG_H_INCLUDED
