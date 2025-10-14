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
    uint8_t delay_timer;  // 60HZ
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
        memset(memory, 0, sizeof(memory)); // m
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
            std::cerr << "Failed to load ROM: " << filename << "\n";
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

    void emulateCycle() // Interpreter
    {
        uint16_t opcode = memory[pc] << 8 | memory[pc + 1]; // Fetch opcode

        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        uint8_t kk = opcode & 0x00FF;
        uint16_t nnn = opcode & 0x0FFF;
        uint8_t n = opcode & 0x000F;

        switch (opcode & 0xF000)
        {
        case 0x0000:
            switch (opcode & 0x00FF)
            {
            case 0xE0: // CLS
                memset(display, 0, sizeof(display));
                pc += 2;
                break;
            case 0xEE: // RET
                sp--;
                pc = stack[sp];
                break;
            default:
                // SYS addr (ignored on modern interpreters)
                pc += 2;
                break;
            }
            break;

        case 0x1000: // JP addr
            pc = nnn;
            break;

        case 0x2000: // CALL addr
            stack[sp] = pc + 2;
            sp++;
            pc = nnn;
            break;

        case 0x3000: // SE Vx, byte
            pc += (V[x] == kk) ? 4 : 2;
            break;

        case 0x4000: // SNE Vx, byte
            pc += (V[x] != kk) ? 4 : 2;
            break;

        case 0x5000: // SE Vx, Vy
            pc += (V[x] == V[y]) ? 4 : 2;
            break;

        case 0x6000: // LD Vx, byte
            V[x] = kk;
            pc += 2;
            break;

        case 0x7000: // ADD Vx, byte
            V[x] += kk;
            pc += 2;
            break;

        case 0x8000: // Arithmetic/logical
            switch (n)
            {
            case 0x0:
                V[x] = V[y];
                break;
            case 0x1:
                V[x] |= V[y];
                break;
            case 0x2:
                V[x] &= V[y];
                break;
            case 0x3:
                V[x] ^= V[y];
                break;
            case 0x4: // ADD Vx, Vy with carry
            {
                uint16_t sum = V[x] + V[y];
                V[0xF] = (sum > 0xFF) ? 1 : 0;
                V[x] = sum & 0xFF;
                break;
            }
            case 0x5: // SUB Vx, Vy
                V[0xF] = (V[x] >= V[y]) ? 1 : 0;
                V[x] -= V[y];
                break;
            case 0x6: // SHR Vx {, Vy}
                V[0xF] = V[x] & 0x1;
                V[x] >>= 1;
                break;
            case 0x7: // SUBN Vx, Vy
                V[0xF] = (V[y] >= V[x]) ? 1 : 0;
                V[x] = V[y] - V[x];
                break;
            case 0xE: // SHL Vx {, Vy}
                V[0xF] = (V[x] & 0x80) >> 7;
                V[x] <<= 1;
                break;
            }
            pc += 2;
            break;

        case 0x9000: // SNE Vx, Vy
            pc += (V[x] != V[y]) ? 4 : 2;
            break;

        case 0xA000: // LD I, addr
            I = nnn;
            pc += 2;
            break;

        case 0xB000: // JP V0, addr
            pc = nnn + V[0];
            break;

        case 0xC000: // RND Vx, byte
            V[x] = (rand() % 256) & kk;
            pc += 2;
            break;

        case 0xD000: // DRW Vx, Vy, nibble
        {
            uint8_t vx = V[x];
            uint8_t vy = V[y];
            uint8_t height = n;
            V[0xF] = 0;

            for (int row = 0; row < height; row++)
            {
                uint8_t sprite = memory[I + row];
                for (int col = 0; col < 8; col++)
                {
                    if ((sprite & (0x80 >> col)) != 0)
                    {
                        int px = (vx + col) % 64;
                        int py = (vy + row) % 32;
                        if (display[py][px] == 1)
                            V[0xF] = 1;
                        display[py][px] ^= 1;
                    }
                }
            }
            pc += 2;
            break;
        }

        case 0xE000: // Key opcodes
            switch (kk)
            {
            case 0x9E: // SKP Vx
                pc += (keypad[V[x]]) ? 4 : 2;
                break;
            case 0xA1: // SKNP Vx
                pc += (!keypad[V[x]]) ? 4 : 2;
                break;
            }
            break;

        case 0xF000: // Misc
            switch (kk)
            {
            case 0x07:
                V[x] = delay_timer;
                break;
            case 0x0A: // LD Vx, K (wait for key press)
            {
                bool keyPressed = false;
                for (int i = 0; i < 16; i++)
                {
                    if (keypad[i])
                    {
                        V[x] = i;
                        keyPressed = true;
                        break;
                    }
                }
                if (!keyPressed)
                    return; // do not advance pc
                break;
            }
            case 0x15:
                delay_timer = V[x];
                break;
            case 0x18:
                sound_timer = V[x];
                break;
            case 0x1E:
                I += V[x];
                break;
            case 0x29:
                I = V[x] * 5;
                break; // sprite location
            case 0x33: // BCD
                memory[I] = V[x] / 100;
                memory[I + 1] = (V[x] / 10) % 10;
                memory[I + 2] = V[x] % 10;
                break;
            case 0x55: // LD [I], V0..Vx
                for (int i = 0; i <= x; i++)
                    memory[I + i] = V[i];
                break;
            case 0x65: // LD V0..Vx, [I]
                for (int i = 0; i <= x; i++)
                    V[i] = memory[I + i];
                break;
            }
            pc += 2;
            break;

        default:
            std::cerr << "Unknown opcode: " << std::hex << opcode << '\n';
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
            std::cout << "BEEP" << '\n';
        }
    }
};

int main()
{
    std::string progName;
    std::cout << "Available ROMs: " << std::endl;
    std::cout << "airplane\n";
    std::cout << "blitz\n";
    std::cout << "chip8logo\n";
    std::cout << "pong\n";
    std::cout << "space invaders\n";

    std::cout << "NOTE:These are the ROMs that comes with this repository,please place them in your compiler directory.\n";
    std::cout << "Program is working at 10 cycles per frame.\n";
    std::cout << "Provide the program name: ";
    getline(std::cin, progName);

    Chip8 chip8;
    chip8.initialize();
    chip8.loadProgram((progName + ".ch8").c_str());

    const int SCALE = 10;
    const int CYCLES_PER_FRAME = 10; // Cycle function needs to be executed this many times in a frame so it can actually show something on the screen
    const int FRAME_DELAY_MS = 16;   // 60 FPS

    // Makes the centered 64x32 window scaled by 10

    SDL_Window *window = SDL_CreateWindow("CHIP-8 Emulator",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          64 * SCALE, 32 * SCALE,
                                          SDL_WINDOW_SHOWN);

    // SDL renderer(the arguments of a function are window,graphic driver(-1 for the next available),and use GPU if available)

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Representation of a pixel

    SDL_Rect rect{0, 0, SCALE, SCALE};
    bool running = true;

    // Event holder

    SDL_Event e;

    // Keyboard

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
        SDL_RenderClear(renderer);                      // CLS

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Puts black as the color
        // Puts the pixels on the screen
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
