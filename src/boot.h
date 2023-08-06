// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot.h
*@brief     Bootloader API
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      06.08.2023
*@version   V0.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BOOT_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __BOOT_H
#define __BOOT_H

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
 *  Module version
 */
#define BOOT_VER_MAJOR          ( 0 )
#define BOOT_VER_MINOR          ( 1 )
#define BOOT_VER_DEVELOP        ( 0 )

/**
 *  Bootloader status
 */
typedef enum
{
    eBOOT_OK        = 0x00U,    /**<Normal operation */
    eBOOT_ERROR     = 0x01U,    /**<General error code */
} boot_status_t;

/**
 *  Bootloader state
 */
typedef enum
{
    eBOOT_STATE_IDLE = 0,   /**<Idle state - waiting for request */
    eBOOT_STATE_PREPARE,    /**<Preparing FLASH memory */
    eBOOT_STATE_FLASH,      /**<Flashing memory with new software */
    eBOOT_STATE_VALIDATE,   /**<Validation of software image */
    eBOOT_STATE_EXIT,       /**<Exit bootloader - Enter application */
 
    eBOOT_STATE_NUM_OF
} boot_state_t;


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
boot_status_t   boot_init       (void);
boot_status_t   boot_hndl       (void);
boot_state_t    boot_get_state  (void);

// Events callback
// TODO: ...
void boot_validation_failed_cb	(void);
void boot_enter_app_cb			(void);

#endif // __BOOT_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
