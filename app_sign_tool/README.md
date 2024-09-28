# **Application Signature Tool**

Application (Firmware) Signature Tool takes binary file, process it and assemble **application header**. Look at the [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module for more informations about application header.

Signature tool is invoked in post-build process, after binary file is composed.

## **Limitations**

### **1. Application header**
Application header is expected to be at the begining of firmware binary and structured according to [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module specifications.

## **STM32 CubeIDE Setup**

For STM32 CubeIDE users go to: *Properties->C/C++ Build->Settings->Build Steps->Post-Build steps*

Example for using signature tool *V1.0.0*, where *${ProjName}.bin* file is inputed and *${ProjName}__BOOT_READY.bin* file is generated for image address "0x08010000":
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.0.0/app_sign_tool__V1_0_0.exe -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}__BOOT_READY.bin -a 0x08010000
```
NOTICE: "mySrc" is name of folder where all user sources are places. This is not a requirement, but rather user choise how to organize directory paths. 

## **Using EAS encryption option**

Invoke script with *-c* switch in order to enable encryption of firmware:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.0.0/app_sign_tool__V1_0_0.exe -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}__BOOT_READY.bin -a 0x08010000 -c
```

## **Using ECDSA signature option**

Invoke script with *-s* switch in order to enable digital signature of firmware. Additional *-k* argument must be passed in to provide *private key*:
```
../"mySrc"/middleware/boot/boot/app_sign_tool/delivery/V1.0.0/app_sign_tool__V1_0_0.exe -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}__BOOT_READY.bin -a 0x08010000 -s -k ../"mySrc"/middleware/boot/private.pem
```

// TODO: Git

// TODO: All togheter

// TODO: Help message




py ../Seltron/middleware/boot/boot/app_sign_tool/src/app_sign_tool.py -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}_BOOT_READY.bin -a 0x08010000 -s -k ../Seltron/middleware/boot/private.pem -c -git

