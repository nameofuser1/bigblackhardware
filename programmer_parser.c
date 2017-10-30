#include "programmer_parser.h"
#include "programmer_config.h"
#include "protocol.h"

#include <stdlib.h>

#define PROG_MEM_FIXED_SIZE     5
#define READ_MEM_FIXED_SIZE     9


AvrMemoryType get_memory_type(_u8 byte)
{
	switch(byte)
	{
		case PL_FLASH_MEMORY_BYTE:
			return MEMORY_FLASH;

		case PL_EEPROM_MEMORY_BYTE:
			return MEMORY_EEPROM;
	}

	return MEMORY_ERR;
}


/**************************************************************
TODO: speed up removing copies
***************************************************************/
_i16 get_prog_mem_data(Packet *packet, AvrProgMemData *mem_data)
{
	_u8 *buf = packet->packet_data;
    _u32 len = packet->header.data_size;

	mem_data->start_address = (buf[0] << 24) | (buf[1] << 16)
			| (buf[2] << 8) | (buf[3]);

	mem_data->memory_type = get_memory_type(buf[4]);
	mem_data->data_len = (len-PROG_MEM_FIXED_SIZE);
	mem_data->data = (_u8*) malloc(sizeof(_u8) * mem_data->data_len);

	memcpy(mem_data->data, buf+PROG_MEM_FIXED_SIZE, mem_data->data_len);

	return 0;
}


_i16 get_read_mem_data(Packet *packet, AvrReadMemData *mem_data) {
    if(packet->header.data_size != READ_MEM_FIXED_SIZE) {
        return -1;
    }

	_u8 *buf = packet->packet_data;

	mem_data->mem_t = get_memory_type(buf[0]);
	mem_data->start_address = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) |
			(buf[4]);

	mem_data->bytes_to_read = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) |
			(buf[8]);

	return 0;
}


_i16 check_cmd_status(_u8 *cmd, _u8 *answer) {
    for(int i=1; i<AVR_CMD_SIZE; i++) {
        if(answer[i] != cmd[i-1]) {
            return -1;
        }
    }

    return 0;
}


_i16 get_mcu_info(Packet *packet, AvrMcuInfo *mcu_data) {
	_u8 *buf = packet->packet_data;
	_u32 k = 0;

	mcu_data->flash_load_lo_len = buf[k++];
	mcu_data->flash_load_lo_pattern = calloc(mcu_data->flash_load_lo_len + 1, sizeof(char));

	for(_u32 i=0; i<mcu_data->flash_load_lo_len; i++)
	{
		mcu_data->flash_load_lo_pattern[i] = buf[k++];
	}
	mcu_data->flash_load_lo_pattern[k] = '\0';

	mcu_data->flash_load_hi_len = buf[k++];
	mcu_data->flash_load_hi_pattern = calloc(mcu_data->flash_load_hi_len + 1, sizeof(char));

	for(_u32 i=0; i<mcu_data->flash_load_hi_len; i++)
	{
		mcu_data->flash_load_hi_pattern[i] = (char)buf[k++];
	}
	mcu_data->flash_load_hi_pattern[k] = '\0';

	mcu_data->flash_read_lo_len = buf[k++];
	mcu_data->flash_read_lo_pattern = calloc(mcu_data->flash_read_lo_len + 1, sizeof(char));

	for(_u32 i=0; i<mcu_data->flash_read_lo_len; i++)
	{
		mcu_data->flash_read_lo_pattern[i] = (char)buf[k++];
	}
	mcu_data->flash_read_lo_pattern[k] = '\0';


	mcu_data->flash_read_hi_len = buf[k++];
	mcu_data->flash_read_hi_pattern = calloc(mcu_data->flash_read_hi_len + 1, sizeof(char));

	for(_u32 i=0; i<mcu_data->flash_read_hi_len; i++)
	{
		mcu_data->flash_read_hi_pattern[i] = buf[k++];
	}
	mcu_data->flash_load_hi_pattern[k] = '\0';

	mcu_data->flash_wait_ms = buf[k++];


	mcu_data->eeprom_write_len = buf[k++];
	mcu_data->eeprom_write_pattern = calloc(mcu_data->eeprom_write_len + 1, sizeof(char));

	for(_u32 i=0; i<mcu_data->eeprom_write_len; i++)
	{
		mcu_data->eeprom_write_pattern[i] = buf[k++];
	}
	mcu_data->eeprom_write_pattern[k] = '\0';


	mcu_data->eeprom_read_len = buf[k++];
	mcu_data->eeprom_read_pattern = calloc(mcu_data->eeprom_read_len + 1, sizeof(char));

	for (_u32 i=0; i<mcu_data->eeprom_read_len; i++)
	{
		mcu_data->eeprom_read_pattern[i] = buf[k++];
	}
	mcu_data->eeprom_read_pattern[k] = '\0';

	mcu_data->eeprom_wait_ms = buf[k++];

	for(_u32 i=0; i<AVR_CMD_SIZE; i++)
	{
		mcu_data->pgm_enable[i] = buf[k++];
	}

	return 0;
}

/*
 * ********************************************************************
 * Miss comparing with 'x' and 'o' as memory filled with zeroes
 * Works for load, write and read commands
 * ********************************************************************
 */
void create_memory_cmd(char *pattern, _u8 pattern_len,
                       _u32 addr, _u8 input, _u8 *cmd)
{
	memset(cmd, 0, sizeof(_u8)*AVR_CMD_SIZE);

	_u8 cmd_byte_offset = 0;
	_i8 cmd_bit_offset = 7;
	_u8 cmd_input_offset = 7;

	for(_u8 j=0; j<pattern_len; j++)
	{
		if(pattern[j] == '1')
		{
			cmd[cmd_byte_offset] |= (1 << cmd_bit_offset);
		}
		else if(pattern[j] == '0')
		{
			cmd[cmd_byte_offset] &= ~(0 << cmd_bit_offset);
		}
		else if(pattern[j] == 'a')
		{
			j++;
			_u8 k = 0;
			char ch_address_shift[3];

			while((pattern[j] >= '0') && (pattern[j] <= '9'))
			{
				ch_address_shift[k++] = pattern[j++];
			}
			/* last j++ plus j++ in cycle causes missing next char */
			j--;
			ch_address_shift[k] = '\0';

			_u32 address_shift = atoi(ch_address_shift);
			_u8 address_bit = (addr >> address_shift) & 0x01;
			cmd[cmd_byte_offset] |= (address_bit << cmd_bit_offset);
		}
		else if(pattern[j] == 'i')
		{
			_u8 input_bit = (input >> cmd_input_offset) & 0x01;
			cmd[cmd_byte_offset] |= (input_bit << cmd_bit_offset);
			cmd_input_offset--;
		}

		if(--cmd_bit_offset < 0)
		{
			cmd_bit_offset = 7;
			cmd_byte_offset += 1;
		}
	}
}
