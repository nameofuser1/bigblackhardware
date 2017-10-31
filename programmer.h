#ifndef PROGRAMMER_H_INCLUDED
#define PROGRAMMER_H_INCLUDED

#include "hw_types.h"
#include "simplelink.h"
#include "osi.h"

#include "programmer_parser.h"

void programmer_set_mcu_info(AvrMcuInfo *info);
_i16 programmer_enable_pgm_mode(void);
_i16 programmer_write_cmd(AvrCommand *cmd, AvrCommand *answer);
_i16 programmer_write_raw_cmd(_u8 *cmd, _u8 *answer);
_i16 programmer_program_memory(AvrProgMemData *mem_data);
_i16 programmer_read_memory(AvrReadMemData *mem_data, _u8 *buf);

#endif // PROGRAMMER_H_INCLUDED
