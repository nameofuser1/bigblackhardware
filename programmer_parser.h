#ifndef PROGRAMMER_PARSER_H_INCLUDED
#define PROGRAMMER_PARSER_H_INCLUDED

#include <simplelink.h>

#include "packets.h"
#include "programmer_config.h"


typedef struct {
	_u8 	flash_load_hi_len;
	char 		*flash_load_hi_pattern;

	_u8		flash_load_lo_len;
	char		*flash_load_lo_pattern;

	_u8		flash_read_lo_len;
	char		*flash_read_lo_pattern;

	_u8		flash_read_hi_len;
	char		*flash_read_hi_pattern;

	_u8		flash_wait_ms;

	_u8		eeprom_write_len;
	char		*eeprom_write_pattern;

	_u8 	eeprom_read_len;
	char		*eeprom_read_pattern;

	_u8		eeprom_wait_ms;
	_u8		pgm_enable[AVR_CMD_SIZE];
} AvrMcuInfo;


typedef enum {
    MEMORY_FLASH = 0,
    MEMORY_EEPROM,
    MEMORY_ERR
} AvrMemoryType;


typedef struct {

	_u32 		    start_address;
	_u32 		    bytes_to_read;
	AvrMemoryType	mem_t;

} AvrReadMemData;


typedef struct {

	_u32 		    start_address;
	AvrMemoryType	memory_type;
	_u8 		    data_len;
	_u8 		    *data;

} AvrProgMemData;


typedef struct {

	_u8 cmd[AVR_CMD_SIZE];

} AvrCommand;


AvrMemoryType   get_memory_type(_u8 byte);
_i16            get_prog_mem_data(Packet *packet, AvrProgMemData *mem_data);
_i16            get_read_mem_data(Packet *packet, AvrReadMemData *mem_data);
_i16            get_mcu_info(Packet *packet, AvrMcuInfo *mcu_data);
_i16            check_cmd_status(_u8 *cmd, _u8 *answer);
void            create_memory_cmd(char *pattern, _u8 pattern_len,
                                  _u32 addr, _u8 input, _u8 *cmd);


#endif // PROGRAMMER_PARSER_H_INCLUDED
