// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot_com.h
*@brief     Bootloader Communication
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      28.09.2024
*@version   V1.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BOOT_COM_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __BOOT_TYPES_H
#define __BOOT_TYPES_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  Bootloader status
 */
typedef enum
{
    eBOOT_OK                = (uint8_t)( 0x00U ),   /**<Normal operation */

    eBOOT_ERROR             = (uint8_t)( 0x01U ),   /**<General error code */
    eBOOT_ERROR_TIMEOUT     = (uint8_t)( 0x02U ),   /**<Frame reception timeout error */
    eBOOT_ERROR_CRC         = (uint8_t)( 0x04U ),   /**<Frame integrity corrupted error */

    eBOOT_WAR_EMPTY         = (uint8_t)( 0x20U ),   /**<Reception queue empty */
    eBOOT_WAR_FULL          = (uint8_t)( 0x40U ),   /**<Reception queue full */
} boot_status_t;

/**
 *  Bootloader FSM state
 */
typedef enum
{
    eBOOT_STATE_IDLE = 0,   /**<Idle state - waiting for request */
    eBOOT_STATE_PREPARE,    /**<Preparing FLASH memory */
    eBOOT_STATE_FLASH,      /**<Flashing memory with new software */
    eBOOT_STATE_EXIT,       /**<Exit bootloader - Enter application */

    eBOOT_STATE_NUM_OF
} boot_state_t;

/**
 *  Message (command) response
 */
typedef enum
{
    eBOOT_MSG_OK                    = (uint8_t) ( 0x00U ),    /**<Normal operation */
    eBOOT_MSG_ERROR_VALIDATION      = (uint8_t) ( 0x01U ),    /**<Validation error */
    eBOOT_MSG_ERROR_INVALID_REQ     = (uint8_t) ( 0x02U ),    /**<Invalid request - Wrong sequence */
    eBOOT_MSG_ERROR_FLASH_WRITE     = (uint8_t) ( 0x04U ),    /**<Writing to FLASH error */
    eBOOT_MSG_ERROR_FLASH_ERASE     = (uint8_t) ( 0x08U ),    /**<Erasing FLASH error */
    eBOOT_MSG_ERROR_FW_SIZE         = (uint8_t) ( 0x10U ),    /**<Firmware image size to big error */
    eBOOT_MSG_ERROR_FW_VER          = (uint8_t) ( 0x20U ),    /**<Uncompatible firmware version error */
    eBOOT_MSG_ERROR_HW_VER          = (uint8_t) ( 0x40U ),    /**<Uncompatible hardware version error */
    eBOOT_MSG_ERROR_SIGNATURE       = (uint8_t) ( 0x80U ),    /**<Invalid digital signature */
} boot_msg_status_t;

/**
 *  Boot reasons
 */
typedef enum
{
	eBOOT_REASON_NONE = 0U, /**<Idle, jumpt to application */
	eBOOT_REASON_COM,       /**<Communication reason to stay in bootloader, expect boot sequence from Bootloader Manager */
	eBOOT_REASON_FLASH,     /**<Boot from external FLAHS memory */

	eBOOT_REASON_NUM_OF
} boot_reason_t;

/**
 *      Shared memory layout
 *
 *  Sizeof: 32bytes
 */
typedef struct __BOOT_CFG_PACKED__
{
    /**     Control fields
     *
     *  Sizeof: 8 bytes
     *
     *  @note   Are fixed, shall not be change during different versions!
     */
    struct
    {
        uint8_t crc;    /**<CRC8 of shared memory */
        uint8_t ver;    /**<Shared memory layout version */
        uint8_t res[6]; /**<Reserved fields */
    } ctrl;

    /**     Data fields
     *
     *  Sizeof: 24 bytes
     *
     *  @note   Data fields can be re-sized between different versions of application header.
     */
    struct
    {
        uint32_t boot_ver;      /**<Bootloader software version */
        uint8_t  boot_reason;   /**<Boot reason. Shall be value of @boot_reason_t */
        uint8_t  boot_cnt;      /**<Boot counter */
        uint8_t  res[18];       /**<Reserved space */
    } data;
} boot_shared_mem_t;

/**
 *  Shared memory size check
 */
_Static_assert( 32 == sizeof(boot_shared_mem_t));

#endif // __BOOT_TYPES_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
