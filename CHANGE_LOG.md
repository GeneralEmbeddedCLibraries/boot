# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V1.0.0 - 28.09.2024

### Notice
Big changes to bootloader concept, not compatible with V0.x.x!

### Added
 - ECDSA Digital signature validation
 - AES-CTR Firmware encryption
 - Option to configure pre/post validation of the image based on its header info
 
### Changes
 - Change of application header structure
 - Changed "prepare" command, now sending over complete image header

---
## V0.2.0 - 15.02.2024

### Added
 - New release of application signature tool: app_sign_tool__V0_3_0.exe (for application header V1)
 - New release of application signature tool: app_sign_tool__V0_4_0.exe (for application header V2)
 - New configuration *"BOOT_CFG_DATA_PAYLOAD_SIZE"*

### Changes
 - Minor bug fixes

---
## V0.1.0 - 20.10.2023

### Notice
 - Initial implemention of bootloader module

### Added
- Bootloader communication specification & implementation
- Bootloader shared memory specification & implementation
- Instruction how to prepare application for bootloader
- Bootloader FSM implementation
- Application SW size and compatibility checks before entering upgrade procedure
- Down-grade upgrade protection
- Added option for cryptography of firmware binary image

---