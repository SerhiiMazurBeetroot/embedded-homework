# Configuration in STM32CubeMX

## 1 Create a project and select an MCU

Open `STM32CubeMX` → `File` →` New Project`.

Part Number Selector, and specify the exact name of the microcontroller `STM32F401CCU6`.

Click Start Project.

---

## 2. Налаштування I²C у STM32CubeMX

Потрібно увімкнути модуль I²C (наприклад, I2C1), до якого підключаються всі 3 пристрої на одну шину.

### 1.Увімкнення I2C1

Розділ Connectivity.
У лівому меню CubeMX перейдіть до розділу Connectivity → I2C1.
У випадаючому списку I2C виберіть режим I2C.
На схемі мікроконтролера автоматично призначаться виводи (зазвичай PB6 — SCL, PB7 — SDA).

---

## 3.Конфігурація параметрів I²C

Вкладка Parameter Settings.

- I2C Speed Mode:
  Виберіть Standard Mode (100 kHz) або Fast Mode (400 kHz). SSD1306 та AT24C32E чудово працюють на 400 kHz, але для DS1307 рекомендується 100 kHz (стандартний режим за специфікацією DS1307).
- Clock Speed: 100000 Hz (100 kHz).
- Addressing Mode: 7-bit (стандартна 7-бітна адресація).

---

## 4.Налаштування RCC (Тактування)

Розділ System Core.

- Перейдіть у System Core → RCC.
- Якщо у вас на платі є зовнішній кварц, у полі HSE виберіть Crystal/Ceramic Resonator.
- Відкрийте вкладку Clock Configuration зверху та виставте максимальну частоту для вашого мікроконтролера (наприклад, 72 MHz для STM32F103 або 84/168 MHz для STM32F4).

---

## 4.Генерація C++ проєкту

Вкладка Project Manager.

- Перейдіть на вкладку Project Manager.
- У полі Toolchain / IDE виберіть середовище розробки: STM32CubeIDE, Keil (MDK-ARM) або CMake
- Переконайтеся, що в коді використовується C++ (у STM32CubeIDE можна перейменувати main.c у main.cpp або включити C++ у властивостях проєкту).
- Натисніть GENERATE CODE у верхньому правому кутку.

---
