#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>

class Chip8
{
    uint8_t memory[4096]; // RAM memory
    uint8_t V[16];        // 8-bit registers from V0 - VF //VF - flag register
    uint16_t I;           // Index register
    uint16_t pc;          // Program counter
    uint16_t stack[16];   // Call stack, used for doing sub-tasks
    uint16_t sp;          // Stack pointer,points to the free space in stack memory
    uint8_t delay_timer;  // Sets delays in the program, counts down at 60Hz
    uint16_t sound_timer;
    uint8_t keypad[16];      // Keyboard input
    uint8_t display[64][32]; // 64x32 px,scaled up to 10

    // Sets everything to 0 or default

    void initialize()
    {
        for (int i = 0; i < 16; i++)
            V[i] = 0;
        delay_timer = 0;
        sound_timer = 0;
        pc = 0x200; // Programs starts here
        for (int i = 0; i < 4096; i++)
            memory[i] = 0;
        I = 0;
        for (int i = 0; i < 16; i++)
            stack[i] = 0;
        sp = 0;
    };

    // Loads the game into memory

    void loadProgram(const char *filename)
    {
        std::ifstream rom(filename, std::ios::binary | std::ios::ate); // Sets it to binary type and we start reading the file from the end
        if (rom.fail())
        {
            std::cerr << "Failed to load ROM: " << filename << std::endl;
            return;
        }
        std::streamsize size = rom.tellg(); // Tells the file size
        rom.seekg(0, std::ios::beg);        // Move pointer to beggining

        std::vector<char> buffer(size); // Temporary array to store the file

        if (rom.read(buffer.data(), size)) // Reads data
        {
            for (int i = 0; i < size; i++)
            {
                memory[0x200 + i] = static_cast<uint8_t>(buffer[i]);
            }
        }
        rom.close(); // Closes the file

        std::cout << "ROM dumped successfully!";
    };

    // CH8 interpreter

    void interpreter() {};
};

int main()
{
}