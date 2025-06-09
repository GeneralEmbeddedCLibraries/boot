# **Application Signature Tool**

This Python script, app_sign_tool.py, is a command-line utility designed to prepare firmware application binaries for deployment. It processes a raw binary file, populates its header with crucial metadata, and can optionally apply cryptographic operations such as digital signing and encryption. Finally, it can merge the processed application with a bootloader to generate a production-ready .hex file.


Signature tool is invoked in post-build process, after binary file is composed.

Help message:
```
>>>app_sign_tool.py --help
====================================================================
     Firmware Application Signature Tool V1.1.0
====================================================================
usage: app_sign_tool.py [-h] -f bin_in -a app_addr_start [-s] [-k private_key] [-c] [-git]

Firmware Application Signature Tool V1.1.0

optional arguments:
  -h, --help         show this help message and exit
  -f bin_in          Input binary file
  -a app_addr_start  Start application address
  -s                 Signing (ECSDA) binary file
  -k private_key     Private key for signature
  -c                 Encrypt (AES-CTR) binary file
  -git               Store Git SHA to image header

Enjoy the program!
```

## **Features**
 - ***Application Header Population***: Fills a 256-byte header with metadata, including version numbers, image size, start address, and checksums.
 - ***Firmware Padding***: Automatically pads the binary to ensure it aligns with a specified block size (64 bytes).
 - ***Image Signing***: Applies an ECDSA (Elliptic Curve Digital Signature Algorithm) signature to the application image for integrity and authenticity verification.
 - ***Image Encryption***: Encrypts the application using AES-CTR (Advanced Encryption Standard in Counter Mode).
 - ***CRC Calculation***: Computes a CRC-8 for the header and a CRC-32 for the application image to ensure data integrity.
 - ***Production File Generation***: Merges the processed application with a bootloader .hex file to create a single, production-ready .hex file.
 - ***Release Information***: Generates a detailed _release_info.txt file containing metadata about the build, including timestamps, machine details, and input/output filenames.

## **Dependencies**

### **Revision** module
Script follows [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module application header structure. 


## **Limitations**

### **1. Application header**
Application header is expected to be at the begining of firmware binary and structured according to [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module specifications.

## **STM32 CubeIDE Setup**

For STM32 CubeIDE users go to: *Properties->C/C++ Build->Settings->Build Steps->Post-Build steps*

Example for using signature tool *V1.1.0*, where *${ProjName}.bin* file is inputed and *${ProjName}__BOOT_READY.bin* file is generated for image address "0x08010000":
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.1.0/app_sign_tool__V1_1_0.exe -f ../${ConfigName}/${ProjName}.bin -a 0x08010000
```
NOTICE: "mySrc" is name of folder where all user sources are places. This is not a requirement, but rather user choise how to organize directory paths. 

## **Using EAS encryption option**

Invoke script with *-c* switch in order to enable encryption of firmware:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.1.0/app_sign_tool__V1_1_0.exe -f ../${ConfigName}/${ProjName}.bin -a 0x08010000 -c
```

## **Using ECDSA signature option**

Invoke script with *-s* switch in order to enable digital signature of firmware. Additional *-k* argument must be passed in to provide *private key*:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.1.0/app_sign_tool__V1_1_0.exe -f ../${ConfigName}/${ProjName}.bin -a 0x08010000 -s -k ../"mySrc"/middleware/boot/private.pem
```

## **Putting it all together**
Use following command to prepare image header, digital signature, firmware encryption and embedding git commit SHA into image header:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.1.0/app_sign_tool__V1_1_0.exe -f ../${ConfigName}/${ProjName}.bin -a 0x08010000 -s -k ../"mySrc"/middleware/boot/private.pem -c -git
```

## **Output directory**
The script creates an *Output* directory in the same location as the input file. Within this directory, files are organized into subfolders based on the software version extracted from the image (application) header.

Each output includes three files:
 - DFU-ready application – prepared for Device Firmware Update.
 - Open (non-encrypted) application – intended for debugging with the bootloader.
 - Production-ready image – a merged file containing both the application and the bootloader, suitable for final deployment.