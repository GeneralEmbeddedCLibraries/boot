// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      boot_com.c
*@brief     Bootloader Communication
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      15.02.2024
*@version   V0.2.0
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
#include <string.h>

#include "boot_com.h"
#include "../../boot_cfg.h"
#include "../../boot_if.h"


////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *      Communication idle timeout
 *
 * @note    Consecutive bytes shall be received within that timeout period!
 *
 *  Unit: ms
 */
#define BOOT_COM_IDLE_TIMEOUT_MS            ( 20U )

/**
 *  Bootloader message preamble
 */
#define BOOT_COM_MSG_PREAMBLE_VAL           ((uint16_t)( 0x07B0U ))

/**
 *  Booloader communication commands
 */
typedef enum
{
    eBOOT_MSG_CMD_CONNECT       = (uint8_t)( 0x10U ),       /**<Connect command */
    eBOOT_MSG_CMD_CONNECT_RSP   = (uint8_t)( 0x11U ),       /**<Connect response command*/
    eBOOT_MSG_CMD_PREPARE       = (uint8_t)( 0x20U ),       /**<Prepare command */
    eBOOT_MSG_CMD_PREPARE_RSP   = (uint8_t)( 0x21U ),       /**<Prepare response command*/
    eBOOT_MSG_CMD_FLASH         = (uint8_t)( 0x30U ),       /**<Flash data command */
    eBOOT_MSG_CMD_FLASH_RSP     = (uint8_t)( 0x31U ),       /**<Flash data response command*/
    eBOOT_MSG_CMD_EXIT          = (uint8_t)( 0x40U ),       /**<Exit command */
    eBOOT_MSG_CMD_EXIT_RSP      = (uint8_t)( 0x41U ),       /**<Exit response command*/
    eBOOT_MSG_CMD_INFO          = (uint8_t)( 0xA0U ),       /**<Information command */
    eBOOT_MSG_CMD_INFO_RSP      = (uint8_t)( 0xA1U ),       /**<Information response command*/
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

/**
 *  Prepare command payload
 */
typedef struct
{
    uint32_t fw_size;   /**<Size of firmware in bytes */
    uint32_t fw_ver;    /**<Firmware version */
    uint32_t hw_ver;    /**<Hardware version */
} boot_prepare_payload_t;


/**
 *  Bootloader Parser Mode
 */
typedef enum
{
    eBOOT_PARSER_IDLE = 0,      /**<No communication ongoing */
    eBOOT_PARSER_RCV_HEADER,    /**<Receiving header */
    eBOOT_PARSER_RCV_PAYLOAD,   /**<Receiving payload */
} boot_parser_mode_t;

/**
 *   Bootloader Parser
 */
typedef struct
{
    uint32_t            last_timestamp;     /**<Timestamp of last received character */

    // Buffer structure
    struct
    {
        uint8_t     mem[BOOT_CFG_RX_BUF_SIZE];  /**<Buffer space */
        uint16_t    idx;                        /**<Current buffer index */
    } buf;

    boot_parser_mode_t  mode;   /**<Parser mode */
} boot_parser_t;

/**
 *  Function pointer for Bootloader parsing
 */
typedef void(*pf_canm_parse_t)(const boot_header_t * const p_header, const uint8_t * const p_data);

/**
 *  Parsing table
 */
typedef struct
{
    pf_canm_parse_t pf_parse;   /**<Pointer to parsing function */
    boot_cmd_opt_t  cmd;        /**<Message command */
} boot_parse_table_t;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static uint8_t          boot_com_calc_crc       (const uint8_t * const p_data, const uint16_t size);
static boot_status_t    boot_parse_idle         (boot_parser_t * const p_parser);
static boot_status_t    boot_parse_rcv_header   (boot_parser_t * const p_parser, boot_header_t ** pp_header);
static boot_status_t    boot_parse_rcv_payload  (boot_parser_t * const p_parser, const boot_header_t * const p_header, uint8_t ** pp_payload);
static bool             boot_timeout_check      (boot_parser_t * const p_parser);
static boot_status_t    boot_parse_hndl         (boot_header_t ** pp_header, uint8_t ** pp_payload);
static boot_status_t    boot_buf_idx_increment  (void);
static boot_status_t    boot_parse              (boot_parser_t * const p_parser, boot_header_t ** pp_header, uint8_t ** pp_payload);

static void 			boot_parse_connect      (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_connect_rsp  (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_prepare      (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_prepare_rsp  (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_flash        (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_flash_rsp    (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_exit         (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_exit_rsp     (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_info         (const boot_header_t * const p_header, const uint8_t * const p_data);
static void 			boot_parse_info_rsp     (const boot_header_t * const p_header, const uint8_t * const p_data);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *  Bootloader parser
 */
static boot_parser_t g_parser = {0};

/**
 *      Bootloader Parsing Table
 */
static const boot_parse_table_t g_parse_table[] =
{
    { .cmd = eBOOT_MSG_CMD_CONNECT,         .pf_parse = boot_parse_connect      },
    { .cmd = eBOOT_MSG_CMD_CONNECT_RSP,     .pf_parse = boot_parse_connect_rsp  },
    { .cmd = eBOOT_MSG_CMD_PREPARE,         .pf_parse = boot_parse_prepare      },
    { .cmd = eBOOT_MSG_CMD_PREPARE_RSP,     .pf_parse = boot_parse_prepare_rsp  },
    { .cmd = eBOOT_MSG_CMD_FLASH,           .pf_parse = boot_parse_flash        },
    { .cmd = eBOOT_MSG_CMD_FLASH_RSP,       .pf_parse = boot_parse_flash_rsp    },
    { .cmd = eBOOT_MSG_CMD_EXIT,            .pf_parse = boot_parse_exit         },
    { .cmd = eBOOT_MSG_CMD_EXIT_RSP,        .pf_parse = boot_parse_exit_rsp     },
    { .cmd = eBOOT_MSG_CMD_INFO,            .pf_parse = boot_parse_info         },
    { .cmd = eBOOT_MSG_CMD_INFO_RSP,        .pf_parse = boot_parse_info_rsp     },
};

/**
 *  Number of entries in Bootloader Parsing Table
 */
static const uint32_t gu32_parse_table_size = (uint32_t) ( sizeof( g_parse_table ) / sizeof( boot_parse_table_t ));


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
*       Calculate CRC-8 of Bootloader packet
*
* @note     Entry "NULL" to "p_payload" for packets without payload!
*
* @param[in]    p_header    - Packet header
* @param[in]    p_payload   - Packet payload
* @return       crc8    - Calculated CRC
*/
////////////////////////////////////////////////////////////////////////////////
static uint8_t boot_com_calc_crc_packet(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    uint8_t crc8 = 0U;

    // Calculate CRC of header
    crc8 ^= boot_com_calc_crc( (uint8_t*) &( p_header->field.length ),  sizeof( p_header->field.length ));
    crc8 ^= boot_com_calc_crc( (uint8_t*) &( p_header->field.source ),  sizeof( p_header->field.source ));
    crc8 ^= boot_com_calc_crc( (uint8_t*) &( p_header->field.command ), sizeof( p_header->field.command ));
    crc8 ^= boot_com_calc_crc( (uint8_t*) &( p_header->field.status ),  sizeof( p_header->field.status ));

    // Include also payload to CRC if needed
    if ( NULL != p_payload )
    {
        crc8 ^= boot_com_calc_crc( p_payload, p_header->field.length );
    }

    return crc8;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader parse idle section of the message
*
* @param[in]    p_parser    - Pointer to parser data
* @return       status      - Always returns empty status code
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_parse_idle(boot_parser_t * const p_parser)
{
    boot_status_t status = eBOOT_WAR_EMPTY;

    // Go receiving header
    p_parser->mode = eBOOT_PARSER_RCV_HEADER;

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader parse header section of the message
*
* @param[in]    p_parser    - Pointer to parser data
* @return       status      - Always returns empty status code
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_parse_rcv_header(boot_parser_t * const p_parser, boot_header_t ** p_header)
{
    boot_status_t   status      = eBOOT_WAR_EMPTY;
    uint8_t         crc_calc    = 0;
    uint8_t         crc_msg     = 0;

    // Size of header is received
    if ( p_parser->buf.idx == sizeof( boot_header_t ))
    {
        // Get header
        *p_header = (boot_header_t*) &p_parser->buf.mem[0];

        // Preamble OK
        if ( BOOT_COM_MSG_PREAMBLE_VAL == (*p_header)->field.preamble )
        {
            // No payload
            if ( 0U == (*p_header)->field.length )
            {
                // Calculate CRC
                crc_calc = boot_com_calc_crc_packet( (*p_header), NULL );

                // Message CRC
                crc_msg = (*p_header)->field.crc;

                // CRC OK
                if ( crc_calc == crc_msg )
                {
                    status = eBOOT_OK;
                }

                // CRC corrupted
                else
                {
                    status = eBOOT_ERROR_CRC;
                    BOOT_DBG_PRINT( "ERROR Message CRC invalid!" );
                }
            }

            // Payload present
            else
            {
                p_parser->mode = eBOOT_PARSER_RCV_PAYLOAD;
            }
        }

        // Preamble not OK --> Wait for timeout to reset buffer
        else
        {
            // No action...
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader parse payload section of the message
*
* @note     This function return:
*               - eBOOT_OK:             When frame is fully received and validated with success
*               - eBOOT_WAR_EMPTY:      When nothing is in buffer or frame is not completely received yet
*               - eBOOT_ERROR_CRC:      When frame data integrity is corrupted
*
* @param[in]    p_parser    - Pointer to parser data
* @param[out]   p_header    - Pointer to header
* @param[out]   pp_payload  - Pointer-pointer to payload
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_parse_rcv_payload(boot_parser_t * const p_parser, const boot_header_t * const p_header, uint8_t ** pp_payload)
{
    boot_status_t   status      = eBOOT_WAR_EMPTY;
    uint8_t         crc_calc    = 0;
    uint8_t         crc_msg     = 0;

    // Complete message payload received
    if ( p_parser->buf.idx == ( p_header->field.length + sizeof( boot_header_t )))
    {
        // Get payload
        *pp_payload = &p_parser->buf.mem[sizeof(boot_header_t)];

        // Calculate CRC
        crc_calc = boot_com_calc_crc_packet( p_header, *pp_payload );

        // Message CRC
        crc_msg = p_header->field.crc;

        // CRC OK
        if ( crc_calc == crc_msg )
        {
            status = eBOOT_OK;
        }

        // CRC corrupted
        else
        {
            status = eBOOT_ERROR_CRC;
            BOOT_DBG_PRINT( "ERROR Message CRC invalid!" );
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Check for frame timeout
*
* @note     Each individual character shall be send within BOOT_CFG_COM_IDLE_TIMEOUT_MS
*           time window.
*
* @param[in]    p_parser    - Pointer to parser data
* @return       timeout     - Timeout flag
*/
////////////////////////////////////////////////////////////////////////////////
static bool boot_timeout_check(boot_parser_t * const p_parser)
{
    bool timeout = false;

    // Check for timeout only when parser is in the middle of work
    // If there is no incoming data then timeout handling is not relevant!
    if ( eBOOT_PARSER_IDLE != p_parser->mode )
    {
        // Timeout
        if ((uint32_t) ( BOOT_GET_SYSTICK() - p_parser->last_timestamp ) >= BOOT_COM_IDLE_TIMEOUT_MS )
        {
            timeout = true;

            // Reset parser
            p_parser->buf.idx = 0;
            p_parser->mode = eBOOT_PARSER_IDLE;
        }
    }

    return timeout;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader communication parser
*
* @brief        This function return:
*               - eBOOT_OK:             When frame is fully received and validated with success
*               - eBOOT_WAR_EMPTY:      When nothing is in buffer or frame is not completely received yet
*               - eBOOT_WAR_FULL:       When reception buffer overflows
*               - eBOOT_ERROR_TIMEOUT:  When frame is not received within specified timeout period
*               - eBOOT_ERROR_CRC:      When frame data integrity is corrupted
*
* @note         If there are two completely received messages inside reception buffer, only first one
*               will be parsed and then calling that function again is mandatory to parse another one.
*
* @param[out]   pp_header   - Pointer-pointer to header, return location of rx buffer starting at header
* @param[out]   pp_payload  - Pointer-pointer to payload, return location of rx buffer starting at payload
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_parse_hndl(boot_header_t ** pp_header, uint8_t ** pp_payload)
{
    boot_status_t status = eBOOT_WAR_EMPTY;

    // Get all data from rx buffers
    while ( eBOOT_OK == boot_if_receive( &g_parser.buf.mem[ g_parser.buf.idx ]))
    {
        // Store timestamp
        g_parser.last_timestamp = BOOT_GET_SYSTICK();

        // Increment buffer index
        status = boot_buf_idx_increment();

        // Buffer OK
        if ( eBOOT_OK == status )
        {
            // Parse message
            status = boot_parse( &g_parser, pp_header, pp_payload );

            // Message completely received
            if  (   ( eBOOT_OK == status )
                ||  ( eBOOT_ERROR_CRC == status ))
            {
                // Reset parser
                g_parser.buf.idx = 0;
                g_parser.mode = eBOOT_PARSER_IDLE;

                // Exit reading data from rx buffer
                break;
            }
        }

        // Rx buffer overflow
        else
        {
            // Reset interface reception buffer
            boot_if_clear_rx_buf();

            // Reset parser
            g_parser.buf.idx = 0;
            g_parser.mode = eBOOT_PARSER_IDLE;

            // Exit reading data from rx buffer
            break;
        }
    }

    // Check for timeout
    if ( true == boot_timeout_check( &g_parser ))
    {
        status = eBOOT_ERROR_TIMEOUT;

        BOOT_DBG_PRINT( "ERROR: Communication Timeout!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Increment parser buffer index
*
* @note In case of buffer full it returns "eBOOT_WAR_FULL"!
*
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_buf_idx_increment(void)
{
    boot_status_t status = eBOOT_OK;

    // Increment received byte counter
    if ( g_parser.buf.idx < ( BOOT_CFG_RX_BUF_SIZE - 1U ))
    {
        g_parser.buf.idx++;
    }
    else
    {
        // Reset buffer index
        g_parser.buf.idx = 0U;

        // Buffer full
        status = eBOOT_WAR_FULL;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader communication parser actions
*
* @param[in]    p_parser    - Pointer to bootloader parser
* @param[out]   pp_header   - Pointer-pointer to header, return location of rx buffer starting at header
* @param[out]   pp_payload  - Pointer-pointer to payload, return location of rx buffer starting at payload
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static boot_status_t boot_parse(boot_parser_t * const p_parser, boot_header_t ** pp_header, uint8_t ** pp_payload)
{
    boot_status_t status = eBOOT_WAR_EMPTY;

    // Parser FSM
    switch( p_parser->mode )
    {
        case eBOOT_PARSER_IDLE:
            status = boot_parse_idle( p_parser );
            break;

        case eBOOT_PARSER_RCV_HEADER:
            status = boot_parse_rcv_header( p_parser, pp_header );
            break;

        case eBOOT_PARSER_RCV_PAYLOAD:
            status = boot_parse_rcv_payload( p_parser, *pp_header, pp_payload );
            break;

        default:
            BOOT_ASSERT( 0 );
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Connect message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_connect(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_header;
    (void) p_payload;

    // Raise callback
    boot_com_connect_msg_rcv_cb();
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Connect Response message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_connect_rsp(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_payload;

    // Raise callback
    boot_com_connect_rsp_msg_rcv_cb( p_header->field.status );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Prepare message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_prepare(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_header;

    // Check for correct lenght
    if ( p_header->field.length == sizeof( ver_image_header_t ))
    {
        // Raise callback
        boot_com_prepare_msg_rcv_cb((const ver_image_header_t *) p_payload );
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Prepare Response message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_prepare_rsp(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_payload;

    // Raise callback
    boot_com_prepare_rsp_msg_rcv_cb( p_header->field.status );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Flash Data message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_flash(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Raise callback
    boot_com_flash_msg_rcv_cb( p_payload, p_header->field.length );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Flash Data Response message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_flash_rsp(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_payload;

    // Raise callback
    boot_com_flash_rsp_msg_rcv_cb( p_header->field.status );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Exit message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_exit(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_header;
    (void) p_payload;

    // Raise callback
    boot_com_exit_msg_rcv_cb();
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Exit Response message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_exit_rsp(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_payload;

    // Raise callback
    boot_com_exit_rsp_msg_rcv_cb( p_header->field.status );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Info message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_info(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    // Unused
    (void) p_header;
    (void) p_payload;

    // Raise callback
    boot_com_info_msg_rcv_cb();
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Bootloader Info Response message parser
*
* @param[in]    p_header    - Pointer to message header
* @param[in]    p_payload   - Pointer to message payload
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void boot_parse_info_rsp(const boot_header_t * const p_header, const uint8_t * const p_payload)
{
    uint32_t boot_ver = 0U;

    // Parse bootloader version
    memcpy( &boot_ver, p_payload, sizeof( boot_ver ));

    // Raise callback
    boot_com_info_rsp_msg_rcv_cb( boot_ver, p_header->field.status );
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

////////////////////////////////////////////////////////////////////////////////
/**
*       Handle Bootloader communication
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_hndl(void)
{
    boot_status_t   status      = eBOOT_OK;
    static boot_header_t * p_header    = NULL;
    static uint8_t *       p_payload   = NULL;

    // Parse received messages
    status = boot_parse_hndl((boot_header_t **) &p_header, (uint8_t**) &p_payload );

    // Msg received OK
    if ( eBOOT_OK == status )
    {
        // Parse received message
        for ( uint32_t cmd = 0; cmd < gu32_parse_table_size; cmd++ )
        {
            if (p_header->field.command == g_parse_table[cmd].cmd )
            {
                // Raise parsing command
                g_parse_table[cmd].pf_parse( p_header, p_payload );
                break;
            }
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get timestamp of last received byte
*
* @return       last_timestamp - Timestamp of last rx byte in ms
*/
////////////////////////////////////////////////////////////////////////////////
uint32_t boot_com_get_last_rx_timestamp(void)
{
    return g_parser.last_timestamp;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Connect Message
*
* @note     Shall only be used by Boot Manager!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_connect(void)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOT_MANAGER;
    header.field.command    = eBOOT_MSG_CMD_CONNECT;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Connect Response Message
*
* @note     Shall only be used by Bootloader!
*
* @param[in]    msg_status  - Response message status
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_connect_rsp(const boot_msg_status_t msg_status)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOTLOADER;
    header.field.command    = eBOOT_MSG_CMD_CONNECT_RSP;
    header.field.status     = msg_status;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Prepare Message
*
* @note     Shall only be used by Boot Manager!
*
* @param[in]    fw_size - Size of firmware in bytes
* @param[in]    fw_ver  - Firmware version
* @param[in]    hw_ver  - Hardware version
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_prepare(const uint32_t fw_size, const uint32_t fw_ver, const uint32_t hw_ver)
{
    boot_status_t           status  = eBOOT_OK;
    boot_header_t           header  = { .U = 0U };
    boot_prepare_payload_t  payload = {0};

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOT_MANAGER;
    header.field.command    = eBOOT_MSG_CMD_PREPARE;

    // Assemble payload
    payload.fw_size = fw_size;
    payload.fw_ver  = fw_ver;
    payload.hw_ver  = hw_ver;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, (uint8_t*) &payload );

    // Send command
    status  = boot_if_transmit( &header.U, sizeof( boot_header_t ));
    status |= boot_if_transmit((uint8_t*) &payload, sizeof( boot_prepare_payload_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Prepare Response Message
*
* @note     Shall only be used by Bootloader!
*
* @param[in]    msg_status  - Response message status
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_prepare_rsp(const boot_msg_status_t msg_status)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOTLOADER;
    header.field.command    = eBOOT_MSG_CMD_PREPARE_RSP;
    header.field.status     = msg_status;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Flash Data Message
*
* @note     Shall only be used by Boot Manager!
*
* @param[in]    p_data  - Flash binary data
* @param[in]    size    - Size of flash binary data in bytes
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_flash(const uint8_t * const p_data, const uint16_t size)
{
    boot_status_t status  = eBOOT_OK;
    boot_header_t header  = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = size;
    header.field.source     = eCOM_MSG_SRC_BOOT_MANAGER;
    header.field.command    = eBOOT_MSG_CMD_FLASH;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, p_data );

    // Send command
    status  = boot_if_transmit( &header.U, sizeof( boot_header_t ));
    status |= boot_if_transmit( p_data, size );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Flash Data Response Message
*
* @note     Shall only be used by Bootloader!
*
* @param[in]    msg_status  - Response message status
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_flash_rsp(const boot_msg_status_t msg_status)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOTLOADER;
    header.field.command    = eBOOT_MSG_CMD_FLASH_RSP;
    header.field.status     = msg_status;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Exit Message
*
* @note     Shall only be used by Boot Manager!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_exit(void)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOT_MANAGER;
    header.field.command    = eBOOT_MSG_CMD_EXIT;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Exit Response Message
*
* @note     Shall only be used by Bootloader!
*
* @param[in]    msg_status  - Response message status
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_exit_rsp(const boot_msg_status_t msg_status)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOTLOADER;
    header.field.command    = eBOOT_MSG_CMD_EXIT_RSP;
    header.field.status     = msg_status;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Info Message
*
* @note     Shall only be used by Boot Manager!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_info(void)
{
    boot_status_t status = eBOOT_OK;
    boot_header_t header = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = 0U;
    header.field.source     = eCOM_MSG_SRC_BOOT_MANAGER;
    header.field.command    = eBOOT_MSG_CMD_INFO;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, NULL );

    // Send command
    status = boot_if_transmit( &header.U, sizeof( boot_header_t ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Send Information Response Message
*
* @note     Shall only be used by Bootloader!
*
* @param[in]    boot_ver    - Bootloader version
* @param[in]    msg_status  - Response message status
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
boot_status_t boot_com_send_info_rsp(const uint32_t boot_ver, const boot_msg_status_t msg_status)
{
    boot_status_t status  = eBOOT_OK;
    boot_header_t header  = { .U = 0U };

    // Assemble command
    header.field.preamble   = BOOT_COM_MSG_PREAMBLE_VAL;
    header.field.length     = sizeof(boot_ver);
    header.field.source     = eCOM_MSG_SRC_BOOTLOADER;
    header.field.command    = eBOOT_MSG_CMD_INFO_RSP;
    header.field.status     = msg_status;

    // Calculate CRC
    header.field.crc = boot_com_calc_crc_packet( &header, (uint8_t*) &boot_ver );

    // Send command
    status  = boot_if_transmit( &header.U, sizeof( boot_header_t ));
    status |= boot_if_transmit((uint8_t*) &boot_ver, sizeof(uint32_t) );

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
* @param[in]    msg_status - Status of connect command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_connect_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{
    // Unused params
    (void) msg_status;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Prepare Bootloader Message Reception Callback
*
* @param[in]    p_head - Image header
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_prepare_msg_rcv_cb(const ver_image_header_t * const p_head)
{
    // Unused params
    (void) p_head;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Prepare Response Bootloader Message Reception Callback
*
* @param[in]    msg_status - Status of prepare command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_prepare_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{
    // Unused params
    (void) msg_status;

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
* @param[in]    msg_status - Status of flash command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_flash_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{
    // Unused params
    (void) msg_status;

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
* @param[in]    msg_status - Status of exit command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_exit_rsp_msg_rcv_cb(const boot_msg_status_t msg_status)
{
    // Unused params
    (void) msg_status;

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
* @param[in]    msg_status  - Bootloader version
* @param[in]    status      - Status of info command
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
__BOOT_CFG_WEAK__ void boot_com_info_rsp_msg_rcv_cb(const uint32_t boot_ver, const boot_msg_status_t msg_status)
{
    // Unused params
    (void) boot_ver;
    (void) msg_status;

    /**
     *  Leave empty for user application purposes...
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
