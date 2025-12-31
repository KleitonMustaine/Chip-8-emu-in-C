#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define FONTSET_SIZE 80

typedef struct{
    
    uint32_t window_width; //SDL WINDOW W
    uint32_t window_heigh; //SDL WINDOW H
    uint32_t fg_color;  
    uint32_t bg_color;
    uint32_t scale_factor;
    uint8_t pixel;

}config_t;

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

typedef struct{

    uint8_t V[16];
    uint8_t memory[4096];
    uint8_t keyboard[16];
    uint8_t display[64 *32];
    uint8_t Soundtimer;
    uint8_t Delaytimer;
    uint16_t PC;
    uint16_t opcode;
    uint16_t stack[16];
    uint16_t I; //index
    uint16_t *SP; //Stack Pointer
    uint16_t NNN; //12 bit address
    uint8_t NN;   //8 bit constant
    uint8_t N;    //4 bit constant
    uint8_t X;    //4 bit register
    uint8_t Y;    //4 bit register
    config_t gfx;
    int QUIT;
    int RUNNING;
    int PAUSED;

}Chip8State;

Chip8State chip;





    //gambiarra de variaveis globais
    unsigned int Start_Address = 0x200;
    const unsigned int FONTSET_START_ADDRESS = 0x50;


    uint8_t fontset[FONTSET_SIZE] = {

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


    int ROM_loader(const char *file){

        memset(chip.memory,0, sizeof chip.memory);

        FILE *ROM;

        ROM= fopen(file,"rb");

        if(ROM == NULL){

            printf("DEU PAU PARCEIRO NAO ACHEI A ROM");
            return 1;
        }


        fseek(ROM, 0,SEEK_END);
        long int tamanho = ftell(ROM);
        rewind(ROM);

        for(long i = 0; i < tamanho; i++){

            fread(&chip.memory[Start_Address + i], 1, 1, ROM);

        }

        fclose(ROM);
        return 0;
}

    int initChip8(){

        chip.RUNNING = 1; 
        chip.PAUSED = 0;
        chip.PC = Start_Address;
        chip.SP = &chip.stack[0];
        chip.I = 0;

        //load font
        for(unsigned int i = 0; i < FONTSET_SIZE; i++){

            chip.memory[FONTSET_START_ADDRESS + i] = fontset[i];

        }

        if(chip.Delaytimer > 0){
            chip.Delaytimer--;
        }
        if(chip.Soundtimer > 0){
            if(chip.Soundtimer == 1){
                printf("BEEP\n");
                chip.Soundtimer--;
            }
        }

        return 0;

    }

    bool init_SDL(sdl_t *sdl, const config_t config ){

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
            SDL_Log("nao foi possivel iniciar o sdl: %s", SDL_GetError());
            return false;
        }

        sdl->window = SDL_CreateWindow("CHIP8 EMU", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.window_width * config.scale_factor,config.window_heigh* config.scale_factor, 0);

        if(!sdl->window){
            SDL_Log("Nao foi possivel criar a janela do SDL %s\n", SDL_GetError());
            return false;
        }

        sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);
        if(!sdl->renderer){
            SDL_Log("Nao foi possivel criar o renderer do SDL %s\n", SDL_GetError());
            return false;
        }

        return true;

    }

    bool set_conf_from_args(config_t *config, int argc, char **argv){

        //defaults
        *config = (config_t){

            .window_width =64, //Chip8 original X pos
            .window_heigh =32, //Chip8 original Y pos
            .fg_color = 0xFFFFFFF, // Yellow
            .bg_color = 0x0000000, //Black
            .scale_factor = 20,   //1280X680 
        };

        //override defaults with passed arg
        for(int i = 1; i < argc; i++){
            argv[i];
        }
        return true;
    }
    
    void CleanupSDL(const sdl_t *sdl){

        SDL_DestroyRenderer(sdl->renderer);
        SDL_DestroyWindow(sdl->window);
        SDL_Quit();

    }

    void clear_window(const config_t config, sdl_t sdl){

        //init screen clear to background color
        const uint8_t r = (config.bg_color >> 24) & 0xFF;
        const uint8_t g = (config.bg_color >> 16) & 0xFF;
        const uint8_t b = (config.bg_color >> 8) & 0xFF;
        const uint8_t a = (config.bg_color >> 0) & 0xFF;

        SDL_SetRenderDrawColor(sdl.renderer, r,g,b,a);
        SDL_RenderClear(sdl.renderer);

    }

    void update_screen(const sdl_t sdl, const config_t config, const Chip8State chip){

        SDL_Rect rect = {
            .x = 0,
            .y = 0,
            .w = config.scale_factor,
            .h = config.scale_factor
        };


        //Grab color values to draw
        const uint8_t fg_r = (config.fg_color >> 24) & 0xFF;
        const uint8_t fg_g = (config.fg_color >> 16) & 0xFF;
        const uint8_t fg_b = (config.fg_color >> 8) & 0xFF;
        const uint8_t fg_a = (config.fg_color >> 0) & 0xFF;

        const uint8_t bg_r = (config.bg_color >> 24) & 0xFF;
        const uint8_t bg_g = (config.bg_color >> 16) & 0xFF;
        const uint8_t bg_b = (config.bg_color >> 8) & 0xFF;
        const uint8_t bg_a = (config.bg_color >> 0) & 0xFF;

        //loop through display pixels, draw a rectangle per pixel to the SDL
        for(uint32_t i =0; i < sizeof chip.display; i++){
            //translate 1D index I value to 2d X/Y coordinates
            rect.x = (i % config.window_width) *config.scale_factor;
            rect.y = (i / config.window_width) *config.scale_factor;

            if(chip.display[i]){
                //pixel off, draw fg
                SDL_SetRenderDrawColor(sdl.renderer, fg_r,fg_g,fg_b,fg_a);
                SDL_RenderFillRect(sdl.renderer, &rect);

            }else{
                //pixel on, draw bg
                SDL_SetRenderDrawColor(sdl.renderer, bg_r,bg_g,bg_b,bg_a);
                SDL_RenderFillRect(sdl.renderer, &rect);
            }

        }
        SDL_RenderPresent(sdl.renderer);

    }
    void handle_input(Chip8State *chip){
        SDL_Event event;

        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                //exit
                chip->RUNNING = 0;
                return;

                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym){
                        case SDLK_ESCAPE:
                        chip->RUNNING = 0;
                        return;

                case SDLK_SPACE:
                        chip->PAUSED = !chip->PAUSED;
                            printf("pausado\n");
                            break;
                    }
                    break;
                case SDL_KEYUP:
            }
        }

    }
    //EMULATE CHIP-8 INSTRUCTIONS
    void instructions(Chip8State *chip){

        //get next opcode from RAM
        chip->opcode = chip->memory[chip->PC] << 8 | chip->memory[chip->PC+1];
        chip->PC +=2; //Pre-increment program counter for next opcode
        
        //Fill out current instruction Format
        //DXYN
        chip->NNN = chip->opcode & 0x0FFF;
        chip->NN = chip->opcode & 0x0FF;
        chip->N = chip->opcode & 0x0F;
        chip->X = (chip->opcode >> 8) & 0x0F;
        chip->Y = (chip->opcode >> 4) & 0x0F;
        printf("\nAddress: 0x%04X, opcode: 0x%04X, Desc:", chip->PC-2, chip->opcode);

        //Emulate Opcode
        switch((chip->opcode >> 12) & 0x0F){
            case 0x0:
                if(chip->NN == 0xE0){
                    //0x00E0 Clear the screen
                    printf("Clear Screen\n");
                    memset(&chip->display[0], false, sizeof chip->display);
                }else if(chip->NN == 0xEE){
                    //0x00EE: Return from subroutine
                    //Set last address from subroutine stack
                    printf("Return from subroutine to address 0x%04X\n", *(chip->SP -1));
                    chip->PC = *--chip->SP;
                }else if(chip->opcode == 0x0000){
                    //NOP memoria vazia
                }
                else{
                    printf("Sys 0x%03X (ignored)\n",chip->NNN);
                }
                break;

                case 0x01:
                //1NNN: just jump to the NNN Address
                chip->PC = chip->NNN;
                break;
            case 0x02:
                //0x2NNN: Call subroutine at NNN
                //Store current address to return to subroutine Stack
                printf("Call subroutine at nnn.\n");
                *chip->SP++ = chip->PC; 
                chip->PC = chip->NNN;
                break;
            case 0x03:
                if(chip->V[chip->X] == chip->NN){
                    printf("Skip next instruction if Vx = kk.\n");
                    chip->PC += 2;
                }
                break;
            case 0x04:
                if(chip->V[chip->X] != chip->NN){
                    printf("Skip next instruction if Vx != kk.\n");
                    chip->PC += 2;
                }
                break;
            case 0x05:
                if(chip->V[chip->X] == chip->V[chip->Y]){
                    printf("Skip next instruction if Vx = Vy.\n");
                    chip->PC += 2;
                }
                break;
            case 0x06:
                printf("Set Vx = kk.\n");
                chip->V[chip->X] = chip->NN;
                break;
            case 0x07:
                printf("Set Vx = Vx + kk.\n");
                chip->V[chip->X] = (chip->V[chip->X] + chip->NN);
                break;
            case 0x0A:
                printf("Sets I to the address NNN.\n");
                chip->I = chip->NNN;
                break;
            case 0x0D:
                uint8_t x = chip->V[chip->X] % chip->gfx.window_width;
                uint8_t y = chip->V[chip->Y] % chip->gfx.window_heigh;
                uint8_t H = chip->N;
                
                chip->V[0xF] = 0;

                for(int row = 0; row < H; row++){
                    uint8_t spriteByte = chip->memory[chip->I + row];

                    for(int col = 0; col < 8; col++){
                        uint8_t spritePixel = spriteByte &(0x80u >> col);

                        if(spritePixel !=0){
                            int px = (x + col) % chip->gfx.window_width;
                            int py = (y + row) % chip->gfx.window_heigh;
                            int index = px +(py*chip->gfx.window_width);

                            if(chip->display[index] == 1){
                                chip->V[0xF] = 1;
                            }

                        chip->display[index] ^= 1;
                            
                        }
                        
                    }
                }
                printf("DYXN Worked");
                break;
                
            default:
                printf("Not implemented yet or invalid opcode\n");
                    break;


        }
    }


    int main(int argc, char *argv[]){

        srand(time(NULL));

        
        const char *arquivo = "Chip8 Picture.ch8";

        initChip8();
        ROM_loader(arquivo);

        //init config options
        config_t config = {0};
        set_conf_from_args(&config,argc,argv);
        chip.gfx = config;

        sdl_t sdl = {0};
        init_SDL(&sdl ,config);

        clear_window(config, sdl);

        //Main Emulator Loop
        while(chip.RUNNING == 1){
            //handle use input
            handle_input(&chip);

            if(chip.PAUSED == 1) continue;

            //get_time();

            //Emulate CHIP8 Instructions
            instructions(&chip);
            //get_time();
            //Delay for approximately 60hz
            SDL_Delay(16);
            //Update window with changes 
            update_screen(sdl, config, chip);

        }

        CleanupSDL(&sdl);

        return 0;
    }
