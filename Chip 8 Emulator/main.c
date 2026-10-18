#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

//  cd "C:\Users\Stephen\Documents\GitHub\Emulators\Chip 8 Emulator"     // change directory to the project folder
// C:\Users\Stephen\Documents\GitHub\Emulators\ROMs\Airplane.ch8        // location of ROM
// C:\Users\Stephen\Documents\GitHub\Emulators\ROMs\IBMLogo.ch8
typedef struct config config_t;
typedef struct chip8_t chip8_t;
bool SDL_start(chip8_t* chip, config_t config);
bool chip_init(chip8_t* chip, const char *rom_file_path, config_t config);
void emulate_instruction(chip8_t* chip, config_t config);
void clear_screen(SDL_Renderer *renderer, config_t config);
void key_input(chip8_t* chip, SDL_Keycode key, bool pressed);
void update_screen(SDL_Renderer *renderer, chip8_t* chip8, config_t config);
void update_timer(chip8_t* chip);

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
    bool display[64 * 32];
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
    uint32_t clock;
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
    config.clock = 700;


    if (!chip_init(&chip, argv[1], config)) {
        return 1;
    }
    srand(time(NULL));

    SDL_start(&chip, config);
    
    return 0;
}
#ifdef DEBUG
void print_debug_info(chip8_t* chip) {
    printf( "Address:0x%04x, Opcode: 0x%04x Desc:\n", chip->PC - 2, chip->instruction.opcode);
    switch  ((chip->instruction.opcode>> 12) & 0x0F){
        case 0x00:
            if (chip ->instruction.opcode == 0x00E0){
            printf("Clearing display\n");
            }
            else if (chip->instruction.opcode == 0x00EE){
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

void emulate_instruction(chip8_t* chip, config_t config) {
    chip->instruction.opcode = (chip->memory[chip->PC] << 8) | chip->memory[chip->PC + 1];
    chip->PC += 2;

    chip ->instruction.NNN = chip->instruction.opcode & 0x0FFF;
    chip ->instruction.NN = chip->instruction.opcode & 0x00FF;
    chip ->instruction.N = chip->instruction.opcode & 0x000F;
    chip ->instruction.X = (chip->instruction.opcode & 0x0F00) >> 8;
    chip ->instruction.Y = (chip->instruction.opcode & 0x00F0) >> 4;
#ifdef DEBUG
    print_debug_info(chip);
#endif
//===========================OPCODE=====================================
    switch  ((chip->instruction.opcode>> 12) & 0x0F){
        case 0x00:
            if (chip ->instruction.opcode == 0x00E0){
            memset(chip->display, 0, sizeof(chip->display));   
            } 
            else if (chip->instruction.opcode == 0x00EE){
                chip->PC = *--chip->SP;
            }
            break;
        case 0x01:
            chip->PC = chip->instruction.NNN;
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
            if (chip->V[chip->instruction.X] == chip->instruction.NN){
                chip->PC += 2;
            }
            break;
        case 0x04:
            if (chip->V[chip->instruction.X] != chip->instruction.NN){
                chip->PC +=2;
            
            }
            break;
        case 0x05:
            if (chip-> V[chip->instruction.X] == chip->V[chip->instruction.Y]){
                chip->PC+=2;
            }
            break;
        case 0x06:
            chip->V[chip -> instruction.X]= chip->instruction.NN;
            break;
        case 0x07:
            chip -> V[chip->instruction.X] += chip -> instruction.NN;
            break;
        case 0x08:
            switch (chip->instruction.opcode &0x000F){
                case 0x0000:
                    chip->V[chip->instruction.X] = chip->V[chip->instruction.Y]; //ok
                    break;
                case 0x0001:
                    chip->V[chip->instruction.X] |= chip->V[chip->instruction.Y]; //K
                    break;
                case 0x0002:
                    chip->V[chip->instruction.X] &= chip->V[chip->instruction.Y];//K
                    break;
                case 0x0003:
                    chip->V[chip->instruction.X] ^= chip->V[chip->instruction.Y]; //K
                    break;
                case 0x0004: {
                    uint16_t sumXY = chip->V[chip->instruction.X] + chip->V[chip->instruction.Y];
                    chip->V[chip->instruction.X] = sumXY & 0xFF;
                    chip->V[0xF] = sumXY> 0xFF;
                    break;
                }
                case 0x0005: {
                    uint8_t flag = chip->V[chip->instruction.X] >= chip-> V[chip->instruction.Y];
                    chip->V[chip->instruction.X] -= chip->V[chip->instruction.Y];
                    chip->V[0xF] = flag;
                    break;
                }
                case 0x0006:{
                    uint8_t flag1 = chip->V[chip->instruction.X] & 1;
                    chip->V[chip->instruction.X] >>=1;
                    chip->V[0xF] = flag1;
                    break;
                }
                case 0x0007: {
                    uint8_t flag = chip->V[chip->instruction.Y] >= chip->V[chip->instruction.X];
                    chip->V[chip->instruction.X] = chip->V[chip->instruction.Y] - chip->V[chip->instruction.X];
                    chip->V[0xF] = flag;
                    break;
                }
                case 0x000E: {
                    uint8_t flag2 = (chip->V[chip->instruction.X] >> 7) & 1;
                    chip->V[chip->instruction.X] <<= 1;
                    chip->V[0xF] = flag2;
                    break;
                }
            }
            break;
        case 0x09:
            if (chip->V[chip->instruction.X] != chip->V[chip->instruction.Y]){
                chip->PC+=2;
            }
            break;
        case 0x0A:
            chip -> I = chip->instruction.NNN;
            break;
        case 0x0B:
            chip-> PC = chip->V[0] + chip->instruction.NNN;
            break;
        case 0x0C:
            chip->V[chip->instruction.X] = rand() % 256 & chip->instruction.NN;
            break;
        case 0x0D:{
            uint8_t X_Coord = chip -> V[chip-> instruction.X] % config.SCREEN_WIDTH;
            uint8_t Y_Coord = chip -> V[chip-> instruction.Y] % config.SCREEN_HEIGHT;
            const uint8_t X_OG = X_Coord;
            chip -> V[0xF] = 0;
            for (uint8_t i = 0; i<chip->instruction.N; i++){
                const uint8_t sprite = chip -> memory[chip->I + i];
                X_Coord = X_OG; 
                for (int8_t j = 7; j>= 0; j--){
                    bool *pixel = &chip->display[Y_Coord * config.SCREEN_WIDTH + X_Coord];
                    if ((sprite &(1<<j)) && *pixel){
                        chip->V[0xf]= 1;
                    }
                    *pixel ^=  ((sprite & (1<<j)) != 0);

                    if (++X_Coord >= config.SCREEN_WIDTH) break;
                    }
                if (++Y_Coord>= config.SCREEN_HEIGHT) break;
                }
            break;
            }
        case 0x0E:
            if (chip->instruction.NN == 0x9E){
                if (chip->keypad[chip->V[chip->instruction.X]])
                chip->PC +=2;
            }else if (chip->instruction.NN == 0xA1){
                if (!chip->keypad[chip->V[chip->instruction.X]])
                chip->PC +=2;
            }
            break;
        case 0x0F:
            switch(chip->instruction.NN){
                case 0x07:
                    chip->V[chip->instruction.X] = chip->delay_timer;
                    break;
                case 0x0A: {
                    bool key_pressed = false;
                    for (uint8_t i = 0; i<sizeof(chip->keypad); i++)
                        if (chip->keypad[i]){
                            chip->V[chip->instruction.X] = i;
                            key_pressed = true;
                            break;
                        }
                    if (!key_pressed) chip->PC -=2;
                    break;
                }
                case 0x15:
                    chip->delay_timer = chip->V[chip->instruction.X];
                    break;
                case 0x18:
                    chip->sound_timer = chip->V[chip->instruction.X];
                    break;
                case 0x1E:
                    chip->I+= chip->V[chip->instruction.X];
                    break;
                case 0x29:
                    chip->I = 0x050 + (chip->V[chip->instruction.X] & 0x0F) * 5;
                    break;
                case 0x33:{
                    uint8_t bcd = chip->V[chip->instruction.X];
                    chip->memory[chip->I+2] = bcd % 10;
                    bcd /=10;
                    chip->memory[chip->I +1] = bcd % 10;
                    bcd /=10;
                    chip->memory[chip->I] =bcd;
                    }
                    break;
                case 0x55:
                    for (uint8_t i = 0; i<=chip->instruction.X;i++){
                        chip->memory[chip->I + i] = chip-> V[i];
                    }
                    break;
                case 0x65:
                    for (uint8_t i = 0; i<=chip->instruction.X;i++){
                        chip->V[i] = chip->memory[chip->I +i];
                    }
                    break;    
                default:
                    printf("Unimplemented opcode: 0x%04X at address 0x%03X\n", chip->instruction.opcode, chip->PC - 2);
            }
            
            break;
        default:{
            printf("Unimplemented opcode: 0x%04X at address 0x%03X\n", chip->instruction.opcode, chip->PC - 2);
        }
    
    }
}



bool chip_init(chip8_t* chip, const char *rom_file_path, config_t config) {
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

    SDL_Window *window = SDL_CreateWindow("Chip 8 Emulator", config.SCREEN_WIDTH *config.scale, config.SCREEN_HEIGHT *config.scale, SDL_WINDOW_RESIZABLE);
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

    SDL_SetRenderLogicalPresentation(renderer, config.SCREEN_WIDTH * config.scale, config.SCREEN_HEIGHT * config.scale, SDL_LOGICAL_PRESENTATION_LETTERBOX);

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
            key_input(chip, event.key.key, true);
            switch (event.key.key){
                case SDLK_ESCAPE:
                    chip->running = false;
                    break;
                case SDLK_SPACE:
                    printf("%s\n", chip->isPaused ? "Resuming..." : "Paused. Press SPACE to resume.");
                    chip->isPaused = !chip->isPaused;
                    break;
                case SDLK_P:
                    clear_screen(renderer, config);
                    break;
                case SDLK_U:
                    config.fg = 0xFF000000;
                    SDL_RenderClear(renderer);
                    break;
            }
            break;
            case SDL_EVENT_KEY_UP:
                key_input(chip, event.key.key, false);
                break;
            }
        }

        if (chip->isPaused) {
            SDL_Delay(16);
            continue;
        }

        const uint64_t start_time = SDL_GetPerformanceCounter();
        for (uint32_t i = 0; i< config.clock/60; i++)
            emulate_instruction(chip, config);
        const uint64_t end_time = SDL_GetPerformanceCounter();
        double time_elapsed = (double)(end_time-start_time) * 1000.0 / SDL_GetPerformanceFrequency();
        SDL_Delay(16.67f>time_elapsed ? 16.67f -time_elapsed:0);
        update_screen (renderer, chip, config);
        update_timer(chip);

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}
void update_screen(SDL_Renderer *renderer, chip8_t* chip, config_t config){
    SDL_FRect rect = {.x = 0, .y = 0, .w = (float)config.scale, .h = (float)config.scale};

    const uint8_t fg_r = (config.fg >> 24) & 0xFF;
    const uint8_t fg_g = (config.fg >> 16) & 0xFF;
    const uint8_t fg_b = (config.fg >> 8) & 0xFF;
    const uint8_t fg_a = config.fg & 0xFF;

    const uint8_t bg_r = (config.bg >> 24) & 0xFF;
    const uint8_t bg_g = (config.bg >> 16) & 0xFF;
    const uint8_t bg_b = (config.bg >> 8) & 0xFF;
    const uint8_t bg_a = config.bg & 0xFF;

    SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, bg_a);
    SDL_RenderClear(renderer);

    for (uint32_t i = 0; i <sizeof chip->display; i++){
        rect.x = (float)(i % config.SCREEN_WIDTH * config.scale);
        rect.y = (float)(i / config.SCREEN_WIDTH * config.scale);

        if (chip ->display[i]) {
            SDL_SetRenderDrawColor(renderer, fg_r,fg_g,fg_b,fg_a);
            SDL_RenderFillRect(renderer, &rect);
            
            SDL_SetRenderDrawColor(renderer, bg_r,bg_g,bg_b,bg_a);
            SDL_RenderRect(renderer, &rect);

        }
    }
    SDL_RenderPresent(renderer);
}

void clear_screen(SDL_Renderer *renderer, config_t config) {
    const uint8_t r = (config.bg >> 24) & 0xFF;
    const uint8_t g = (config.bg >> 16) & 0xFF;
    const uint8_t b = (config.bg >> 8) & 0xFF;
    const uint8_t a = config.bg & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);
}

void key_input(chip8_t* chip, SDL_Keycode key, bool pressed){
    switch (key){
        case SDLK_1: chip->keypad[0x1] = pressed; break;
        case SDLK_2: chip->keypad[0x2] = pressed; break;
        case SDLK_3: chip->keypad[0x3] = pressed; break;
        case SDLK_4: chip->keypad[0xC] = pressed; break;

        case SDLK_Q: chip->keypad[0x4] = pressed; break;
        case SDLK_W: chip->keypad[0x5] = pressed; break;
        case SDLK_E: chip->keypad[0x6] = pressed; break;
        case SDLK_R: chip->keypad[0xD] = pressed; break;

        case SDLK_A: chip->keypad[0x7] = pressed; break;
        case SDLK_S: chip->keypad[0x8] = pressed; break;
        case SDLK_D: chip->keypad[0x9] = pressed; break;
        case SDLK_F: chip->keypad[0xE] = pressed; break;

        case SDLK_Z: chip->keypad[0xA] = pressed; break;
        case SDLK_X: chip->keypad[0x0] = pressed; break;
        case SDLK_C: chip->keypad[0xB] = pressed; break;
        case SDLK_V: chip->keypad[0xF] = pressed; break;
    }
}
void update_timer(chip8_t* chip){
    if (chip->delay_timer>0){
    chip->delay_timer -=1;
    }
    if (chip->sound_timer>0){
    chip->sound_timer -=1;
    }
}