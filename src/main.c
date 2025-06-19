#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320

const int FRAME_DELAY = 1000 / 60; // ~16.67 ms per frame (60 FPS)

typedef unsigned char t_byte;
typedef unsigned short t_word;

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
bool drawFlag;
SDL_Renderer* renderer;

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

    drawFlag = true;
}

void cycle()
{
    // Fetch opcode
    opcode = memory[pc] << 8 | memory[pc + 1];
    //printf("opcode: 0x%X\n", opcode);

    // Decode opcode and execute
    switch(opcode & 0xF000) {
        default:
            //printf("Unknown opcode: 0x%X\n", opcode);
            break;
    }

    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) {
        if (sound_timer == 1) printf("BEEP!\n");
        sound_timer--;
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
    // TODO: use gfx array to draw pixels
    SDL_Rect pixel = {10, 10, 10, 10};
    SDL_RenderFillRect(renderer, &pixel);
}

int main()
{
    printf("chip8\n");

    init();
    // TODO: Load game
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Chip-8",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    SDL_Event event;
    int frame_start, frame_time;

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

        cycle();

        if (drawFlag) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            draw();
            SDL_RenderPresent(renderer);

            drawFlag = 0;
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
