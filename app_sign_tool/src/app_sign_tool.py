## Copyright (c) 2025 Ziga Miklosic
## All Rights Reserved
#################################################################################################
##
## @file:       app_sign_tool.py
## @brief:      This script fills up application header informations
## @date:		09.06.2025
## @author:		Ziga Miklosic
## @version:    V1.1.0
##
#################################################################################################

#################################################################################################
##  IMPORTS
#################################################################################################
import argparse
import shutil
import subprocess
import argparse
import subprocess
import datetime
import os
import sys
import platform
import struct

import binascii
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util import Counter

import hashlib
from ecdsa import SigningKey
from ecdsa.util import sigencode_string
from binascii import hexlify
from intelhex import IntelHex

#################################################################################################
##  DEFINITIONS
#################################################################################################

# Script version
MAIN_SCRIPT_VER     = "V1.1.0"

# Tool description
TOOL_DESCRIPTION = \
"Firmware Application Signature Tool %s" % MAIN_SCRIPT_VER

# Expected application header version
APP_HEADER_VER_EXPECTED         = 1

# Application header addresses
APP_HEADER_CRC_ADDR             = 0x00
APP_HEADER_VER_ADDR             = 0x01
APP_HEADER_IMG_TYPE_ADDR        = 0x02  # Image type [0-Application, 1-Custom]

# Image type
class ImageType():
    APPLICATION = 0
    CUSTOM      = 1

# Application header data fields
# For more info about image header look at Revision module specifications: "revision\doc\Revision_Specifications.xlsx"
APP_HEADER_SW_VER_ADDR          = 0x08  # Software version
APP_HEADER_HW_VER_ADDR          = 0x0C  # Hardware version
APP_HEADER_IMAGE_SIZE_ADDR      = 0x10  # Image size in bytes
APP_HEADER_IMAGE_ADDR_ADDR      = 0x14  # Image start address
APP_HEADER_IMAGE_CRC_ADDR       = 0x18  # Image CRC32
APP_HEADER_ENC_TYPE_ADDR        = 0x1C  # Encryption type [0-None, 1-AES-CTR]
APP_HEADER_SIG_TYPE_ADDR        = 0x1D  # Signature type [0-None, 1-ECSDA]
APP_HEADER_SIGNATURE_ADDR       = 0x1E  # Signature of the image. Size: 64 bytes
APP_HEADER_HASH_ADDR            = 0x5E  # Image hash - SHA256. Size: 32 bytes
APP_HEADER_GIT_SHA_ADDR         = 0x7E  # Git commit hash. Size: 8 byte
APP_HEADER_ENC_IMAGE_CRC_ADDR   = 0x86  # Encrypted image CRC32

# Encryption types
class EncType():
    NONE    = 0
    AES_CTR = 1

# Digital signature types
class SigType():
    NONE    = 0
    ECDSA   = 1

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

# Output directory name
OUTPUT_DIR_NAME                = "Outputs"

#################################################################################################
##  FUNCTIONS
#################################################################################################

# ===============================================================================
# @brief:   Get current year
#
# @return:  current year
# ===============================================================================  
def get_year():
    return int(datetime.datetime.now().year)

# ===============================================================================
# @brief:   Get current month
#
# @return:  current month
# =============================================================================== 
def get_month():
    return int(datetime.datetime.now().month)

# ===============================================================================
# @brief:   Get current day
#
# @return:  current day
# =============================================================================== 
def get_day():
    return int(datetime.datetime.now().day)

# ===============================================================================
# @brief:   Get current hour
#
# @return:  current hour
# =============================================================================== 
def get_hour():
    return int(datetime.datetime.now().hour)

# ===============================================================================
# @brief:   Get current minute
#
# @return:  current minute
# =============================================================================== 
def get_minute():
    return int(datetime.datetime.now().minute)

# ===============================================================================
# @brief:   Get current second
#
# @return:  current second
# =============================================================================== 
def get_second():
    return int(datetime.datetime.now().second)


# ===============================================================================
# @brief:   Argument parser
#
# @return:       various arguments
# ===============================================================================
def arg_parser():

    # Arg parser
    parser = argparse.ArgumentParser( 	description=TOOL_DESCRIPTION, 
                                        epilog="Enjoy the program!")

    # Add arguments
    parser.add_argument("-f",   help="Input binary file",             metavar="bin_in",           type=str,   required=True )
    parser.add_argument("-a",   help="Start application address",     metavar="app_addr_start",   type=str,   required=True )
    parser.add_argument("-s",   help="Signing (ECSDA) binary file",   action="store_true",                    required=False )
    parser.add_argument("-k",   help="Private key for signature",     metavar="private_key",                  required=False )    
    parser.add_argument("-c",   help="Encrypt (AES-CTR) binary file", action="store_true",                    required=False )
    parser.add_argument("-git", help="Store Git SHA to image header", action="store_true",                    required=False )

    # Get args
    args = parser.parse_args()

    # Convert namespace to dict
    args = vars(args)

    # Get arguments
    file_in         = args["f"]

    # Convert to number
    app_addr_start  = int(args["a"], 16)

    return file_in, app_addr_start, args["c"], args["s"], args["k"], args["git"]

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
# @brief:   Find bootloader
#
# @return:      args
# ===============================================================================
def find_bootloader_image():

    boot_file = None

    # Get this file path
    if getattr(sys, 'frozen', False):  # Check if the program is run as a bundle
        file_dir = os.path.dirname(sys.executable)  # Get the directory of the executable
    else:
        # If run as a script, get the directory of the script
        file_dir = os.path.dirname(os.path.abspath(__file__))

    # Get path to DFU_BLE configuration directory
    dfu_dir = "/".join( file_dir.split("\\")[0:file_dir.split("\\").index("boot")+1] )

    # Check all files is dfu module config space
    for file in os.listdir( dfu_dir ):
        try:
            # Get file extension
            file_ext = file.split(".")[1]

            # Check for HEX images
            if "hex" == file_ext:
                boot_file = dfu_dir + "/" + file
        except:
            pass

    return boot_file

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

# ===============================================================================
# @brief  Generate ECDSA signature
#
# @note     Outputed signature is 64 bytes long!
#
# @param[in]    data        - Inputed data to sign
# @param[in]    key_file    - Private key file location
# @return       sig         - Digital signature
# ===============================================================================
def generate_signature(data, key_file):

    with open(key_file, "r") as f:
        key_pem = f.read()

    key = SigningKey.from_pem(key_pem)
    sig = key.sign_deterministic(data, hashfunc=hashlib.sha256, sigencode=sigencode_string)

    return bytearray( sig )

# ===============================================================================
# @brief  Generate SHA256 hash
#
# @note     Outputed hash is 32 bytes long!
#
# @param[in]    data - Inputed data to hash 
# @return       hash - Hash of inputed data
# ===============================================================================
def generate_hash(data):

    # Create an SHA-256 hash object
    sha256_hash = hashlib.sha256()

    # Update the hash object with the data
    sha256_hash.update(data)

    # Get the hexadecimal digest of the hash
    hash = sha256_hash.digest()

    return bytearray( hash )

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
    file_path_in, app_addr_start, crypto_en, sign_en, private_key, git_en = arg_parser()

    # Check for correct file extension 
    if "bin" != file_path_in.split(".")[-1]:
        print( "ERROR: Invalid file format" )
        raise RuntimeError 
    
    if True == sign_en and None == private_key:
        print( "ERROR: Missing private key when application signing is enabled!" )
        raise RuntimeError 
    
    # Both files are binary
    else:

        # Get software version from inputed binary file
        in_file = BinFile( file_path_in, access=BinFile.READ_ONLY)
        sw_ver = in_file.read( APP_HEADER_SW_VER_ADDR, 4 )

        # Add version to output directory name
        global OUTPUT_DIR_NAME
        if 0 != sw_ver[0]:
            OUTPUT_DIR_NAME += "/V%s.%s.%s.%s" % (sw_ver[3], sw_ver[2], sw_ver[1], sw_ver[0])
        else:
            OUTPUT_DIR_NAME += "/V%s.%s.%s" % (sw_ver[3], sw_ver[2], sw_ver[1])

        # Check if output directory exists, if not, create it
        if not os.path.exists(OUTPUT_DIR_NAME):
            os.makedirs(OUTPUT_DIR_NAME)

        # Create output file
        if 0 != sw_ver[0]:
            file_path_out = OUTPUT_DIR_NAME + "/" + file_path_in.split("/")[-1].split(".")[0] + "__V%s_%s_%s_%s" % (sw_ver[3], sw_ver[2], sw_ver[1], sw_ver[0]) + ".bin"
        else:
            file_path_out = OUTPUT_DIR_NAME + "/" + file_path_in.split("/")[-1].split(".")[0] + "__V%s_%s_%s" % (sw_ver[3], sw_ver[2], sw_ver[1]) + ".bin"

        # Copy inputed binary file
        shutil.copyfile( file_path_in, file_path_out )

        # Open outputed binary file
        out_file = BinFile( file_path_out, access=BinFile.READ_WRITE)

        # Is application header version supported
        if APP_HEADER_VER_EXPECTED == out_file.read( APP_HEADER_VER_ADDR, 1 )[0]:

            ######################################################################################
            ## GENERAL HEADER INFO
            ######################################################################################

            # Preparing image header for application
            out_file.write( APP_HEADER_IMG_TYPE_ADDR, [ImageType.APPLICATION] )

            # Write app start address into application header
            out_file.write( APP_HEADER_IMAGE_ADDR_ADDR, struct.pack('I', int(app_addr_start)))

            # Git SHA info
            if git_en:

                # Get commit SHA
                GIT_COMMIT_SHA_CMD = "git rev-parse HEAD"
                commit_sha = subprocess.check_output( GIT_COMMIT_SHA_CMD )[:-1] 

                # Write Git SHA to application header
                out_file.write( APP_HEADER_GIT_SHA_ADDR, commit_sha[:8] )

            ######################################################################################
            ## IMAGE PADDING
            ######################################################################################

            # Count application size
            app_size = out_file.size()

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
            out_file.write( APP_HEADER_IMAGE_SIZE_ADDR, struct.pack('I', int(app_size)))


            ######################################################################################
            ## CALCULATE OPEN APPLICATION PART OF IMAGE CRC
            ######################################################################################

            # Calculate application CRC
            # NOTE: Start calculation after application header and after crypting of the image!
            app_crc = calc_crc32( out_file.read( APP_HEADER_SIZE_BYTE, None ))

            # Write app CRC into application header
            out_file.write( APP_HEADER_IMAGE_CRC_ADDR, struct.pack('I', int(app_crc)))            


            ######################################################################################
            ## IMAGE SIGNING
            ######################################################################################

            # Signing application
            if sign_en:
                
                # Generate signature
                signature = generate_signature( out_file.read( APP_HEADER_SIZE_BYTE, None ), private_key )

                # Generate hash
                hash = generate_hash( out_file.read( APP_HEADER_SIZE_BYTE, None ))

                # Add signature to application header
                out_file.write( APP_HEADER_SIGNATURE_ADDR, signature )

                # Add signature type
                out_file.write( APP_HEADER_SIG_TYPE_ADDR, [SigType.ECDSA] )

                # Add hash to application header
                out_file.write( APP_HEADER_HASH_ADDR, hash )                

                # Succes info
                print("SUCCESS: Firmware image successfully signed!")  

            else:

                # Add signature type
                out_file.write( APP_HEADER_SIG_TYPE_ADDR, [SigType.NONE] )


            ######################################################################################
            ## NON-CRYPTED FILE 
            # (used for debugging with bootloader and for production ready file generation)
            ######################################################################################
            non_crypted_out_file_path = OUTPUT_DIR_NAME + "/" + file_path_out.split("/")[-1].split(".")[0] + "_NON_CRYPTED.bin"

            # Close the output file before copying
            out_file.file.close()

            # Copy inputed binary file
            shutil.copyfile( file_path_out, non_crypted_out_file_path )

            # Reopen the output file after copying
            out_file = BinFile(file_path_out, access=BinFile.READ_WRITE)
            
            non_crypted_out_file = BinFile( non_crypted_out_file_path, access=BinFile.READ_WRITE)


            ######################################################################################
            ## IMAGE ENCRYPTION
            ######################################################################################

            # Add encryption type if encryption enabled
            if crypto_en:

                # Set encryption type 
                out_file.write( APP_HEADER_ENC_TYPE_ADDR, [EncType.AES_CTR] ) 
                non_crypted_out_file.write( APP_HEADER_ENC_TYPE_ADDR, [EncType.AES_CTR] )   

                # Encrypt application part, skip application header
                #file_crypted_out.write( APP_HEADER_SIZE_BYTE, aes_encode( out_file.read( APP_HEADER_SIZE_BYTE, out_file.size() - APP_HEADER_SIZE_BYTE )))
                out_file.write( APP_HEADER_SIZE_BYTE, aes_encode( out_file.read( APP_HEADER_SIZE_BYTE, None )))

                # Succes info
                print("SUCCESS: Firmware image successfully crypted!")    

                # Calculate encrypted application CRC
                # NOTE: Start calculation after application header and after crypting of the image!
                app_crc = calc_crc32( out_file.read( APP_HEADER_SIZE_BYTE, None ))

                # Write encrypted app CRC into application header
                out_file.write( APP_HEADER_ENC_IMAGE_CRC_ADDR, struct.pack('I', int(app_crc)))
                non_crypted_out_file.write( APP_HEADER_ENC_IMAGE_CRC_ADDR, struct.pack('I', int(app_crc)))

            else:
                # Set encryption type
                out_file.write( APP_HEADER_ENC_TYPE_ADDR, [EncType.NONE] ) 
                non_crypted_out_file.write( APP_HEADER_ENC_TYPE_ADDR, [EncType.NONE] ) 

            ######################################################################################
            ## LAST STEP IS TO CALCULATE IMAGE (APP) HEADER CRC 
            ######################################################################################

            # Calculate application header CRC after all fields are header fields are setup!
            # NOTE: Ignore first field as it is CRC value itself!
            app_header_crc = calc_crc8( out_file.read( 1, APP_HEADER_SIZE_BYTE - 1 ))

            # Write application header crc
            out_file.write( APP_HEADER_CRC_ADDR, [app_header_crc] )
            non_crypted_out_file.write( APP_HEADER_CRC_ADDR, [app_header_crc] )

            # Success info
            print("SUCCESS: Image (application) header successfully filled!")   

            # Close files   
            out_file.file.close()
            non_crypted_out_file.file.close()
         

            ######################################################################################
            ## GENERATE PRODUCTION READY FILE
            ######################################################################################

            # Find bootloader file
            boot_file = find_bootloader_image()

            if None is not boot_file:

                # Load bootloader hex
                ih = IntelHex()
                ih.loadhex( boot_file )

                # Load application binary at specified offset
                with open(non_crypted_out_file_path, "rb") as f:
                    app_data = f.read()
                    ih.frombytes(app_data, offset=app_addr_start)

                # Write the merged hex file
                if 0 != sw_ver[0]:
                    production_out_file_path = OUTPUT_DIR_NAME + "/" + file_path_in.split("/")[-1].split(".")[0] + "_boot__V%s_%s_%s_%s" % (sw_ver[3], sw_ver[2], sw_ver[1], sw_ver[0]) + ".hex"
                else:
                    production_out_file_path = OUTPUT_DIR_NAME + "/" + file_path_in.split("/")[-1].split(".")[0] + "_boot__V%s_%s_%s" % (sw_ver[3], sw_ver[2], sw_ver[1]) + ".hex"
                ih.write_hex_file(production_out_file_path)

                # Success info
                print("SUCCESS: Production ready file generated!")
            else:
                print("WARNING: Bootloader hex file not found in the \"st_boot\" module configuration space.")
                print("INFO: To generate a production-ready file, ensure the bootloader hex file is placed in the \"st_boot\" module configuration directory.")

            ######################################################################################
            ## README
            ######################################################################################
            
            # Generate release info file name
            if 0 != sw_ver[0]:
                release_info_file_path = OUTPUT_DIR_NAME + "/" + file_path_in.split("/")[-1].split(".")[0] + "_release_info" + "__V%s_%s_%s_%s" % (sw_ver[3], sw_ver[2], sw_ver[1], sw_ver[0]) + ".txt"
            else:
                release_info_file_path = OUTPUT_DIR_NAME + "/" + file_path_in.split("/")[-1].split(".")[0] + "_release_info" + "__V%s_%s_%s" % (sw_ver[3], sw_ver[2], sw_ver[1]) + ".txt"

            # Generate release info file
            path = "/".join( file_path_in.split("/")[:-1] )
            readme_file = open( release_info_file_path, "w" )

            # Fill with informations
            readme_file.write( "======================================================================================================\n")
            readme_file.write( "        RELEASE PACKAGE INFORMATIONS\n")
            readme_file.write( "======================================================================================================\n")
            readme_file.write("     File: %s\n"% release_info_file_path.split("/")[-1] )
            readme_file.write("     Date: %02d.%02d.%04d\n"% ( get_day(), get_month(), get_year()))
            readme_file.write("     Time: %02d:%02d:%02d\n"% ( get_hour(), get_minute(), get_second()))
            readme_file.write("\n")
            readme_file.write("    PC INFORMATIONS\n" )
            readme_file.write(" Username: %s\n" % os.getenv("USERNAME"))
            readme_file.write("  PC name: %s\n" % os.getenv("COMPUTERNAME"))
            readme_file.write("       OS: %s\n" % platform.platform())
            readme_file.write("======================================================================================================\n")
            readme_file.write("    INPUTED FILES\n")
            readme_file.write(" Application firmware file: %s\n" % file_path_in )
            if None is not boot_file:
                readme_file.write("  Bootloader firmware file: %s\n" % boot_file.split("/")[-1] )
            readme_file.write("======================================================================================================\n")
            readme_file.write("    DFU RELEASE FILE\n")
            readme_file.write(" DFU release file: %s\n" % file_path_out.split("/")[-1])
            readme_file.write("\n")
            readme_file.write("NOTE: This file is intended for use with the dedicated bootloader only! \n")
            readme_file.write("======================================================================================================\n")
            
            if None is not boot_file:
                readme_file.write("    PRODUCTION READY FILE\n")
                readme_file.write(" Production ready file: %s\n" % production_out_file_path.split("/")[-1])
                readme_file.write("\n")
                readme_file.write("NOTE: This file is intended for production use only!\nIt contains both the bootloader and the application in an unencrypted format.\nEnsure this file is handled securely to prevent unauthorized access or modification.\n")
                readme_file.write("======================================================================================================\n")

            # Close file
            readme_file.close()

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

