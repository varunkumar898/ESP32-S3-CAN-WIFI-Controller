# CAN Protocol

## Main load

CAN ID `0x100`, DLC 4.

Bytes 0-3 contain a signed 32-bit load count.

## Angles

CAN ID `0x101`, DLC 8.

- Bytes 0-3: X angle as IEEE-754 float
- Bytes 4-7: Y angle as IEEE-754 float

## Auxiliary load

CAN ID `0x102`, DLC 4.

Bytes 0-3 contain a signed 32-bit auxiliary load count.
