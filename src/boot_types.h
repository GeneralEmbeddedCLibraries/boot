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
    eBOOT_OK                    = (uint8_t) ( 0x00U ),    /**<Normal operation */
    eBOOT_ERROR_VALIDATION      = (uint8_t) ( 0x01U ),    /**<Validation error */
    eBOOT_ERROR_INVALID_REQ     = (uint8_t) ( 0x02U ),    /**<Invalid request - Wrong sequence */
    eBOOT_ERROR_FLASH_WRITE     = (uint8_t) ( 0x04U ),    /**<Writing to FLASH error */
    eBOOT_ERROR_FLASH_ERASE     = (uint8_t) ( 0x08U ),    /**<Erasing FLASH error */
    eBOOT_ERROR_FW_SIZE         = (uint8_t) ( 0x10U ),    /**<Firmware image size to big error */
    eBOOT_ERROR_FW_VER          = (uint8_t) ( 0x20U ),    /**<Uncompatible firmware version error */
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



#endif // __BOOT_TYPES_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
