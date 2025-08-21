#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef __APPLE__
#include <unistd.h>
#include <pthread.h>
#elif __linux__
#include <unistd.h>
#include <pthread.h>
#elif _WIN32
#include <windows.h>
#endif

#define MD5_IMPLEMENTATION
#include "./md5.h"
#include "version.h"

#define ALLOWED_MASTER_KEY_LEN 1024
#define INPUT_MAX_LEN 1024
#define SLEEP_FOR 10 // in seconds

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

#define BASE_FONT_SIZE 14
#define MASTER_KEY_FONT_SIZE 20
#define VAULT_CONTENTS_FONT_SIZE 14
#define INPUT_FONT_SIZE 14

#define FRAME_DELAY 16 // in milliseconds; ~60FPS
#define SCROLLBAR_PADDING 20

#ifdef __APPLE__
#define GUI_FONT "/System/Library/Fonts/Monaco.ttf"
#elif __linux__
#define GUI_FONT "/usr/share/fonts/truetype/freefont/FreeMono.ttf"
#elif _WIN32
#define GUI_FONT "TODO:"
#endif

#define rgb_to_sdl_color(rgb)                                                  \
  ((SDL_Color){rgb >> (8 * 2) & 0xFF, rgb >> (8 * 1) & 0xFF,                   \
               rgb >> (8 * 0) & 0xFF, 0xFF})

#define rgba_to_sdl_color(rgba)                                                \
  ((SDL_Color){                                                                \
      rgba >> (8 * 3) & 0xFF,                                                  \
      rgba >> (8 * 2) & 0xFF,                                                  \
      rgba >> (8 * 1) & 0xFF,                                                  \
      rgba >> (8 * 0) & 0xFF,                                                  \
  })

#define BLACK rgb_to_sdl_color(0x000000)
#define WHITE rgb_to_sdl_color(0xF0EEE9)
#define RED rgb_to_sdl_color(0xdc322f)
#define BLUE rgb_to_sdl_color(0x268bd2)

// MAYBE: TODO: use calloc instead of malloc

typedef struct Alloc {
    void *ptr;
    struct Alloc *next; 
} Alloc;

Alloc *alloced_ptrs = NULL;

void mfree() {
    while (alloced_ptrs != NULL) {
        Alloc *me = alloced_ptrs;
        alloced_ptrs = alloced_ptrs->next;
        if (me != NULL) {
            if (me->ptr != NULL) {
                free(me->ptr);
            }
            free(me);
        }
    }
}

void *mmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR]: malloc returned NULL\n");
        mfree();
    }
    assert(ptr != NULL && "unexpected NULL from malloc");
    memset(ptr, 0, size);

    Alloc *alloced = malloc(sizeof(Alloc)*1);
    if (alloced == NULL) {
        fprintf(stderr, "[ERROR]: malloc returned NULL on Alloc\n");
        mfree();
    }
    assert(alloced != NULL && "unexpected NULL from malloc");
    alloced->ptr = ptr;
    alloced->next = alloced_ptrs;
    alloced_ptrs = alloced;
    return ptr;
}

long file_size(char *filename) {
  FILE *fptr = fopen(filename, "r");
  fseek(fptr, 0, SEEK_END);
  long fsize = ftell(fptr);
  fclose(fptr);
  return fsize;
}

char *carve_str(char *contents, int contents_len, int offset, int *carved_len) {
    char c;
    while (offset+(*carved_len) < contents_len && (c = contents[offset+(*carved_len)]) != ' ' && c != '\n' && c != EOF) {
        *carved_len += 1;
    }
    // *carved_len -= 1; // account for ' ' || '\n'
    char *carved = mmalloc(sizeof(char)*(*carved_len+1)); // +1 for \0
    memcpy(carved, contents+offset, *carved_len);
    return carved;
}

// read_vault_contents reads vault contents, parses it and returns list of list of strings
char ***read_vault_contents(char *arg_filename, int *line_count) {
    long fsize = file_size(arg_filename);
    FILE *fptr = fopen(arg_filename, "r");
    char *contents = mmalloc(sizeof(char)*(fsize + 1));
    size_t read_bytes = fread(contents, sizeof(char), fsize, fptr);
    fclose(fptr);
    if (read_bytes != fsize) {
        fprintf(stdout, "[WARNING]: fsize is %ld, but read %ld\n",fsize, read_bytes);
    }
    for (int i=0; i<read_bytes; ++i) {
        if (contents[i] == '\n') {
            *line_count += 1;
        }
    }

    char ***vault_contents = mmalloc(sizeof(char **)*(*line_count));
    int offset = 0;
    for (int i=0; i<(*line_count); ++i) {
        vault_contents[i] = mmalloc(sizeof(char *)*4); // salt pepper domain "<additional>"
        for (int j=0; j<4; ++j) {
            if (contents[offset] == '\n') {
                offset += 1; // +1 jump over '\n'
                break;
            }
            int carved_len = 0;
            char *carved = carve_str(contents, read_bytes, offset, &carved_len);
            vault_contents[i][j] = carved;
            if (contents[offset+carved_len] == '\n') {
                offset += carved_len+1; // +1 to jump over '\n'
                break;
            }
            offset += carved_len+1; // +1 to jump over that ' ' and '\n'
        }
    }
    return vault_contents;
}

void vault_contents_printer(char ***vault_contents, int line_count) {
    for (int i=0; i<line_count; ++i) {
        if (vault_contents[i][3] == NULL) {
            printf("line %d: salt '%s', pepper '%s', domain '%s'\n", 
                i+1,
                vault_contents[i][0],
                vault_contents[i][1],
                vault_contents[i][2]);
        } else {
            printf("line %d: salt '%s', pepper '%s', domain '%s', digest_len '%s'\n", 
                i+1,
                vault_contents[i][0],
                vault_contents[i][1],
                vault_contents[i][2],
                vault_contents[i][3]);
        }
    }
}

char *pw(char *key, char *salt, char *pepper, char *domain, int digest_len) {
    int domain_len = strlen(domain);
    int key_len = strlen(key);
    int salt_len = strlen(salt);
    int pepper_len = strlen(pepper);
    int message_len = domain_len + key_len + salt_len;
    char message[message_len + 1];
    message[message_len] = '\0';
    memcpy(message, domain, domain_len);
    memcpy(message + domain_len, key, key_len);
    memcpy(message + domain_len + key_len, salt, salt_len);

    unsigned char digest[16];
    md5(message, digest);
    
    char *pw = mmalloc(sizeof(char)*(digest_len*2+pepper_len+1));
    for (int i=0; i<digest_len; ++i) {
        snprintf(pw+i*2, 2+1, "%02x", digest[i]);
    }
    snprintf(pw+digest_len*2, pepper_len+1, "%s", pepper);
    return pw;
}

void help() {
    printf("TODO: help\n");
}

typedef struct {
    char *master_key;
    char *salt;
    char *pepper;
    char *domain;
    int digest_len;
} PwData;

typedef struct {
    SDL_Rect *rect;
    SDL_Texture *t;
} Texture;

char *ask_master_key(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font) {
    TTF_SetFontSize(font, MASTER_KEY_FONT_SIZE);

    int master_key_max_len = ALLOWED_MASTER_KEY_LEN;
    int input_offset = 0;
    int prev_input_offset = 0;

    char *master_key = mmalloc(sizeof(char)*(master_key_max_len+1));

    char *master_key_prompt = "Provide Master Key";

    Texture *master_key_texture = mmalloc(sizeof(Texture)*1);
    master_key_texture->rect = mmalloc(sizeof(SDL_Rect)*1);

    int window_w;
    int window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    
    // MasterKeyPrompt texture
    SDL_Surface *master_key_prompt_surface = TTF_RenderUTF8_Solid(font, master_key_prompt, BLACK);
    if (master_key_prompt_surface == NULL) {
        fprintf(stderr, "failed to create text surface: %s\n", TTF_GetError());
        return NULL;
    }

    master_key_texture->rect->x = (window_w-master_key_prompt_surface->w)/2;
    master_key_texture->rect->y = (window_h-master_key_prompt_surface->h)/2;
    master_key_texture->rect->w = master_key_prompt_surface->w;
    master_key_texture->rect->h = master_key_prompt_surface->h;
    master_key_texture->t = SDL_CreateTextureFromSurface(renderer, master_key_prompt_surface);
    if (master_key_texture->t == NULL) {
        SDL_FreeSurface(master_key_prompt_surface);
        fprintf(stderr, "failed to create text texture: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_FreeSurface(master_key_prompt_surface);

    SDL_GetWindowSize(window, &window_w, &window_h);    
    SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);
    SDL_RenderCopy(renderer, master_key_texture->t, NULL, master_key_texture->rect);
    SDL_RenderPresent(renderer);

    float elapsed = 0;
    bool asking_for_master_key = true;
    Uint32 start = SDL_GetTicks64();
    Uint32 end = SDL_GetTicks64();    
    SDL_StartTextInput();

    while (asking_for_master_key) {
        start = SDL_GetTicks64();
        SDL_Event sdl_event = {0};
        while (SDL_PollEvent(&sdl_event) > 0) {
            // QUIT START
            if (sdl_event.type == SDL_QUIT || 
                (sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_CLOSE) || 
                    (sdl_event.type == SDL_KEYDOWN &&
                        sdl_event.key.state == SDL_PRESSED &&
                        sdl_event.key.keysym.sym == SDLK_c &&
                        sdl_event.key.keysym.mod & KMOD_CTRL) || 
                        (sdl_event.type == SDL_KEYDOWN &&
                            sdl_event.key.state == SDL_PRESSED &&
                            sdl_event.key.keysym.sym == SDLK_d &&
                            sdl_event.key.keysym.mod & KMOD_CTRL)) {
                asking_for_master_key = false;
                master_key = NULL; // NOTE: NULL for early return
                break;
            // QUIT END

            // BACKSPACE START
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_BACKSPACE) {
                master_key[input_offset] = '\0';
                if (input_offset > 0){
                    input_offset -= 1;
                }
            // BACKSPACE END

            // ENTER START
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_RETURN) {
                asking_for_master_key = false;
                break;
            // ENTER END

            // TEXT START
            } else if (sdl_event.type == SDL_TEXTINPUT) {
                size_t event_text_len = strlen(sdl_event.text.text);
                if (input_offset+event_text_len <= master_key_max_len) {
                    memcpy(master_key+input_offset, sdl_event.text.text, event_text_len);
                    input_offset += event_text_len;
                }
            // TEXT END

            // WINDOW RESIZE START
            } else if (sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_GetWindowSize(window, &window_w, &window_h);
                master_key_texture->rect->x = (window_w-master_key_texture->rect->w)/2;
                master_key_texture->rect->y = (window_h-master_key_texture->rect->h)/2;
            // WINDOW RESIZE END
            }
        }

        SDL_GetWindowSize(window, &window_w, &window_h);
        if (prev_input_offset == 0 && 
            0 < input_offset && input_offset < master_key_max_len) { // from 0
                SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        } else if (0 < prev_input_offset && prev_input_offset < master_key_max_len && 
            input_offset == 0) { // to 0
                SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);
        } else if (0 < prev_input_offset && prev_input_offset < master_key_max_len &&
            input_offset == master_key_max_len) { // to max
                SDL_SetRenderDrawColor(renderer, RED.r, RED.g, RED.b, RED.a);
        } else if (prev_input_offset == master_key_max_len && 
            0 < input_offset && input_offset < master_key_max_len) { // from max
                SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, master_key_texture->t, NULL, master_key_texture->rect);
        SDL_RenderPresent(renderer);

        prev_input_offset = input_offset;

        end = SDL_GetTicks64();
        elapsed = end - start;
        if (elapsed <= FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - elapsed);
        }
    }

    SDL_StopTextInput();
    SDL_DestroyTexture(master_key_texture->t);
    return master_key;
}

Texture *create_input_texture(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font, Texture *input_texture, char *input_buf) {
    TTF_SetFontSize(font, INPUT_FONT_SIZE);

    int window_w;
    int window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);

    SDL_Surface *input_prompt_surface = TTF_RenderUTF8_Solid(font, input_buf, BLACK);
    if (input_prompt_surface == NULL) {
        fprintf(stderr, "[WARNING]: failed to create input text surface: %s\n", TTF_GetError());
        return NULL;
    }

    input_texture->rect->x = 0;
    input_texture->rect->y = window_h-INPUT_FONT_SIZE;
    input_texture->rect->w = input_prompt_surface->w;
    input_texture->rect->h = input_prompt_surface->h;
    input_texture->t = SDL_CreateTextureFromSurface(renderer, input_prompt_surface);
    if (input_texture->t == NULL) {
        SDL_FreeSurface(input_prompt_surface);
        fprintf(stderr, "[WARNING]: failed to create input text texture: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_FreeSurface(input_prompt_surface);
    return input_texture;
}


Texture **create_vault_contents_textures(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font, char ***vault_contents, int line_count) {
    TTF_SetFontSize(font, VAULT_CONTENTS_FONT_SIZE);

    int window_w;
    int window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    int h_offset = 0;

    Texture **textures = mmalloc(sizeof(Texture *)*line_count);
    for (int i=0; i<line_count; i++) {
        if (vault_contents[i][2] == NULL) {
            textures[i] = NULL;
        } else {
            textures[i] = mmalloc(sizeof(Texture)*1);
            textures[i]->rect = mmalloc(sizeof(SDL_Rect)*1);
        }
    }
    
    for (int i=0; i<line_count; i++) {
        if (vault_contents[i] == NULL || vault_contents[i][2] == NULL) {
            continue;
        }

        SDL_Surface *surface = TTF_RenderUTF8_Solid(font, vault_contents[i][2], BLACK);
        if (surface == NULL) {
            fprintf(stderr, "[WARNING]: failed to create text surface: %s; skipping %s\n", TTF_GetError(), vault_contents[i][2]);
            continue;
        }

        textures[i]->rect->x = SCROLLBAR_PADDING;
        textures[i]->rect->y = h_offset;
        textures[i]->rect->w = surface->w;
        textures[i]->rect->h = surface->h;
        h_offset += surface->h;

        textures[i]->t = SDL_CreateTextureFromSurface(renderer, surface);
        if (textures[i]->t == NULL) {
            SDL_FreeSurface(surface);
            fprintf(stderr, "[WARNING]: failed to create text texture: %s; skipping %s\n", SDL_GetError(), vault_contents[i][2]);
            continue;
        }
        SDL_FreeSurface(surface);
    }

    return textures;
}

int select_vault_content_idx(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font, char ***vault_contents, int line_count) {

    // TODO: scrollbar
    // TODO: text input
    // TODO: fuzzy-ish finding
    int result_idx = -1;

    int window_w;
    int window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    int vertical_scroll = 0; // TODO:

    SDL_Color prev_renderer_color = {0};
    SDL_GetRenderDrawColor(renderer, (Uint8 *)&prev_renderer_color.r, (Uint8 *)&prev_renderer_color.g, (Uint8 *)&prev_renderer_color.b, (Uint8 *)&prev_renderer_color.a);

    char *input_prompt = "\t> ";
    size_t input_prompt_len = strlen(input_prompt);

    int input_max_len = INPUT_MAX_LEN+input_prompt_len;
    int input_offset = 0;
    char *input_buf = mmalloc(sizeof(char)*(input_max_len+1));

    memcpy(input_buf+input_offset, input_prompt, input_prompt_len);
    input_offset += input_prompt_len;


    Texture *input_texture = mmalloc(sizeof(Texture)*1);
    input_texture->rect = mmalloc(sizeof(SDL_Rect)*1);

    input_texture = create_input_texture(window, renderer, font, input_texture, input_buf);
    if (input_texture == NULL) {
        return result_idx;
    }

    Texture **vault_contents_textures = create_vault_contents_textures(window, renderer, font, vault_contents, line_count);

    float elapsed = 0;
    bool select_vault_content = true;
    size_t event_text_len;
    Uint32 start = SDL_GetTicks64();
    Uint32 end = SDL_GetTicks64();    
    SDL_StartTextInput();

    while (select_vault_content) {
        start = SDL_GetTicks64();
        SDL_Event sdl_event = {0};
        while (SDL_PollEvent(&sdl_event) > 0) {
            // QUIT START
            if (sdl_event.type == SDL_QUIT || 
                (sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_CLOSE) || 
                    (sdl_event.type == SDL_KEYDOWN &&
                        sdl_event.key.state == SDL_PRESSED &&
                        sdl_event.key.keysym.sym == SDLK_c &&
                        sdl_event.key.keysym.mod & KMOD_CTRL) || 
                        (sdl_event.type == SDL_KEYDOWN &&
                            sdl_event.key.state == SDL_PRESSED &&
                            sdl_event.key.keysym.sym == SDLK_d &&
                            sdl_event.key.keysym.mod & KMOD_CTRL)) {
                select_vault_content = false;
                result_idx = -1; // NOTE: NULL for early return
                break;
            // QUIT END

            // BACKSPACE START
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_BACKSPACE && 
                    input_offset > input_prompt_len) {

                input_offset -= 1;
                input_buf[input_offset] = '\0';
                SDL_DestroyTexture(input_texture->t);
                input_texture = create_input_texture(window, renderer, font, input_texture, input_buf);
            // BACKSPACE END

            // // ENTER START
            // } else if (sdl_event.type == SDL_KEYDOWN &&
            //         sdl_event.key.state == SDL_PRESSED &&
            //         sdl_event.key.keysym.sym == SDLK_RETURN) {
            //     select_vault_content = false;
            //     break;
            // // ENTER END

            // TEXT START
            } else if (sdl_event.type == SDL_TEXTINPUT && (event_text_len = strlen(sdl_event.text.text)) && input_offset+event_text_len <= input_max_len) {
                memcpy(input_buf+input_offset, sdl_event.text.text, event_text_len);
                input_offset += event_text_len;
                SDL_DestroyTexture(input_texture->t);
                input_texture = create_input_texture(window, renderer, font, input_texture, input_buf);
            // TEXT END
            
            // WINDOW RESIZE START
            } else if(sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_GetWindowSize(window, &window_w, &window_h);
                input_texture->rect->y = window_h-INPUT_FONT_SIZE;
            // WINDOW RESIZE END
            }
        }

        SDL_RenderClear(renderer);
        // TODO: fuzzy finding - firstly naive O(n) solution
        // TODO: handle highlight
        for (int i=0; i<line_count; i++) {
            if (vault_contents_textures[i] == NULL) {
                continue;
            }
            if (vault_contents_textures[i]->rect->y+VAULT_CONTENTS_FONT_SIZE + INPUT_FONT_SIZE < window_h) {
                if (i == 0) {// NOTE: just highlight the first row for now
                    SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
                    SDL_RenderFillRect(renderer, &(SDL_Rect){
                        .x=vault_contents_textures[i]->rect->x, 
                        .y=vault_contents_textures[i]->rect->y,
                        .w=window_w, 
                        .h=INPUT_FONT_SIZE
                    });

                    SDL_RenderCopy(renderer, vault_contents_textures[i]->t, NULL, vault_contents_textures[i]->rect);
                    SDL_SetRenderDrawColor(renderer, prev_renderer_color.r, prev_renderer_color.g, prev_renderer_color.b, prev_renderer_color.a);    
                } else {
                    SDL_RenderCopy(renderer, vault_contents_textures[i]->t, NULL, vault_contents_textures[i]->rect);

                }
            }
        }

        {
            SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
            SDL_RenderFillRect(renderer, &(SDL_Rect){.x=0, .y=window_h - INPUT_FONT_SIZE,.w=window_w, .h=1});
            SDL_RenderCopy(renderer, input_texture->t, NULL, input_texture->rect);
            SDL_SetRenderDrawColor(renderer, prev_renderer_color.r, prev_renderer_color.g, prev_renderer_color.b, prev_renderer_color.a);
        }
        
        SDL_RenderPresent(renderer);

        end = SDL_GetTicks64();
        elapsed = end - start;
        if (elapsed <= FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - elapsed);
        }
    }

    SDL_StopTextInput();

    for (int i=0; i<line_count; i++) {
        if (vault_contents[i][2] == NULL) {
            continue;
        } else {
            SDL_DestroyTexture(vault_contents_textures[i]->t);
        }
    }
    SDL_DestroyTexture(input_texture->t);
    return result_idx;
}

// TODO: GUI for selecting domain
PwData *gui(char ***vault_contents, int line_count) {

    ///////////////////////////
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: '%s'\n", SDL_GetError());
        return NULL;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init failed: '%s'\n", TTF_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Window *window = SDL_CreateWindow("vault", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH,
                                        WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

    if (window == NULL) {
        fprintf(stderr, "failed to create window: '%s'\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0); // SDL_RENDERER_ACCELERATED | SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        fprintf(stderr, "failed to create renderer: '%s'\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    TTF_Font *font = TTF_OpenFont(GUI_FONT, BASE_FONT_SIZE);
    if (font == NULL) {
        fprintf(stderr, "failed to load font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    return NULL;
    }

    SDL_RenderClear(renderer);

    char *master_key = ask_master_key(window, renderer, font);

    int window_w;
    int window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);

    SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);
    SDL_RenderFillRect(renderer, &(SDL_Rect){0, 0, window_w, window_h});

    int idx = -1;
    if (master_key != NULL) {
        idx = select_vault_content_idx(window, renderer, font, vault_contents, line_count);
    }

    SDL_HideWindow(window);
    // NOTE: hopefully sdl hide event is handled in 100ms, otherwise the window hangs until sleep is over
    // we're going to accept this behavior for now
    for (int i = 0; i < 10; ++i) {
        SDL_Delay(10);
        SDL_PumpEvents();
    } 

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    ///////////////////////////

    if (master_key == NULL || idx < 0) { // NOTE: cancelled the pw - early exit
        return NULL;
    }

    PwData *pw_data = mmalloc(sizeof(PwData)*1);
    
    pw_data->master_key = master_key;

    pw_data->salt = vault_contents[idx][0];
    pw_data->pepper = vault_contents[idx][1];
    pw_data->domain = vault_contents[idx][2];
    pw_data->digest_len = 16;
    if (vault_contents[idx][3] != NULL) {
        int tmp_digest_len = atoi(vault_contents[idx][3]);
        if (0 < tmp_digest_len && tmp_digest_len <= 16) {
            pw_data->digest_len = tmp_digest_len;
        } else {
            fprintf(stderr, "[WARNING]: invalid digest length for %s: expected value between (0, 16], but got '%s'\n", pw_data->domain, vault_contents[idx][3]);
        }
    }

    return pw_data;
}


void write_to_clipboard(const char *text);
char *read_from_clipboard();
void sleep_for(int amount);

#ifdef __APPLE__

char *read_from_clipboard() {
    int buf_size = sizeof(char)*(1024+1);
    char *buf = mmalloc(sizeof(char)*buf_size);

    FILE *pipe = popen("pbpaste", "r");
    if (pipe == NULL) {
        return "";
    }
    // MAYBE: TODO: handle longer than set buffer size from clipboard
    fgets(buf, buf_size, pipe);
    pclose(pipe);
    return buf;
}

void write_to_clipboard(const char *text) {
    FILE *pipe = popen("pbcopy", "w");
    if (pipe == NULL) {
        return;
    }
    fputs(text, pipe);
    pclose(pipe);
}

void sleep_for(int amount) {
    sleep(amount);
}

#elif __linux__

char *read_from_clipboard() {
    int buf_size = sizeof(char)*(1024+1);
    char *buf = mmalloc(sizeof(char)*buf_size);

    FILE *pipe = popen("xclip -o -selection clipboard", "r");
    if (pipe == NULL) {
        return "";
    }
    // MAYBE: TODO: handle longer than set buffer size from clipboard
    fgets(buf, buf_size, pipe);
    pclose(pipe);
    return buf;
}

void write_to_clipboard(const char *text) {
    FILE *pipe = popen("xclip -selection clipboard", "w");
    if (pipe == NULL) {
        return;
    }
    fputs(text, pipe);
    pclose(pipe);
}


void sleep_for(int amount) {
    sleep(amount);
}

#elif _WIN32

// is correct, mmalloc is self defined wrapper around malloc:
char *read_from_clipboard() {
    int buf_size = 1024;
    char *buf = mmalloc(sizeof(char)*(buf_size+1));

    if (OpenClipboard(NULL) == 0) {
        goto fastexit_win_read_clipboard;
    }
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL) {
        goto exit_win_read_clipboard;
    }

    char *clipboard = (char *)GlobalLock(hData);
    if (clipboard == NULL) {
        CloseClipboard();
        goto exit_win_read_clipboard;
    }
    size_t clipboard_len = strlen(clipboard);
    size_t size = clipboard_len;
    if (buf_size < clipboard_len) {
        size = buf_size;
    }
    memcpy(buf, clipboard, size);

    GlobalUnlock(hData);
exit_win_read_clipboard:
    CloseClipboard();
fastexit_win_read_clipboard:
    return buf;
}

void write_to_clipboard(const char *text) {
    size_t text_len = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text_len); // hMem to SetClipboardData hands over ownership of the alloced memory
    if (hMem == NULL) {
        return;
    }
    char *clipboard = (char *)GlobalLock(hMem);
    if (clipboard == NULL) {
        GlobalFree(hMem);
        return;
    }
    memcpy(clipboard, text, text_len);
    GlobalUnlock(hMem);
    if (OpenClipboard(NULL) == 0) {
        GlobalFree(hMem);
        return;
    }
    EmptyClipboard();
    if (SetClipboardData(CF_TEXT, hMem) == NULL) {
        GlobalFree(hMem);
    }
    CloseClipboard();
}

void sleep_for(int amount) {
    Sleep(1000*amount);
}
#endif

int main(int argc, char **argv) {
    char *arg_filename = NULL;
    int arg_sleep = SLEEP_FOR;
    int exit_code = 0;

    for (int i=0; i<argc; ++i) {
        if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0) && i+1 < argc) {
            size_t argv_len = strlen(argv[i+1]);
            size_t filename_size = sizeof(char)*argv_len+1; // +1 for \0
            arg_filename = mmalloc(filename_size);
            memcpy(arg_filename, argv[i+1], argv_len);
        } else if ((strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) || strcmp(argv[i], "help") == 0) {
            help();
            exit(0);
        } else if (strcmp("-v", argv[i]) == 0 || strcmp("version", argv[i]) == 0 || strcmp("-version", argv[i]) == 0 || strcmp("--version", argv[i]) == 0) {
            printf("version: %s\n", VERSION);
            return 0;
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sleep") == 0)) {
            int atoi_res = atoi(argv[i]);
            if (atoi_res > 0) {
                arg_sleep = atoi_res;
            }
        }
    }
    
    if (arg_filename == NULL) {
        fprintf(stderr, "invalid argument: filename missing\n");
        exit_code = 1;
        goto exit_main;
    }

    int line_count = 0;
    char ***vault_contents = read_vault_contents(arg_filename, &line_count);
    // vault_contents_printer(vault_contents, line_count); // REMOVEME:
    if (line_count == 0) {
        fprintf(stdout, "vault is empty\n");
        goto exit_main;
    }

    PwData *pw_data = gui(vault_contents, line_count);
    if (pw_data == NULL) { // NOTE: pw calc cancelled
        goto exit_main;
    }

    char *password = pw(pw_data->master_key, pw_data->salt, pw_data->pepper, pw_data->domain, pw_data->digest_len);
    printf("%s\n", password); // REMOVEME:

    char *prev_clipboard = read_from_clipboard();
    write_to_clipboard((const char *)password);

    sleep_for(arg_sleep); // TODO: add note to README that recommended to run this program in background because of this line

    char *cur_clipboard = read_from_clipboard();
    if (strcmp(cur_clipboard, password) == 0) {
        write_to_clipboard(prev_clipboard);
    }

exit_main:
    mfree();
    return exit_code;
}