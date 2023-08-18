// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot.c
*@brief     Bootloader
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      06.08.2023
*@version   V0.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup Bootloader
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot.h"
#include "boot_com.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#if ( 1 == BOOT_CFG_DEBUG_EN )

    /**
     *  Bootloader message status description
     */
    typedef struct
    {
        const char *        desc;       /**<Status description string */
        boot_msg_status_t   status;     /**<Status enumeration */
    } boot_msg_status_desc_t;

#endif // ( 1 == BOOT_CFG_DEBUG_EN )


////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

#if ( 1 == BOOT_CFG_DEBUG_EN )

    /**
     *  Message status descriptions
     */
    static const boot_msg_status_desc_t g_msg_status_desc[] =
    {
        { .status = eBOOT_MSG_OK,                   .desc = "OK"                         	},
        { .status = eBOOT_MSG_ERROR_VALIDATION,     .desc = "ERROR - VALIDATION"         	},
        { .status = eBOOT_MSG_ERROR_INVALID_REQ,    .desc = "ERROR - INVALID REQUEST"    	},
        { .status = eBOOT_MSG_ERROR_FLASH_WRITE,    .desc = "ERROR - WRITE TO FLASH"     	},
        { .status = eBOOT_MSG_ERROR_FLASH_ERASE,    .desc = "ERROR - FLASH PREPARE (ERASE)"  },
        { .status = eBOOT_MSG_ERROR_FW_SIZE,        .desc = "ERROR - FW SIZE"                },
        { .status = eBOOT_MSG_ERROR_FW_VER,         .desc = "ERROR - FW VERSION"             },
    };

    /**
     *  Number of all message status descriptions
     */
    static const uint16_t gu16_msg_status_des_num_of = (uint16_t) ( sizeof( g_msg_status_desc ) / sizeof( boot_msg_status_desc_t ));

#endif // ( 1 == BOOT_CFG_DEBUG_EN )


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


#if ( 1 == BOOT_CFG_DEBUG_EN )
    static const char * boot_get_msg_status_str(const boot_msg_status_t msg_status);
#endif

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
/**
*       Connect Bootloader Message Reception Callback
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_connect_msg_rcv_cb(void)
{


    BOOT_DBG_PRINT( "Connect Bootloader Message Reception Callback..." );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Connect Response Bootloader Message Reception Callback
*
* @param[in]    msg_status - Status of connect command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_connect_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{



    BOOT_DBG_PRINT( "Connect Response Bootloader Message Reception Callback. Status: %s", boot_get_msg_status_str( msg_status ));
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Prepare Bootloader Message Reception Callback
*
* @param[in]    fw_size     - Firmware size in bytes
* @param[in]    fw_ver      - Firmware version
* @param[in]    hw_ver      - Hardware version
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_prepare_msg_rcv_cb(const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver)
{


    BOOT_DBG_PRINT( "Prepare Bootloader Message Reception Callback..." );
    BOOT_DBG_PRINT( "    - fw_size: %d bytes", fw_size );
    BOOT_DBG_PRINT( "    - fw_ver:  0x%08X", fw_ver );
    BOOT_DBG_PRINT( "    - hw_ver:  0x%08X", hw_ver );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Prepare Response Bootloader Message Reception Callback
*
* @param[in]    msg_status - Status of prepare command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_prepare_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{


    BOOT_DBG_PRINT( "Prepare Response Bootloader Message Reception Callback. Status: %s", boot_get_msg_status_str( msg_status ));
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Flash Bootloader Message Reception Callback
*
* @param[in]    p_data  - Flash binary data
* @param[in]    size    - Size of flash data in bytes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_flash_msg_rcv_cb(const uint8_t * const p_data, const uint16_t size)
{

    // Unused param
    (void) p_data;

    BOOT_DBG_PRINT( "Flash Bootloader Message Reception Callback..." );
    BOOT_DBG_PRINT( "    - size: %d bytes", size );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Flash Response Bootloader Message Reception Callback
*
* @param[in]    msg_status - Status of flash command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_flash_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{


    BOOT_DBG_PRINT( "Flash Response Bootloader Message Reception Callback. Status: %s", boot_get_msg_status_str( msg_status ));
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Exit Bootloader Message Reception Callback
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_exit_msg_rcv_cb(void)
{


    BOOT_DBG_PRINT( "Exit Bootloader Message Reception Callback..." );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Exit Response Bootloader Message Reception Callback
*
* @param[in]    msg_status - Status of exit command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_exit_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{


    BOOT_DBG_PRINT( "Exit Response Bootloader Message Reception Callback. Status: %s", boot_get_msg_status_str( msg_status ));
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Info Bootloader Message Reception Callback
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_info_msg_rcv_cb(void)
{


    BOOT_DBG_PRINT( "Info Bootloader Message Reception Callback..." );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Info Response Bootloader Message Reception Callback
*
* @param[in]    msg_status  - Bootloader version
* @param[in]    status      - Status of info command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_info_rsp_msg_rcv_cb(const uint32_t boot_ver, const boot_msg_status_t msg_status)
{


    BOOT_DBG_PRINT( "Info Response Bootloader Message Reception Callback" );
    BOOT_DBG_PRINT( "    - boot_ver: 0x%08X", boot_ver );
    BOOT_DBG_PRINT( "    - Status:   %s", boot_get_msg_status_str( msg_status ));
}

#if ( 1 == BOOT_CFG_DEBUG_EN )

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *       Get command message status string
    *
    * @param[in]    cmd_type    - SCP command type
    * @return       str         - SCP command type description
    */
    ////////////////////////////////////////////////////////////////////////////////
    static const char * boot_get_msg_status_str(const boot_msg_status_t msg_status)
    {
        uint16_t i = 0;
        const char * str = "N/A";

        for ( i = 0; i < gu16_msg_status_des_num_of; i++ )
        {
            if ( msg_status == g_msg_status_desc[i].status )
            {
                str =  (const char*) g_msg_status_desc[i].desc;
                break;
            }
        }

        return str;
    }

#endif // ( 1 == BOOT_CFG_DEBUG_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BOOT_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of Bootloader API.
*/
////////////////////////////////////////////////////////////////////////////////


boot_status_t boot_init(void)
{
    boot_status_t status = eBOOT_OK;

    // Initialize communication
    status |= boot_com_init();

    return status;
}


boot_status_t boot_hndl(void)
{
    boot_status_t status = eBOOT_OK;

    // Handle bootlaoder communication
    status |= boot_com_hndl();



    /// TESTING::::

    //boot_com_send_connect();
    //boot_com_send_flash((uint8_t*) "Hello", 5U );


    return status;
}


boot_state_t boot_get_state(void)
{
    boot_state_t state = eBOOT_STATE_IDLE;

    // TODO: ...

    return state;
}


////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
