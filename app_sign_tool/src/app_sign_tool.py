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



# TODO: 
#       1. Application header start & size customization
#       2. 
#
#

#################################################################################################
##  DEFINITIONS
#################################################################################################

# Script version
MAIN_SCRIPT_VER     = "V0.1.0"

# Tool description
TOOL_DESCRIPTION = \
"Firmware Application Signature Tool %s" % MAIN_SCRIPT_VER

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
        self.file = file

        try:
            if os.path.isfile(self.file):
                self.file = open( self.file, access )
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
    
    file_path_in, file_path_out = arg_parser()

    print( "Inputed file: %s" % file_path_in )
    print( "Outputed file: %s" % file_path_out )

    print( file_path_in.split(".") )

    if "bin" != file_path_in.split(".")[1] or "bin" != file_path_out.split(".")[1]:
        print( "ERROR: Invalid file format" ) 
    
    else:
        # Copy inputed binary file
        shutil.copyfile( file_path_in, file_path_out )

        # Open outputed binary file
        #bin_out_file = open( file_path_out, "ab")

        out_file = BinFile( file_path_out, access=BinFile.READ_WRITE)

        out_file.write( 0xFF, [0xAA, 0x12, 0x51] )

        for byte in out_file.read( 0xFF, 4 ):
            print( "%02X" % byte )


        #bin_out_file.write( bytearray( [0x11, 0xAA, 0x55] ) )

        # Calculate CRC
        # TODO:

        # Count application size
        # TODO: 

        # Open file
        with open(file_path_in, mode='rb') as file: 
            fileContent = file.read()

            for n, byte in enumerate( fileContent ):
                pass














        print("")
        print("====================================================================")
        print("     %s" % TOOL_DESCRIPTION )
        print("====================================================================")
        print("Firmware image successfuly signed!\n")









#################################################################################################
##  MAIN ENTRY
#################################################################################################   
if __name__ == "__main__":
    main()


#################################################################################################
##  END OF FILE
#################################################################################################  

