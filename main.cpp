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

        // Function logs

        std::cout << "RAM log: 0KB loaded";
        for (int i = 0; i < 0xF + 1; i++)
            std::cout << "Register V" << i + 1 << V[i] << '\n';
        std::cout << "Delay timer: " << delay_timer << "Hz" << '\n';
        std::cout << "Sound timer: " << sound_timer << '\n';
        std::cout << "Program counter is set to: " << pc << '\n';
        std::cout << "Index pointer: " << I << '\n';
        for (int i = 0; i < 16; i++)
            std::cout << "Stack #" << i + 1 << ":" << stack[i] << '\n';
        std::cout << "Stack pointer: " << sp << '\n';
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

        // Logs

        std::cout << "ROM dumped successfully!\n";
        std::cout << size << "/" << "4096B loaded\n";
    };

    // CH8 interpreter

    void interpreter()
    {
        uint16_t opcode = memory[pc] << 8 | memory[pc + 1]; // Fetch opcodes(one is 2 bytes)

        // Highest nibble

        switch (opcode & 0xF000)
        {
        case 0x0000:
            switch (opcode & 0x00FF)
            {
            case 0x00E0: // CLS
                for (int i = 0; i < 64; i++)
                {
                    for (int j = 0; j < 32; j++)
                    {
                        display[i][j] = 0;
                    }
                }
                pc += 2;
                break;
            case 0x00EE: // RET: Return from subroutine
                sp--;
                pc = stack[sp];
                pc += 2;
                break;
            default:
                std::cout << "Unknown 0x0000 opcode: " << std::hex << opcode << "\n";
                break;
            }
            break;

        case 0x1000: // Jump to address
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: // Call Stack
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
            break;

        case 0x3000: // SE Vx byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            if (V[x] == kk)
            {
                pc += 4;
            }
            else
                pc += 2;
            break;
        case 0x4000: // LD Vx byte
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;
            V[x] = kk;
            pc += 2;
            break;
        default:
            std::cout << "Unknown opcode: " << std::hex << opcode << '\n';
            break;
        };

        // Timers

        if (delay_timer > 0)
            delay_timer--;
        if (sound_timer > 0)
        {
            sound_timer--;
            if (sound_timer == 0)
                std::cout << "BEEP!" << "\n";
        }
    };
};

int main()
{
}