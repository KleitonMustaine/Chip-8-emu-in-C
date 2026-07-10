#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#define FONTSET_SIZE 80
#define ROMS_DIR "Roms"
#define MAX_ROMS 256
#define MAX_NAME 256


typedef struct{
    
    uint32_t window_width; //SDL WINDOW W
    uint32_t window_heigh; //SDL WINDOW H
    uint32_t fg_color;  
    uint32_t bg_color;
    uint32_t scale_factor;
    uint8_t pixel;  
    uint32_t audio_sample_rate; //44100 Hz
    uint32_t square_wave_freq; //440 Hz
    int16_t volume; //volume

}config_t;

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;
} sdl_t;

typedef struct{

    uint8_t V[16];
    uint8_t memory[4096];
    uint8_t keyboard[16];
    uint8_t display[64 *32];
    uint8_t Soundtimer;
    uint8_t Delaytimer;
    uint32_t last_timer_tick;
    int waitkey;
    uint8_t wait_reg;
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

    int scan_roms(char lista[MAX_ROMS][MAX_NAME]) {
        DIR *dir = opendir(ROMS_DIR);
        if(dir == NULL){
            printf("Nao foi possivel abrir o diretorio de ROMs\n");
            return 0;
        }

        int count = 0;
        struct dirent *entry;

        while((entry = readdir(dir)) != NULL && count < MAX_ROMS){

            const char *ext = strrchr(entry->d_name, '.');

            if(ext != NULL && strcmp(ext, ".ch8") == 0){
                strncpy(lista[count], entry->d_name, MAX_NAME);
                lista[count][MAX_NAME - 1] = '\0';
                count++;
            }

        }
        closedir(dir);
        return count;
    }

    int ROM_loader(const char *file){

        memset(chip.memory,0, sizeof chip.memory);

        char caminho[512];
        snprintf(caminho, sizeof caminho, "%s/%s", ROMS_DIR, file);

        FILE *ROM;

        ROM = fopen(caminho,"rb");

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
        chip.Delaytimer = 0;
        chip.Soundtimer = 0;
        chip.last_timer_tick = SDL_GetTicks();
        srand(time(0));
        memset(chip.display,0,sizeof chip.display);
        memset(chip.keyboard,0,sizeof chip.keyboard);

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
       int keymap(SDL_Keycode key){

        switch(key){

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

    void audio_callback(void *userdata, uint8_t *stream, int len){
        config_t *config = (config_t *)userdata;

        int16_t *audio_data = (int16_t *)stream;
        static uint32_t running_sample_index = 0;

        const int32_t square_wave_period = config->audio_sample_rate / config->square_wave_freq;
        const int32_t half_period = square_wave_period / 2;

        for(int i = 0; i < len / 2; i++){
           audio_data[i] = ((running_sample_index++ / square_wave_period) % 2) ? config->volume : -config->volume;
        

        }

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

        sdl->want = (SDL_AudioSpec){
            .freq = config.audio_sample_rate,
            .format = AUDIO_S16SYS,
            .channels = 1,
            .samples = 2048,
            .callback = audio_callback,
            .userdata = (void *)&chip.gfx
        };
        sdl->dev = SDL_OpenAudioDevice(NULL, 0, &sdl->want, NULL, 0);
        if(sdl->dev == 0){
            SDL_Log("Nao foi possivel abrir o dispositivo de audio: %s\n", SDL_GetError());
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

        SDL_CloseAudioDevice(sdl->dev);
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
            int key = keymap(event.key.keysym.sym);
            switch(event.type){
                case SDL_QUIT:
                //exit
                chip->RUNNING = 0;
                return;

                case SDL_KEYDOWN:
 
                    SDL_Keycode sym = event.key.keysym.sym;
                    
                    if(sym == SDLK_ESCAPE){
                        chip->RUNNING = 0;
                        break;
                    }
                    if(sym == SDLK_SPACE){
                        chip->PAUSED = !chip->PAUSED;
                        printf("pausado\n");
                        break;
                    }
                if(key != -1){
                    chip->keyboard[key] = 1;
                }
                if(chip->waitkey && key != -1){
                    chip->V[chip->wait_reg] = key;
                    chip->waitkey = 0;
                }
                break;
                case SDL_KEYUP:
                if(key != -1){
                    chip->keyboard[key] = 0;
                }
                break;
            }
        }
    }
    
    void get_time(Chip8State *chip){

        uint32_t now = SDL_GetTicks();

        if(now - chip->last_timer_tick >= 1000/60){
            
            if(chip->Delaytimer > 0){
                chip->Delaytimer--;
            }
            if(chip->Soundtimer > 0){
                chip->Soundtimer--;
            }
            chip->last_timer_tick = now;
        }
    }
    
    //EMULATE CHIP-8 INSTRUCTIONS
    void instructions(Chip8State *chip){

        //get next opcode from RAM
        chip->opcode = chip->memory[chip->PC] << 8 | chip->memory[chip->PC+1];
        
        //Fill out current instruction Format
        //DXYN
        chip->NNN = chip->opcode & 0x0FFF;
        chip->NN = chip->opcode & 0x0FF;
        chip->N = chip->opcode & 0x0F;
        chip->X = (chip->opcode >> 8) & 0x0F;
        chip->Y = (chip->opcode >> 4) & 0x0F;
        printf("\nAddress: 0x%04X, opcode: 0x%04X, Desc:", chip->PC-2, chip->opcode);

        chip->PC +=2; //Pre-increment program counter for next opcode

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
                }else if(chip->opcode == 0x00){
                    //NOP memoria vazia
                }
                else{
                    printf("Sys 0x%03X (ignored)\n",chip->NNN);
                }
                break;

                case 0x1:
                //1NNN: just jump to the NNN Address
                chip->PC = chip->NNN;
                break;
            case 0x2:
                //0x2NNN: Call subroutine at NNN
                //Store current address to return to subroutine Stack
                printf("Call subroutine at nnn.\n");
                *chip->SP++ = chip->PC; 
                chip->PC = chip->NNN;
                break;
            case 0x3:
                if(chip->V[chip->X] == chip->NN){
                    printf("Skip next instruction if Vx = kk.\n");
                    chip->PC += 2;
                }
                break;
            case 0x4:
                if(chip->V[chip->X] != chip->NN){
                    printf("Skip next instruction if Vx != kk.\n");
                    chip->PC += 2;
                }
                break;
            case 0x5:
                if(chip->N == 0 && chip->V[chip->X] == chip->V[chip->Y]){
                    printf("Skip next instruction if Vx = Vy.\n");
                    chip->PC += 2;
                }
                break;
            case 0x6:
                printf("Set Vx = kk.\n");
                chip->V[chip->X] = chip->NN;
                break;
            case 0x7:
                printf("Set Vx = Vx + kk.\n");
                chip->V[chip->X] = (chip->V[chip->X] + chip->NN);
                break;
            case 0x8:
                if(chip->N == 0x0){
                    chip->V[chip->X] = chip->V[chip->Y];
                }
                else if(chip->N == 0x1){
                    chip->V[chip->X] |= chip->V[chip->Y];
                }
                else if(chip->N == 0x2){
                    chip->V[chip->X] &= chip->V[chip->Y];
                }
                else if(chip->N == 0x3){
                    chip->V[chip->X] ^= chip->V[chip->Y];
                }
                else if(chip->N == 0x4){
                    uint16_t sum;
                    sum = chip->V[chip->Y] + chip->V[chip->X];
                    chip->V[0xF] = (sum > 0xFF);
                    chip->V[chip->X] = sum & 0xFF;
                }
                else if(chip->N == 0x5){
                    chip->V[0xF] = (chip->V[chip->X] >= chip->V[chip->Y]);
                    chip->V[chip->X] -= chip->V[chip->Y];
                }
                else if(chip->N == 0x6){
                    chip->V[0xF] = chip->V[chip->X] &0x01;
                    chip->V[chip->X] >>= 1;
                }
                else if(chip->N == 0x7){
                    chip->V[0xF] = chip->V[chip->Y] >= chip->V[chip->X];
                    chip->V[chip->X] = chip->V[chip->Y] - chip->V[chip->X];
                }
                else if(chip->N == 0xE){
                    chip->V[0xF] = (chip->V[chip->X] & 0x80) >> 7;
                    chip->V[chip->X] <<= 1;
                }
                break;
            case 0x9:
                if(chip->N == 0x0 && chip->V[chip->X] != chip->V[chip->Y]){
                    chip->PC += 2;
                }
                break;

            case 0xE:
                if(chip->NN ==0x9E){
                    if(chip->keyboard[chip->V[chip->X]]){
                        chip->PC +=2;
                }
            }
            else if(chip->NN == 0xA1){
                    if(!chip->keyboard[chip->V[chip->X]]){
                        chip->PC +=2;
                }
            }
                break;
            case 0xF:
                if(chip->NN == 0x7){
                    chip->V[chip->X] = chip->Delaytimer;
                }
                else if(chip->NN == 0x15){
                    chip->Delaytimer = chip->V[chip->X];
                }
                else if(chip->NN == 0x18){
                    chip->Soundtimer = chip->V[chip->X];
                }
                else if(chip->NN == 0x0A){
                    chip->waitkey = 1;
                    chip->wait_reg = chip->X; 
                    chip->PC -=2;
                    return;
                    
                }
                else if(chip->NN == 0x1E){
                    chip->I += chip->V[chip->X];
                }
                else if(chip->NN == 0x29){
                    chip->I = FONTSET_START_ADDRESS + (chip->V[chip->X]*5);
                }
                else if(chip->NN == 0x33){
                    uint8_t v= chip->V[chip->X];
                    chip->memory[chip->I] = v/100;
                    chip->memory[chip->I + 1] = (v /10) % 10;
                    chip->memory[chip->I + 2] = v % 10;
                }
                else if(chip->NN == 0x55){
                    for(int i =0; i<=chip->X; i++){
                        chip->memory[chip->I + i] = chip->V[i];
                    }
                }
                else if(chip->NN == 0x65){
                    for(int i =0; i<=chip->X; i++){
                        chip->V[i] = chip->memory[chip->I + i];
                    }
                }
                break;
            case 0xC:
                printf("Sets VX a random number\n");
                chip->V[chip->X] = (rand() % 0xFF) & chip->NN;
                break;
            case 0xA:
                printf("Sets I to the address NNN.\n");
                chip->I = chip->NNN;
                break;
            case 0xB:
                chip->PC = (chip->V[0x0] + chip->NNN);
                break;
            //Super Chip-8 instructions
            
            
            case 0xD:
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

        
        int op;
        const char *arquivo = "";
        char roms[MAX_ROMS][MAX_NAME];
        int total = scan_roms(roms);

        if(total == 0){
            printf("Nao foram encontrados arquivos .ch8 no diretorio %s\n", ROMS_DIR);
        }

        printf("Chose what ROM you want To Test:\n");
        for(int i = 0; i < total; i++){
            printf("%d: %s\n", i+1, roms[i]);
        }
        printf("-> ");
        scanf("%d", &op);

        if(op < 1 || op > total){
            printf("Opcao estranho\n");
            return 1;
        }

        initChip8();
        ROM_loader(roms[op - 1]);

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

            get_time(&chip);

            //Emulate CHIP8 Instructions
            instructions(&chip);
            get_time(&chip);
            //toca o audio se o timer de som estiver ativo
            if(chip.Soundtimer > 0){
                SDL_PauseAudioDevice(sdl.dev, 0); // 0 = tocando
            }
            else{
                SDL_PauseAudioDevice(sdl.dev, 1); // 1 = pausado
            }
            //Delay for approximately 60hz
            SDL_Delay(1);
            //Update window with changes 
            clear_window(config, sdl);
            update_screen(sdl, config, chip);

        }

        CleanupSDL(&sdl);

        return 0;
    }
