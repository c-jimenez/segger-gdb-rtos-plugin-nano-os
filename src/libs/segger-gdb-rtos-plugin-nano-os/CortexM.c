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

/** \brief Cortex-M3 register description */
static const nano_os_cpu_reg_t cortex_m3_registers[] = {
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

                                                            { 32u, "FPSR", 0x00, 4u },
                                                            { 33u, "S0", 0x00, 4u },
                                                            { 34u, "S1", 0x00, 4u },
                                                            { 35u, "S2", 0x00, 4u },
                                                            { 36u, "S3", 0x00, 4u },
                                                            { 37u, "S4", 0x00, 4u },
                                                            { 38u, "S5", 0x00, 4u },
                                                            { 39u, "S6", 0x00, 4u },
                                                            { 40u, "S7", 0x00, 4u },
                                                            { 41u, "S8", 0x00, 4u },
                                                            { 42u, "S9", 0x00, 4u },
                                                            { 43u, "S10", 0x00, 4u },
                                                            { 44u, "S11", 0x00, 4u },
                                                            { 45u, "S12", 0x00, 4u },
                                                            { 46u, "S13", 0x00, 4u },
                                                            { 47u, "S14", 0x00, 4u },
                                                            { 48u, "S15", 0x00, 4u },
                                                            { 49u, "S16", 0x00, 4u },
                                                            { 50u, "S17", 0x00, 4u },
                                                            { 51u, "S18", 0x00, 4u },
                                                            { 52u, "S19", 0x00, 4u },
                                                            { 53u, "S20", 0x00, 4u },
                                                            { 54u, "S21", 0x00, 4u },
                                                            { 55u, "S22", 0x00, 4u },
                                                            { 56u, "S23", 0x00, 4u },
                                                            { 57u, "S24", 0x00, 4u },
                                                            { 58u, "S25", 0x00, 4u },
                                                            { 59u, "S26", 0x00, 4u },
                                                            { 60u, "S27", 0x00, 4u },
                                                            { 61u, "S28", 0x00, 4u },
                                                            { 62u, "S29", 0x00, 4u },
                                                            { 63u, "S30", 0x00, 4u },
                                                            { 64u, "S31", 0x00, 4u },

                                                            { 0u, NULL, 0x00, 0u }
                                                          };


/** \brief Supported Cortex-M cores */
const nano_os_cpu_port_t g_cortex_m_cores[] = {
                                                { JLINK_CORE_CORTEX_M0, "cortex-m0", DESCENDING_STACK, cortex_m0_registers, 17u },
                                                { JLINK_CORE_CORTEX_M1, "cortex-m1", DESCENDING_STACK, cortex_m0_registers, 17u },
                                                { JLINK_CORE_CORTEX_M3, "cortex-m3", DESCENDING_STACK, cortex_m3_registers, 17u },
                                                { JLINK_CORE_CORTEX_M3_R1P0, "cortex-m3", DESCENDING_STACK, cortex_m3_registers, 17u },
                                                { JLINK_CORE_CORTEX_M3_R1P1, "cortex-m3", DESCENDING_STACK, cortex_m3_registers, 17u },
                                                { JLINK_CORE_CORTEX_M3_R2P0, "cortex-m3", DESCENDING_STACK, cortex_m3_registers, 17u },
                                                { JLINK_CORE_CORTEX_M4, "cortex-m4", DESCENDING_STACK, cortex_m_vfp_registers, 17u },
                                                { JLINK_CORE_CORTEX_M7, "cortex-m7", DESCENDING_STACK, cortex_m_vfp_registers, 17u },
                                                { JLINK_CORE_CORTEX_M_V8MAINL, "cortex-m_v8", DESCENDING_STACK, cortex_m_vfp_registers, 17u },
                                                {0u, NULL, DESCENDING_STACK, NULL, 0u }
                                              };




