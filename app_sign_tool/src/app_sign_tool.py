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
        bin_out_file    = open( file_path_out, "ab")

        # Calculate CRC
        # TODO:

        # Count application size
        # TODO: 

        # Open file
        with open(file_path_in, mode='rb') as file: 
            fileContent = file.read()

            binary_line = ""
            for n, byte in enumerate( fileContent ):
                
                if n % 16 == 0 and n > 0:
                    print( binary_line )
                    binary_line = ""
            
                binary_line += "%02X " % byte












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

