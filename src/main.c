#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/cpu.h"
#include "../include/memory.h"
#include "../include/nes.h"
#include "../include/palette.h"
#include "../include/ppu.h"
#include "../include/ppu_internal.h"

#define SCALE          2
#define DISPLAY_WIDTH  256
#define DISPLAY_HEIGHT 240

void setRenderDrawColor(SDL_Renderer* renderer, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

static SDL_Window* window     = NULL;
static SDL_Renderer* renderer = NULL;

static byte display[DISPLAY_WIDTH][DISPLAY_HEIGHT];

static unsigned long long current_frame = 0;

static bool initialize(void) {
    /* Initialize SDL. */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    /* Create window. */
    window = SDL_CreateWindow("NES Emulator", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, SCALE * DISPLAY_WIDTH, SCALE * DISPLAY_HEIGHT,
            SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created. SDL_ERROR: %s\n", SDL_GetError());
        return false;
    }

    /* Create renderer. */
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created. SDL_ERROR: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

static bool load_rom(char *path) {
    FILE *file = fopen(path, "rb");

    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *data = malloc(length);

        if (data) {
            fread(data, 1, length, file);
            fclose(file);
            return nes_insert_cartridge(data, length);
        }
        else {
            fclose(file);
            return false;
        }
    }

    return false;
}

static void draw_display(SDL_Renderer* renderer) {
    /* Clear the screen. */
    setRenderDrawColor(renderer, PALETTE[0]);
    SDL_RenderClear(renderer);

    /* Draw all pixels on the display. */
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            setRenderDrawColor(renderer, PALETTE[ppu_get_pixel(x, y)]);
            SDL_Rect pixel = {SCALE * x, SCALE * y, SCALE, SCALE};
            SDL_RenderFillRect(renderer, &pixel);
        }
    }

    /* Present the result on screen. */
    SDL_RenderPresent(renderer);
}

static void close(void) {
    /* Delete window and renderer. */
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = NULL;
    window = NULL;

    /* Quit SDL subsystems. */
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    /* Parse command line arguments. */
    if (argc < 2) {
        printf("Error: missing argument.\n");
        printf("Usage: ./nes_emulator <path-to-rom>\n");
        return 1;
    }

    /* Start up SDL and create window. */
    if (!initialize()) {
        printf("Failed to initialize SDL.\n");
        return 1;
    }

    /* Attempt to load the ROM. */
    if (!load_rom(argv[1])) {
        printf("Failed to load ROM.\n");
        return 1;
    }

    cpu_init();

    SDL_Event event;
    while (1) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                close();
                return 0;
            }
        }

        unsigned long long timestamp = cpu_get_ticks();
        cpu_execute();
        ppu_catch_up();

        if (ppu.status_vblank && current_frame < ppu.frame) {
            draw_display(renderer);
            current_frame = ppu.frame;
        }
    }

    close();

    return 0;
}
