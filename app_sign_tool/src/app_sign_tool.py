## Copyright (c) 2023 Ziga Miklosic
## All Rights Reserved
#################################################################################################
##
## @file:       app_sign_tool.py
## @brief:      This script fills up application header informations
## @date:		20.08.2024
## @author:		Ziga Miklosic
## @version:    V0.4.0
##
#################################################################################################

#################################################################################################
##  IMPORTS
#################################################################################################
import argparse
import shutil

import os
import struct

import binascii
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util import Counter

import hashlib
from ecdsa import SigningKey
from ecdsa.util import sigencode_string
from binascii import hexlify


#################################################################################################
##  DEFINITIONS
#################################################################################################

# Script version
MAIN_SCRIPT_VER     = "V0.4.0"

# Tool description
TOOL_DESCRIPTION = \
"Firmware Application Signature Tool %s" % MAIN_SCRIPT_VER

# Expected application header version
APP_HEADER_VER_EXPECTED         = 1

# Application header addresses
APP_HEADER_CRC_ADDR             = 0x00
APP_HEADER_VER_ADDR             = 0x01
APP_HEADER_IMG_TYPE_ADDR        = 0x02  # Image type [0-Application, 1-Custom]

# Application header data fields
# For more info about image header look at Revision module specifications: "revision\doc\Revision_Specifications.xlsx"
APP_HEADER_SW_VER_ADDR          = 0x08  # Software version
APP_HEADER_HW_VER_ADDR          = 0x0C  # Hardware version
APP_HEADER_SIZE_ADDR            = 0x10  # Image size in bytes
APP_HEADER_IMAGE_ADDR_ADDR      = 0x14  # Image address in case of "custom", for application left empty
APP_HEADER_IMAGE_CRC_ADDR       = 0x18  # Image CRC32
APP_HEADER_ENC_TYPE_ADDR        = 0x1C  # Encryption type [0-None, 1-AES-CTR]
APP_HEADER_SIG_TYPE_ADDR        = 0x1D  # Signature type [0-None, 1-ECSDA]
APP_HEADER_SIGNATURE_ADDR       = 0x1E  # Signature of the image. Size: 64 bytes
APP_HEADER_HASH_ADDR            = 0x5E  # Image hash - SHA256. Size: 32 bytes
APP_HEADER_GIT_SHA_ADDR         = 0x7E  # Git commit hash. Size: 8 byte

# Encryption types
ENC_TYPE_NONE = 0
ENC_TYPE_AES_CTR = 1

# Digital signature types
DIG_SIG_TYPE_NONE = 0
DIG_SIG_TYPE_ECDSA = 1

# Application header size in bytes
APP_HEADER_SIZE_BYTE            = 256 # bytes

# Enable padding
PAD_ENABLE                      = True

# Pad value
PAD_VALUE                       = 0x00

# Pad block
# Size of block to be padded. If application binary is not
# wihtin that block size it will be padded to be multiple of
# pad block size.
PAD_BLOCK_SIZE_BYTE             = 64 #bytes 


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
    parser.add_argument("-f", help="Input binary file", metavar="bin_in", type=str, required=True )
    parser.add_argument("-o", help="Output binary file. Filling only image (application) header", metavar="bin_out", type=str, required=True )
    parser.add_argument("-s", help="Signing (ECSDA) binary file", action="store_true", required=False )
    parser.add_argument("-private_key", help="Private key for signature", metavar="private_key", required=False )    
    parser.add_argument("-c", help="Encrypt (AES-CTR) binary file", action="store_true", required=False )

    # Get args
    args = parser.parse_args()

    # Convert namespace to dict
    args = vars(args)

    # Get arguments
    file_in     = args["f"]
    file_out    = args["o"]

    return file_in, file_out, args["c"], args["s"], args["private_key"]

# ===============================================================================
# @brief  Calculate CRC-32
#
# @param[in]    data    - Inputed data
# @return       crc32   - Calculated CRC-32
# ===============================================================================
def calc_crc32(data):
    poly = 0x04C11DB7
    seed = 0x10101010
    crc32 = seed

    for byte in data:
        crc32 = (( crc32 ^ byte ) & 0xFFFFFFFF )

        for n in range( 32 ):
            if 0x80000000 == ( crc32 & 0x80000000 ):
                crc32 = (( crc32 << 1 ) ^ poly )
            else:
                crc32 = ( crc32 << 1 );

    return crc32 & 0xFFFFFFFF 

# ===============================================================================
# @brief  Calculate CRC-8
#
# @param[in]    data    - Inputed data
# @return       crc8    - Calculated CRC8
# ===============================================================================
def calc_crc8(data):
    poly = 0x07
    seed = 0xB6
    crc8 = seed

    for byte in data:
        crc8 = (( crc8 ^ byte ) & 0xFF )

        for n in range( 8 ):
            if 0x80 == ( crc8 & 0x80 ):
                crc8 = (( crc8 << 1 ) ^ poly )
            else:
                crc8 = ( crc8 << 1 );

    return crc8 & 0xFF

# ===============================================================================
# @brief  Crypt plaing data to AES with key and initial vector
#
# @param[in]    plain_data      - Inputed non-cryptic data
# @return       crypted_data    - Outputed cryptic data
# ===============================================================================
def aes_encode(plain_data):

    # AES Key and IV
    key = b"\x1b\x0e\x6c\x90\x34\xda\x00\x32\x33\xdd\x54\x54\x09\xcf\x23\x41"
    iv = b"\x45\xf2\x34\x12\xa3\x32\x34\xfd\xab\xcc\x1c\xed\x1c\x41\x20\x0f"

    # Create cipher
    ctr = Counter.new(128, initial_value=int(binascii.hexlify(iv), 16))
    cipher = AES.new(key, AES.MODE_CTR, counter=ctr)

    # Encode
    return cipher.encrypt( bytearray( plain_data ))

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
            # Open file if exsist
            if os.path.isfile(file):
                self.file = open( file, access )
            else:
                self.file = open( file, BinFile.WRITE_ONLY )
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


def gen_binary_signature(data, key_filename):

    with open(key_filename, "r") as f:
        key_pem = f.read()

    key = SigningKey.from_pem(key_pem)
    sig = key.sign_deterministic(data, hashfunc=hashlib.sha256, sigencode=sigencode_string)

    return bytearray( sig )


# ===============================================================================
# @brief:  Main 
#
# @return: void
# ===============================================================================
def main():
    
    # Intro informations
    print("====================================================================")
    print("     %s" % TOOL_DESCRIPTION )
    print("====================================================================")

    # Get arguments
    file_path_in, file_path_out, crypto_en, sign_en, private_key = arg_parser()

    # Check for correct file extension 
    if "bin" != file_path_in.split(".")[-1] or "bin" != file_path_out.split(".")[-1]:
        print( "ERROR: Invalid file format" )
        raise RuntimeError 
    
    if True == sign_en and None == private_key:
        print( "ERROR: Missing private key when application signing is enabled!" )
        raise RuntimeError 
    
    # Both files are binary
    else:
        # Copy inputed binary file
        shutil.copyfile( file_path_in, file_path_out )

        # Open outputed binary file
        out_file = BinFile( file_path_out, access=BinFile.READ_WRITE)

        # Is application header version supported
        if APP_HEADER_VER_EXPECTED == out_file.read( APP_HEADER_VER_ADDR, 1 )[0]:

            # Count application size
            app_size = out_file.size()


            ######################################################################################
            ## IMAGE PADDING
            ######################################################################################

            # Is pad enable
            if PAD_ENABLE:

                # Calculate number of bytes need to be padded
                num_of_bytes_to_pad = ( PAD_BLOCK_SIZE_BYTE - ( app_size % PAD_BLOCK_SIZE_BYTE ))

                # Binary needs to be padded
                if ( num_of_bytes_to_pad < PAD_BLOCK_SIZE_BYTE ):

                    # Pad file
                    out_file.write( app_size, [ PAD_VALUE for _ in range( num_of_bytes_to_pad ) ])

                    print("INFO: Binary padded with %d byte!" % num_of_bytes_to_pad )

            # Get application size
            ## NOTE: Application size exclude size of header!
            app_size = ( out_file.size() - APP_HEADER_SIZE_BYTE )

            # Write app lenght into application header
            out_file.write( APP_HEADER_SIZE_ADDR, struct.pack('I', int(app_size)))


            ######################################################################################
            ## IMAGE SIGNING
            ######################################################################################

            # Signing application
            if sign_en:
                
                # Create signature
                signature = gen_binary_signature( out_file.read( APP_HEADER_SIZE_BYTE, None ), private_key )

                # Add signature to application header
                out_file.write( APP_HEADER_SIGNATURE_ADDR, signature )

                # Add signature type
                out_file.write( APP_HEADER_SIG_TYPE_ADDR, [DIG_SIG_TYPE_ECDSA] )

                # Succes info
                print("SUCCESS: Firmware image successfully signed!")  


            ######################################################################################
            ## IMAGE ENCRYPTION
            ######################################################################################

            # Add encryption type if encryption enabled
            if crypto_en:

                # Set encryption type
                out_file.write( APP_HEADER_ENC_TYPE_ADDR, [ENC_TYPE_AES_CTR] )  

                # Encrypt application part, skip application header
                #file_crypted_out.write( APP_HEADER_SIZE_BYTE, aes_encode( out_file.read( APP_HEADER_SIZE_BYTE, out_file.size() - APP_HEADER_SIZE_BYTE )))
                out_file.write( APP_HEADER_SIZE_BYTE, aes_encode( out_file.read( APP_HEADER_SIZE_BYTE, None )))

                # Succes info
                print("SUCCESS: Firmware image successfully crypted!")    


            ######################################################################################
            ## CALCULATE APPLICATION PART OF IMAGE CRC
            ######################################################################################

            # Calculate application CRC
            # NOTE: Start calculation after application header and after crypting of the image!
            app_crc = calc_crc32( out_file.read( APP_HEADER_SIZE_BYTE, None ))

            # Write app CRC into application header
            out_file.write( APP_HEADER_IMAGE_CRC_ADDR, struct.pack('I', int(app_crc)))


            ######################################################################################
            ## LAST STEP IS TO CALCULATE IMAGE (APP) HEADER CRC 
            ######################################################################################

            # Calculate application header CRC after all fields are header fields are setup!
            # NOTE: Ignore first field as it is CRC value itself!
            app_header_crc = calc_crc8( out_file.read( 1, APP_HEADER_SIZE_BYTE - 1 ))

            # Write application header crc
            out_file.write( APP_HEADER_CRC_ADDR, [app_header_crc] )

            # Success info
            print("SUCCESS: Image (application) header successfully filled!")            

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

