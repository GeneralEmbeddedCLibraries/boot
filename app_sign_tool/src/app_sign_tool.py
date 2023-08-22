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
    
    file_in, file_out = arg_parser()

    print( "Inputed file: %s" % file_in )
    print( "Outputed file: %s" % file_out )


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

