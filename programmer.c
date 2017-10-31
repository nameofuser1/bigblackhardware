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

#include "packet_manager.h"


/* Current SPI bitrate */
static int spi_bitrate = PROG_SPI_DEFAULT_FREQ;

/* Used to resume thread */
static OsiSyncObj_t resume_obj;

/* Running flag */
static _u8 running = FALSE;

/* Contains current MCU info */
static AvrMcuInfo *mcu_info = NULL;

/* Task prototype */
static void prog_task(void *pvParameters);

/* Static methods prototypes */
static _i16 read_memory(AvrReadMemData *mem_data, _u8 *buf);
static _i16 program_memory(AvrProgMemData *mem_data);

static inline void spi_enable(void);
static inline void spi_disable(void);
static inline void configure_spi(unsigned long spi_rate);


/*******************************************************/
void programmer_set_mcu_info(AvrMcuInfo *info) {
    if(mcu_info != NULL) {
        free(mcu_info);
    }

    mcu_info = info;
}


_i16 programmer_enable_pgm_mode(void) {
    return 0;
}


_i16 programmer_write_cmd(AvrCommand *cmd, AvrCommand *answer) {
    _i16 status;

    status = programmer_write_raw_cmd(cmd->cmd, answer->cmd);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


_i16 programmer_write_raw_cmd(_u8 *cmd, _u8 *answer) {
    _i16 status;
    _u8 temp_answer[AVR_CMD_SIZE];
    MAP_SPITransfer(PROG_SPI_BASE, cmd, temp_answer, AVR_CMD_SIZE, 0);

    status = check_cmd_status(cmd, temp_answer);

    if(answer != NULL) {
        memcpy(answer, temp_answer, AVR_CMD_SIZE);
    }

    return status;
}




/************************************************************/
/**               PROGRAMMING MEMORY SECTION               **/
/************************************************************/
static _i16 load_memory_cmd(_u8 *cmd, _u32 delay);
static _i16 program_eeprom_memory(AvrProgMemData *prog_data);
static _i16 program_flash_memory(AvrProgMemData *mem_data);


_i16 programmer_program_memory(AvrProgMemData *mem_data) {
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
		create_memory_cmd(mcu_info->flash_load_lo_pattern, mcu_info->flash_load_lo_len,
                          address, data_byte, cmd);

        status = load_memory_cmd(cmd, mcu_info->flash_wait_ms);
        OSI_ASSERT_ON_ERROR(status);

		/* Loading  high byte */
		data_byte = mem_data->data[i+1];
		create_memory_cmd(mcu_info->flash_load_hi_pattern, mcu_info->flash_load_hi_len,
				address, data_byte, cmd);

        status = load_memory_cmd(cmd, mcu_info->flash_wait_ms);
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
		create_memory_cmd(mcu_info->eeprom_write_pattern, mcu_info->eeprom_write_len,
				address, data_byte, cmd);

		status = load_memory_cmd(cmd, mcu_info->eeprom_wait_ms);
		OSI_ASSERT_ON_ERROR(status);

		address++;
	}

	return status;
}


static _i16 load_memory_cmd(_u8 *cmd, _u32 delay) {
    _i16 status;
    _i8 answer[AVR_CMD_SIZE];

    status = programmer_write_raw_cmd(cmd, answer);
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
_i16 programmer_read_memory(AvrReadMemData *mem_data, _u8 *buf)
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
		create_memory_cmd(mcu_info->flash_read_hi_pattern, mcu_info->flash_read_hi_len,
				address, 0, cmd);

		status = programmer_write_raw_cmd(cmd, res);
		OSI_ASSERT_ON_ERROR(status);

		buf[answer_counter++] = res[AVR_CMD_SIZE-1];

		/* Read low byte */
		create_memory_cmd(mcu_info->flash_read_lo_pattern, mcu_info->flash_read_lo_len,
				address, 0, cmd);

		status = programmer_write_raw_cmd(cmd, res);
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

	char 	*read_pattern 	 = mcu_info->eeprom_read_pattern;
	_u8 read_pattern_len = mcu_info->eeprom_read_len;

	for(int i=0; i<mem_info->bytes_to_read; i++)
	{
		create_memory_cmd(read_pattern, read_pattern_len,
				address, 0, cmd);

		status = programmer_write_raw_cmd(cmd, res);
		OSI_ASSERT_ON_ERROR(status);

		buf[answer_counter++] = res[AVR_CMD_SIZE-1];
		address++;
	}

	return mem_info->bytes_to_read;
}



/**************************************************************/
/**                 ADDITIONAL FUNCTIONS SECTION             **/
/**************************************************************/



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


