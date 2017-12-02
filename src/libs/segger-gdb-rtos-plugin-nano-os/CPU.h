/*
Copyright(c) 2017 Cedric Jimenez

This file is part of Nano-OS.

Nano-OS is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Nano-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Nano-OS.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CPU_H
#define CPU_H

#include "RTOSPlugin.h"

/** \brief Invalid stack offset */
#define CPU_REG_INVALID_OFFSET  -1

/** \brief Stack pointer stack offset */
#define CPU_REG_SP_OFFSET       -2

/** \brief Ascending stack */
#define ASCENDING_STACK          1

/** \brief Descending stack */
#define DESCENDING_STACK        -1


/** \brief CPU register description */
typedef struct _nano_os_cpu_reg_t
{
    /** \brief Id */
    U32 id;
    /** \brief Name */
    const char* name;
    /** \brief Stack offset */
    I32 stack_offset;
    /** \brief Size in bytes */
    U8 size;
} nano_os_cpu_reg_t;

/** \brief Description of a CPU register set */
typedef struct _nano_os_cpu_register_set_t
{
    /** \brief CPU registers */
    const nano_os_cpu_reg_t* registers;
    /** \brief Number of registers to output */
    U32 output_reg_count;
} nano_os_cpu_register_set_t;

/** \brief Function which retrieve a set of CPU registers */
typedef const nano_os_cpu_register_set_t* (*fp_cpu_registers_get)(const GDB_API* gdb_api, const char* const port_name, const U32 task_port_data_address);

/** \brief Description of a CPU port */
typedef struct _nano_os_cpu_port_t
{
    /** \brief JLink Core ID */
    U32 core_id;
    /** \brief CPU name */
    const char* cpu_name;
    /** \brief Stack growth direction */
    I8 stack_growth_dir;
    /** \brief Function which retrieve the set of CPU registers */
    fp_cpu_registers_get registers_get;
} nano_os_cpu_port_t;



/** \brief Compute the stack frame size for a given CPU */
U32 CPU_computeStackFrameSize(const nano_os_cpu_register_set_t* const cpu_reg_set);

/** \brief Find a register indentified by its id for a given CPU */
const nano_os_cpu_reg_t* CPU_findRegister(const nano_os_cpu_register_set_t* const cpu_reg_set, const U32 register_id);

/** \brief Get the value of a given register */
char* CPU_getRegValue(const nano_os_cpu_port_t* const cpu, const nano_os_cpu_reg_t* const cpu_reg, 
                      const U32 top_of_stack_address, const U8* top_of_stack, char* value);


#endif /* CPU_H */
