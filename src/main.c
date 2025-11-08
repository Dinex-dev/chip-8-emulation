#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "constants.h"
#include "registers.h"

void load_rom(const char *filename, unsigned char *memory, size_t memory_size)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Error opening ROM file: %s\n", filename);
        return;
    }
    size_t bytes_read = fread(memory + ROM_START_ADDRESS, sizeof(unsigned char), memory_size - ROM_START_ADDRESS, file);
    if (bytes_read == 0)
    {
        fprintf(stderr, "Error reading ROM file: %s\n", filename);
    }
    fclose(file);
}

void initialize_memory()
{
    // Initialize memory with fontset
    for (int i = 0; i < FONTSET_SIZE; i++)
    {
        memory[i] = fontset[i];
    }
}

char rand_byte()
{
    // Generate a random byte
    return (unsigned char)(rand() % 256);
}
short get_op_code()
{
    if (pc + 1 >= MEMORY_SIZE)
    {
        fprintf(stderr, "Program counter out of bounds: 0x%04X\n", pc);
        return 0x0000;
    }
    // Get the current opcode from memory
    unsigned short opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2; // Move the program counter to the next instruction
    if (pc >= MEMORY_SIZE)
    {
        fprintf(stderr, "Program counter out of bounds after fetching opcode: 0x%04X\n", pc);
        return 0x0000;
    }
    return opcode;
}

void initialize_window(SDL_Window **window, SDL_Renderer **renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    *window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * DISPLAY_SCALE, DISPLAY_HEIGHT * DISPLAY_SCALE, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    if (*window == NULL)
    {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(*renderer, DISPLAY_SCALE, DISPLAY_SCALE);
    if (*renderer == NULL)
    {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }
}

void clear_display()
{
    // Clear the display
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
        for (int x = 0; x < DISPLAY_WIDTH; x++)
        {
            display[x][y] = false;
        }
    }
}

void start_emulator()
{
    opcode = get_op_code();
    // Process the opcode here
    unsigned char x = (opcode & 0x0F00) >> 8;
    unsigned char y = (opcode & 0x00F0) >> 4;
    unsigned char nn = opcode & 0x00FF;
    switch (opcode & 0xF000)
    {
    case 0x0000:
        switch (opcode & 0x00FF)
        {
        case 0x00E0:
            clear_display();
            break;
        case 0x00EE:
            // Return from subroutine
            if (sp > 0)
            {
                pc = pop_stack();
            }
            else
            {
                fprintf(stderr, "Stack underflow on return.\n");
            }
            break;
        default:
            fprintf(stderr, "Unknown opcode: 0x%04X\n", opcode);
            break;
        }
        break;
    case 0x1000:
        // Jump to address NNN
        pc = opcode & 0x0FFF;
        break;
    case 0x2000:
        // Call subroutine at NNN
        if (sp < STACK_SIZE)
        {
            push_stack(pc);
            pc = opcode & 0x0FFF;
        }
        else
        {
            fprintf(stderr, "Stack overflow on subroutine call.\n");
        }
        break;
    case 0x3000:
        // Skip next instruction if Vx == NN

        if (V[x] == nn)
        {
            pc += 2; // Skip next instruction
        }
        break;
    case 0x4000:
        // Skip next instruction if Vx != NN

        if (V[x] != nn)
        {
            pc += 2; // Skip next instruction
        }
        break;
    case 0x5000:
        // Skip next instruction if Vx == Vy

        if (V[x] == V[y])
        {
            pc += 2; // Skip next instruction
        }
        break;
    case 0x6000:
        // Set Vx = nn

        V[x] = nn;
        break;
    case 0x7000:
        // Set Vx = Vx + nn

        V[x] += nn;
        break;
    case 0x8000:
        switch (opcode & 0x000F)
        {

        case 0x0000:
            // Set Vx = Vy
            V[x] = V[y];
            break;
        case 0x0001:
            // Set Vx = Vx OR Vy
            V[x] |= V[y];
            break;
        case 0x0002:
            // Set Vx = Vx AND Vy
            V[x] &= V[y];
            break;
        case 0x0003:
            // Set Vx = Vx XOR Vy
            V[x] ^= V[y];
            break;
        case 0x0004:
            // Set Vx = Vx + Vy, set VF = carry
            if (V[x] + V[y] > 255)
            {
                V[0xF] = 1; // Set carry flag
            }
            else
            {
                V[0xF] = 0;
            }
            V[x] += V[y];
            break;
        case 0x0005:
            // Set Vx = Vx - Vy, set VF = NOT borrow
            if (V[x] >= V[y])
            {
                V[0xF] = 1; // No borrow
            }
            else
            {
                V[0xF] = 0; // Borrow occurred
            }
            V[x] -= V[y];
            break;
        case 0x0006:
            // Set Vx = Vx SHR 1, set VF = LSB of Vx before shift
            char lsb = V[x] & 0x01; // Get least significant bit
            V[0xF] = lsb;           // Set VF to LSB
            V[x] >>= 1;             // Shift right / divide by 2
            break;
        case 0x0007:
            // Set Vx = Vy - Vx, set VF = NOT borrow
            if (V[y] >= V[x])
            {
                V[0xF] = 1; // No borrow
            }
            else
            {
                V[0xF] = 0; // Borrow occurred
            }
            V[x] = V[y] - V[x];
            break;
        case 0x000E:
            // Set Vx = Vx SHL 1, set VF = MSB of Vx before shift
            char msb = (V[x] & 0x80) >> 7; // Get most significant bit
            V[0xF] = msb;                  // Set VF to MSB
            V[x] <<= 1;                    // Shift left / multiply by 2
            break;
        default:
            fprintf(stderr, "Unknown opcode: 0x%04X\n", opcode);
            break;
        }
        break;
    case 0x9000:
        // Skip next instruction if Vx != Vy

        if (V[x] != V[y])
        {
            pc += 2; // Skip next instruction
        }
        break;
    case 0xA000:
        // Set I = NNN
        I = opcode & 0x0FFF;
        break;
    case 0xB000:
        // Jump to address NNN + V0
        pc = (opcode & 0x0FFF) + V[0];
        break;
    case 0xC000:
        // Set Vx = random byte AND NN

        V[x] = rand_byte() & nn;
        break;

    case 0xD000:
        // Draw sprite at (Vx, Vy) with height N

        unsigned char height = opcode & 0x000F;
        V[0xF] = 0; // Clear collision flag
        for (int row = 0; row < height; row++)
        {
            unsigned char sprite_row = memory[I + row];
            for (int col = 0; col < 8; col++)
            {
                if ((sprite_row & (0x80 >> col)) != 0)
                {
                    int pixel_x = (V[x] + col) % DISPLAY_WIDTH;
                    int pixel_y = (V[y] + row) % DISPLAY_HEIGHT;
                    if (display[pixel_x][pixel_y])
                    {
                        V[0xF] = 1; // Collision detected
                    }
                    display[pixel_x][pixel_y] ^= true; // Toggle pixel
                }
            }
        }
        break;
    case 0xE000:
        switch (opcode & 0x00FF)
        {
        case 0x009E:
            // Skip next instruction if key Vx is pressed
            if (keypad[V[x]] != 0)
            {
                pc += 2; // Skip next instruction
            }
            break;
        case 0x00A1:
            // Skip next instruction if key Vx is not pressed

            if (keypad[V[x]] == 0)
            {
                pc += 2; // Skip next instruction
            }
            break;
        default:
            fprintf(stderr, "Unknown opcode: 0x%04X\n", opcode);
            break;
        }
        break;
    case 0xF000:
        switch (opcode & 0x00FF)
        {

        case 0x0007:
            // Set Vx = delay timer value
            V[x] = delay;
            break;
        case 0x000A:
            // Wait for a key press, store the value in Vx
            bool key_pressed = false;
            for (int i = 0; i < NUM_KEYS; i++)
            {
                if (keypad[i] != 0)
                {
                    V[x] = i;
                    key_pressed = true;
                    break;
                }
            }
            if (!key_pressed)
            {
                pc -= 2; // Go back to repeat this instruction until key is pressed
            }
            break;
        case 0x0015:
            // Set delay timer = Vx
            delay = V[x];
            break;
        case 0x0018:
            // Set sound timer = Vx
            sound = V[x];
            break;
        case 0x001E:
            // Set I = I + Vx
            I += V[x];
            break;
        case 0x0029:
            // Set I = location of sprite for digit Vx
            I = V[x] * 5; // Each sprite is 5 bytes
            break;
        case 0x0033:
            // Store BCD representation of Vx in memory at I
            memory[I] = V[x] / 100;
            memory[I + 1] = (V[x] / 10) % 10;
            memory[I + 2] = V[x] % 10;
            break;
        case 0x0055:
            // Store registers V0 to Vx in memory starting at I
            for (int i = 0; i <= x; i++)
            {
                memory[I + i] = V[i];
            }
            break;
        case 0x0065:
            // Read registers V0 to Vx from memory starting at I
            for (int i = 0; i <= x; i++)
            {
                V[i] = memory[I + i];
            }
            break;
        default:
            fprintf(stderr, "Unknown opcode: 0x%04X\n", opcode);
            break;
        }
        break; // break for 0xF000 case
    default:
        fprintf(stderr, "Unknown opcode: 0x%04X\n", opcode);
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }
    initialize_memory();
    load_rom(argv[1], memory, MEMORY_SIZE);
    
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    initialize_window(&window, &renderer);
    if (window == NULL || renderer == NULL)
    {
        fprintf(stderr, "Failed to initialize window or renderer.\n");
        return 1;
    }
    // Main loop
    bool running = true;
    SDL_Event event;

    while (running)
    {
        SDL_Delay(16); // ~60 FPS

        // Decrement timers at 60Hz
        if (delay > 0) delay--;
        if (sound > 0) sound--;

        start_emulator();

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                // Handle key press
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    keypad[0x1] = 1;
                    break;
                case SDLK_2:
                    keypad[0x2] = 1;
                    break;
                case SDLK_3:
                    keypad[0x3] = 1;
                    break;
                case SDLK_4:
                    keypad[0xC] = 1;
                    break;
                case SDLK_q:
                    keypad[0x4] = 1;
                    break;
                case SDLK_w:
                    keypad[0x5] = 1;
                    break;
                case SDLK_e:
                    keypad[0x6] = 1;
                    break;
                case SDLK_r:
                    keypad[0xD] = 1;
                    break;
                case SDLK_a:
                    keypad[0x7] = 1;
                    break;
                case SDLK_s:
                    keypad[0x8] = 1;
                    break;
                case SDLK_d:
                    keypad[0x9] = 1;
                    break;
                case SDLK_f:
                    keypad[0xE] = 1;
                    break;
                case SDLK_z:
                    keypad[0xA] = 1;
                    break;
                case SDLK_x:
                    keypad[0x0] = 1;
                    break;
                case SDLK_c:
                    keypad[0xB] = 1;
                    break;
                case SDLK_v:
                    keypad[0xF] = 1;
                    break;
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    keypad[0x1] = 0;
                    break;
                case SDLK_2:
                    keypad[0x2] = 0;
                    break;
                case SDLK_3:
                    keypad[0x3] = 0;
                    break;
                case SDLK_4:
                    keypad[0xC] = 0;
                    break;
                case SDLK_q:
                    keypad[0x4] = 0;
                    break;
                case SDLK_w:
                    keypad[0x5] = 0;
                    break;
                case SDLK_e:
                    keypad[0x6] = 0;
                    break;
                case SDLK_r:
                    keypad[0xD] = 0;
                    break;
                case SDLK_a:
                    keypad[0x7] = 0;
                    break;
                case SDLK_s:
                    keypad[0x8] = 0;
                    break;
                case SDLK_d:
                    keypad[0x9] = 0;
                    break;
                case SDLK_f:
                    keypad[0xE] = 0;
                    break;
                case SDLK_z:
                    keypad[0xA] = 0;
                    break;
                case SDLK_x:
                    keypad[0x0] = 0;
                    break;
                case SDLK_c:
                    keypad[0xB] = 0;
                    break;
                case SDLK_v:
                    keypad[0xF] = 0;
                    break;
                }
            }
        }

        // Draw the display
        for (int y = 0; y < DISPLAY_HEIGHT; y++)
        {
            for (int x = 0; x < DISPLAY_WIDTH; x++)
            {
                if (display[x][y])
                {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                }
                else
                {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                }
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
        // Update the screen
        SDL_RenderPresent(renderer);
        // Delay to control frame rate
    }

    return 0;
}