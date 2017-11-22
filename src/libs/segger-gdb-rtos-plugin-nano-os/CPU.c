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

#include "CPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/** \brief Compute the stack frame size for a given CPU */
U32 CPU_computeStackFrameSize(const nano_os_cpu_port_t* const cpu)
{
    U32 stack_frame_size = 0u;

    /* Go through all CPU registers */
    const nano_os_cpu_reg_t* cpu_reg = cpu->registers;
    while (cpu_reg->name != NULL)
    {
        /* Compute size only for stacked registers */
        if (cpu_reg->stack_offset > 0)
        {
            stack_frame_size += cpu_reg->size;
        }

        /* Next register */
        cpu_reg++;
    }

    return stack_frame_size;
}


/** \brief Find a register indentified by its id for a given CPU */
const nano_os_cpu_reg_t* CPU_findRegister(const nano_os_cpu_port_t* const cpu, const U32 register_id)
{
    bool found = false;

    /* Go through all CPU registers */
    const nano_os_cpu_reg_t* cpu_reg = cpu->registers;
    while ((cpu_reg->name != NULL) && !found)
    {
        /* Check register id */
        if (cpu_reg->id == register_id)
        {
            found = true;
        }
        else
        {
            /* Next register */
            cpu_reg++;
        }
    }

    return cpu_reg;
}


/** \brief Get the value of a given register */
char* CPU_getRegValue(const nano_os_cpu_port_t* const cpu, const nano_os_cpu_reg_t* const cpu_reg,
                      const U32 top_of_stack_address, const U8* top_of_stack, char* value)
{
    U8 i;
    char* reg_value = value;

    /* Compute register address */
    U32 null_value = 0u;
    const U8* reg_address = 0u;
    if (cpu_reg->stack_offset >= 0)
    {
        reg_address = top_of_stack - cpu->stack_growth_dir * cpu_reg->stack_offset;
    }
    else if (cpu_reg->stack_offset == CPU_REG_SP_OFFSET)
    {
        reg_address = (const U8*)&top_of_stack_address;
    }
    else
    {
        reg_address = (const U8*)&null_value;
    }

    /* Compute register value */
    for (i = 0; i < cpu_reg->size; i++)
    {
        reg_value += snprintf(reg_value, 3u, "%02x", (*reg_address));
        reg_address++;
    }

    return reg_value;
}
