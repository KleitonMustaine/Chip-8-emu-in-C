#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

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



    //gambiarra de variaveis globais
    unsigned int Start_Address = 0x200;
    const unsigned int FONTSET_START_ADDRESS = 0x50;
    const unsigned int FONTSET_SIZE = 80;


    int ROM_loader(char const *file){

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

    int init_SDL(){

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
            
            SDL_Log("nao foi possivel iniciar o sdl: %s", SDL_GetError());
            
            return 1;
        }

        return 0;

    }

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

    void chip8_cycle(Chip8State *chip){

        

    }


    int main(int argc, char *argv[]){

        srand(time(NULL));
        
        const char *arquivo = "IBM Logo.ch8";

        initChip8();
        ROM_loader(&arquivo);

        

        if(init_SDL() != 0){

            return EXIT_FAILURE;
        }

        int running = 1;

        while(running){



        }

        

        return 0;
    }
