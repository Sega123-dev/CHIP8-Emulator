// No using winMain()
#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cstdlib>
#include <ctime>

class Chip8
{
public:
    uint8_t memory[4096]; // RAM memory
    uint8_t V[16];        // Registers V0 - VF (16),VF-flag register that saves state and events
    uint16_t I;           // Index pointer - holds adresses of sprites
    uint16_t pc;          // Program counter - used for moving through instructions
    uint16_t stack[16];   // Stack - holds memory adresses for doing sub-tasks
    uint16_t sp;          // Stack pointer - used to point to a specific adress in stack
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t keypad[16];
    uint8_t display[32][64];

    void initialize()
    {
        pc = 0x200; // All Chip8 programs start here
        I = 0;
        sp = 0;
        delay_timer = 0;
        sound_timer = 0;
        srand(time(nullptr)); // For DRW

        // Clear memory, registers, display, stack
        memset(memory, 0, sizeof(memory));
        memset(V, 0, sizeof(V));
        memset(stack, 0, sizeof(stack));
        memset(display, 0, sizeof(display));
        memset(keypad, 0, sizeof(keypad));

        addFontset();
    }

    void addFontset()
    {
        // Tile system (5x4 pixels)
        uint8_t chip8_fontset[80] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
        for (int i = 0; i < 80; i++)
            memory[i] = chip8_fontset[i];
    }

    void loadProgram(const char *filename)
    {
        std::ifstream rom(filename, std::ios::binary | std::ios::ate); // Transforms the file into raw binary and starts reading in from the end
        if (!rom)
        {
            std::cerr << "Failed to load ROM: " << filename << std::endl;
            return;
        }

        std::streamsize size = rom.tellg(); // Tells the file size in bytes
        rom.seekg(0, std::ios::beg);        // Goes on beggining for reading opcodes

        std::vector<char> buffer(size);    // Dynamic array,holds the file size so we can put the program in the RAM
        if (rom.read(buffer.data(), size)) // Reads the file by its size
        {
            for (int i = 0; i < size; i++)
                memory[0x200 + i] = static_cast<uint8_t>(buffer[i]); // Puts the file in memory
        }
    }

    void emulateCycle() // CHIP8 interpreter
    {
        uint16_t opcode = memory[pc] << 8 | memory[pc + 1]; // Extracting the opcode

        switch (opcode & 0xF000)
        {
        case 0x0000:
            if ((opcode & 0x00FF) == 0x00E0)
            {
                memset(display, 0, sizeof(display));
                pc += 2;
            }
            else if ((opcode & 0x00FF) == 0x00EE)
            {
                sp--;
                pc = stack[sp];
            }
            else
                pc += 2;
            break;
        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        case 0x2000:
            stack[sp] = pc + 2;
            sp++;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            if (V[x] == (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
            break;
        }
        case 0x6000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] = opcode & 0x00FF;
            pc += 2;
            break;
        }
        case 0x7000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] += opcode & 0x00FF;
            pc += 2;
            break;
        }
        case 0xA000:
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0xD000:
        {
            uint8_t x = V[(opcode & 0x0F00) >> 8];
            uint8_t y = V[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;
            V[0xF] = 0;

            for (int row = 0; row < height; row++)
            {
                uint8_t sprite = memory[I + row];
                for (int col = 0; col < 8; col++)
                {
                    if ((sprite & (0x80 >> col)) != 0)
                    {
                        int px = (x + col) % 64;
                        int py = (y + row) % 32;
                        if (display[py][px] == 1)
                            V[0xF] = 1;
                        display[py][px] ^= 1;
                    }
                }
            }

            pc += 2;
            break;
        }
        case 0xC000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] = (rand() % 256) & (opcode & 0x00FF);
            pc += 2;
            break;
        }
        case 0xE000:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            if ((opcode & 0x00FF) == 0x9E)
            {
                if (keypad[V[x]])
                    pc += 4;
                else
                    pc += 2;
            }
            else if ((opcode & 0x00FF) == 0xA1)
            {
                if (!keypad[V[x]])
                    pc += 4;
                else
                    pc += 2;
            }
            break;
        }
        default:
            pc += 2;
            break;
        }

        // Timers
        if (delay_timer > 0)
            delay_timer--;
        if (sound_timer > 0)
        {
            sound_timer--;
            if (sound_timer == 0)
                Beep(440, 200);
        }
    }
};

int main()
{
    Chip8 chip8;
    chip8.initialize();
    chip8.loadProgram("pong.ch8");

    const int SCALE = 10;
    const int CYCLES_PER_FRAME = 60; // Cycle function needs to be executed this many times in a frame so it can actually show something on the screen
    const int FRAME_DELAY_MS = 16;   // 60 FPS

    // Makes the centered 64x32 window scaled by 10

    SDL_Window *window = SDL_CreateWindow("CHIP-8 Emulator",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          64 * SCALE, 32 * SCALE,
                                          SDL_WINDOW_SHOWN);
    // IDK NEXT

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Rect rect{0, 0, SCALE, SCALE};
    bool running = true;
    SDL_Event e;

    const SDL_Keycode keymap[16] = {
        SDLK_x, SDLK_1, SDLK_2, SDLK_3,
        SDLK_q, SDLK_w, SDLK_e, SDLK_a,
        SDLK_s, SDLK_d, SDLK_z, SDLK_c,
        SDLK_4, SDLK_r, SDLK_f, SDLK_v};

    while (running)
    {
        // Event handling
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
                bool pressed = e.type == SDL_KEYDOWN;
                for (int i = 0; i < 16; i++)
                    if (e.key.keysym.sym == keymap[i])
                        chip8.keypad[i] = pressed;
            }
        }

        // Run cycles
        for (int i = 0; i < CYCLES_PER_FRAME; i++)
            chip8.emulateCycle();

        // Render display
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 64; x++)
                if (chip8.display[y][x])
                {
                    rect.x = x * SCALE;
                    rect.y = y * SCALE;
                    SDL_RenderFillRect(renderer, &rect);
                }

        SDL_RenderPresent(renderer);
        SDL_Delay(FRAME_DELAY_MS);
    }

    // Ends the SDL display process to save memory

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
