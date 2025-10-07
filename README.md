# CHIP8 emulator

CH8 is a programming language used for making CHIP8 programs,they were made in the 60s and 70s.It was interpreted and used as a replacement of languages like BASIC. It runs on a virtual machine,usually implemented on 4K systems like Cosmac VIP and Telmac 1800.

# Virtual machine specifications

- RAM: 4KB (512bytes reserved for CHIP8 fontset,stack and display refresh). 
- Registers: 16 V0 to VF 16 bit registers.
- Stack: Used to store adresses during subroutines,16 levels.
- Delay timer
- Sound timer
- Hex keyboard with 16 keys
- Display: Monochrome 64x32 scaled 10 times

## Opcode table

CHIP8 has 35 official opcodes.Symbols:
- NNN: address
- NN: 8-bit constant
- N: 4-bit constant
- X and Y: 4-bit register identifier
- PC : Program Counter
- I : 12bit register (For memory address) (Similar to void pointer);
- VN: One of the 16 available variables. N may be 0 to F (hexadecimal);

# Technologies used:

*C++* - used for opcodes,timing,graphics,memory...
*SDL* - used for rendering graphics and input
