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

#include "boot_types.h"

// TODO: Remove
#include "boot_com.h"

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
 *  Boot reasons
 */


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
boot_status_t   boot_init       (void);
boot_status_t   boot_hndl       (void);
boot_state_t    boot_get_state  (void);

boot_status_t   boot_shared_mem_get_version         (uint8_t * const p_version);
boot_status_t   boot_shared_mem_set_boot_reason     (const boot_reason_t reason);
boot_status_t   boot_shared_mem_get_boot_reason     (boot_reason_t * const p_reason);
boot_status_t   boot_shared_mem_set_boot_cnt        (const uint8_t cnt);
boot_status_t   boot_shared_mem_get_boot_cnt        (uint8_t * const p_cnt);



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
