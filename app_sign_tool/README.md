# **Firmware Signature Tool**


## **Limitations**

### **1. Application header**
Application header is expected at the begining of firmware binary and structured according to [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module specifications






## **STM32 CubeIDE Setup**

```
../"my_src"/middleware/boot/boot/app_sign_tool/delivery/V0.1.0/app_sign_tool.exe -h


../OrderBob/middleware/boot/boot/app_sign_tool/delivery/V0.1.0/app_sign_tool.exe -f ../${ConfigName}/${ProjName}.bin -o ../${ConfigName}/${ProjName}__SIGNED.bin
```