// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot_com.c
*@brief     Bootloader Communication
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      10.08.2023
*@version   V0.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup Bootloader Communication
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "boot_com.h"
#include "../../boot_cfg.h"


////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  Booloader communication commands
 */
typedef enum
{
    eBOOT_CMD_CONNECT       = (uint8_t)( 0x10U ),       /**<Connect command */
    eBOOT_CMD_CONNECT_RSP   = (uint8_t)( 0x11U ),       /**<Connect response command*/
    eBOOT_CMD_PREPARE       = (uint8_t)( 0x20U ),       /**<Prepare command */
    eBOOT_CMD_PREPARE_RSP   = (uint8_t)( 0x21U ),       /**<Prepare response command*/
    eBOOT_CMD_FLASH         = (uint8_t)( 0x30U ),       /**<Flash data command */
    eBOOT_CMD_FLASH_RSP     = (uint8_t)( 0x31U ),       /**<Flash data response command*/
    eBOOT_CMD_EXIT          = (uint8_t)( 0x40U ),       /**<Exit command */
    eBOOT_CMD_EXIT_RSP      = (uint8_t)( 0x41U ),       /**<Exit response command*/
    eBOOT_CMD_INFO          = (uint8_t)( 0xA0U ),       /**<Information command */
    eBOOT_CMD_INFO_RSP      = (uint8_t)( 0xA1U ),       /**<Information response command*/
} boot_cmd_opt_t;

/**
 *  Bootloader message source
 */
typedef enum
{
    eCOM_MSG_SRC_BOOT_MANAGER   = (uint8_t)( 0x2BU ),     /**<Message from Boot Manager (pc app) */
    eCOM_MSG_SRC_BOOTLOADER     = (uint8_t)( 0xB2U ),     /**<Message from Bootloader (embedded) */
} boot_msg_src_t;

/**
 *  Bootloader header fields
 */
typedef struct
{
    uint16_t    preamble;   /**<Message preamble */
    uint16_t    length;     /**<Message length */
    uint8_t     source;     /**<Message source */
    uint8_t     command;    /**<Message command */
    uint8_t     status;     /**<Message status */
    uint8_t     crc;        /**<Message CRC8 */
} boot_header_field_t;

/**
 *  Bootloader header
 */
typedef union
{
    boot_header_field_t     field;      /**<Field access */
    uint8_t                 U;          /**<Unsigned access */
} boot_header_t;

/**
 *  Compiler compatibility check
 */
BOOT_CFG_STATIC_ASSERT( sizeof(boot_header_t) == 8U );


////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static uint8_t boot_com_calc_crc(const uint8_t * const p_data, const uint16_t size);



////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*       Calculate CRC-8
*
* @param[in]    p_data  - Pointer to data
* @param[in]    size    - Size of data to calc crc
* @return       crc8    - Calculated CRC
*/
////////////////////////////////////////////////////////////////////////////////
static uint8_t boot_com_calc_crc(const uint8_t * const p_data, const uint16_t size)
{
    const   uint8_t poly    = 0x07U;    // CRC-8-CCITT
    const   uint8_t seed    = 0xB6U;    // Custom seed
            uint8_t crc8    = seed;

    // Check input
    BOOT_ASSERT( NULL != p_data );
    BOOT_ASSERT( size > 0 );

    for (uint16_t i = 0; i < size; i++)
    {
        crc8 = ( crc8 ^ p_data[i] );

        for (uint16_t j = 0U; j < 8U; j++)
        {
            if ( crc8 & 0x80U )
            {
                crc8 = (( crc8 << 1U ) ^ poly );
            }
            else
            {
                crc8 = ( crc8 << 1U );
            }
        }
    }

    return crc8;
}




////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BOOT_COM_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of Bootloader Communication API.
*/
////////////////////////////////////////////////////////////////////////////////


boot_status_t boot_com_init(void)
{
    boot_status_t status = eBOOT_OK;


    boot_com_calc_crc( 0, 0 );


    return status;
}

boot_status_t boot_com_hndl(void)
{
    boot_status_t status = eBOOT_OK;


    return status;
}



boot_status_t boot_com_send_connect(void)
{
    boot_status_t status = eBOOT_OK;


    return status;
}


boot_status_t boot_com_send_connect_rsp(const boot_status_t stat)
{
    boot_status_t status = eBOOT_OK;

    // Unused params
    (void) stat;

    return status;
}


boot_status_t boot_com_send_prepare(const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver)
{
    boot_status_t status = eBOOT_OK;

    // Unused params
    (void) fw_size;
    (void) fw_ver;
    (void) hw_ver;

    return status;
}


boot_status_t boot_com_send_prepare_rsp(const boot_status_t stat)
{
    boot_status_t status = eBOOT_OK;


    // Unused params
    (void) stat;


    return status;
}


boot_status_t boot_com_send_flash(const uint8_t * const p_data, const uint16_t size)
{
    boot_status_t status = eBOOT_OK;

    // Unused params
    (void) p_data;
    (void) size;

    return status;
}


boot_status_t boot_com_send_flash_rsp(const boot_status_t stat)
{
    boot_status_t status = eBOOT_OK;

    // Unused params
    (void) stat;


    return status;
}


boot_status_t boot_com_send_exit(void)
{
    boot_status_t status = eBOOT_OK;


    return status;
}


boot_status_t boot_com_send_exit_rsp(const boot_status_t stat)
{
    boot_status_t status = eBOOT_OK;

    // Unused params
    (void) stat;

    return status;
}


boot_status_t boot_com_send_info(void)
{
    boot_status_t status = eBOOT_OK;


    return status;
}


boot_status_t boot_com_send_info_rsp(const uint32_t boot_ver, const boot_status_t stat)
{
    boot_status_t status = eBOOT_OK;

    // Unused params
    (void) boot_ver;
    (void) stat;

    return status;
}


////////////////////////////////////////////////////////////////////////////////
/**
*       Connect Bootloader Message Reception Callback
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_connect_msg_rcv_cb(void)
{
    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Connect Response Bootloader Message Reception Callback
*
* @param[in]    status - Status of connect command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_connect_msg_cmd_rcv_cb(const boot_status_t status)
{
    // Unused params
    (void) status;

    /**
     *  Leave empty for user application purposes...
     */
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
__BOOT_CFG_WEAK__ void boot_com_prepare_msg_rcv_cb(const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver)
{
    // Unused params
    (void) fw_size;
    (void) fw_ver;
    (void) hw_ver;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Prepare Response Bootloader Message Reception Callback
*
* @param[in]    status - Status of prepare command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_prepare_rsp_msg_rcv_cb(const boot_status_t status)
{
    // Unused params
    (void) status;

    /**
     *  Leave empty for user application purposes...
     */
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
__BOOT_CFG_WEAK__ void boot_com_flash_msg_rcv_cb(const uint8_t * const p_data, const uint16_t size)
{
    // Unused params
    (void) p_data;
    (void) size;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Flash Response Bootloader Message Reception Callback
*
* @param[in]    status - Status of flash command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_flash_rsp_msg_rcv_cb(const boot_status_t status)
{
    // Unused params
    (void) status;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Exit Bootloader Message Reception Callback
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_exit_msg_rcv_cb(void)
{
    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Exit Response Bootloader Message Reception Callback
*
* @param[in]    status - Status of exit command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_exit_rsp_msg_rcv_cb(const boot_status_t status)
{
    // Unused params
    (void) status;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Info Bootloader Message Reception Callback
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_info_msg_rcv_cb(void)
{
    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Info Response Bootloader Message Reception Callback
*
* @param[in]    boot_ver    - Bootloader version
* @param[in]    status      - Status of info command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_info_rsp_msg_rcv_cb(const uint32_t boot_ver, const boot_status_t status)
{
    // Unused params
    (void) boot_ver;
    (void) status;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
