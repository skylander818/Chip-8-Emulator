#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


//  cd "C:\Users\Stephen\Documents\GitHub\Emulators\Chip 8 Emulator"     // change directory to the project folder
// C:\Users\Stephen\Documents\GitHub\Emulators\ROMs\Airplane.ch8        // location of ROM
// C:\Users\Stephen\Documents\GitHub\Emulators\ROMs\IBMLogo.ch8
typedef struct config config_t;
typedef struct chip8_t chip8_t;
bool SDL_start(chip8_t* chip, config_t config);
bool chip_init(chip8_t* chip, const char *rom_file_path);
void emulate_instruction(chip8_t* chip);
void clear_screen(SDL_Renderer *renderer, config_t config);
typedef struct instructions_t{
    uint16_t opcode;
    uint16_t NNN; // 12 bit address/counter
    uint8_t NN; // 8 bit immediate number
    uint8_t N; // 4 bit number
    uint8_t X; 
    uint8_t Y;
} instructions_t;

typedef struct chip8_t {
    uint8_t memory[4096]; //Allocate memory
    uint8_t V[16]; //Allocate registers
    uint16_t I; //Allocate index register
    uint16_t PC; //Allocate program counter
    uint16_t stack[16];
    uint16_t *SP; // Points to the next free stack slot
    uint8_t sound_timer;
    uint8_t delay_timer;
    uint8_t keypad[16];
    uint8_t display[64 * 32];
    instructions_t instruction;
    bool running;
    bool isPaused;
} chip8_t;

typedef struct config{
    uint32_t SCREEN_WIDTH;
    uint32_t SCREEN_HEIGHT;
    uint32_t fg;
    uint32_t bg;
    uint32_t scale;
}config_t;



int main(int argc, char *argv[]){
    if (argc<2){
        printf("Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }
    chip8_t chip = {0};
    config_t config = {0};
    config.SCREEN_WIDTH = 64;
    config.SCREEN_HEIGHT = 32;
    config.fg = 0xFFFFFFFF;
    config.bg = 0x00000000;
    config.scale = 16;
    if (!chip_init(&chip, argv[1])) {
        return 1;
    }

    SDL_start(&chip, config);
    return 0;
}
#ifdef DEBUG
void print_debug_info(chip8_t* chip) {
    printf( "Address:0x%04x, Opcode: 0x%04x Desc:\n", chip->PC - 2, chip->instruction.opcode);
    switch  ((chip->instruction.opcode>> 12) & 0x0F){
        case 0x00:
            if (chip ->instruction.opcode == 0xE0){
            memset(chip->display, 0, sizeof(chip->display));
            printf("Clearing display\n");   
            } 
            else if (chip->instruction.opcode == 0xEE){
                if (chip->SP > chip->stack) {
                    printf("Returning from subroutine. New PC: 0x%03X\n", *(chip->SP - 1));
                } else {
                    printf("Cannot return: stack is empty\n");
                }
            }
            break;
        case 0x02:
            printf("Calling subroutine at 0x%03X\n", chip->instruction.NNN);
            break;
        case 0x06:
            printf("Set register V%X to NN (0x%02x)\n", chip->instruction.X, chip->instruction.NN);
            break;
        default:
            printf("Unknown opcode: 0x%X\n", chip->instruction.opcode);
            break;
    }
}
#endif

void emulate_instruction(chip8_t* chip) {
    chip->instruction.opcode = (chip->memory[chip->PC] << 8) | chip->memory[chip->PC + 1];
    chip->PC += 2;

    chip ->instruction.NNN = chip->instruction.opcode & 0x0FFF;
    chip ->instruction.NN = chip->instruction.opcode & 0x00FF;
    chip ->instruction.N = chip->instruction.opcode & 0x000F;
    chip ->instruction.X = (chip->instruction.opcode & 0x0F00) >> 8;
    chip ->instruction.Y = (chip->instruction.opcode & 0x00F0) >> 4;
#if DEBUG
    print_debug_info(chip);
#endif
//===========================OPCODE=====================================
    switch  ((chip->instruction.opcode>> 12) & 0x0F){
        case 0x00:
            if (chip ->instruction.opcode == 0xE0){
            memset(chip->display, 0, sizeof(chip->display));   
            } 
            else if (chip->instruction.opcode == 0xEE){
                chip->PC = *--chip->SP;
            }
            break;
        case 0x02:
            if (chip->SP < chip->stack + 16) {
                *chip->SP++ = chip->PC;
                chip->PC = chip->instruction.NNN;
            } else {
                printf("Cannot call subroutine: stack is full\n");
            }
            break;
        case 0x03:
            if (chip-> instruction.X == chip-> instruction.NN){
                chip->PC += 2;
            }
            break;
        case 0x06:
            chip->V[chip -> instruction.X]= chip->instruction.NN;
            break;
        case 0x0A:
            chip -> I = chip->instruction.NNN;
            break;
        case 0x0D:
            break;
            
        
        

        default:
            printf("Unknown opcode: 0x%X\n", chip->instruction.opcode);
            break;
    }
}



bool chip_init(chip8_t* chip, const char *rom_file_path) {
    const uint8_t Font[80] ={
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
    
    memcpy(&chip->memory[0x050], Font, sizeof(Font));
    chip->PC = 0x200; // Program counter starts at 0x200
    chip->I = 0;      // Reset index register
    chip->sound_timer = 0;
    chip->delay_timer = 0;
    chip->SP = chip->stack; // Reset stack pointer
    FILE *rom_file = fopen(rom_file_path, "rb");
    if (!rom_file) {
        printf("Failed to open ROM file: %s\n", rom_file_path);
        return false;
    }

    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    rewind(rom_file);

    if (rom_size > (4096 - 512)) {
        printf("ROM file is too large to fit in memory\n");
        fclose(rom_file);
        return false;
    }
    fread(&chip->memory[0x200], rom_size, 1, rom_file);

    fclose(rom_file);

    return true;
}



bool SDL_start(chip8_t* chip, config_t config) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_Window *window = SDL_CreateWindow("Chip 8 Emulator", config.SCREEN_WIDTH, config.SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    if (!SDL_SetRenderDrawColor(renderer, (config.bg >> 24) & 0xFF, (config.bg >> 16) & 0xFF, (config.bg >> 8) & 0xFF, config.bg & 0xFF)) {
        printf("SDL_SetRenderDrawColor failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    chip-> running = true;
    chip -> isPaused = false;
    SDL_Event event;

    while (chip->running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type){
                case SDL_EVENT_QUIT:
                chip->running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.repeat){
                    break;
                }
            switch (event.key.key){
                case SDLK_ESCAPE:
                    chip->running = false;
                    break;
                case SDLK_SPACE:
                    printf("Emulator paused. Press 'SPACE' to resume.\n");
                    printf("%s", chip->isPaused ? "Resuming..." : "Pausing...");
                    chip->isPaused = !chip->isPaused;
                    break;
                case SDLK_C:
                    clear_screen(renderer, config);
                    break;
                case SDLK_U:
                    SDL_SetRenderDrawColor(renderer, 255,0,0,0);
                    SDL_RenderClear(renderer);
                    break;
            }
            break;
            }
        }

        if (chip->isPaused) {
            SDL_Delay(16);
            continue;
        }
        emulate_instruction(chip);

        if (!SDL_RenderClear(renderer)) {
            printf("SDL_RenderClear failed: %s\n", SDL_GetError());
            chip->running = false;
            break;
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

void clear_screen(SDL_Renderer *renderer, config_t config) {
    const uint8_t r = (config.bg >> 24) & 0xFF;
    const uint8_t g = (config.bg >> 16) & 0xFF;
    const uint8_t b = (config.bg >> 8) & 0xFF;
    const uint8_t a = config.bg & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);
}
