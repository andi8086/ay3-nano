# AY3 File Format

Version 1.0, (C)2023 Andreas J. Reichel, MIT License

The AY3 File Format is a simple register instruction
file for the AY-3-8910 sound chip.

The file has no header.

The file is a stream of 16-bit values in little endian format,
with the following meaning:

```
MSB             LSB
15        7       0
0000 0000 0000 0000     - Start of Frame, wait until the
                          next 20 ms frame begins

1000 rrrr bbbb bbbb     - Write Value bbbb bbbb into register rrrr

0ttt tttt tttt tttt     - Offset in micro seconds to next
                          register write (0-32767) it is possible to delay
                          beyond the next frame start this way.
```
