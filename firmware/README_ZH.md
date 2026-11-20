# 出厂固件

| 文件 | 适用硬件与用途 | 烧录偏移 |
| --- | --- | --- |
| [ESP32-C5-Touch-LCD-1.69_FactoryOnly_20260626.bin](ESP32-C5-Touch-LCD-1.69_FactoryOnly_20260626.bin) | ESP32-C5-Touch-LCD-1.69（16 MB Flash），用于恢复出厂演示 | `0x0` |

开发板进入下载模式后，在当前目录执行。

```sh
esptool --chip esp32c5 write-flash 0x0 ESP32-C5-Touch-LCD-1.69_FactoryOnly_20260626.bin
```
