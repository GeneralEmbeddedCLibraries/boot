# **Application Signature Tool**

Application (Firmware) Signature Tool prepares application header, it calculated CRC and binary size. That informations are later crutial for bootloader validation logic to determine if new application is suitable for given system. Tool is invoked in post-build process, after binary file is composed. 

## **Limitations**

### **1. Application header**
Application header is expected at the begining of firmware binary and structured according to [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module specifications.

## **STM32 CubeIDE Setup**

For STM32 CubeIDE users go to: *Properties->C/C++ Build->Settings->Build Steps->Post-Build steps*

Example for signature tool V0.1.0:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V0.1.0/app_sign_tool__V0_1_0.exe -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}__SIGNED.bin
```
NOTICE: "mySrc" is name of folder where all user sources are places. This is not a requirement, but rather user choise how to organize directory paths. 