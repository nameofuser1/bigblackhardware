#include "programmer.h"

#include "hw_memmap.h"
#include "rom_map.h"
#include "prcm.h"
#include "spi.h"
#include "config.h"

#include "sys.h"
#include "packets.h"
#include "logging.h"

#include "programmer_config.h"
#include "programmer_parser.h"
#include "protocol.h"


/* Current SPI bitrate */
static int spi_bitrate = PROG_SPI_DEFAULT_FREQ;

/* Used to resume thread */
static OsiSyncObj_t resume_obj;

/* Running flag */
static _u8 running = FALSE;

/* Contains current MCU info */
static AvrMcuInfo mcu_info;

/* Task prototype */
static void prog_task(void *pvParameters);

/**/
typedef struct {
    OsiMsgQ_t *in_queue;
    OsiMsgQ_t *out_queue;
} MsgQueues;

/* Keep in/out queues */
static MsgQueues queues;

/* Static methods prototypes */
static _i16 read_memory(AvrReadMemData *mem_data, _u8 *buf);
static _i16 program_memory(AvrProgMemData *mem_data);

static inline _i16 send_ack(_i16 status);
static inline _i16 create_send_packet(PacketType type, _u8 *data, _u16 data_len);
static inline _i16 send_packet(Packet *packet, PacketType type, _u16 data_len);
static inline _i16 read_packet(Packet **packet);
static inline _i16 write_packet(Packet *packet);
static inline _i16 enable_pgm_mode(void);
static inline void spi_enable(void);
static inline void spi_disable(void);
static inline void configure_spi(unsigned long spi_rate);


_i16 programmer_start(void) {
    _i16 status;

    /* Set SPI frequency up */
    configure_spi(spi_bitrate);

    /* Thread synchronization object */
    status = osi_SyncObjCreate(&resume_obj);
    OSI_ASSERT_ON_ERROR(status);

    /* Create programmer task */
    status = osi_TaskCreate(prog_task, PROGRAMMER_TASK_NAME, PROGRAMMER_TASK_STACK_SIZE,
                            NULL, PROGRAMMER_TASK_PRIO, NULL);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


_i16 programmer_pause(void) {
    running = FALSE;
    return 0;
}



_i16 programmer_resume(void) {
    _i16 status;

    /* Set up the running flag */
    running = TRUE;

    /* Wake up thread */
    status = osi_SyncObjSignal(&resume_obj);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


_i16 programmer_set_queues(OsiMsgQ_t *in_queue, OsiMsgQ_t *out_queue) {
    if(running) {
        return -1;
    }

    queues.in_queue = in_queue;
    queues.out_queue = out_queue;

    return 0;
}


/***********************************************************/
/**              MAIN PROGRAMMER TASK                     **/
/***********************************************************/
static _i16 process_load_mcu_info_packet(Packet *packet);
static _i16 process_cmd_packet(Packet *packet);
static _i16 process_program_memory_packet(Packet *packet);
static _i16 process_read_memory_packet(Packet *packet);

typedef _i16 (*PacketProcessingFcn)(Packet *packet);

/* Lookup table for packet processing functions */
PacketProcessingFcn packet_processors[PROGRAMMER_PACKETS_NUM] =
{
    process_load_mcu_info_packet,
    process_program_memory_packet,
    process_read_memory_packet,
    NULL,
    process_cmd_packet
};


static void prog_task(void *pvParameters) {
    _i16 status;
    Packet *packet;

    prog_entry:
        OSI_COMMON_LOG("Programmer is waiting on sync object...\r\n");
        osi_SyncObjWait(&resume_obj, OSI_WAIT_FOREVER);

        spi_enable();

        while(1) {
            status = sys_queue_read_ptr(queues.in_queue, (void**)&packet, 10);

            if(status >= 0) {
                OSI_COMMON_LOG("Programmer got new packet\r\n");

                /* Call corresponding packet handler */
                PacketType ptype = packet->header.type;
                status = packet_processors[ptype-PROGRAMMER_PACKETS_SHIFT](packet);
                if(status < 0) {
                    OSI_COMMON_LOG("Failed to process packet\r\n");
                    print_packet(packet);
                }

                release_packet(packet);
            }

            if(!running) {
                spi_disable();
                goto prog_entry;
            }

            osi_Sleep(2000);
        }
}



/*************************************************************/
/**               PACKET PROCESSING FUNCTIONS               **/
/*************************************************************/
static _i16 send_command(_u8 *cmd, _u8 *answer);


static _i16 process_cmd_packet(Packet *packet) {
    _i16 status = 0;
    _u8 *cmd = packet->packet_data;
    _u8 cmd_size = packet->header.data_size;

    if(cmd_size != AVR_CMD_SIZE) {
        OSI_COMMON_LOG("Received CMD has wrong size of %d\r\n", cmd_size);
        return -1;
    }

    _u8 answer[AVR_CMD_SIZE];
    status = send_command(cmd, answer);

    if(status < 0) {
        OSI_COMMON_LOG("Failed to load AVR command\r\n");
    }

    _i16 send_status = send_ack(status);
    OSI_ASSERT_ON_ERROR(send_status);

    return status;
}


static _i16 process_load_mcu_info_packet(Packet *packet) {
    _i16 status;

    status = get_mcu_info(packet, &mcu_info);
    OSI_ASSERT_ON_ERROR(status);

    for(int i=0; i<ENTER_PGM_ATTEMPS; i++) {
        status = enable_pgm_mode();

        if(status >= 0) {
            break;
        }
    }

    if(status < 0) {
        OSI_COMMON_LOG("Failed to enter PGM mode\r\n");
    }

    _i16 send_status = send_ack(status);
    OSI_ASSERT_ON_ERROR(send_status);

    return status;
}


static _i16 process_program_memory_packet(Packet *packet) {
    _i16 status;
    AvrProgMemData mem_data;
    _u8 pgm_cmd[4];

    status = get_prog_mem_data(packet, &mem_data);
    OSI_ASSERT_ON_ERROR(status);

    status = program_memory(&mem_data);

    _i16 send_status = send_ack(status);
    OSI_ASSERT_ON_ERROR(send_status);

    return status;
}


static _i16 process_read_memory_packet(Packet *packet) {
    _i16 status;
    AvrReadMemData mem_data;

    status = get_read_mem_data(packet, &mem_data);
    OSI_ASSERT_ON_ERROR(status);

    /* Create packet by hands in order to avoid data copy */
    Packet *memory_packet;
    status = get_packet_from_pool(&memory_packet);
    OSI_ASSERT_ON_ERROR(status);

    status = read_memory(&mem_data, memory_packet->packet_data);

    _i16 send_status;
    send_status = send_ack(status);
    OSI_ASSERT_ON_ERROR(send_status);

    if(status >= 0) {
        send_status = send_packet(packet, MemoryPacket, mem_data.bytes_to_read);
        OSI_ASSERT_ON_ERROR(send_status);
    }

    return status;
}


static _i16 send_command(_u8 *cmd, _u8 *answer) {
    _i16 status;
    _u8 temp_answer[AVR_CMD_SIZE];
    MAP_SPITransfer(PROG_SPI_BASE, cmd, temp_answer, AVR_CMD_SIZE, 0);

    status = check_cmd_status(cmd, temp_answer);
    if(status < 0) {
        OSI_ARRAY_LOG("Command loading failed.\r\n\t Command: ", cmd, AVR_CMD_SIZE);
        OSI_ARRAY_LOG("\tAnswer: ", answer, AVR_CMD_SIZE);
    }

    if(answer != NULL) {
        memcpy(answer, cmd, AVR_CMD_SIZE);
    }

    return status;
}




/************************************************************/
/**               PROGRAMMING MEMORY SECTION               **/
/************************************************************/
static _i16 load_memory_cmd(_u8 *cmd, _u32 delay);
static _i16 program_eeprom_memory(AvrProgMemData *prog_data);
static _i16 program_flash_memory(AvrProgMemData *mem_data);


static _i16 program_memory(AvrProgMemData *mem_data) {
    _i16 status;

    if(mem_data->memory_type == MEMORY_FLASH) {
        OSI_COMMON_LOG("Programming flash memory\r\n");

        status = program_flash_memory(mem_data);
        OSI_ASSERT_ON_ERROR(status);
    }
    else if(mem_data->memory_type == MEMORY_EEPROM) {
        OSI_COMMON_LOG("Programming EEPROM memory\r\n");

        status = program_eeprom_memory(mem_data);
        OSI_ASSERT_ON_ERROR(status);
    }
    else {
        status = -1;
    }

    return status;
}


static _i16 program_flash_memory(AvrProgMemData *mem_data) {
	if(mem_data->data_len % 2 != 0)
	{
		return -1;
	}

	_i16 status;
	_u32 address = mem_data->start_address;
	_u8 answer[AVR_CMD_SIZE];
	_u8 cmd[AVR_CMD_SIZE];

	_u8 data_byte = 0xFF;

	for(_u8 i=0; i<mem_data->data_len; i+=2)
	{
		/* Loading low byte */
		data_byte = mem_data->data[i];
		create_memory_cmd(mcu_info.flash_load_lo_pattern, mcu_info.flash_load_lo_len,
                          address, data_byte, cmd);

        status = load_memory_cmd(cmd, mcu_info.flash_wait_ms);
        OSI_ASSERT_ON_ERROR(status);

		/* Loading  high byte */
		data_byte = mem_data->data[i+1];
		create_memory_cmd(mcu_info.flash_load_hi_pattern, mcu_info.flash_load_hi_len,
				address, data_byte, cmd);

        status = load_memory_cmd(cmd, mcu_info.flash_wait_ms);
		OSI_ASSERT_ON_ERROR(status);

		address++;
	}

	return status;
}


static _i16 program_eeprom_memory(AvrProgMemData *prog_data)
{
    _i16 status;
	_u32 address = prog_data->start_address;
	_u8 answer[4];
	_u8 cmd[4];

	for(int i=0; i<prog_data->data_len; i++)
	{
		_u8 data_byte = prog_data->data[i];
		create_memory_cmd(mcu_info.eeprom_write_pattern, mcu_info.eeprom_write_len,
				address, data_byte, cmd);

		status = load_memory_cmd(cmd, mcu_info.eeprom_wait_ms);
		OSI_ASSERT_ON_ERROR(status);

		address++;
	}

	return status;
}


static _i16 load_memory_cmd(_u8 *cmd, _u32 delay) {
    _i16 status;
    _i8 answer[AVR_CMD_SIZE];

    status = send_command(cmd, answer);
    OSI_ASSERT_ON_ERROR(status);

    osi_Sleep(delay);
    return status;
}



/*************************************************************/
/**                    READ MEMORY SECTION                  **/
/*************************************************************/
static _i16 _read_flash_memory(AvrReadMemData *mem_data, _u8 *buf);
static _i16 _read_eeprom_memory(AvrReadMemData *mem_data, _u8 *buf);

/*
 * **********************************************************
 * Read memory command by command and return packet
 * With read bytes
 *
 * Arguments:
 * 		mem_data	---	Pointer to AvrReadMem data.
 * 							Contains information for reading
 *
 * 		buf			---	buffer to save data into.
 * ***********************************************************
 */
static _i16 read_memory(AvrReadMemData *mem_data, _u8 *buf)
{
	_i16 status;
	//_log_read_mem_info(mem_data);

	if(mem_data->mem_t == MEMORY_FLASH)
	{
		OSI_COMMON_LOG("Reading flash memory");

		status = _read_flash_memory(mem_data, buf);
		OSI_ASSERT_ON_ERROR(status);
	}
	else if(mem_data->mem_t == MEMORY_EEPROM)
	{
		OSI_COMMON_LOG("Reading eeprom memory");

		status = _read_eeprom_memory(mem_data, buf);
		OSI_ASSERT_ON_ERROR(status);
	}
	else
	{
		status = -1;
	}

	return status;
}


/*
 * ************************************************************
 * Reads chunk of flash memory into given buffer
 *
 * Arguments:
 * 	AvrReadMemData *mem_data	---	contains information about
 * 										read operation
 *
 * 	_u8 *buf				---	used to save data into it
 */
static _i16 _read_flash_memory(AvrReadMemData *mem_data, _u8 *buf)
{
    _i16 status;
	_u32 address = mem_data->start_address;
	_u8 answer_counter = 0;

	_u8 cmd[AVR_CMD_SIZE];
	_u8 res[AVR_CMD_SIZE];

	for(int i=0; i<mem_data->bytes_to_read; i+=2)
	{
	    /* Read high byte */
		create_memory_cmd(mcu_info.flash_read_hi_pattern, mcu_info.flash_read_hi_len,
				address, 0, cmd);

		status = send_command(cmd, res);
		OSI_ASSERT_ON_ERROR(status);

		buf[answer_counter++] = res[AVR_CMD_SIZE-1];

		/* Read low byte */
		create_memory_cmd(mcu_info.flash_read_lo_pattern, mcu_info.flash_read_lo_len,
				address, 0, cmd);

		status = send_command(cmd, res);
		OSI_ASSERT_ON_ERROR(status);

		buf[answer_counter++] = res[AVR_CMD_SIZE-1];
		address++;
	}

	return mem_data->bytes_to_read;
}


static _i16 _read_eeprom_memory(AvrReadMemData *mem_info, _u8 *buf)
{
    _i16 status;
	_u32 address = mem_info->start_address;
	_u16 answer_counter = 0;

	_u8 cmd[AVR_CMD_SIZE];
	_u8 res[AVR_CMD_SIZE];

	char 	*read_pattern 	 = mcu_info.eeprom_read_pattern;
	_u8 read_pattern_len = mcu_info.eeprom_read_len;

	for(int i=0; i<mem_info->bytes_to_read; i++)
	{
		create_memory_cmd(read_pattern, read_pattern_len,
				address, 0, cmd);

		status = send_command(cmd, res);
		OSI_ASSERT_ON_ERROR(status);

		buf[answer_counter++] = res[AVR_CMD_SIZE-1];
		address++;
	}

	return mem_info->bytes_to_read;
}



/**************************************************************/
/**                 ADDITIONAL FUNCTIONS SECTION             **/
/**************************************************************/
static inline _i16 send_ack(_i16 status) {
    _i16 send_status;
    _u8 ack_data[1];
    set_ack_data(ack_data, status);

    send_status = create_send_packet(ACKPacket, ack_data, 1);
    OSI_ASSERT_ON_ERROR(send_status);

    return send_status;
}


static inline _i16 create_send_packet(PacketType type, _u8 *data, _u16 data_len) {
    _i16 status;
    Packet *packet;

    status = get_packet_from_pool(&packet);
    OSI_ASSERT_ON_ERROR(status);

    status = create_packet(packet, type, COMPRESSION_OFF, SIGN_OFF, ENCRYPTION_OFF,
                data, data_len);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


static inline _i16 send_packet(Packet *packet, PacketType type, _u16 data_len) {
    _i16 status;

    /* Will not change packet_data field */
    status = create_packet(packet, type, COMPRESSION_OFF, SIGN_OFF, ENCRYPTION_OFF,
                           NULL, data_len);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


static inline _i16 read_packet(Packet **packet) {
    _i16 status;

    status = sys_queue_read_ptr(queues.in_queue, (void**)packet, 10);
    return status;
}


static inline _i16 write_packet(Packet *packet) {
    _i16 status;

    status = sys_queue_write_ptr(queues.out_queue, packet, 10);
    return status;
}


static _i16 enable_pgm_mode(void) {

}


static inline void spi_enable(void) {
    MAP_SPIEnable(PROG_SPI_BASE);
}


static inline void spi_disable(void) {
    MAP_SPIDisable(PROG_SPI_BASE);
}

static inline void configure_spi(unsigned long spi_rate) {
    spi_disable();

    MAP_SPIReset(PROG_SPI_BASE);
    MAP_SPIConfigSetExpClk(PROG_SPI_BASE,
                           MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           spi_rate,
                           SPI_MODE_MASTER,
                           SPI_SUB_MODE_0,
                           (SPI_SW_CTRL_CS |
                            SPI_4PIN_MODE |
                            SPI_TURBO_OFF |
                            SPI_CS_ACTIVELOW |
                            SPI_WL_8));
}


