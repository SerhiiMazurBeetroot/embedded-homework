# Модуль 4.2. I²C: Синхронний послідовний інтерфейс із адресацією (TWI)

## План ініціалізації дисплея SSD1306 через I²C

```text

Display off
0xAE
    |
Set geometry
0xA8, 0xD3, 0x40
    |
Addressing mode
0x20, 0x00, 0xA1, 0xC8
    |
Hardware config
0xDA, 0x81, 0xD5
    |
Timing settings
0xD9, 0xDB
    |
Timing settings
0xD9, 0xDB
    |
Display on
0xAF

```
