#include <iostream>
#include <cstdint>

class Chip8
{
    uint8_t memory[4096]; // RAM memory
    uint8_t V[16];        // 8-bit registers from V0 - VF //VF - flag register
    uint16_t I;           // Index register
    uint16_t pc;          // Program counter
    uint16_t stack[16];   // Call stack, used for doing sub-tasks
    uint16_t sp;          // Stack pointer
    uint8_t delay_timer;  // Sets delays in the program, counts down at 60Hz
    uint16_t sound_timer;
    uint8_t keypad[16];      // Keyboard input
    uint8_t display[64][32]; // 64x32 px,scaled up to 10

    void initialize() {};
    void loadProgram(const std::string *filename) {};
    void emulateCycle() {};
};

int main()
{
}