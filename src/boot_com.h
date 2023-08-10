// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot_com.h
*@brief     Bootloader Communication
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      10.08.2023
*@version   V0.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BOOT_COM_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __BOOT_COM_H
#define __BOOT_COM_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../boot_cfg.h"
#include "boot_types.h"


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
// Functions
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_init(void);

#endif // __BOOT_COM_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
