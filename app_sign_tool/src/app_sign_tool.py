## Copyright (c) 2023 Ziga Miklosic
## All Rights Reserved
#################################################################################################
##
## @file:       app_sign_tool.py
## @brief:      This script fills up application header informations
## @date:		22.08.2023
## @author:		Ziga Miklosic
## @version:    V0.1.0
##
#################################################################################################

#################################################################################################
##  IMPORTS
#################################################################################################
import argparse
import shutil

import os
import struct

#################################################################################################
##  DEFINITIONS
#################################################################################################

# Script version
MAIN_SCRIPT_VER     = "V0.1.0"

# Tool description
TOOL_DESCRIPTION = \
"Firmware Application Signature Tool %s" % MAIN_SCRIPT_VER

# Expected application header version
APP_HEADER_VER_EXPECTED         = 1

# Application header addresses
APP_HEADER_APP_SIZE_ADDR        = 0x08
APP_HEADER_APP_CRC_ADDR         = 0x0C
APP_HEADER_VER_ADDR             = 0xFE
APP_HEADER_CRC_ADDR             = 0xFF

# Application header size in bytes
APP_HEADER_SIZE_BYTE            = 0x100

#################################################################################################
##  FUNCTIONS
#################################################################################################

# ===============================================================================
# @brief:   Argument parser
#
# @return:       TODO:...
# ===============================================================================
def arg_parser():

    # Arg parser
    parser = argparse.ArgumentParser( 	description=TOOL_DESCRIPTION, 
                                        epilog="Enjoy the program!")

    # Add arguments
    parser.add_argument("-f",   help="Input binary file",               metavar="bin_in",           type=str,   required=True )
    parser.add_argument("-o",   help="Output signed binary file",       metavar="bin_out",          type=str,   required=True )

    # Get args
    args = parser.parse_args()

    # Convert namespace to dict
    args = vars(args)

    # Get arguments
    file_in     = args["f"]
    file_out    = args["o"]

    return file_in, file_out

# ===============================================================================
# @brief  Calculate CRC-32
#
# @param[in]    data    - Inputed data
# @return       crc32   - Calculate CRC-32
# ===============================================================================
def calc_crc32(data):
    poly = 0x04C11DB7
    seed = 0x10101010
    crc32 = seed

    for byte in data:
        crc32 = (( crc32 ^ byte ) & 0xFFFFFFFF )

        for n in range( 32 ):

            if crc32 & 0x80000000:
                crc32 = (( crc32 << 1 ) ^ poly )
            else:
                crc32 = ( crc32 << 1 );

    return crc32 & 0xFFFFFFFF 

# ===============================================================================
# @brief  Calculate CRC-8
#
# @param[in]    data    - Inputed data
# @return       crc8    - Calculate CRC8
# ===============================================================================
def calc_crc8(data):
    poly = 0x07
    seed = 0xB6
    crc8 = seed

    for byte in data:
        crc8 = (( crc8 ^ byte ) & 0xFF )

        for n in range( 8 ):

            if crc8 & 0x80:
                crc8 = (( crc8 << 1 ) ^ poly )
            else:
                crc8 = ( crc8 << 1 );

    return crc8 & 0xFF

# ===============================================================================
# @brief  Binary file Class
# ===============================================================================
class BinFile:

    # Access type
    READ_WRITE  = "r+b"      # Puts pointer to start of file 
    WRITE_ONLY  = "wb"       # Erase complete file
    READ_ONLY   = "rb"
    APPEND      = "a+b"      # This mode puts pointer to EOF. Access: Read & Write

    # ===============================================================================
    # @brief  Binary file constructor
    #
    # @param[in]    file    - File name
    # @param[in]    access  - Access level
    # @return       void
    # ===============================================================================
    def __init__(self, file, access=READ_ONLY):

        # Store file name
        self.file_name = file

        try:
            if os.path.isfile(file):
                self.file = open( file, access )
        except Exception as e:
            print(e)

    # ===============================================================================
    # @brief  Write to binary file
    #
    # @param[in]    addr    - Address to write to
    # @param[in]    val     - Value to write as list
    # @return       void
    # ===============================================================================
    def write(self, addr, val):
        self.__set_ptr(addr)
        self.file.write( bytearray( val ))

    # ===============================================================================
    # @brief  Read from binary file
    #
    # @param[in]    addr    - Address to read from
    # @param[in]    size    - Sizeof read in bytes
    # @return       data    - Readed data
    # ===============================================================================
    def read(self, addr, size):
        self.__set_ptr(addr)
        data = self.file.read(size)
        return data
    
    # ===============================================================================
    # @brief  Get size of binary file
    #
    # @return       data    - Readed data
    # ===============================================================================    
    def size(self):
        return len( self.read( 0, None ))

    # ===============================================================================
    # @brief  Set file pointer
    #
    # @note     Pointer is being evaluated based on binary file value
    #
    # @param[in]    offset  - Pointer offset
    # @return       void
    # ===============================================================================
    def __set_ptr(self, offset):
        self.file.seek(offset)                    


# ===============================================================================
# @brief:  Main 
#
# @return: void
# ===============================================================================
def main():
    
    # Get arguments
    file_path_in, file_path_out = arg_parser()

    if "bin" != file_path_in.split(".")[1] or "bin" != file_path_out.split(".")[1]:
        print( "ERROR: Invalid file format" )
        raise RuntimeError 
    
    else:
        # Copy inputed binary file
        shutil.copyfile( file_path_in, file_path_out )

        # Open outputed binary file
        out_file = BinFile( file_path_out, access=BinFile.READ_WRITE)

        # Is application header version supported
        if APP_HEADER_VER_EXPECTED == out_file.read( APP_HEADER_VER_ADDR, 1 )[0]:

            # Count application size
            app_size = out_file.size()

            # Calculate application CRC
            app_crc = calc_crc32( out_file.read( 0, None ))

            # Write app lenght into application header
            out_file.write( APP_HEADER_APP_SIZE_ADDR, struct.pack('i', int(app_size)) )

            # Write app CRC into application header
            out_file.write( APP_HEADER_APP_CRC_ADDR, struct.pack('i', int(app_crc)) )

            # Calculate application header CRC
            app_header_crc = calc_crc8( out_file.read( 0, APP_HEADER_SIZE_BYTE ))

            # Write application header crc
            out_file.write( APP_HEADER_CRC_ADDR, [app_header_crc] )

            print("")
            print("====================================================================")
            print("     %s" % TOOL_DESCRIPTION )
            print("====================================================================")
            print("Firmware image successfuly signed!\n")

        else:
            print( "ERROR: Application header version not supported!" ) 
            raise RuntimeError 

#################################################################################################
##  MAIN ENTRY
#################################################################################################   
if __name__ == "__main__":
    main()

#################################################################################################
##  END OF FILE
#################################################################################################  

