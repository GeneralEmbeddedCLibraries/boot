# **Boot**
Bootloader implementation in C code for general use in embedded systems.


## **Bootloader Interface**
Bootloader has custom, lightweight and pyhsical layer agnostics communication interface. Detailed specifications of interface can be found in [Bootloader_Interface_Specifications.xlsx](doc/Bootloader_Interface_Specifications.xlsx).

## **Bootloader Sequence**

![](doc/pic/Bootloader_Sequence.png)


## **Dependencies**

### **1. Flash memory map**
Bootloader expect predefined application binary code as shown in picture bellow. 

![](doc/pic/Flash_MemoryMap.png)

### **2. Application header**
Application code must have a ***Application Header*** in order to validate data integritiy of image.

### **3. Revision module**
Revision module provides information of application header.
TODO: Add more info here...


## **Limitations**


## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 

```
root/middleware/boot/boot/"module_space"
```

## **API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **boot_init** | Initialization of Bootloader module | boot_status_t boot_init(void) |




## **Usage**

**GENERAL NOTICE: Put all user code between sections: USER CODE BEGIN & USER CODE END!**

1. Copy template files to root directory of module.
2. Configure Boot module for application needs. Configuration options are following:

| Configuration | Description |
| --- | --- |
| **BOOT_CFG_FW_SIZE_CHECK_EN** 			| Enable/Disable new firmware size check |
