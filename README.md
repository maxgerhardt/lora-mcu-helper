# LoRa MCU-Helper

### Description
Firmware for doing SPI transfers and interrupts over a UART / USB interface.

### Features

* Setup a SPI interface with different slave select pins
* Execute a SPI transfer and return read data
* Register interrupt pins and modes
* Be asynchronously notified when and what interrupt occurs

### Purpose

To be used with an upcoming TTN gateway software

### How to flash

You should use PlatformIO. Make sure to select your mbedos compatible board in the `platformio.ini`.

To flash:

```sh
$ pio run -t upload
```

For debug UART, see `project_settings.h`.
