#include <stdio.h>

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
int drawFlag;

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
  for (int i = 0; i < 16; i++)
  {
    V[i] = 0;
    key[i] = 0;
    stack[i] = 0;
  } 

  drawFlag = 1;
}

void cycle()
{
  // Fetch opcode
  opcode = memory[pc] << 8 | memory[pc + 1];
  printf("opcode: 0x%X\n", opcode);

  // Decode opcode and execute
  switch(opcode & 0xF000)
  {
    default:
      printf("Unknown opcode: 0x%X\n", opcode);
  }

  if (delay_timer > 0) delay_timer--;
  if (sound_timer > 0)
  {
    if (sound_timer == 1) printf("BEEP!\n");
    sound_timer--;
  }
}

void draw()
{

}

void set_keys()
{

}

int main()
{
  printf("chip8\n");

  init();

  // TODO: Setup graphics
  // TODO: Setup inputs
  // TODO: Load game
  
  while (42)
  {
    // TODO: Cap at 60 FPS
    cycle();

    if (drawFlag)
    {
      draw();
      drawFlag = 0;
    }

    set_keys();
  }

  return 0;
}
