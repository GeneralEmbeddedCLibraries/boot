# Application Signature Tool Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V0.3.0 - 22.12.2023

### Added 
 - Pading binary image to configured block size

---
## V0.2.1 - 20.10.2023

### Changed
 - Changed AES key and initial vector (IV) 

---
## V0.2.0 - 19.10.2023

### Added
- Added encryption of binary file based on AES CTR mode (use "-c" switch)

---
## V0.1.0 - 24.08.2023

### Notice
 - This release expects Application Header V1 according to [Revision](https://github.com/GeneralEmbeddedCLibraries/revision) module

### Added
- Initial implementation
- Application binary size calculation and write to app header
- Application binary CRC calculation and write to app header
- Application header CRC calculation and write to app header

---
