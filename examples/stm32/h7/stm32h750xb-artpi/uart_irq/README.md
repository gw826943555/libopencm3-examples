# README

This example program echoes data sent in on UART4 on the
rt-thread STM32h750XB-ARTPI board. Uses interrupts for that purpose.

The sending is done in a nonblocking way.

## Board connections

| Port   | Function      | Description                       |
| ------ | ------------- | --------------------------------- |
| `PA0`  | `(UART4_TX)`  | TTL serial output `(115200,8,N,1)`|
| `PI9`  | `(UART4_RX)`  | TTL serial input `(115200,8,N,1)` |

## Notes

UART4 is connected to ST-Link VCP port
