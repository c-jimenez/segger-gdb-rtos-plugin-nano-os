/*********************************************************************
*                SEGGER MICROCONTROLLER SYSTEME GmbH                 *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (C) 2004-2009    SEGGER Microcontroller Systeme GmbH        *
*                                                                    *
*      Internet: www.segger.com    Support:  support@segger.com      *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : RTOSPlugin.c
Purpose     : Extracts information about tasks from RTOS.

Additional information:
  Eclipse based debuggers show information about threads.

---------------------------END-OF-HEADER------------------------------
*/

#include "RTOSPlugin.h"
#include "JLINKARM_Const.h"
#include <stdio.h>
#include <stdbool.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/** \brief DLL export */
#ifdef WIN32
  #define EXPORT __declspec(dllexport)
#else
  #define EXPORT __attribute__((visibility("default")))
#endif


/** \brief Invalid 8 bits data structure offset */
#define NANO_OS_PLUGIN_INVALID_OFFSET8  0xFFu

/** \brief Invalid 16 bits data structure offset */
#define NANO_OS_PLUGIN_INVALID_OFFSET16 0xFFFFu



/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/** \brief The plugin version number as unsigned integer: 100 * [major] + [minor] */
#define NANO_OS_PLUGIN_VERSION                  100u

/** \brief Maximum number of threads */
#define NANO_OS_PLUGIN_MAX_THREAD_COUNT         1024u


/** \brief Enable debug prints */
#define NANO_OS_PLUGIN_DEBUG_PRINT_ENABLED      1

/** \brief Enable error prints */
#define NANO_OS_PLUGIN_ERROR_PRINT_ENABLED      1


#if (NANO_OS_PLUGIN_DEBUG_PRINT_ENABLED == 1)
/** \brief Macro to print a debug string */
#define LOG_DEBUG(string, ...)                  gdb_api->pfDebugOutf((string), ##__VA_ARGS__)
#else
#define LOG_DEBUG(string, ...)
#endif /* (NANO_OS_PLUGIN_DEBUG_PRINT_ENABLED == 1) */

#if (NANO_OS_PLUGIN_ERROR_PRINT_ENABLED == 1)
/** \brief Macro to print an error string */
#define LOG_ERROR(string, ...)                  gdb_api->pfErrorOutf((string), ##__VA_ARGS__)
#else
#define LOG_ERROR(string, ...)
#endif /* (NANO_OS_PLUGIN_ERROR_PRINT_ENABLED == 1) */

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/


/** \brief Nano OS debug informations for Segger GDB RTOS plugin */
typedef struct _nano_os_data_structure_offsets_t
{
    /** \brief Offset of pointer to current task in nano_os_t structure */
    U16 current_task_offset;
    /** \brief Offset of tick count in nano_os_t structure */
    U16 tick_count_offset;
    /** \brief Offset of global task list in nano_os_t structure */
    U16 task_list_offset;

    /** \brief Offset of top of stack in nano_os_task_t structure */
    U8 top_of_stack_offset;
    /** \brief Offset of stack origin in nano_os_task_t structure */
    U8 stack_origin_offset;
    /** \brief Offset of stack size in nano_os_task_t structure */
    U8 stack_size_offset;
    /** \brief Offset of task name in nano_os_task_t structure */
    U8 task_name_offset;
    /** \brief Offset of task state in nano_os_task_t structure */
    U8 task_state_offset;
    /** \brief Offset of task priority in nano_os_task_t structure */
    U8 task_priority_offset;
    /** \brief Offset of task id in nano_os_task_t structure */
    U8 task_id_offset;
    /** \brief Offset of task wait object in nano_os_task_t structure */
    U8 task_wait_object_offset;
    /** \brief Offset of task wait timeout in nano_os_task_t structure */
    U8 task_wait_timeout_offset;
    /** \brief Offset of task time slice in nano_os_task_t structure */
    U8 task_time_slice_offset;
    /** \brief Offset of next task in nano_os_task_t structure */
    U8 next_task_offset;

    /** \brief Offset of wait object type in nano_os_wait_object_t structure */
    U8 wait_object_type_offset;
    /** \brief Offset of wait object id in nano_os_wait_object_t structure */
    U8 wait_object_id_offset;
    /** \brief Offset of wait object name in nano_os_wait_object_t structure */
    U8 wait_object_name_offset;

} nano_os_data_structure_offsets_t;

/** \brief Nano OS wait object data */
typedef struct _nano_os_wait_object_t
{
    /** \brief Id */
    U16 id;
    /** \brief Name */
    char name[255u];
    /** \brief Type */
    U8 type;
} nano_os_wait_object_t;

/** \brief Nano OS thread data */
typedef struct _nano_os_thread_t
{
    /** \brief Id */
    U16 id;
    /** \brief Name */
    char name[255u];
    /** \brief State */
    U8 state;
    /** \brief Priority */
    U8 priority;
    /** \brief Stack size */
    U32 stack_size;
    /** \brief Indicate if the stack has already been loaded */
    bool stack_loaded;
    /* Top of thread stack (contains thread context) */
    U32 stack[60u];
    /** \brief Wait object */
    nano_os_wait_object_t wait_object;
    /** \brief Wait timeout */
    U32 wait_timeout;
    /** \brief Next thread address */
    U32 next_thread;
} nano_os_thread_t;



/** \brief Nano OS plugin internal data */
typedef struct _nano_os_plugin_t
{
    /** \brief Indicate if the OS is tarted */
    bool os_started;

    /** \brief Indicate if the data structure offsets have been loaded */
    bool offsets_loaded;

    /** \brief Nano OS data structure offsets */
    nano_os_data_structure_offsets_t offsets;

    /** \brief Thread count */
    U32 thread_count;
    /** \brief Thread list */
    nano_os_thread_t threads[NANO_OS_PLUGIN_MAX_THREAD_COUNT];
    /** \brief Tick count */
    U32 tick_count;

    /** \brief Current thread */
    nano_os_thread_t* current_thread;
    /** \brief Current thread index */
    U32 current_thread_index;

    /** \brief Thread list address in the target memory */
    U32 target_thread_list_address;
    /** \brief Current thread address in the target memory */
    U32 target_current_thread_address;
} nano_os_plugin_t;

/** \brief Nano OS task states */
typedef enum _nano_os_task_state_t
{
    /** \brief Invalid */
    NOS_TS_INVALID = -1,
    /** \brief Unused task */
    NOS_TS_FREE = 0u,
    /** \brief Ready task */
    NOS_TS_READY = 1u,
    /** \brief Pending task */
    NOS_TS_PENDING = 2u,
    /** \brief Running task */
    NOS_TS_RUNNING = 3u,
    /** \brief Dead task */
    NOS_TS_DEAD = 4u,
    /** \brief State value limit */
    NOS_TS_MAX = 5u
} nano_os_task_state_t;

/** \brief Wait object type */
typedef enum _nano_os_wait_object_type_t
{
    /** \brief Invalid */
    WOT_INVALID = -1,
    /** \brief Not initialized */
    WOT_NOT_INIT = 0u,
    /** \brief Task */
    WOT_TASK = 1u,
    /** \brief Semaphore */
    WOT_SEMAPHORE = 2u,
    /** \brief Mutex */
    WOT_MUTEX = 3u,
    /** \brief Condition variable */
    WOT_COND_VAR = 4u,
    /** \brief Timer */
    WOT_TIMER = 5u,
    /** \brief Mailbox */
    WOT_MAILBOX = 6u,
    /** \brief Flag set */
    WOT_FLAG_SET = 7u,
    /** \brief Type value limit */
    WOT_MAX = 8u
} nano_os_wait_object_type_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/** \brief Pointer to the RTOS symbol table */
static RTOS_SYMBOLS nano_os_symbols[] = {
                                            { "g_nano_os", 0, 0 },
                                            { "g_nano_os_debug_infos", 0, 0 },
                                            { NULL, 0, 0 }
                                        };

/** \brief GDB plugin API */
static const GDB_API* gdb_api = NULL;

/** \brief Nano OS plugin data */
static nano_os_plugin_t nano_os_plugin;


/** \brief Nano OS thread state strings */
static const char* nano_os_thread_states[] = {
                                                "FREE",
                                                "READY",
                                                "PENDING",
                                                "RUNNING",
                                                "DEAD"
                                             };

/** \brief Nano OS wait object type strings */
static const char* nano_os_wait_object_types[] = {
                                                    "NOT_INIT",
                                                    "TASK",
                                                    "SEMAPHORE",
                                                    "MUTEX",
                                                    "COND_VAR",
                                                    "TIMER",
                                                    "MAILBOX",
                                                    "FLAG_SET"
                                                 };


/*********************************************************************
*
*       Static functions declaration
*
**********************************************************************
*/


/** \brief Read a string in target memory */
static bool readString(const U32 string_address, char string[], const U32 string_size);

/** \brief Look for a thread with the given id */
static nano_os_thread_t* findThread(const U32 id);

/** \brief Fill Nano OS offsets */
static bool fillNanoOsOffsets(void);

/** \brief Fill informations about Nano OS */
static bool fillNanoOsInfos(void);

/** \brief Fill a thread information */
static bool fillNanoOsThreadInfos(const U32 thread_address, nano_os_thread_t* const thread);

/** \brief Fill a wait object information */
static bool fillNanoOsWaitObjectInfos(const U32 wait_object_address, nano_os_wait_object_t* const wait_object);


/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/** \brief Initializes RTOS plug-in for further usage */
EXPORT int RTOS_Init(const GDB_API *pAPI, U32 core) 
{
    int ret;

    /* Check selected core */
    gdb_api = pAPI;
    switch (core)
    {
        case JLINK_CORE_CORTEX_M0:
        case JLINK_CORE_CORTEX_M3:
        case JLINK_CORE_CORTEX_M4:
        case JLINK_CORE_CORTEX_M7:
        case JLINK_CORE_CORTEX_A5:
        {
            /* Supported */
            LOG_DEBUG("Initialized for core 0x%x\n", core);
            memset(&nano_os_plugin, 0, sizeof(nano_os_plugin));
            ret = 1;
            break;
        }

        default:
        {
            /* Unsupported */
            LOG_DEBUG("Unsupported core 0x%x\n", core);
            ret = 0;
            break;
        }
    }

    return ret;
}

/** \brief Returns the RTOS plugin version */
EXPORT U32 RTOS_GetVersion() 
{
    return NANO_OS_PLUGIN_VERSION;
}

/** \brief Returns a pointer to the RTOS symbol table */
EXPORT RTOS_SYMBOLS* RTOS_GetSymbols() 
{
    return nano_os_symbols;
}

/** \brief Returns the number of threads */
EXPORT U32 RTOS_GetNumThreads() 
{
    U32 ret = 1;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        if (nano_os_plugin.current_thread != NULL)
        {
            ret = nano_os_plugin.thread_count;
        }
    }

    return ret;
}

/** \brief Returns the ID of the currently running thread */
EXPORT U32 RTOS_GetCurrentThreadId() 
{
    int ret = 0;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        if (nano_os_plugin.current_thread != NULL)
        {
            ret = nano_os_plugin.current_thread->id;
        }
    }

    return ret;
}

/** \brief Returns the ID of the thread with index number n */
EXPORT U32 RTOS_GetThreadId(U32 n) 
{
    int ret = 0;

    /* Check index */
    if (n < nano_os_plugin.thread_count)
    {
        ret = nano_os_plugin.threads[n].id;
    }

    return ret;
}

/** \brief Prints the thread’s name to pDisplay. The name may contain extra information about the
           tread’s status (running/suspended, priority, etc.) */
EXPORT int RTOS_GetThreadDisplay(char *pDisplay, U32 threadid) 
{
    int ret = 0;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        /* Look for the thread */
        nano_os_thread_t* thread = findThread(threadid);
        if (thread != NULL)
        {
            /* Create the thread name */
            if (thread->state == NOS_TS_PENDING)
            {
                char timeout_str[30u];
                const U32 timeout = thread->wait_timeout - nano_os_plugin.tick_count;
                const char* wait_object_type_name = "UNKNOWN";
                if (thread->wait_object.type < WOT_MAX)
                {
                    wait_object_type_name = nano_os_wait_object_types[thread->wait_object.type];
                }
                if (timeout > 0xF0000000u)
                {
                    snprintf(timeout_str, sizeof(timeout_str), "forever");
                }
                else
                {
                    snprintf(timeout_str, sizeof(timeout_str), "%u ticks", timeout);
                }
                if (thread->wait_object.name[0u] != 0u)
                {
                    ret = snprintf(pDisplay, 256u, "%s - %s [%s : %s - %s] - P%03d",
                                   thread->name,
                                   nano_os_thread_states[thread->state],
                                   wait_object_type_name,
                                   thread->wait_object.name,
                                   timeout_str,
                                   thread->priority);
                }
                else
                {
                    ret = snprintf(pDisplay, 256u, "%s - %s [%s : %d - %s] - P%03d",
                                   thread->name,
                                   nano_os_thread_states[thread->state],
                                   wait_object_type_name,
                                   thread->wait_object.id,
                                   timeout_str,
                                   thread->priority);
                }
            }
            else if (thread->state < NOS_TS_MAX)
            {
                ret = snprintf(pDisplay, 256u, "%s - %s - P%03d", 
                                                thread->name, 
                                                nano_os_thread_states[thread->state], 
                                                thread->priority);
            }
            else
            {
                ret = snprintf(pDisplay, 256u, "%s - UNKNOWN - P%03d",
                               thread->name,
                               thread->priority);
            }
        }
        else
        {
            ret = snprintf(pDisplay, 256u, "Unknown thread");
        }
    }
    else
    {
        ret = snprintf(pDisplay, 256u, "CPU startup - Nano OS not started");
    }

    return ret;
}

/** \brief Copys the thread’s register value to pRegValue as HEX string.
           If the register value has to be read directly from the CPU, the function must return a value
           <0. The register value is then read from the CPU by the GDB server itself */
EXPORT int RTOS_GetThreadReg(char *pHexRegVal, U32 RegIndex, U32 threadid) 
{
    int ret = -1;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        /* Look for the thread */
        nano_os_thread_t* thread = findThread(threadid);
        if ((thread != NULL) && (thread != nano_os_plugin.current_thread))
        {
            /* Dump thread stack */
            // TODO
            ret = -1;
        }
    }

    return ret;
}

/** \brief Copys the thread’s general registers to pHexRegList as HEX string.
           If the register values have to be read directly from the CPU, the function must return a
           value <0. The register values are then read from the CPU by the GDB server itself */
EXPORT int RTOS_GetThreadRegList(char *pHexRegList, U32 threadid) 
{
    int ret = -1;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        /* Look for the thread */
        nano_os_thread_t* thread = findThread(threadid);
        if ((thread != NULL) && (thread != nano_os_plugin.current_thread))
        {
            /* Dump thread stack */
            // TODO
            ret = -1;
        }
    }

    return ret;
}

/** \brief Sets the thread’s register to pRegValue, given as HEX string.
           If the register value has to be written directly to the CPU, the function must return a value
           <0. The register value is then written to the CPU by the GDB server itself */
EXPORT int RTOS_SetThreadReg(char* pHexRegVal, U32 RegIndex, U32 threadid) 
{
    int ret = -1;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        /* Look for the thread */
        nano_os_thread_t* thread = findThread(threadid);
        if ((thread != NULL) && (thread != nano_os_plugin.current_thread))
        {
            /* Dump thread stack */
            // TODO
            ret = -1;
        }
    }

    return ret;
}

/** \brief Sets the thread’s registers to pHexRegList, given as HEX string.
           If the register values have to be written directly to the CPU, the function must return a
           value <0. The register values are then written to the CPU by the GDB server itself */
EXPORT int RTOS_SetThreadRegList(char *pHexRegList, U32 threadid) 
{
    int ret = -1;

    /* Check OS state */
    if (nano_os_plugin.os_started)
    {
        /* Look for the thread */
        nano_os_thread_t* thread = findThread(threadid);
        if ((thread != NULL) && (thread != nano_os_plugin.current_thread))
        {
            /* Dump thread stack */
            // TODO
            ret = -1;
        }
    }

    return ret;
}

/** \brief Updates the thread information from the target.
           For efficiency purposes, the plug-in should read all required information within this function
           at once, so later requests can be served without further communication to the target */
EXPORT int RTOS_UpdateThreads() 
{
    int ret = -1;
    bool success;

    // Fill informations about Nano OS
    success = fillNanoOsOffsets();
    if (success && nano_os_plugin.offsets_loaded)
    {
        success = success && fillNanoOsInfos();
        if (success)
        {
            // Go through the OS thread list to refresh thread infos
            U32 thread_address = nano_os_plugin.target_thread_list_address;
            nano_os_plugin.thread_count = 0u;
            nano_os_plugin.current_thread = NULL;
            while (success && (thread_address != 0u))
            {
                // Fill thread infos
                success = fillNanoOsThreadInfos(thread_address, &nano_os_plugin.threads[nano_os_plugin.thread_count]);
                if (success)
                {
                    // Check if this is the current running thread
                    if (thread_address == nano_os_plugin.target_current_thread_address)
                    {
                        nano_os_plugin.current_thread = &nano_os_plugin.threads[nano_os_plugin.thread_count];
                    }

                    // Next thread
                    thread_address = nano_os_plugin.threads[nano_os_plugin.thread_count].next_thread;
                    nano_os_plugin.thread_count++;
                    if (nano_os_plugin.thread_count == NANO_OS_PLUGIN_MAX_THREAD_COUNT)
                    {
                        success = false;
                    }
                }
            }
            if (success)
            {
                ret = 0;
            }
        }
    }

    return ret;
}




/*********************************************************************
*
*       Static functions implementation
*
**********************************************************************
*/

/** \brief Read a string in target memory */
static bool readString(const U32 string_address, char string[], const U32 string_size)
{
    int err;
    bool ret = true;

    /* Read the string content address */
    U32 string_content_address = 0u;
    err = gdb_api->pfReadU32(string_address, &string_content_address);
    ret = ret && (err == 0);
    if (ret)
    {
        /* Read the whole string */
        err = gdb_api->pfReadMem(string_content_address, string, string_size);
        ret = ret && (err != 0);
    }

    /* Terminate string */
    string[string_size - 1u] = 0;

    return ret;
}


/** \brief Look for a thread with the given id */
static nano_os_thread_t* findThread(const U32 id)
{
    U32 index;
    nano_os_thread_t* thread = NULL;

    for (index = 0u; (index < nano_os_plugin.thread_count) && (thread == NULL); index++)
    {
        if (nano_os_plugin.threads[index].id == id)
        {
            thread = &nano_os_plugin.threads[index];
        }
    }

    return thread;
}

/** \brief Macro to read 8 bits data structure offsets */
#define READ_DATA_STRUCTURE_OFFSET8(value)  err = gdb_api->pfReadU8(nano_os_symbols[1u].address + offset, &nano_os_plugin.offsets.value); \
                                            ret = ret && (err == 0); \
                                            offset += 1u;

/** \brief Macro to read 16 bits data structure offsets */
#define READ_DATA_STRUCTURE_OFFSET16(value)     err = gdb_api->pfReadU16(nano_os_symbols[1u].address + offset, &nano_os_plugin.offsets.value); \
                                                ret = ret && (err == 0); \
                                                offset += 2u;


/** \brief Fill Nano OS offsets */
static bool fillNanoOsOffsets(void)
{
    bool ret = true;

    /* CHeck if the offsets have already been loaded */
    if (!nano_os_plugin.offsets_loaded)
    {
        int err;
        U16 offset = 0;

        /* Offsets in nano_os_t structure */
        READ_DATA_STRUCTURE_OFFSET16(current_task_offset);
        READ_DATA_STRUCTURE_OFFSET16(tick_count_offset);
        READ_DATA_STRUCTURE_OFFSET16(task_list_offset);

        /* Offsets in nano_os_task_t structure */
        READ_DATA_STRUCTURE_OFFSET8(top_of_stack_offset);
        READ_DATA_STRUCTURE_OFFSET8(stack_origin_offset);
        READ_DATA_STRUCTURE_OFFSET8(stack_size_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_name_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_state_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_priority_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_id_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_wait_object_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_wait_timeout_offset);
        READ_DATA_STRUCTURE_OFFSET8(task_time_slice_offset);
        READ_DATA_STRUCTURE_OFFSET8(next_task_offset);

        /* Offsets in nano_os_wait_object_type_t structure */
        READ_DATA_STRUCTURE_OFFSET8(wait_object_type_offset);
        READ_DATA_STRUCTURE_OFFSET8(wait_object_id_offset);
        READ_DATA_STRUCTURE_OFFSET8(wait_object_name_offset);

        /* Check if data has been successfully loaded */
        if (ret && !((nano_os_plugin.offsets.current_task_offset == 0u) && (nano_os_plugin.offsets.tick_count_offset == 0u)))
        {
            nano_os_plugin.offsets_loaded = true;
        }
    }

    return ret;
}


/** \brief Fill informations about Nano OS */
static bool fillNanoOsInfos(void)
{
    int err;
    bool ret = true;

    /* Read the current thread address */
    err = gdb_api->pfReadU32(nano_os_symbols[0u].address + nano_os_plugin.offsets.current_task_offset, &nano_os_plugin.target_current_thread_address);
    ret = ret && (err == 0);
    if (ret && (nano_os_plugin.target_current_thread_address != 0u))
    {
        nano_os_plugin.os_started = true;
    }
    
    /* Read the thread list address */
    err = gdb_api->pfReadU32(nano_os_symbols[0u].address + nano_os_plugin.offsets.task_list_offset, &nano_os_plugin.target_thread_list_address);
    ret = ret && (err == 0);

    /* Read the tick count */
    err = gdb_api->pfReadU32(nano_os_symbols[0u].address + nano_os_plugin.offsets.tick_count_offset, &nano_os_plugin.tick_count);
    ret = ret && (err == 0);

    return ret;
}


/** \brief Fill a thread information */
static bool fillNanoOsThreadInfos(const U32 thread_address, nano_os_thread_t* const thread)
{
    int err;
    bool ret = true;

    /* Read the thread id */
    err = gdb_api->pfReadU16(thread_address + nano_os_plugin.offsets.task_id_offset, &thread->id);
    ret = ret && (err == 0);

    /* Read the thread name */
    if (nano_os_plugin.offsets.task_name_offset == NANO_OS_PLUGIN_INVALID_OFFSET8)
    {
        strcpy(thread->name, "Unknown task");
    }
    else
    {
        ret = readString(thread_address + nano_os_plugin.offsets.task_name_offset, thread->name, sizeof(thread->name));
    }

    /* Read the thread state */
    err = gdb_api->pfReadU8(thread_address + nano_os_plugin.offsets.task_state_offset, &thread->state);
    ret = ret && (err == 0);

    /* Read the thread priority */
    err = gdb_api->pfReadU8(thread_address + nano_os_plugin.offsets.task_priority_offset, &thread->priority);
    ret = ret && (err == 0);

    /* Read the stack size */
    err = gdb_api->pfReadU32(thread_address + nano_os_plugin.offsets.stack_size_offset, &thread->stack_size);
    ret = ret && (err == 0);

    /* Delay stack load */
    thread->stack_loaded = false;

    /* Read the wait object */
    U32 wait_object_address = 0u;
    err = gdb_api->pfReadU32(thread_address + nano_os_plugin.offsets.task_wait_object_offset, &wait_object_address);
    ret = ret && (err == 0);
    if (wait_object_address != 0u)
    {
        ret = fillNanoOsWaitObjectInfos(wait_object_address, &thread->wait_object);
    }
    else
    {
        memset(&thread->wait_object, 0, sizeof(nano_os_wait_object_t));
    }

    /* Read the wait timeout */
    err = gdb_api->pfReadU32(thread_address + nano_os_plugin.offsets.task_wait_timeout_offset, &thread->wait_timeout);
    ret = ret && (err == 0);

    /* Read the next task address */
    err = gdb_api->pfReadU32(thread_address + nano_os_plugin.offsets.next_task_offset, &thread->next_thread);
    ret = ret && (err == 0);
    
    return ret;
}

/** \brief Fill a wait object information */
static bool fillNanoOsWaitObjectInfos(const U32 wait_object_address, nano_os_wait_object_t* const wait_object)
{
    int err;
    bool ret = true;

    /* Read the id */
    err = gdb_api->pfReadU16(wait_object_address + nano_os_plugin.offsets.wait_object_id_offset, &wait_object->id);
    ret = ret && (err == 0);

    /* Read the name */
    if (nano_os_plugin.offsets.wait_object_name_offset == NANO_OS_PLUGIN_INVALID_OFFSET8)
    {
        strcpy(wait_object->name, "");
    }
    else
    {
        ret = readString(wait_object_address + nano_os_plugin.offsets.wait_object_name_offset, wait_object->name, sizeof(wait_object->name));
    }

    /* Read the type */
    err = gdb_api->pfReadU8(wait_object_address + nano_os_plugin.offsets.wait_object_type_offset, &wait_object->type);
    ret = ret && (err == 0);

    return ret;
}

