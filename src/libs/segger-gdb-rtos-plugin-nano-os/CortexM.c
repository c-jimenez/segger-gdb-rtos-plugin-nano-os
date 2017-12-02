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

#include "CortexM.h"
#include "JLINKARM_Const.h"

#include <stdlib.h>


/** \brief Cortex-M0 register description */
static const nano_os_cpu_reg_t cortex_m0_registers[] = {
                                                        { 0u, "R0", 0x20, 4u },
                                                        { 1u, "R1", 0x24, 4u },
                                                        { 2u, "R2", 0x28, 4u },
                                                        { 3u, "R3", 0x2C, 4u },
                                                        { 4u, "R4", 0x10, 4u },
                                                        { 5u, "R5", 0x14, 4u },
                                                        { 6u, "R6", 0x18, 4u },
                                                        { 7u, "R7", 0x1C, 4u },
                                                        { 8u, "R8", 0x00, 4u },
                                                        { 9u, "R9", 0x04, 4u },
                                                        { 10u, "R10", 0x08, 4u },
                                                        { 11u, "R11", 0x0C, 4u },
                                                        { 12u, "R12", 0x30, 4u },
                                                        { 13u, "SP", CPU_REG_SP_OFFSET, 4u },
                                                        { 14u, "LR", 0x34, 4u },
                                                        { 15u, "PC", 0x38, 4u },

                                                        { 25u, "XPSR", 0x3C, 4u },
                                                        { 26u, "MSP", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 27u, "PSP", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 28u, "PRIMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 29u, "BASEPRI", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 30u, "FAULTMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 31u, "CONTROL", CPU_REG_INVALID_OFFSET, 4u },

                                                        { 0u, NULL, 0x00, 0u }
                                                       };


/** \brief Cortex-M0+ register description */
static const nano_os_cpu_reg_t cortex_m0p_registers[] = {
                                                            { 0u, "R0", 0x24, 4u },
                                                            { 1u, "R1", 0x28, 4u },
                                                            { 2u, "R2", 0x2C, 4u },
                                                            { 3u, "R3", 0x30, 4u },
                                                            { 4u, "R4", 0x14, 4u },
                                                            { 5u, "R5", 0x18, 4u },
                                                            { 6u, "R6", 0x1C, 4u },
                                                            { 7u, "R7", 0x20, 4u },
                                                            { 8u, "R8", 0x04, 4u },
                                                            { 9u, "R9", 0x08, 4u },
                                                            { 10u, "R10", 0x0C, 4u },
                                                            { 11u, "R11", 0x10, 4u },
                                                            { 12u, "R12", 0x34, 4u },
                                                            { 13u, "SP", CPU_REG_SP_OFFSET, 4u },
                                                            { 14u, "LR", 0x38, 4u },
                                                            { 15u, "PC", 0x3C, 4u },

                                                            { 25u, "XPSR", 0x40, 4u },
                                                            { 26u, "MSP", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 27u, "PSP", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 28u, "PRIMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 29u, "BASEPRI", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 30u, "FAULTMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 31u, "CONTROL", 0x00, 4u },

                                                            { 0u, NULL, 0x00, 0u }
                                                          };

/** \brief Cortex-Mx base register description */
static const nano_os_cpu_reg_t cortex_m_registers[] =  {
                                                        { 0u, "R0", 0x24, 4u },
                                                        { 1u, "R1", 0x28, 4u },
                                                        { 2u, "R2", 0x2C, 4u },
                                                        { 3u, "R3", 0x30, 4u },
                                                        { 4u, "R4", 0x04, 4u },
                                                        { 5u, "R5", 0x08, 4u },
                                                        { 6u, "R6", 0x0C, 4u },
                                                        { 7u, "R7", 0x10, 4u },
                                                        { 8u, "R8", 0x14, 4u },
                                                        { 9u, "R9", 0x18, 4u },
                                                        { 10u, "R10", 0x1C, 4u },
                                                        { 11u, "R11", 0x20, 4u },
                                                        { 12u, "R12", 0x34, 4u },
                                                        { 13u, "SP", CPU_REG_SP_OFFSET, 4u },
                                                        { 14u, "LR", 0x38, 4u },
                                                        { 15u, "PC", 0x3C, 4u },

                                                        { 25u, "XPSR", 0x40, 4u },
                                                        { 26u, "MSP", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 27u, "PSP", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 28u, "PRIMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 29u, "BASEPRI", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 30u, "FAULTMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                        { 31u, "CONTROL", 0x00, 4u },

                                                        { 0u, NULL, 0x00, 0u }
                                                       };

/** \brief Cortex-Mx with VFP register description */
static const nano_os_cpu_reg_t cortex_m_vfp_registers[] = {
                                                            { 0u, "R0", 0x64, 4u },
                                                            { 1u, "R1", 0x68, 4u },
                                                            { 2u, "R2", 0x6C, 4u },
                                                            { 3u, "R3", 0x70, 4u },
                                                            { 4u, "R4", 0x44, 4u },
                                                            { 5u, "R5", 0x48, 4u },
                                                            { 6u, "R6", 0x4C, 4u },
                                                            { 7u, "R7", 0x50, 4u },
                                                            { 8u, "R8", 0x54, 4u },
                                                            { 9u, "R9", 0x58, 4u },
                                                            { 10u, "R10", 0x5C, 4u },
                                                            { 11u, "R11", 0x60, 4u },
                                                            { 12u, "R12", 0x74, 4u },
                                                            { 13u, "SP", CPU_REG_SP_OFFSET, 4u },
                                                            { 14u, "LR", 0x78, 4u },
                                                            { 15u, "PC", 0x7C, 4u },

                                                            { 25u, "XPSR", 0x80, 4u },
                                                            { 26u, "MSP", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 27u, "PSP", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 28u, "PRIMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 29u, "BASEPRI", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 30u, "FAULTMASK", CPU_REG_INVALID_OFFSET, 4u },
                                                            { 31u, "CONTROL", 0x40, 4u },

                                                            { 32u, "FPSR", 0xC4, 4u },
                                                            { 33u, "S0", 0x84, 4u },
                                                            { 34u, "S1", 0x88, 4u },
                                                            { 35u, "S2", 0x8C, 4u },
                                                            { 36u, "S3", 0x90, 4u },
                                                            { 37u, "S4", 0x94, 4u },
                                                            { 38u, "S5", 0x98, 4u },
                                                            { 39u, "S6", 0x9C, 4u },
                                                            { 40u, "S7", 0xA0, 4u },
                                                            { 41u, "S8", 0xA4, 4u },
                                                            { 42u, "S9", 0xA8, 4u },
                                                            { 43u, "S10", 0xAC, 4u },
                                                            { 44u, "S11", 0xB0, 4u },
                                                            { 45u, "S12", 0xB4, 4u },
                                                            { 46u, "S13", 0xB8, 4u },
                                                            { 47u, "S14", 0xBC, 4u },
                                                            { 48u, "S15", 0xC0, 4u },
                                                            { 49u, "S16", 0x00, 4u },
                                                            { 50u, "S17", 0x04, 4u },
                                                            { 51u, "S18", 0x08, 4u },
                                                            { 52u, "S19", 0x0C, 4u },
                                                            { 53u, "S20", 0x10, 4u },
                                                            { 54u, "S21", 0x14, 4u },
                                                            { 55u, "S22", 0x18, 4u },
                                                            { 56u, "S23", 0x1C, 4u },
                                                            { 57u, "S24", 0x20, 4u },
                                                            { 58u, "S25", 0x24, 4u },
                                                            { 59u, "S26", 0x28, 4u },
                                                            { 60u, "S27", 0x2C, 4u },
                                                            { 61u, "S28", 0x30, 4u },
                                                            { 62u, "S29", 0x34, 4u },
                                                            { 63u, "S30", 0x38, 4u },
                                                            { 64u, "S31", 0x3C, 4u },

                                                            { 0u, NULL, 0x00, 0u }
                                                          };


/** \brief Cortex-M0 register set */
static const nano_os_cpu_register_set_t cortex_m0_register_set = { cortex_m0_registers, 17u };

/** \brief Cortex-M0+ register set */
static const nano_os_cpu_register_set_t cortex_m0p_register_set = { cortex_m0p_registers, 17u };

/** \brief Cortex-Mx base register set */
static const nano_os_cpu_register_set_t cortex_m_register_set = { cortex_m_registers, 17u };

/** \brief Cortex-Mx with VFP register set */
static const nano_os_cpu_register_set_t cortex_m_vfp_register_set = { cortex_m_vfp_registers, 17u };



/** \brief Function which retrieve the set of CPU registers for Cortex-M0 */
static const nano_os_cpu_register_set_t* CORTEXM0_CpuRegistersGet(const GDB_API* gdb_api, const char* const port_name, const U32 task_port_data_address)
{
    (void)gdb_api;
    (void)task_port_data_address;
    const nano_os_cpu_register_set_t* ret = &cortex_m0_register_set;
    if (strcmp(port_name, "cortex-m0+") == 0)
    {
        ret = &cortex_m0p_register_set;
    }
    return ret;
}

/** \brief Function which retrieve the set of CPU registers for Cortex-M3 */
static const nano_os_cpu_register_set_t* CORTEXM3_CpuRegistersGet(const GDB_API* gdb_api, const char* const port_name, const U32 task_port_data_address)
{
    (void)gdb_api;
    (void)port_name;
    (void)task_port_data_address;
    return &cortex_m_register_set;
}

/** \brief Function which retrieve the set of CPU registers for Cortex-Mx with VFP */
static const nano_os_cpu_register_set_t* CORTEXMxVFP_CpuRegistersGet(const GDB_API* gdb_api, const char* const port_name, const U32 task_port_data_address)
{
    U8 use_float;
    (void)port_name;
    const nano_os_cpu_register_set_t* ret = NULL;

    /* Read the floating point usage flag */
    const int err = gdb_api->pfReadU8(task_port_data_address, &use_float);
    if (err == 0u)
    {
        if (use_float == 0u)
        {
            ret = &cortex_m_register_set;
        }
        else
        {
            ret = &cortex_m_vfp_register_set;
        }
    }

    return ret;
}




/** \brief Supported Cortex-M cores */
const nano_os_cpu_port_t g_cortex_m_cores[] = {
                                                { JLINK_CORE_CORTEX_M0, "cortex-m0", DESCENDING_STACK, CORTEXM0_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M1, "cortex-m1", DESCENDING_STACK, CORTEXM0_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M3, "cortex-m3", DESCENDING_STACK, CORTEXM3_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M3_R1P0, "cortex-m3", DESCENDING_STACK, CORTEXM3_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M3_R1P1, "cortex-m3", DESCENDING_STACK, CORTEXM3_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M3_R2P0, "cortex-m3", DESCENDING_STACK, CORTEXM3_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M4, "cortex-m4", DESCENDING_STACK, CORTEXMxVFP_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M7, "cortex-m7", DESCENDING_STACK, CORTEXMxVFP_CpuRegistersGet },
                                                { JLINK_CORE_CORTEX_M_V8MAINL, "cortex-m_v8", DESCENDING_STACK, CORTEXMxVFP_CpuRegistersGet },
                                                {0u, NULL, DESCENDING_STACK, NULL }
                                              };


