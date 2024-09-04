// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot.c
*@brief     Bootloader
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      15.02.2024
*@version   V0.2.0
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
#include <string.h>

#include "boot.h"
#include "boot_com.h"
#include "../../boot_if.h"

// Middleware
#include "middleware/fsm/fsm/src/fsm.h"

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

// TODO: Update compatibility checks as revision submodule will become V2.0.0
_Static_assert( 1 == VER_VER_MAJOR );
_Static_assert( 3 <= VER_VER_MINOR );

/**
 *  Compatibility check with FSM
 *
 *  Support version V1.1.x up
 */

// TODO: Update compatibility checks!
_Static_assert( 1 == VER_VER_MAJOR );
_Static_assert( 1 <= VER_VER_MINOR );

/**
 *  Compiler compatibility check
 */
BOOT_CFG_STATIC_ASSERT( sizeof(boot_shared_mem_t) == 32U );
BOOT_CFG_STATIC_ASSERT( sizeof(ver_app_header_t) == 256U );

/**
 *      Shared memory layout version
 */
#define BOOT_SHARED_MEM_VER                     ( 1 )

/**
 *  Reset vector function pointer
 */
typedef void (*p_func)(void);

/**
 *  Flashing data info
 */
typedef struct
{
    uint32_t working_addr;      /**<Working address of FLASH */
    uint32_t flashed_bytes;     /**<Number of flashed bytes */
    uint32_t fw_size;           /**<New firmware image size in bytes */
} boot_flashing_t;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static uint8_t              boot_calc_crc               (const uint8_t * const p_data, const uint16_t size);
static boot_status_t        boot_app_head_read          (ver_app_header_t * const p_head);
static boot_status_t        boot_app_head_erase         (void);
static uint8_t              boot_app_head_calc_crc      (const ver_app_header_t * const p_head);
static uint32_t             boot_fw_image_calc_crc      (const uint32_t size);
static boot_status_t        boot_fw_image_validate      (void);
static boot_status_t        boot_start_application      (void);
static boot_status_t        boot_shared_mem_calc_crc    (const boot_shared_mem_t * const p_mem);
static void                 boot_init_shared_mem        (void);
static void                 boot_wait                   (const uint32_t ms);
static boot_msg_status_t    boot_fw_size_check          (const uint32_t fw_size);
static boot_msg_status_t    boot_fw_ver_check           (const uint32_t fw_ver);
static boot_msg_status_t    boot_hw_ver_check           (const uint32_t hw_ver);
static void                 boot_init_boot_counter      (void);

// FSM state handlers
static void boot_fsm_idle_hndl      (const p_fsm_t fsm_inst);
static void boot_fsm_prepare_hndl   (const p_fsm_t fsm_inst);
static void boot_fsm_flash_hndl     (const p_fsm_t fsm_inst);
static void boot_fsm_exit_hndl      (const p_fsm_t fsm_inst);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
*  Shared memory between app and bootloader
*/
static volatile boot_shared_mem_t __BOOT_CFG_SHARED_MEM__ g_boot_shared_mem;

/**
 *  Bootloader FSM
 */
static p_fsm_t g_boot_fsm = NULL;

/**
 * FSM state table
 */
static const fsm_cfg_t g_boot_fsm_cfg_table =
{

    // TODO: Profile states...
    .p_states = (fsm_state_cfg_t[])
    {
        [eBOOT_STATE_IDLE]      = {.on_entry=NULL, .on_activity=boot_fsm_idle_hndl,     .on_exit=NULL, .name="IDLE"     },
        [eBOOT_STATE_PREPARE]   = {.on_entry=NULL, .on_activity=boot_fsm_prepare_hndl,  .on_exit=NULL, .name="PREPARE"  },
        [eBOOT_STATE_FLASH]     = {.on_entry=NULL, .on_activity=boot_fsm_flash_hndl,    .on_exit=NULL, .name="FLASH"    },
        [eBOOT_STATE_EXIT]      = {.on_entry=NULL, .on_activity=boot_fsm_exit_hndl,     .on_exit=NULL, .name="EXIT"     },
    },
    .name   = "Boot FSM",
    .num_of = eBOOT_STATE_NUM_OF,
};

/**
 *  Flashing data
 */
static boot_flashing_t g_boot_flashing = { 0 };

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
static uint8_t boot_calc_crc(const uint8_t * const p_data, const uint16_t size)
{
    const   uint8_t poly    = 0x07U;    // CRC-8-CCITT
    const   uint8_t seed    = 0xB6U;    // Custom seed
            uint8_t crc8    = seed;

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
*       Read application header
*
* @param[in]    p_head  - Pointer to application header
* @return       status	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_app_head_read(ver_app_header_t * const p_head)
{
    boot_status_t status = eBOOT_OK;

    // Read application header
    if ( eBOOT_OK == boot_if_flash_read( BOOT_CFG_APP_HEAD_ADDR, sizeof(ver_app_header_t), (uint8_t*) p_head ))
    {
        // Calculate application header crc
        const uint8_t app_head_crc_calc = boot_app_head_calc_crc( p_head );

        // Check CRC
        if ( app_head_crc_calc != p_head->ctrl.crc )
        {
            // Application header corrupted
            status = eBOOT_ERROR_CRC;
            BOOT_DBG_PRINT( "ERROR: Application header corrupted!" );
        }
    }
    else
    {
        status = eBOOT_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Erase application header
*
* @note     This function is being used for safety reasons when upgrade process
*           gets either canceled, timeouted or interrupted of any other reasons.
*           Erasing application header creates fresh starts for upgrade process.
*
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_app_head_erase(void)
{
    boot_status_t status = eBOOT_OK;

    // Erase application header
    if ( eBOOT_OK != boot_if_flash_erase( BOOT_CFG_APP_HEAD_ADDR, sizeof(ver_app_header_t)))
    {
        status = eBOOT_ERROR;
    }

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
    uint8_t crc8 = 0U;

    // Calculate CRC
    // NOTE: Skip CRC at the end and start calculation at version field!
    crc8 = boot_calc_crc((uint8_t*) &(p_head->ctrl.ver), ( sizeof(ver_app_header_t) - 1U ));

    return crc8;
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

    // Calculate size of app image without application header
    const uint32_t app_size = ( size - sizeof(ver_app_header_t));

    for (uint32_t i = 0; i < app_size; i++)
    {
    	// Ignore application header from CRC calc
        const uint32_t addr = BOOT_CFG_APP_HEAD_ADDR + sizeof(ver_app_header_t) + i;

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
* 			Time execution on Cortex-M4 @150MHz:
* 			    -O0:    137 ms
* 			    -Ofast: 110 ms
*
* @return       status - Status of validation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_fw_image_validate(void)
{
            boot_status_t    status = eBOOT_OK;
    static  ver_app_header_t app_header = {0};

    // Read application header
    status = boot_app_head_read((ver_app_header_t*) &app_header );

    // Application header OK
    if ( eBOOT_OK == status )
    {
    	// Calculate firmware image crc
    	const uint32_t fw_crc_calc = boot_fw_image_calc_crc( app_header.data.app_size );

    	// FW image CRC valid
    	if ( app_header.data.app_crc == fw_crc_calc )
    	{
    		BOOT_DBG_PRINT( "Firmware image OK!" );
    	}

    	// FW image corrupted
    	else
    	{
        	status = eBOOT_ERROR_CRC;

            /**
             *  Erase application header -> This will enable re-flashing
             *  same version of application as FW compatibility checks
             *  will not failed!
             */
        	(void) boot_app_head_erase();

        	BOOT_DBG_PRINT( "ERROR: Firmware image corrupted!" );
    	}
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
		__set_MSP( BOOT_CFG_APP_START_ADDR );

		// Next address is reset vector for app
		const uint32_t app_addr = *(uint32_t*)( BOOT_CFG_APP_START_ADDR + 4U );
		p_func p_app = (p_func) app_addr;

		// Start Application
		p_app();
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Calculate shared memory CRC
*
* @param[in]    p_mem   - Pointer to shared memory
* @return       status  - Status of validation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_shared_mem_calc_crc(const boot_shared_mem_t * const p_mem)
{
    uint8_t crc8 = 0U;

    // Calculate crc
    // NOTE: Ignore CRC value at the end (-1)
    crc8 = boot_calc_crc((uint8_t*) &( p_mem->ctrl.ver ), ( sizeof(boot_shared_mem_t) - 1U ));

    return crc8;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Initialize shared memory
*
* @note     In case of CRC failure -> all fields are set to default!
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_init_shared_mem(void)
{
    // Shared memory CRC OK
    if ( boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem ) == g_boot_shared_mem.ctrl.crc )
    {
        // Count boot ups
        if ( g_boot_shared_mem.data.boot_cnt < UINT8_MAX )
        {
            g_boot_shared_mem.data.boot_cnt++;
        }
    }

    // CRC error
    else
    {
        g_boot_shared_mem.data.boot_cnt      = 0U;
        g_boot_shared_mem.data.boot_reason   = eBOOT_REASON_NONE;

        BOOT_DBG_PRINT( "ERROR: Shared memory corrupted!" );
    }

    // Set shared memory data
    g_boot_shared_mem.ctrl.ver      = BOOT_SHARED_MEM_VER;
    g_boot_shared_mem.data.boot_ver = version_get_sw().U;

    // Calculate CRC
    g_boot_shared_mem.ctrl.crc = boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Wait (delay) and handle bootloader in between
*
* @param[in]    ms  - Time to wait in miliseconds
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_wait(const uint32_t ms)
{
    uint32_t safety_cnt = 0U;

    // Get current ticks
    const uint32_t now = BOOT_GET_SYSTICK();

    if ( ms > 0 )
    {
        // Wait
        while(      ((uint32_t)( BOOT_GET_SYSTICK() - now ) <= ms )
                &&  ( safety_cnt < 0xEFFFFFFFU ))
        {
            // Handle bootloader tasks
            boot_hndl();

            // Increment safety counter
            safety_cnt++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Check for new application image size
*
* @param[in]    fw_size     - New application image size in bytes
* @return       status      - Status of check
*/
////////////////////////////////////////////////////////////////////////////////
static boot_msg_status_t boot_fw_size_check(const uint32_t fw_size)
{
    boot_msg_status_t status = eBOOT_MSG_OK;

    #if ( 1 == BOOT_CFG_FW_SIZE_CHECK_EN )

        // New application image to big!
        if ( fw_size > BOOT_CFG_APP_SIZE_MAX )
        {
            status = eBOOT_MSG_ERROR_FW_SIZE;
        }

    #else
        // Unused
        (void) fw_size;
    #endif

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Check for new application SW version compatibility
*
* @param[in]    fw_ver  - New application FW version in form of uint32_t: 0xMAJOR.MINOR.DEVELOP.TEST
* @return       status  - Status of check
*/
////////////////////////////////////////////////////////////////////////////////
static boot_msg_status_t boot_fw_ver_check(const uint32_t fw_ver)
{
    boot_msg_status_t status = eBOOT_MSG_OK;

    #if ( 1 == BOOT_CFG_FW_VER_CHECK_EN )

        // Assemble limit FW version (compatible up to that version/limit value)
        const uint32_t fw_ver_lim = (uint32_t) ((( BOOT_CFG_FW_VER_MAJOR & 0xFFU ) << 24U )    | (( BOOT_CFG_FW_VER_MINOR & 0xFFU ) << 16U )
                                            |   (( BOOT_CFG_FW_VER_DEVELOP & 0xFFU ) << 8U )   | ( BOOT_CFG_FW_VER_TEST & 0xFFU ));

        // New application firmware version not supported!
        if ( fw_ver > fw_ver_lim )
        {
            status = eBOOT_MSG_ERROR_FW_VER;
        }

    #else
        // Unused
        (void) fw_ver;
    #endif

#if ( 0 == BOOT_CFG_FW_DOWNGRADE_EN )

        static ver_app_header_t app_header = {0};

        // Application header valid
        if ( eBOOT_OK == boot_app_head_read( &app_header ))
        {
            // If new application version is older of the same -> invalid firmware version
            if ( fw_ver <= app_header.sw_ver )
            {
                status = eBOOT_MSG_ERROR_FW_VER;
            }
        }

#endif

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Check for new application HW version compatibility
*
* @param[in]    hw_ver  - New application HW version in form of uint32_t: 0xMAJOR.MINOR.DEVELOP.TEST
* @return       status  - Status of check
*/
////////////////////////////////////////////////////////////////////////////////
static boot_msg_status_t boot_hw_ver_check(const uint32_t hw_ver)
{
    boot_msg_status_t status = eBOOT_MSG_OK;

    #if ( 1 == BOOT_CFG_HW_VER_CHECK_EN )

        // Assemble limit HW version (compatible up to that version/limit value)
        const uint32_t hw_ver_lim = (uint32_t) ((( BOOT_CFG_HW_VER_MAJOR & 0xFFU ) << 24U )    | (( BOOT_CFG_HW_VER_MINOR & 0xFFU ) << 16U )
                                            |   (( BOOT_CFG_HW_VER_DEVELOP & 0xFFU ) << 8U )   | ( BOOT_CFG_HW_VER_TEST & 0xFFU ));

        // New application firmware version not supported!
        if ( hw_ver > hw_ver_lim )
        {
            status = eBOOT_MSG_ERROR_HW_VER;
        }

    #else
        // Unused
        (void) hw_ver;
    #endif

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Initialize (handle) boot counter
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_init_boot_counter(void)
{
    #if ( 1 == BOOT_CFG_APP_BOOT_CNT_CHECK_EN )

        uint8_t cnt = 0U;

        // Get boot counter
        if ( eBOOT_OK == boot_shared_mem_get_boot_cnt( &cnt ))
        {
            // Limit reached
            if ( cnt >= BOOT_CFG_BOOT_CNT_LIMIT )
            {
                // Stay in bootloader
                boot_shared_mem_set_boot_reason( eBOOT_REASON_COM );

                // Corrupt app header in order to prevent entering app
                boot_app_head_erase();

                BOOT_DBG_PRINT( "Boot counts limit reached! Declaring invalid application!" );
            }
        }

        // Store boot counter
        (void) boot_shared_mem_set_boot_cnt( cnt );

    #endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*       IDLE bootloader FSM state
*
* @param[in]    fsm_inst - FMS instance
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_fsm_idle_hndl(const p_fsm_t fsm_inst)
{
    static bool try_to_leave = false;

    // On first entry
    if ( true == fsm_get_first_entry( fsm_inst ))
    {
        // Clear flashing data informations
        memset( &g_boot_flashing, 0U, sizeof( g_boot_flashing ));

        // Reset decryption engine
        #if ( 1 == BOOT_CFG_CRYPTION_EN )
            boot_if_decrypt_reset();
        #endif

        // Clear flag
        try_to_leave = false;
    }

    // Get idle time duration
    const uint32_t idle_duration = fsm_get_duration( fsm_inst );

    // Exit bootloader if idle for too long, but try only once
    if  (   ( idle_duration >= BOOT_CFG_JUMP_TO_APP_TIMEOUT_MS )
        &&  ( false == try_to_leave ))
    {
        try_to_leave = true;

        BOOT_DBG_PRINT( "Nothing to do... Exiting bootloader..." );

        // Application image validated OK
        if ( eBOOT_OK == boot_fw_image_validate())
        {
            // Clear reason to stay in bootloader
            (void) boot_shared_mem_set_boot_reason( eBOOT_REASON_NONE );

            // Exit boot by starting application
            boot_start_application();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       PREPARE bootloader FSM state
*
* @param[in]    fsm_inst - FMS instance
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_fsm_prepare_hndl(const p_fsm_t fsm_inst)
{
    // Get time in that state
    const uint32_t state_duration = fsm_get_duration( fsm_inst );

    // Boot process idle for too long -> enter IDLE
    if ( state_duration >= BOOT_CFG_PREPARE_IDLE_TIMEOUT_MS )
    {
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );

        // Erase application header
        (void) boot_app_head_erase();

        BOOT_DBG_PRINT( "ERROR: Prepare state timeouted!" );
    }







}

////////////////////////////////////////////////////////////////////////////////
/**
*       FLASH bootloader FSM state
*
* @param[in]    fsm_inst - FMS instance
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_fsm_flash_hndl(const p_fsm_t fsm_inst)
{
    // Get time in that state
    const uint32_t state_duration = fsm_get_duration( fsm_inst );

    // Calculate time pass from last rx packet
    const uint32_t time_from_last_rx = (uint32_t) ( BOOT_GET_SYSTICK() - boot_com_get_last_rx_timestamp());

    // After a while in flash state
    if ( state_duration >= BOOT_CFG_FLASH_IDLE_TIMEOUT_MS )
    {
        // Check for communication timeout
        if ( time_from_last_rx >= BOOT_CFG_FLASH_IDLE_TIMEOUT_MS )
        {
            fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );

            // Erase application header
            (void) boot_app_head_erase();

            BOOT_DBG_PRINT( "ERROR: Communication timeouted!" );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       EXIT bootloader FSM state
*
* @param[in]    fsm_inst - FMS instance
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_fsm_exit_hndl(const p_fsm_t fsm_inst)
{
    // Get time in that state
    const uint32_t state_duration = fsm_get_duration( fsm_inst );

    // Boot process idle for too long -> enter IDLE
    if ( state_duration >= BOOT_CFG_EXIT_IDLE_TIMEOUT_MS )
    {
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );

        // Erase application header
        (void) boot_app_head_erase();

        BOOT_DBG_PRINT( "ERROR: Exit state timeouted!" );
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Connect Bootloader Message Reception Callback
*
* @param[in]    fsm_inst - FMS instance
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_connect_msg_rcv_cb(void)
{
    boot_msg_status_t msg_status = eBOOT_MSG_OK;

    // In IDLE state
    if ( eBOOT_STATE_IDLE == boot_get_state())
    {
        // Stay in bootloader -> reason communication
        boot_shared_mem_set_boot_reason( eBOOT_REASON_COM );

        // Enter PREPARE state
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_PREPARE );
    }

    // Not in IDLE state
    else
    {
        // Enter IDLE state -> cancel boot operation
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );

        // Return "Invalid Request" message
        msg_status = eBOOT_MSG_ERROR_INVALID_REQ;
    }

    // Send connect msg response
    boot_com_send_connect_rsp( msg_status );

    BOOT_DBG_PRINT( "Connect msg received...");
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
    // Unused
    (void) msg_status;

    // No actions...
    // TODO: Boot Manager implementation here...
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Prepare Bootloader Message Reception Callback
*
* @param[in]    fw_size     - Firmware size in bytes
* @param[in]    fw_ver      - Firmware version
* @param[in]    hw_ver      - Hardware version
*
*  TODO: Image info as input, where all related info shall be passed!
*
*       Image address as well -> this shall setup "g_boot_flashing.working_addr"
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void boot_com_prepare_msg_rcv_cb(const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver)
{
    boot_msg_status_t msg_status = eBOOT_MSG_OK;

    // In PREPARE state
    if ( eBOOT_STATE_PREPARE == boot_get_state())
    {
        // Check for FW size
        msg_status |= boot_fw_size_check( fw_size );

        // Check for FW version compatibility
        msg_status |= boot_fw_ver_check( fw_ver );

        // Check for HW version compatibility
        msg_status |= boot_hw_ver_check( hw_ver );

        // Check authenticy (digital signature)
        // TODO: ...

        // Everything OK
        if ( eBOOT_MSG_OK == msg_status )
        {

            // TODO: Split flash erase into individual segments and kick wdt in between!!!

            //uint32_t addr = BOOT_CFG_APP_HEAD_ADDR + sector_size;

            //flash_erase( addr, sector_size );

            // Erase application region flash
            if ( eBOOT_OK != boot_if_flash_erase( BOOT_CFG_APP_HEAD_ADDR, BOOT_CFG_APP_SIZE_MAX ))
            {
                msg_status = eBOOT_MSG_ERROR_FLASH_ERASE;
            }
        }
    }

    // Not in PREPARE state
    else
    {
        msg_status = eBOOT_MSG_ERROR_INVALID_REQ;
    }

    // Enter FLASH state if every operation is OK
    if ( eBOOT_MSG_OK == msg_status )
    {
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_FLASH );

        // Prepare flashing data
        g_boot_flashing.fw_size         = fw_size;
        g_boot_flashing.working_addr    = BOOT_CFG_APP_HEAD_ADDR;
    }

    // Some problems during prepare operation -> enter IDLE state and wait for next command from Boot Manager
    else
    {
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );
    }

    // Send prepare msg response
    boot_com_send_prepare_rsp( msg_status );

    BOOT_DBG_PRINT( "Prepare msg received...");
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
    // Unused
    (void) msg_status;

    // No actions...
    // TODO: Boot Manager implementation here...
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
    boot_msg_status_t msg_status = eBOOT_MSG_OK;

    // In FLASHING state
    if ( eBOOT_STATE_FLASH == boot_get_state())
    {
        // All data has been flashed
        if ( g_boot_flashing.flashed_bytes < g_boot_flashing.fw_size )
        {
         #if ( 1 == BOOT_CFG_CRYPTION_EN )

            static uint8_t decrypted_data[BOOT_CFG_DATA_PAYLOAD_SIZE] = {0};

            BOOT_ASSERT( size < BOOT_CFG_DATA_PAYLOAD_SIZE );

            // Decrypt data
            boot_if_decrypt_data( p_data, (uint8_t*) &decrypted_data, size );

            // Flash decrypted data
            if ( eBOOT_OK == boot_if_flash_write( g_boot_flashing.working_addr, size, (uint8_t*) &decrypted_data ))
          #else
            // Flash data
            if ( eBOOT_OK == boot_if_flash_write( g_boot_flashing.working_addr, size, p_data ))
         #endif
            {
                // Increment working address
                g_boot_flashing.working_addr += size;

                // Increment flashed bytes
                g_boot_flashing.flashed_bytes += size;

                // Complete FW image flashed
                if ( g_boot_flashing.flashed_bytes == g_boot_flashing.fw_size )
                {
                    // Image flashed completely -> enter EXIT state
                    fsm_goto_state( g_boot_fsm, eBOOT_STATE_EXIT );
                }
            }

            // Flashing error
            else
            {
                msg_status = eBOOT_MSG_ERROR_FLASH_WRITE;
            }
        }

        // Shall not ended up here as compete firmware are flashed
        else
        {
            msg_status = eBOOT_MSG_ERROR_FLASH_WRITE;
        }
    }

    // Not in FLASH state
    else
    {
        msg_status = eBOOT_MSG_ERROR_INVALID_REQ;
    }

    if ( eBOOT_MSG_OK != msg_status )
    {
        // Something not OK, enter IDLE state
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );

        // Erase application header
        (void) boot_app_head_erase();
    }

    // Send flash msg response
    boot_com_send_flash_rsp( msg_status );
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
    // Unused
    (void) msg_status;

    // No actions...
    // TODO: Boot Manager implementation here...
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
    boot_msg_status_t msg_status = eBOOT_MSG_OK;

    // In EXIT state
    if ( eBOOT_STATE_EXIT == boot_get_state())
    {
        // Application image validated OK
        if ( eBOOT_OK == boot_fw_image_validate())
        {
            // Send exit msg response
            boot_com_send_exit_rsp( eBOOT_MSG_OK );

            // Wait for response to get send
            boot_wait( 5U );

            // Clear boot reason & counter
            boot_shared_mem_set_boot_reason( eBOOT_REASON_NONE );
            boot_shared_mem_set_boot_cnt( 0U );

            // Jump to application
            boot_start_application();

            // This line is not reached as cpu starts executing application code...
        }

        // Application image validated ERROR
        else
        {
            msg_status = eBOOT_MSG_ERROR_VALIDATION;

            // Erase application header
            (void) boot_app_head_erase();

            // Something not OK, enter IDLE state
            fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );
        }
    }

    // Not in PREPARE state
    else
    {
        msg_status = eBOOT_MSG_ERROR_INVALID_REQ;

        // Something not OK, enter IDLE state
        fsm_goto_state( g_boot_fsm, eBOOT_STATE_IDLE );
    }

    // Send exit msg response
    boot_com_send_exit_rsp( msg_status );
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
    // Unused
    (void) msg_status;

    // No actions...
    // TODO: Boot Manager implementation here...
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
    boot_msg_status_t   msg_status  = eBOOT_MSG_OK;
    uint32_t            boot_ver    = 0U;

    // In IDLE state
    if ( eBOOT_STATE_IDLE == boot_get_state())
    {
        boot_ver = version_get_sw().U;
    }

    // Not in IDLE state
    else
    {
        msg_status = eBOOT_MSG_ERROR_INVALID_REQ;
    }

    // Send info msg response
    boot_com_send_info_rsp( boot_ver, msg_status );
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
    // Unused
    (void) boot_ver;
    (void) msg_status;

    // No actions...
    // TODO: Boot Manager implementation here...
}


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

////////////////////////////////////////////////////////////////////////////////
/**
*       Initialize bootloader
*
* @note     Shall be used only by bootloader code!
*           Do not call this function from application code!
*
* @note     If there is a valid application already placed in flash
*           then this function will not return as app code will be started!
*
* @return       status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_init(void)
{
    boot_status_t status = eBOOT_OK;

    // Set shared memory version
    boot_init_shared_mem();

    // Init bootloader FSM
    if ( eFSM_OK != fsm_init( &g_boot_fsm, &g_boot_fsm_cfg_table ))
    {
        status = eBOOT_ERROR;
    }

    // Initialize bootloader interfaces
    // TODO: CHeck if makes sense to split memory interface with communication interface!?
    status |= boot_if_init();

    // Iniatilize (handle) boot counter
    boot_init_boot_counter();

    // No reason to stay in bootloader
    if ( eBOOT_REASON_NONE == g_boot_shared_mem.data.boot_reason )
    {
        // Application image validated OK
        if ( eBOOT_OK == boot_fw_image_validate())
        {
            // Back door entry for bootloader
            boot_wait( BOOT_CFG_WAIT_AT_STARTUP_MS );

            // Check if reason has change from the back door
            if ( eBOOT_REASON_NONE == g_boot_shared_mem.data.boot_reason )
            {
                // Jump to application
                boot_start_application();
            }

            // This line is not reached as CPU starts executing application code...
        }
    }
    else
    {
        BOOT_DBG_PRINT( "Booting reason: %d", g_boot_shared_mem.data.boot_reason );
    }

    /**
     *  @note   Bootloader ended up here only if:
     *
     *          1. There is a reason to stay in bootloader
     *      OR  2. Application image is corrupted
     */

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Handle bootloader logic
*
* @return       status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_hndl(void)
{
    boot_status_t status = eBOOT_OK;

    // Handle bootloader communication
    status |= boot_com_hndl();

    // Handle FSM
    (void) fsm_hndl( g_boot_fsm );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get bootloader FSM state
*
* @return       state - Current state of bootloader FSM
*/
////////////////////////////////////////////////////////////////////////////////
boot_state_t boot_get_state(void)
{
    boot_state_t state = eBOOT_STATE_IDLE;

    // Get state
    state = (boot_state_t) fsm_get_state( g_boot_fsm );

    return state;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get shared memory layout version
*
* @note     Function return "eBOOT_ERROR_CRC" in case of shared memory
*           data corruption!
*
* @param[out]   p_version   - Shared memory layout version
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_shared_mem_get_version(uint8_t * const p_version)
{
    boot_status_t status = eBOOT_OK;

    BOOT_ASSERT( NULL != p_version );

    if ( NULL != p_version )
    {
        // Validate shared memory
        if ( g_boot_shared_mem.ctrl.crc == boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem ))
        {
            *p_version = g_boot_shared_mem.ctrl.ver;
        }
        else
        {
            status = eBOOT_ERROR_CRC;
        }
    }
    else
    {
        status = eBOOT_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*      Set boot reason
*
* @param[in]    reason    - Boot reason
* @return       status    - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_shared_mem_set_boot_reason(const boot_reason_t reason)
{
    boot_status_t status = eBOOT_OK;

    // Set reason
    g_boot_shared_mem.data.boot_reason = reason;

    // Calculate CRC
    g_boot_shared_mem.ctrl.crc = boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get booting reason
*
* @note     Function return "eBOOT_ERROR_CRC" in case of shared memory
*           data corruption!
*
* @param[out]   p_reason    - Boot reason
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_shared_mem_get_boot_reason(boot_reason_t * const p_reason)
{
    boot_status_t status = eBOOT_OK;

    BOOT_ASSERT( NULL != p_reason );

    if ( NULL != p_reason )
    {
        // Validate shared memory
        if ( g_boot_shared_mem.ctrl.crc == boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem ))
        {
            *p_reason = g_boot_shared_mem.data.boot_reason;
        }
        else
        {
            status = eBOOT_ERROR_CRC;
        }
    }
    else
    {
        status = eBOOT_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*      Set boot counts
*
* @param[in]    reason    - Boot reason
* @return       status    - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_shared_mem_set_boot_cnt(const uint8_t cnt)
{
    boot_status_t status = eBOOT_OK;

    // Set counts
    g_boot_shared_mem.data.boot_cnt = cnt;

    // Calculate CRC
    g_boot_shared_mem.ctrl.crc = boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get number of boots
*
* @brief    Boot counts is used as a safety mechanism to prevent jumping to
*           malfunctioned application.
*
* @note     Function return "eBOOT_ERROR_CRC" in case of shared memory
*           data corruption!
*
* @param[out]   p_cnt    - Boot counts
* @return       status   - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_shared_mem_get_boot_cnt(uint8_t * const p_cnt)
{
    boot_status_t status = eBOOT_OK;

    BOOT_ASSERT( NULL != p_cnt );

    if ( NULL != p_cnt )
    {
        // Validate shared memory
        if ( g_boot_shared_mem.ctrl.crc == boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem ))
        {
            *p_cnt = g_boot_shared_mem.data.boot_cnt;
        }
        else
        {
            status = eBOOT_ERROR_CRC;
        }
    }
    else
    {
        status = eBOOT_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get bootloader version
*
* @note     Function return "eBOOT_ERROR_CRC" in case of shared memory
*           data corruption!
*
* @param[out]   p_boot_ver  - Bootloader software verion
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_shared_mem_get_boot_ver(uint32_t * const p_boot_ver)
{
    boot_status_t status = eBOOT_OK;

    BOOT_ASSERT( NULL != p_boot_ver );

    if ( NULL != p_boot_ver )
    {
        // Validate shared memory
        if ( g_boot_shared_mem.ctrl.crc == boot_shared_mem_calc_crc((const boot_shared_mem_t *) &g_boot_shared_mem ))
        {
            *p_boot_ver = g_boot_shared_mem.data.boot_ver;
        }
        else
        {
            status = eBOOT_ERROR_CRC;
        }
    }
    else
    {
        status = eBOOT_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
