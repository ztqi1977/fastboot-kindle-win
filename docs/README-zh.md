--

## Kindle Fastboot for Windows

本项目源代码基于yifanlu的原始工作，其将Android版本的fastboot移植到Kindle设备上，以及hostar进一步将其适配为Windows版。我们要对他们的辛勤劳动表示感谢。当前版本的代码是在hostar的版本基础上进行了修改，主要有以下几个改动：

1. 不再需要使用GCC进行编译，现在可以使用Visual Studio进行编译。
2. 之前的Endpoint Address和Max Packet Size是固定值，无法适应不同的Kindle设备，现在改为从Kindle设备中读取。
3. 之前发送烧写指令后立即读取回应，通常会导致失败，因为Kindle设备需要一定时间完成烧写过程，现在改为最大等待30秒。
4. 之前认为释放USB句柄会导致问题，但在查看了libusb示例代码后，确认不需要释放。

本项目使用的`libusb.lib`文件来自`libusb-win32-bin-1.2.6.0\lib\msvc_x64\libusb.lib`。如果您安装了其他版本的驱动程序，您需要自行替换并重新编译代码。

### 使用条件

1. USB驱动必须是`libusb-win32`，最好使用1.2.6.0版本，这样您可以直接使用预编译的`fastboot.exe`。
2. 该程序搜索的USB设备条件是VID=0x1949，请确保只有一个设备满足此条件。

---
