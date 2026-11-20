# Factory firmware

| File | Compatible hardware and purpose | Flash offset |
| --- | --- | --- |
| [ESP32-C5-Touch-LCD-1.69_FactoryOnly_20260626.bin](ESP32-C5-Touch-LCD-1.69_FactoryOnly_20260626.bin) | ESP32-C5-Touch-LCD-1.69 (16 MB Flash), restores the factory demo | `0x0` |

Run from this directory with the board in download mode.

```sh
esptool --chip esp32c5 write-flash 0x0 ESP32-C5-Touch-LCD-1.69_FactoryOnly_20260626.bin
```
