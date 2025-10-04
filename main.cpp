#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cstdlib>
#include <ctime>

class Chip8
{
    uint8_t memory[4096]; // RAM memory
    uint8_t V[16];        // 8-bit registers from V0 - VF //VF - flag register
    uint16_t I;           // Index register
    uint16_t pc;          // Program counter
    uint16_t stack[16];   // Call stack, used for doing sub-tasks
    uint16_t sp;          // Stack pointer,points to the free space in stack memory
    uint8_t delay_timer;  // Sets delays in the program, counts down at 60Hz
    uint8_t sound_timer;
    uint8_t keypad[16];      // Keyboard input
    uint8_t display[32][64]; // 32x64 px,scaled up to 10

    // Sets everything to 0 or default
public:
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

        srand(time(nullptr)); // Generating a random number for RND Vx, byte

        // Function logs

        std::cout
            << "RAM log: 0KB loaded";
        for (int i = 0; i < 0xF + 1; i++)
            std::cout << "Register V" << i << ":" << V[i] << '\n';
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

    void emulateCycle()
    {
        uint16_t opcode = memory[pc] << 8 | memory[pc + 1]; // Fetch opcodes(one is 2 bytes)

        // Highest nibble

        switch (opcode & 0xF000)
        {
        case 0x0000:
            switch (opcode & 0x00FF)
            {
            case 0x00E0: // CLS
                for (int i = 0; i < 32; i++)
                {
                    for (int j = 0; j < 64; j++)
                    {
                        display[i][j] = 0;
                    }
                }
                pc += 2;

                std::cout << "Screen cleared\n";

                break;
            case 0x00EE: // RET: Return from subroutine
                sp--;
                pc = stack[sp];

                std::cout << "[RET]Current instruction adress: " << pc << '\n';

                break;
            default:
                std::cout << "Unknown 0x0000 opcode: " << std::hex << opcode << "\n";
                break;
            }
            break;

        case 0x1000: // Jump to address
            pc = opcode & 0x0FFF;
            std::cout << "Jumped to adress: " << pc << '\n';
            break;

        case 0x2000: // Call Stack
            stack[sp] = pc + 2;
            sp++;
            pc = opcode & 0x0FFF;
            std::cout << "New instruction placed in stack: " << stack[sp] << '\n';
            break;
            // SE Vx byte
        case 0x3000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            if (V[x] == kk)
            {
                pc += 4; // Skip the next instruction
                std::cout << "Opcode: " << opcode << ". Instruction skipped" << "\n";
            }
            else
                pc += 2; // Move to next instruction
            break;
        }
            // SNE Vx, byte
        case 0x4000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;
            if (V[x] != kk)
            {
                pc += 4;
                std::cout << "Opcode: " << opcode << ". Instruction skipped" << "\n";
            }
            else
                pc += 2;
            break;
        }
        // SNE Vx, byte
        case 0x5000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (V[x] == V[y])
            {
                pc += 4;
                std::cout << "Opcode: " << opcode << ". Instruction skipped" << "\n";
            }
            else
            {
                pc += 2;
            }
            break;
        }
        // LD Vx,byte
        case 0x6000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            V[x] = kk;
            pc += 2;
            break;
        }
        // ADD Vx,byte
        case 0x7000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t kk = opcode & 0x00FF;

            V[x] = V[x] + kk;
            pc += 2;
            break;
        }
        // Arithmetical/logical instructions
        case 0x8000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            uint8_t operation = opcode & 0x000F;

            switch (operation)
            {
            case 0x0:
                V[x] = V[y];
                pc += 2;
                break;
            case 0x1:
                V[x] = V[x] | V[y];
                pc += 2;
                break;
            case 0x2:
                V[x] = V[x] & V[y];
                pc += 2;
                break;
            case 0x3:
                V[x] = V[x] ^ V[y];
                pc += 2;
                break;
            case 0x4:
            {
                uint8_t sum = V[x] + V[y];
                sum > 255 ? V[0xF] = 1 : V[0xF] = 0;
                V[x] = sum & 0xFF;
                pc += 2;
                break;
            }

            case 0x5:
                V[0xF] = (V[x] >= V[y]) ? 1 : 0;
                V[x] = V[x] - V[y];
                pc += 2;
                break;
            case 0x6:
                V[0xF] = V[x] & 0x1; // LSB
                V[x] = V[x] >> 1;
                pc += 2;
                break;
            case 0x7:
            {
                V[0xF] = (V[y] >= V[x]) ? 1 : 0;
                V[x] = V[y] - V[x];
                pc += 2;
                break;
            }
            case 0xE:
                V[0xF] = (V[x] & 0x80) >> 7; // MSB
                V[x] = V[x] << 1;
                pc += 2;
                break;
            }
            break;
        }
        // SNE Vx, Vy
        case 0x9000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            if (V[x] != V[y])
            {
                pc += 4;
                std::cout << "Opcode: " << opcode << ". Instruction skipped.";
            }
            else
            {
                pc += 2;
            }
            break;
        }
        // LD I, addr
        case 0xA000:
        {
            uint16_t address = opcode & 0x0FFF;
            I = address;
            break;
        }
        // JP V0, addr
        case 0xB000:
        {
            uint16_t address = opcode & 0x0FFF;
            pc = address + V[0];
            break;
        }
        // RND Vx, byte
        case 0xC000:
        {
            uint8_t random = rand() % 256;
            uint8_t kk = opcode & 0x00FF;
            uint8_t x = (opcode & 0x0F00) >> 8;

            V[x] = random & kk;
            pc += 2;
            break;
        }
        // DRW Vx, Vy, nibble
        case 0xD000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            uint8_t k = opcode & 0x000F;
            uint8_t spriteRow = 0;
            V[0xF] = 0;
            for (int i = 0; i < k; i++)
            {
                spriteRow = memory[I + i];
                for (int j = 0; j < 8; j++)
                {
                    uint8_t spriteBit = (spriteRow & (0x80 >> j)) ? 1 : 0;
                    uint8_t &pixel = display[(V[y] + i) % 32][(V[x] + j) % 64]; // wrap around screen
                    if (pixel == 1 && spriteBit == 1)
                        V[0xF] = 1;
                    pixel ^= spriteBit;
                }
            }
            std::cout << "Sprite has been drawn!";
            pc += 2;
            break;
        }
        case 0xE000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            switch (opcode & 0x00FF)
            {
            case 0x9E: // SKP Vx
                if (keypad[V[x]])
                {
                    pc += 4;
                    std::cout << "Opcode: " << opcode << ". Instruction skipped.";
                }
                else
                    pc += 2;
                break;

            case 0xA1: // SKNP Vx
                if (!keypad[V[x]])
                {
                    pc += 4;
                    std::cout << "Opcode: " << opcode << ". Instruction skipped.";
                }
                else
                    pc += 2;
                break;
            }
            break;
        };
        default:
            std::cout << "Unknown opcode: " << std::hex << opcode << '\n';
            break;
        }

        // Timers

        if (delay_timer > 0)
            delay_timer--;
        if (sound_timer > 0)
        {
            sound_timer--;
            if (sound_timer == 0)
            {
                std::cout << "BEEP!" << "\n";
                Beep(440, 1200);
            }
        }
    };
};

int main()
{
    Chip8 chip8;
    chip8.initialize();
    chip8.loadProgram("pong.ch8");

    for (int i = 0; i < 10; i++)
    {
        chip8.emulateCycle();
    }

    return 0;
}