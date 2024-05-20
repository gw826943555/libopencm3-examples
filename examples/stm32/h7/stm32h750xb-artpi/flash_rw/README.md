# README

This small example program operates on internal FLASH memory by using
libopencm3.

It's intended for ARTPI with STM32h750 microcontrollers. To use this
example it is essential to use UART4 port.  It writes text string entered via
serial port terminal (ex. teraterm) into internal FLASH memory and then it
reads it.
