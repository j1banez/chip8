#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
#define CYCLES_PER_SECOND 600

const int FRAME_DELAY = 1000 / 60; // ~16.67 ms per frame (60 FPS)

typedef unsigned char t_byte;
typedef unsigned short t_word; // Two bytes

t_byte memory[4096];

t_word opcode;
t_word pc; // Program counter

t_byte V[16]; // Registers

t_word stack[16];
t_word sp;

t_word I; // Index register

t_byte delay_timer;
t_byte sound_timer;

t_byte key[16]; // Keypad state, 0 if not pressed, 1 if pressed

t_byte gfx[64 * 32]; // Screen buffer
bool draw_flag;
SDL_Renderer* renderer;

t_byte fontset[80] =
{
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

void init()
{
    pc = 0x200; // Program starts at 0x200
    opcode = 0;
    I = 0;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;

    for (int i = 0; i < 4096; i++) memory[i] = 0;
    for (int i = 0; i < 2048; i++) gfx[i] = 0;
    for (int i = 0; i < 16; i++) {
        V[i] = 0;
        key[i] = 0;
        stack[i] = 0;
    }

    // Load fontset
    for (int i = 0; i < 80; i++) memory[i] = fontset[i];

    draw_flag = true;
    srand(time(NULL));
}

int load_game(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file) return 1;

    // Load game into memory starting at 0x200
    fread(memory + 0x200, sizeof(t_byte), 4096 - 0x200, file);
    fclose(file);

    return 0;
}

void cycle()
{
    // Fetch opcode
    opcode = memory[pc] << 8 | memory[pc + 1];

    printf("PC: %03X OPCODE: %04X\n", pc, opcode);

    // Decode opcode and execute
    switch(opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // 00E0 - CLS - Clear the screen
                    for (int i = 0; i < 2048; i++) gfx[i] = 0;
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE: // 00EE - RET - Return from subroutine
                    if (sp == 0) {
                        printf("Stack underflow!\n");
                        exit(1);
                    }
                    sp--;
                    pc = stack[sp];
                    break;
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
        case 0x1000: // 1NNN - JP addr - Jump to location NNN
            pc = opcode & 0x0FFF;
            break;
        case 0x2000: // 2NNN - Call addr - Call subroutine at NNN
            if (sp == 16) {
              printf("Stack overflow!\n");
              exit(1);
            }
            // Set to the next instruction to avoid infinite loop when returning.
            stack[sp] = pc + 2;
            sp++;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000: // 3XKK - SE Vx, byte - Skip next instruction if Vx = kk
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
              pc += 2;
            }
            pc += 2;
            break;
        case 0x4000: // 4XKK - SNE Vx, byte - Skip next instruction if Vx != kk
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
              pc += 2;
            }
            pc += 2;
            break;
        case 0x5000: // 5XY0 - SE Vx, Vy - Skip next instruction if Vx = Vy
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
              pc += 2;
            }
            pc += 2;
            break;
        case 0x6000: // 6XKK - LD Vx, byte - Set Vx = kk
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000: // 7xkk - ADD Vx, byte - Set Vx = Vx + kk
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;
        case 0x8000:
            switch(opcode & 0x000F) {
              case 0x0000: // 8XY0 - LD Vx, Vy - Set Vx = Vy
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                pc += 2;
                break;
              case 0x0001: // 8XY1 - OR Vx, Vy - Set Vx = Vx OR Vy
                V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                pc += 2;
                break;
              case 0x0002: // 8XY2 - AND Vx, Vy - Set Vx = Vx AND Vy
                V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                pc += 2;
                break;
              case 0x0003: // 8XY3 - XOR Vx, Vy - Set Vx = Vx XOR Vy
                V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                pc += 2;
                break;
              case 0x0004: // 8XY4 - ADD Vx, Vy - Set Vx = Vx + Vy, set VF = carry
                V[0xF] = V[(opcode & 0x0F00) >> 8] > (255 - V[(opcode & 0x00F0) >> 4]) ? 1 : 0;
                V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                pc += 2;
                break;
              case 0x0005: // 8XY5 - SUB Vx, Vy - Set Vx = Vx - Vy, set VF = NOT borrow
                V[0xF] = V[(opcode & 0x0F00) >> 8] >= V[(opcode & 0x00F0) >> 4] ? 1 : 0;
                V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                pc += 2;
                break;
              case 0x0006: // 8XY6 - SHR Vx {, Vy} - Set Vx = Vx SHR 1
                V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                V[(opcode & 0x0F00) >> 8] >>= 1;
                pc += 2;
                break;
              case 0x0007: // 8XY7 - SUBN Vx, Vy - Set Vx = Vy - Vx, set VF = NOT borrow
                V[0xF] = V[(opcode & 0x00F0) >> 4] >= V[(opcode & 0x0F00) >> 8] ? 1 : 0;
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                pc += 2;
                break;
              case 0x000E: // 8XYE - SHL Vx {, Vy} - Set Vx = Vx SHL 1
                V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                V[(opcode & 0x0F00) >> 8] <<= 1;
                pc += 2;
                break;
            }
            break;
        case 0x9000: // 9XY0 - SNE Vx, Vy - Skip next instruction if Vx != Vy
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
              pc += 2;
            }
            pc += 2;
            break;
        case 0xA000: // ANNN - LD I, addr - Set I = nnn
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0xB000: // BNNN - JP V0, addr - Jump to location nnn + V0
            pc = (opcode & 0x0FFF) + V[0];
            break;
        case 0xC000: // CXKK - RND Vx, byte - Set Vx = random byte AND kk
            V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
            pc += 2;
            break;
        // DXYN - DRW Vx, Vy, nibble Display n-byte sprite starting
        // at memory location I at (Vx, Vy), set VF = collision
        case 0xD000: {
            t_byte x = (opcode & 0x0F00) >> 8;
            t_byte y = (opcode & 0x00F0) >> 4;
            t_byte n = opcode & 0x000F;
            t_byte sprite_line;

            V[0xF] = 0;

            // Loop over sprite height (n)
            for (int i = 0; i < n; i++) {
                sprite_line = memory[I + i];
                // Loop over sprite width, always 8
                for (int j = 0; j < 8; j++) {
                    // Compare each pixel of the current sprite line
                    if (sprite_line & (0x80 >> j)) {
                        // Handle screen warping
                        int px = (V[x] + j) % 64;
                        int py = (V[y] + i) % 32;

                        if (gfx[(py * 64) + px] == 1) V[0xF] = 1;
                        gfx[(py * 64) + px] ^= 1;
                    }
                }
            }

            draw_flag = true;
            pc += 2;
            break;
        }
        case 0xE000:
            switch (opcode & 0x000F) {
                // EX9E - SKP Vx
                // Skip next instruction if key with the value of Vx is pressed
                case 0x000E:
                    if (key[V[(opcode & 0x0F00) >> 8]] == 1) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
                // EXA1 - SKNP Vx
                // Skip next instruction if key with the value of Vx is not pressed
                case 0x0001:
                    if (key[V[(opcode & 0x0F00) >> 8]] == 0) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                // FX07 - LD Vx, DT - Set Vx = delay timer value
                case 0x0007:
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;
                // FX0A - LD Vx, K
                // Wait for a key press, store the value of the key in Vx
                case 0x000A: {
                    bool key_pressed = false;

                    for (int i = 0; i < 16; i++) {
                        if (key[i]) {
                            V[(opcode & 0x0F00) >> 8] = i;
                            pc += 2;
                            key_pressed = true;
                            break;
                        }
                    }

                    // Ensure this cycle does not continue
                    if (!key_pressed) return;

                    break;
                }
                // FX15 - LD DT, Vx - Set delay timer = Vx
                case 0x0015:
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                // FX18 - LD ST, Vx - Set sound timer = Vx
                case 0x0018:
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                // FX1E - ADD I, Vx - Set I = I + Vx
                case 0x001E:
                    I = I + V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                // FX29 - LD F, Vx - Set I = location of sprite for digit Vx
                case 0x0029:
                    I = V[(opcode & 0x0F00) >> 8] * 5;
                    pc += 2;
                    break;
                // FX33 - LD B, Vx
                // Store BCD representation of Vx in memory locations I, I+1, and I+2
                case 0x0033: {
                    t_byte x = (opcode & 0x0F00) >> 8;
                    memory[I] = V[x] / 100;
					memory[I + 1] = (V[x] / 10) % 10;
					memory[I + 2] = V[x] % 10;
					pc += 2;
                    break;
                }
                // FX55 - LD [I], Vx
                // Store registers V0 through Vx in memory starting at location I
                case 0x0055: {
                    t_byte x = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    pc += 2;
                    break;
                }
                // FX65 - LD Vx, [I]
                // Read registers V0 through Vx from memory starting at location I
                case 0x0065: {
                    t_byte x = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    pc += 2;
                    break;
                }
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
        default:
            printf("Unknown opcode: 0x%X\n", opcode);
            break;
    }
}

/*
** Map SDL keys to Chip-8 keys
**
** 1 2 3 4 -> 1 2 3 C
** Q W E R -> 4 5 6 D
** A S D F -> 7 8 9 E
** Z X C V -> A 0 B F
*/
int map_sdl_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;
        case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;
        case SDLK_v: return 0xF;
        default: return -1;
    }
}

/*
** Draw the screen buffer to the SDL renderer
*/
void draw()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < 2048; i++) {
      if (gfx[i]) {
        int x = i % 64;
        int y = i / 64;
        // x10 scaling
        SDL_Rect pixel = {x * 10, y * 10, 10, 10};
        SDL_RenderFillRect(renderer, &pixel);
      }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <chip8_rom>\n", argv[0]);
        return 1;
    }

    init();

    if (load_game(argv[1]) == 1) {
        fprintf(stderr, "Failed to load game: %s\n", argv[1]);
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Chip-8",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    SDL_Event event;
    int frame_start, frame_time;

    printf("Press ESC to exit.\n");

    while (running)
    {
        frame_start = SDL_GetTicks();

        // Handle inputs
        while (SDL_PollEvent(&event)) {
            int mapped;

            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else {
                        // printf("Key down: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                        mapped = map_sdl_key(event.key.keysym.sym);
                        if (mapped != -1) key[mapped] = 1; // Set key state to pressed
                    }
                    break;
                case SDL_KEYUP:
                    // printf("Key up: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                    mapped = map_sdl_key(event.key.keysym.sym);
                    if (mapped != -1) key[mapped] = 0; // Set key state to released
                    break;
                default:
                    break;
            }
        }

        for (int i = 0; i < (CYCLES_PER_SECOND / 60); i++) cycle();

        if (delay_timer > 0) delay_timer--;
        if (sound_timer > 0) {
            printf("BEEP!\n");
            sound_timer--;
        }

        if (draw_flag) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            draw();
            SDL_RenderPresent(renderer);

            draw_flag = 0;
        }

        // Delay to maintain frame rate at ~60 FPS
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
