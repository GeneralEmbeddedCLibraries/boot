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




////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_init(void);
boot_status_t boot_com_hndl(void);

// Message send functions
boot_status_t boot_com_send_connect     (void);
boot_status_t boot_com_send_connect_rsp (const boot_status_t stat);
boot_status_t boot_com_send_prepare     (const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver);
boot_status_t boot_com_send_prepare_rsp (const boot_status_t stat);
boot_status_t boot_com_send_flash     	(const uint8_t * const p_data, const uint16_t size);
boot_status_t boot_com_send_flash_rsp 	(const boot_status_t stat);
boot_status_t boot_com_send_exit     	(void);
boot_status_t boot_com_send_exit_rsp 	(const boot_status_t stat);
boot_status_t boot_com_send_info        (void);
boot_status_t boot_com_send_info_rsp    (const uint32_t boot_ver, const boot_status_t status);

// Message receive callback functions
void boot_com_connect_msg_rcv_cb        (void);
void boot_com_connect_msg_cmd_rcv_cb    (const boot_status_t status);
void boot_com_prepare_msg_rcv_cb        (const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver);
void boot_com_prepare_rsp_msg_rcv_cb    (const boot_status_t status);
void boot_com_flash_msg_rcv_cb          (const uint8_t * const p_data, const uint16_t size);
void boot_com_flash_rsp_msg_rcv_cb      (const boot_status_t status);
void boot_com_exit_msg_rcv_cb           (void);
void boot_com_exit_rsp_msg_rcv_cb       (const boot_status_t status);
void boot_com_info_msg_rcv_cb           (void);
void boot_com_info_rsp_msg_rcv_cb       (const uint32_t boot_ver, const boot_status_t status);

#endif // __BOOT_COM_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
