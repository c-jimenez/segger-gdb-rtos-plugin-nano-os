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

#ifdef WIN32
  #define EXPORT __declspec(dllexport)
#else
  #define EXPORT __attribute__((visibility("default")))
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/** \brief Enable debug prints */
#define NANO_OS_PLUGIN_DEBUG_PRINT_ENABLED      1

/** \brief Enable error prints */
#define NANO_OS_PLUGIN_ERROR_PRINT_ENABLED      1


/** \brief The plugin version number as unsigned integer: 100 * [major] + [minor] */
#define NANO_OS_PLUGIN_VERSION                  100u



#if (NANO_OS_PLUGIN_DEBUG_PRINT_ENABLED == 1)
/** \brief Macro to print a debug string */
#define LOG_DEBUG(string, ...)                  gdb_api->pfDebugOutf((string), __VA_ARGS__)
#else
#define LOG_DEBUG(string, ...)
#endif /* (NANO_OS_PLUGIN_DEBUG_PRINT_ENABLED == 1) */

#if (NANO_OS_PLUGIN_ERROR_PRINT_ENABLED == 1)
/** \brief Macro to print an error string */
#define LOG_ERROR(string, ...)                  gdb_api->pfErrorOutf((string), __VA_ARGS__)
#else
#define LOG_ERROR(string, ...)
#endif /* (NANO_OS_PLUGIN_ERROR_PRINT_ENABLED == 1) */

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/


/** \brief Nano OS thread data */
typedef struct _nano_os_thread_t
{
    /** \brief Id */
    U32 id;
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
    /** \brief Wait object id */
    U16 wait_object_id;
    /** \brief Wait timeout */
    U32 wait_timeout;
} nano_os_thread_t;


/** \brief Nano OS plugin internal data */
typedef struct _nano_os_plugin_t
{
    /** \brief Indicate if the OS is tarted */
    bool os_started;

    /** \brief Thread count */
    U32 thread_count;
    /** \brief Thread list */
    nano_os_thread_t* threads;

    /** \brief Current thread */
    nano_os_thread_t* current_thread;
    /** \brief Current thread index */
    U32 current_thread_index;



    /** \brief Thread list address in the target memory */
    U32 target_thread_list_address;
    /** \brief Current thread address in the target memory */
    U32 target_current_thread_address;
} nano_os_plugin_t;


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/** \brief Pointer to the RTOS symbol table */
static RTOS_SYMBOLS nano_os_symbols[] = {
                                            { "g_nano_os", 0, 0 },
                                            { NULL, 0, 0 }
                                        };

/** \brief GDB plugin API */
static GDB_API* gdb_api = NULL;

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


/*********************************************************************
*
*       Static functions declaration
*
**********************************************************************
*/

/** \brief Print the symbol table */
static void printSymbolTable(void);

/** \brief Look for a thread with the given id */
static nano_os_thread_t* findThread(const U32 id);

/** \brief Fill informations about Nano OS */
static bool fillNanoOsInfos(void);


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
            LOG_DEBUG("Initialized for core %d\n", core);
            memset(&nano_os_plugin, 0, sizeof(nano_os_plugin));
            ret = 1;
            break;
        }

        default:
        {
            /* Unsupported */
            LOG_DEBUG("Unsupported core %d\n", core);
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
    return nano_os_plugin.thread_count;
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
            ret = snprintf(pDisplay, 256u, "%s - %s - P%3d", 
                                            thread->name, 
                                            nano_os_thread_states[thread->state], 
                                            thread->priority);
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
    success = fillNanoOsInfos();
    if (success)
    {

        ret = 0;
    }

    return ret;
}




/*********************************************************************
*
*       Static functions implementation
*
**********************************************************************
*/


/** \brief Print the symbol table */
static void printSymbolTable(void)
{
    RTOS_SYMBOLS* current_symbol = nano_os_symbols;

    LOG_DEBUG("Symbols table :\n");
    while (current_symbol->name != NULL)
    {
        LOG_DEBUG(" - %s : 0x%8x\n", current_symbol->name, current_symbol->address);
        current_symbol++;
    }
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

/** \brief Fill informations about Nano OS */
static bool fillNanoOsInfos(void)
{
    int err;
    bool ret = true;

    /* Read the current thread address */
    err = gdb_api->pfReadU32(nano_os_symbols[0u].address, &nano_os_plugin.target_current_thread_address);
    ret = ret && (err == 0);
    
    /* Read the thread list address */
    err = gdb_api->pfReadU32(nano_os_symbols[0u].address + 8u, &nano_os_plugin.target_thread_list_address);
    ret = ret && (err == 0);

    /* Read Nano OS started flag */
    err = gdb_api->pfReadU8(nano_os_symbols[0u].address + 12u, &nano_os_plugin.os_started);
    ret = ret && (err == 0);

    return ret;
}

