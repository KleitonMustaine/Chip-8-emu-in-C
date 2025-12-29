#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define FONTSET_SIZE 80


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
    uint16_t pc;
    uint16_t opcode;
    uint16_t stack[16];
    uint16_t index;
    uint8_t sp;

}Chip8State;

Chip8State chip;

typedef struct{
    
    uint32_t window_width; //SDL WINDOW W
    uint32_t window_heigh; //SDL WINDOW H
    uint32_t fg_color;  
    uint32_t bg_color;

}config_t;

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

    void initChip8(){

        chip.pc = Start_Address;

        for(unsigned int i = 0; i < FONTSET_SIZE; i++){

            chip.memory[FONTSET_START_ADDRESS + i] = fontset[i];

        }

    }

    bool init_SDL(sdl_t *sdl, const config_t config ){

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
            SDL_Log("nao foi possivel iniciar o sdl: %s", SDL_GetError());
            return false;
        }

        sdl->window = SDL_CreateWindow("CHIP8 EMU", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.window_width,config.window_heigh, 0);

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

            .window_width =1280, //Chip8 original X pos
            .window_heigh =720, //Chip8 original Y pos
            .fg_color = 0xFFFFFFF, // Yellow
            .bg_color = 0x0000000, //Black
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


    void chip8_cycle(Chip8State *chip){

    
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

    void update_screen(const sdl_t sdl){
        SDL_RenderPresent(sdl.renderer);

    }

    void displayTex(const config_t config, sdl_t sdl){

        SDL_Surface* imageSurface = NULL;

        imageSurface = SDL_LoadBMP("Pekkie.bmp");
        if(imageSurface == NULL){
            printf("nao foi possivel carregar a imagem %s", SDL_GetError());
        }

        SDL_Texture* texture = NULL;
        
        texture = SDL_CreateTextureFromSurface(sdl.renderer, imageSurface);

        SDL_FreeSurface(imageSurface);

        if (texture == NULL) {
            printf("erro ao criar textura: %s\n", SDL_GetError());
            return;
        }

        SDL_Rect dst;
        dst.x =0;
        dst.y =0;
        dst.w =1280;
        dst.h =720;

        SDL_RenderCopy(sdl.renderer, texture, NULL, &dst);
        SDL_RenderPresent(sdl.renderer);
        SDL_DestroyTexture(texture);
    
    }

    int main(int argc, char *argv[]){

        srand(time(NULL));

        
        const char *arquivo = "IBM Logo.ch8";

        initChip8();
        ROM_loader(arquivo);

        
        //init config options
        config_t config = {0};
        if(set_conf_from_args(&config, argc, argv)) EXIT_FAILURE;


        sdl_t sdl = {0};
        if(!init_SDL(&sdl, config)) exit(EXIT_FAILURE);

        displayTex(config, sdl);

        clear_window(config, sdl);

        //Main Emulator Loop
        while(true){

            //get_time();
            //Emulate CHIP8 Instructions
            //get_time();
            //Delay for approximately 60hz
            SDL_Delay(16);
            displayTex(config, sdl);
            //Update window with changes 
            update_screen(sdl);

        }

        CleanupSDL(&sdl);

        return 0;
    }
