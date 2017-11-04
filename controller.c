#include "controller.h"

#include "packet_handler.h"
#include "packet_manager.h"

#include "programmer.h"
#include "sys.h"
#include "config.h"

#include "logging.h"

#include <stdlib.h>


/* Packet handling functions */
static _i16 process_prog_init(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_prog_stop(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_uart_init(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_uart_stop(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_reset(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_close_conn(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_enable_encryption(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_enable_sign(OsiMsgQ_t *out_queue, Packet *packet);


static _i16 process_load_mcu_info_packet(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_cmd_packet(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_program_memory_packet(OsiMsgQ_t *out_queue, Packet *packet);
static _i16 process_read_memory_packet(OsiMsgQ_t *out_queue, Packet *packet);


/* Mapping from packet type to corresponding handler */
typedef _i16 (*PacketHandler)(OsiMsgQ_t *out_queue, Packet *packet);

static PacketHandler packet_handlers[PacketsNumber] = {
    [ProgrammerInitPacket] = process_prog_init,
    [ProgrammerStopPacket] = process_prog_stop,
    [UartInitPacket] = process_uart_init,
    [UartStopPacket] = process_uart_stop,
    [ResetPacket] = process_reset,
    [CloseConnectionPacket] = NULL, // Handled on lower level in packet_handler.c
    [EnableEncryptionPacket] = process_enable_encryption,
    [EnableSignPacket] = process_enable_sign,
    [ErrorPacket] = NULL, // Do not receive it

    [LoadMCUInfoPacket] = process_load_mcu_info_packet,
    [ProgramMemoryPacket] = process_program_memory_packet,
    [ReadMemoryPacket] = process_read_memory_packet,
    [MemoryPacket] = NULL,  // Do not receive it
    [CMDPacket] = process_cmd_packet
};



_i16 process_packet(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status;
    PacketHeader header = packet->header;
    print_packet(packet);

    status = packet_handlers[header.type](out_queue, packet);

    return status;
}



/*************************** CONTROL PACKETS ********************************/
/**                                                                        **/
/****************************************************************************/
_i16 process_prog_init(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status;

    /* Check for uart, save its' state and pause */

    return status;
}


static _i16 process_prog_stop(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status;
    /* Check for previous uart status and  */

    return status;
}


static _i16 process_uart_init(OsiMsgQ_t *out_queue, Packet *packet) {
    return 0;
}


static _i16 process_uart_stop(OsiMsgQ_t *out_queue, Packet *packet) {
    return 0;
}

static _i16 process_reset(OsiMsgQ_t *out_queue, Packet *packet) {
    if(packet->packet_data[0] == 1) {
        sys_reset_mcu(MCU_RESET_ON);
    }
    else if(packet->packet_data[1] == 0){
        sys_reset_mcu(MCU_RESET_OFF);
    }
    else {
        return -1;
    }

    return 0;
}


static _i16 process_enable_encryption(OsiMsgQ_t *out_queue, Packet *packet) {
    return 0;
}


static _i16 process_enable_sign(OsiMsgQ_t *out_queue, Packet *packet) {
    return 0;
}




/************************** PROGRAMMER PACKETS ******************************/
/**                                                                        **/
/****************************************************************************/
static _i16 process_cmd_packet(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status = 0;
    _u8 cmd_size = packet->header.data_size;

    if(cmd_size != AVR_CMD_SIZE) {
        OSI_COMMON_LOG("Received CMD has wrong size of %d\r\n", cmd_size);
        return -1;
    }

    AvrCommand answer;
    AvrCommand cmd;
    memcpy(cmd.cmd, packet->packet_data, AVR_CMD_SIZE);

    status = programmer_write_cmd(&cmd, &answer);
    if(status < 0) {
        OSI_ARRAY_LOG("Command loading failed.\r\n\t Command: ", cmd.cmd, AVR_CMD_SIZE);
        OSI_ARRAY_LOG("\tAnswer: ", answer.cmd, AVR_CMD_SIZE);

        send_error("Failed to load command into MCU\r\n", out_queue);
        OSI_ASSERT_WITHOUT_EXIT(status);
    }

    status = send_avr_cmd(answer.cmd, out_queue);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


static _i16 process_load_mcu_info_packet(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status;

    AvrMcuInfo *info = malloc(sizeof *info);

    status = get_mcu_info(packet, info);
    OSI_ASSERT_ON_ERROR(status);

    /* If previous info exists it will be freed automatically */
    programmer_set_mcu_info(info);

    for(int i=0; i<ENTER_PGM_ATTEMPS; i++) {
        status = programmer_enable_pgm_mode();

        if(status >= 0) {
            break;
        }
    }

    if(status < 0) {
        status = send_error("Failed to enter PGM mode\r\n", out_queue);
        SYS_ASSERT_CRITICAL(status);
    }

    status = send_ack(status, out_queue);
    SYS_ASSERT_CRITICAL(status);

    return status;
}


static _i16 process_program_memory_packet(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status;
    AvrProgMemData mem_data;
    _u8 pgm_cmd[4];

    status = get_prog_mem_data(packet, &mem_data);
    OSI_ASSERT_ON_ERROR(status);

    status = programmer_program_memory(&mem_data);

    if(status < 0) {
        send_error("Failed to program memory\r\n", out_queue);
        SYS_ASSERT_CRITICAL(status);
    }

    status = send_ack(status, out_queue);
    SYS_ASSERT_CRITICAL(status);

    return status;
}


static _i16 process_read_memory_packet(OsiMsgQ_t *out_queue, Packet *packet) {
    _i16 status;
    AvrReadMemData mem_data;

    status = get_read_mem_data(packet, &mem_data);
    OSI_ASSERT_ON_ERROR(status);

    /* Create packet by hands in order to avoid data copy */
    Packet *memory_packet;
    status = get_packet_from_pool(&memory_packet);
    OSI_ASSERT_ON_ERROR(status);

    status = programmer_read_memory(&mem_data, memory_packet->packet_data);
    if(status < 0) {
        status = send_error("Failed to read memory\r\n", out_queue);
        OSI_ASSERT_ON_ERROR(status);
    }

    if(status >= 0) {
        status = send_packet(packet, MemoryPacket, mem_data.bytes_to_read, out_queue);
        OSI_ASSERT_ON_ERROR(status);
    }

    return status;
}
