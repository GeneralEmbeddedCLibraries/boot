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
#include "../../boot_if.h"

// Revision
#include "revision/revision/src/version.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////


/**
 *  Compatibility check with REVISION
 *
 *  Support version V1.3.x up
 */
_Static_assert( 1 == VER_VER_MAJOR );
_Static_assert( 3 >= VER_VER_MAJOR );


/**
 *  	Start of application address
 *
 *  @note 	Right after application header!
 */
#define BOOT_APP_START_ADDR				( BOOT_CFG_APP_HEAD_ADDR + BOOT_CFG_APP_HEAD_SIZE )


/**
 *  Reset vector function pointer
 */
typedef void (*p_func)(void);


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
static boot_status_t    boot_app_head_read      (ver_app_header_t * const p_head);
static uint8_t          boot_app_head_calc_crc  (const ver_app_header_t * const p_head);
static uint32_t         boot_fw_image_calc_crc  (const uint32_t size);
static boot_status_t	boot_fw_image_validate	(void);
static boot_status_t 	boot_start_application	(void);


#if ( 1 == BOOT_CFG_DEBUG_EN )
    static const char * boot_get_msg_status_str(const boot_msg_status_t msg_status);
#endif

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*       Read application header
*
* @param[in]    p_head  - Pointer to application header
* @return       status	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_app_head_read(ver_app_header_t * const p_head)
{
    boot_status_t status = eBOOT_OK;

    status = boot_if_flash_read( BOOT_CFG_APP_HEAD_ADDR, BOOT_CFG_APP_HEAD_SIZE, (uint8_t*) p_head );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Calculate application header CRC
*
* @param[in]    p_head  - Pointer to application header
* @return       crc8    - CRC8 of application header
*/
////////////////////////////////////////////////////////////////////////////////
static uint8_t boot_app_head_calc_crc(const ver_app_header_t * const p_head)
{
    const   uint8_t         poly    = 0x07U;    // CRC-8-CCITT
    const   uint8_t         seed    = 0xB6U;    // Custom seed
            uint8_t         crc8    = seed;
    const   uint8_t * const p_data  = (uint8_t*) p_head;

    // NOTE: Ignore last CRC field (BOOT_CFG_APP_HEAD_SIZE - 1U)
    for (uint32_t i = 0; i < ( BOOT_CFG_APP_HEAD_SIZE - 1U ); i++)
    {
        crc8 = (( crc8 ^ p_data[i] ) & 0xFFU );

        for (uint8_t j = 0U; j < 8U; j++)
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

    return ( crc8 & 0xFFU );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Calculate firmware image CRC
*
* @note	 	Firmware image CRC is being calculated only across application code,
* 			application header is not included into CRC calculations.
*
* @note		This function expects application header at the top
* 			of new firmware image!
*
* @param[in]    size 	- Size of firmware image in bytes
* @return       crc32   - CRC32 of application image
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t boot_fw_image_calc_crc(const uint32_t size)
{
    const   uint32_t    poly    = 0x04C11DB7;
    const   uint32_t    seed    = 0x10101010;
            uint32_t    crc32   = seed;
            uint8_t     data    = 0U;

    for (uint32_t i = 0; i < ( size - BOOT_CFG_APP_HEAD_SIZE ); i++)
    {
    	// Ignore application header from CRC calc
        const uint32_t addr = BOOT_CFG_APP_HEAD_ADDR + BOOT_CFG_APP_HEAD_SIZE + i;

        // Read byte from application
        (void) boot_if_flash_read( addr, 1U, (uint8_t*)&data );

        // Calc CRC-32
        crc32 = (( crc32 ^ data ) & 0xFFFFFFFFU );

        for (uint8_t j = 0U; j < 32U; j++)
        {
            if ( crc32 & 0x80000000U )
            {
                crc32 = (( crc32 << 1U ) ^ poly );
            }
            else
            {
                crc32 = ( crc32 << 1U );
            }
        }
    }

    return ( crc32 & 0xFFFFFFFFU );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Validate firmware image
*
* @note 	That function checks for application header CRC and complete
* 			firmware image CRC.
*
* @return       status - Status of validation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_fw_image_validate(void)
{
	boot_status_t 	 status 	= eBOOT_OK;
    ver_app_header_t app_header = {0};

    // Read application header
    boot_app_head_read( &app_header );

    // Calculate application header crc
    const uint8_t app_head_crc_calc = boot_app_head_calc_crc( &app_header );

    // Check app header CRC is valid
    if ( app_header.crc == app_head_crc_calc )
    {
    	// Calculate firmware image crc
    	const uint32_t fw_crc_calc = boot_fw_image_calc_crc( app_header.app_size );

    	// FW image CRC valid
    	if ( app_header.app_crc == fw_crc_calc )
    	{
    		BOOT_DBG_PRINT( "Firmware image OK!" );
    	}

    	// FW image corrupted
    	else
    	{
        	status = eBOOT_ERROR_CRC;
        	BOOT_DBG_PRINT( "ERROR: Firmware image corrupted!" );
    	}
    }

    // Application header corrupted
    else
    {
    	status = eBOOT_ERROR_CRC;
    	BOOT_DBG_PRINT( "ERROR: Application header corrupted!" );
    }

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Start (Jump) to application code
*
* @note 	That function has ARM Cortex-M specific commands! If bootloader will
* 			be used on other uC then this part of code shall be adopted!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_start_application(void)
{
	boot_status_t status = eBOOT_OK;

	// Disable interrupts
	__disable_irq();

	// De-init application level code
	status = boot_if_deinit();

	if ( eBOOT_OK == status )
	{
		// Set stack pointer
		__set_MSP( BOOT_APP_START_ADDR );

		// Next address is reset vector for app
		uint32_t app_addr = *(uint32_t*)( BOOT_APP_START_ADDR + 4U );
		p_func p_app = (p_func)app_addr;

		// Start Application
		p_app();
	}

	return status;
}

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

    // Initialize bootloader interfaces
    status |= boot_if_init();


    if ( eBOOT_OK == boot_fw_image_validate())
    {
    	// Jump to application
    	if ( 1 )
    	{
    		boot_start_application();
    	}
    }


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
