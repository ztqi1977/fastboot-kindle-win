---

## Kindle Fastboot for Windows

This project is based on the original work by yifanlu, who ported the Android version of fastboot to Kindle devices, and hostar, who further adapted it for Windows. We would like to express our gratitude for their hard work. The current version of the code is a modification of hostar's version, with the following main changes:

1. Instead of requiring GCC for compilation, it can now be compiled using Visual Studio.
2. The previous fixed values for Endpoint Address and Max Packet Size have been replaced with values read from the Kindle device, making it compatible with different Kindle devices.
3. Previously, the code would immediately read the response after sending the flash command, which often resulted in failure because the Kindle device needs time to complete the flashing process. It has now been modified to wait for a maximum of 30 seconds.
4. It was previously believed that releasing the USB handle would cause issues, but after reviewing the libusb sample code, it was confirmed that releasing the handle is unnecessary.

The `libusb.lib` file used in this project is obtained from `libusb-win32-bin-1.2.6.0\lib\msvc_x64\libusb.lib`. If you have installed a different version of the driver, you will need to replace it and recompile the code.

### Prerequisites

1. The USB driver must be `libusb-win32`, preferably version 1.2.6.0. This will allow you to directly use the precompiled `fastboot.exe`.
2. The program searches for USB devices with a Vendor ID (VID) of `0x1949`. Therefore, please ensure that only one device meets this condition.

---
