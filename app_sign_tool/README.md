# **Application Signature Tool**

Application (Firmware) Signature Tool takes binary file, process it and assemble **application header**. Look at the [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module for more informations about application header.

Signature tool is invoked in post-build process, after binary file is composed.

## **Limitations**

### **1. Application header**
Application header is expected to be at the begining of firmware binary and structured according to [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module specifications.

## **STM32 CubeIDE Setup**

For STM32 CubeIDE users go to: *Properties->C/C++ Build->Settings->Build Steps->Post-Build steps*

Example for using signature tool *V0.1.0*, where *${ProjName}.bin* file is inputed and *${ProjName}__SIGNED.bin* file is generated:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V0.1.0/app_sign_tool__V0_1_0.exe -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}__SIGNED.bin
```
NOTICE: "mySrc" is name of folder where all user sources are places. This is not a requirement, but rather user choise how to organize directory paths. 