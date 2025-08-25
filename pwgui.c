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

#define MMALLOC_IMPLEMENTATION
#include "./mmalloc.h"

#define PW_IMPLEMENTATION
#include "./pw.h"

#include "version.h"

#define ALLOWED_MASTER_KEY_LEN 1024
#define INPUT_MAX_LEN 1024
#define SLEEP_FOR 10 // in seconds

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 540

#define FONT_SIZE 13
#define PADDING 5
#define FRAME_DELAY 16 // in milliseconds; ~60FPS
#define MILLISECOND 1

#ifdef __APPLE__
#define FONT "/System/Library/Fonts/Menlo.ttc"
#elif __linux__
#define FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#elif _WIN32
#define FONT "C:\\Windows\\Fonts\\segoeui.ttf"
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
#define SCROLLBAR rgb_to_sdl_color(0xA39F94)

float scale = 1.0f;
float font_size = FONT_SIZE;
float padding = PADDING;
float vault_contents_lower_limit = WINDOW_HEIGHT - FONT_SIZE - PADDING;

// TODO: remove `window_h-font` commented out code

int clamp(int target, int lower_bound, int upper_bound) {
    return target * (lower_bound <= target && target <= upper_bound) +
            lower_bound * (target < lower_bound) +
            upper_bound * (upper_bound < target);
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
    int content_count = 0;
    int offset = 0;
    for (int i=0; i<(*line_count); ++i) {
        vault_contents[content_count] = mmalloc(sizeof(char *)*4); // salt pepper domain "<additional>"
        for (int j=0; j<4; ++j) {
            if (contents[offset] == '\n') {
                offset += 1; // +1 jump over '\n'
                break;
            }
            int carved_len = 0;
            char *carved = carve_str(contents, read_bytes, offset, &carved_len);
            vault_contents[content_count][j] = carved;
            if (contents[offset+carved_len] == '\n') {
                offset += carved_len+1; // +1 to jump over '\n'
                break;
            }
            offset += carved_len+1; // +1 to jump over that ' ' and '\n'
        }
        if (vault_contents[content_count][2] != NULL) {
            content_count += 1;
        }
    }
    *line_count = content_count;
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

void help() {
    printf("TODO: help\n");
}

bool fuzzy_match(const char *text, const char *pattern) {
    if (*pattern == '\0') { // NOTE: empty pattern, match all
        return true;
    }
    while (*text != '\0' && *pattern != '\0') {
        if (*text == *pattern) {
            pattern++;
        }
        text++;
    }
    return *pattern == '\0';
}

typedef struct {
    char *master_key;
    char *salt;
    char *pepper;
    char *domain;
    int length;
} PwData;

typedef struct {
    int idx;
    SDL_Rect *rect;
    SDL_Texture *t;
} Texture;

char *ask_master_key(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font) {

    int master_key_max_len = ALLOWED_MASTER_KEY_LEN;
    int input_offset = 0;
    int prev_input_offset = 0;

    char *master_key = mmalloc(sizeof(char)*(master_key_max_len+1));

    char *master_key_prompt = "Provide Master Key";

    Texture *master_key_texture = mmalloc(sizeof(Texture)*1);
    master_key_texture->rect = mmalloc(sizeof(SDL_Rect)*1);

    int window_w;
    int window_h;
    SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
    
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

    SDL_GetRendererOutputSize(renderer, &window_w, &window_h);    
    SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);

    float elapsed = 0;
    bool asking_for_master_key = true;
    Uint32 start = SDL_GetTicks64();
    Uint32 end = SDL_GetTicks64();    
    SDL_StartTextInput();

    goto master_key_loop_render; // NOTE: render on first iter
    while (asking_for_master_key) {
        start = SDL_GetTicks64();
        SDL_Event sdl_event = {0};
        bool any_changes = false;
        while (SDL_PollEvent(&sdl_event) > 0) {
            // START QUIT
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
            // END QUIT

            // START BACKSPACE
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_BACKSPACE) {
                any_changes = true;

                master_key[input_offset] = '\0';
                if (input_offset > 0){
                    input_offset -= 1;
                }
            // END BACKSPACE

            // START ENTER
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_RETURN) {
                asking_for_master_key = false;
                break;
            // END ENTER

            // START TEXT
            } else if (sdl_event.type == SDL_TEXTINPUT) {
                any_changes = true;

                size_t event_text_len = strlen(sdl_event.text.text);
                if (input_offset+event_text_len <= master_key_max_len) {
                    memcpy(master_key+input_offset, sdl_event.text.text, event_text_len);
                    input_offset += event_text_len;
                }
            // END TEXT

            // START WINDOW RESIZE
            } else if (sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_RESIZED) {
                any_changes = true;

                SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
                master_key_texture->rect->x = (window_w-master_key_texture->rect->w)/2;
                master_key_texture->rect->y = (window_h-master_key_texture->rect->h)/2;
            // END WINDOW RESIZE
            }
        }

        if (!any_changes) {
            goto key_loop_delay;
        }

master_key_loop_render:
        SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
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

key_loop_delay:
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

    int window_w;
    int window_h;
    SDL_GetRendererOutputSize(renderer, &window_w, &window_h);

    SDL_Surface *input_prompt_surface = TTF_RenderUTF8_Solid(font, input_buf, BLACK);
    if (input_prompt_surface == NULL) {
        fprintf(stderr, "[WARNING]: failed to create input text surface: %s\n", TTF_GetError());
        return NULL;
    }

    input_texture->rect->x = padding;
    input_texture->rect->y = vault_contents_lower_limit; // window_h-font_size;
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

Texture **create_vault_contents_textures(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font, char ***vault_contents, int line_count, int *max_offset_h) {

    int window_w;
    int window_h;
    SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
    int h_offset = 0;

    Texture **textures = mmalloc(sizeof(Texture *)*line_count);
    for (int i=0; i<line_count; i++) {
        textures[i] = mmalloc(sizeof(Texture)*1);
        textures[i]->rect = mmalloc(sizeof(SDL_Rect)*1);
        textures[i]->idx = i;
    }
    
    for (int i=0; i<line_count; i++) {
        SDL_Surface *surface = TTF_RenderUTF8_Solid(font, vault_contents[i][2], BLACK);
        if (surface == NULL) {
            fprintf(stderr, "[WARNING]: failed to create text surface: %s; skipping %s\n", TTF_GetError(), vault_contents[i][2]);
            continue;
        }

        textures[i]->rect->x = padding;
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
    *max_offset_h = h_offset;
    return textures;
}

int select_vault_content_idx(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font, char ***vault_contents, int line_count) {

    int window_w;
    int window_h;
    SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
    vault_contents_lower_limit = window_h - font_size - padding; // NOTE: just in case recalc, as window might've been resized when asking master key

    SDL_Color prev_renderer_color = {0};
    SDL_GetRenderDrawColor(renderer, (Uint8 *)&prev_renderer_color.r, (Uint8 *)&prev_renderer_color.g, (Uint8 *)&prev_renderer_color.b, (Uint8 *)&prev_renderer_color.a);

    char *input_prompt = "> ";
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
        return -1;
    }

    int max_offset_h = 0;
    Texture **vault_contents_textures = create_vault_contents_textures(window, renderer, font, vault_contents, line_count, &max_offset_h);

    Texture **print_these_textures = mmalloc(sizeof(Texture *)*line_count);
    int print_textures_count = 0;

    for (int i=0; i<line_count; i++) { // NOTE: print all in the beginning
        print_these_textures[i] = vault_contents_textures[i];
        print_textures_count += 1;
    }

    float elapsed = 0;
    bool select_vault_content = true;
    int selected_idx = 0;
    int vertical_offset_idx = 0;
    size_t event_text_len;
    Uint32 start = SDL_GetTicks64();
    Uint32 end = SDL_GetTicks64();  
    Uint64 last_mouse_click_tick = 0;  
    SDL_StartTextInput();

    goto vault_loop_render; // NOTE: to draw on first iteration

    while (select_vault_content) {
        start = SDL_GetTicks64();
        SDL_Event sdl_event = {0};
        bool any_changes = false;
        while (SDL_PollEvent(&sdl_event) > 0) {
            // START QUIT
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
                selected_idx = -1; // NOTE: -1 for early return
                break;
            // END QUIT

            // START BACKSPACE
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_BACKSPACE && 
                    input_offset > input_prompt_len) {
                any_changes = true;

                input_offset -= 1;
                input_buf[input_offset] = '\0';
                SDL_DestroyTexture(input_texture->t);
                input_texture = create_input_texture(window, renderer, font, input_texture, input_buf);
                    
                memset(print_these_textures, 0, print_textures_count); // NOTE: just in case
                print_textures_count = 0;
                for (int i=0; i<line_count; i++) {
                    if (input_offset >= input_prompt_len && fuzzy_match(vault_contents[i][2], input_buf+input_prompt_len)) {
                        print_these_textures[print_textures_count] = vault_contents_textures[i];
                        print_textures_count += 1;
                    }
                }

                if (selected_idx > print_textures_count-1) { // NOTE: keep selection in-place when possible
                    selected_idx = 0;
                }
                vertical_offset_idx = 0;
            // END BACKSPACE

            // START ENTER
            } else if (sdl_event.type == SDL_KEYDOWN &&
                    sdl_event.key.state == SDL_PRESSED &&
                    sdl_event.key.keysym.sym == SDLK_RETURN) {
                select_vault_content = false;
                break;
            // END ENTER

            // START TEXT
            } else if (sdl_event.type == SDL_TEXTINPUT && (event_text_len = strlen(sdl_event.text.text)) && input_offset+event_text_len <= input_max_len) {
                any_changes = true;

                memcpy(input_buf+input_offset, sdl_event.text.text, event_text_len);
                input_offset += event_text_len;
                SDL_DestroyTexture(input_texture->t);
                input_texture = create_input_texture(window, renderer, font, input_texture, input_buf);
 
                memset(print_these_textures, 0, print_textures_count); // NOTE: just in case
                print_textures_count = 0;
                for (int i=0; i<line_count; i++) {
                    if (input_offset >= input_prompt_len && fuzzy_match(vault_contents[i][2], input_buf+input_prompt_len)) {
                        print_these_textures[print_textures_count] = vault_contents_textures[i];
                        print_textures_count += 1;
                    }
                }
                if (selected_idx > print_textures_count-1) { // NOTE: keep selection in-place when possible
                    selected_idx = 0;
                }
                vertical_offset_idx = 0;
            // END TEXT

            // START SELECT UP
            } else if (
                (0 < selected_idx && selected_idx < print_textures_count) && (
                        (sdl_event.type == SDL_KEYDOWN && sdl_event.key.state == SDL_PRESSED && sdl_event.key.keysym.sym == SDLK_UP)  ||
                        (sdl_event.type == SDL_MOUSEWHEEL && sdl_event.wheel.y > 0)
                    )
                ) {
                any_changes = true;

                selected_idx -= 1;
                if (0 < vertical_offset_idx && 
                    print_these_textures[selected_idx]->rect->y < 0) {
                    vertical_offset_idx -= 1;
                }
            // END SELECT UP

            // START SELECT DOWN
            } else if (
                (0 <= selected_idx && selected_idx < print_textures_count-1) && (
                        (sdl_event.type == SDL_KEYDOWN && sdl_event.key.state == SDL_PRESSED && sdl_event.key.keysym.sym == SDLK_DOWN) ||
                        (sdl_event.type == SDL_MOUSEWHEEL && sdl_event.wheel.y < 0)
                    )
                ) {
                any_changes = true;
        
                selected_idx += 1;
                if (vault_contents_lower_limit < print_these_textures[selected_idx]->rect->y+print_these_textures[selected_idx]->rect->h) {
                    vertical_offset_idx += 1;
                }
            // END SELECT DOWN

            // START MOUSE SELECT
            } else if (sdl_event.type == SDL_MOUSEBUTTONDOWN  &&
                sdl_event.button.state == SDL_PRESSED &&
                sdl_event.button.button == SDL_BUTTON_LEFT) {
            any_changes = true;

            int mouse_y;
            SDL_GetMouseState(NULL, &mouse_y);
            mouse_y *= scale;
            int clicked_idx = -1;

            for (int i=vertical_offset_idx; i<print_textures_count; i++) {
                int rect_h_start = print_these_textures[i]->rect->y;
                int rect_h_end = print_these_textures[i]->rect->y + print_these_textures[i]->rect->h;
                int rect_h = print_these_textures[i]->rect->h;
                if (mouse_y + rect_h < vault_contents_lower_limit /*window_h-font_size*/ && rect_h_start <= mouse_y && mouse_y < rect_h_end) {
                    clicked_idx = print_these_textures[i]->idx;
                    break;
                }
            }

            Uint64 current_tick = SDL_GetTicks64();
            if (selected_idx == clicked_idx && current_tick - last_mouse_click_tick <= 250 * MILLISECOND) { // NOTE: double-click equals to enter
                select_vault_content = false;
                break;
            }
            last_mouse_click_tick = current_tick;

            if (clicked_idx >= 0) {
                selected_idx = clicked_idx;
            }
            // END MOUSE SELECT
            
            // WINDOW RESIZE START
            } else if(sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_RESIZED) {
                any_changes = true;

                SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
                vault_contents_lower_limit = window_h - font_size - padding;
                input_texture->rect->y = vault_contents_lower_limit; // window_h-font_size;

                int offset_h = 0;
                for (int i=0; i<selected_idx; i++) { // NOTE: only change if selected_idx wouldn't fit anymore - then select last visible
                    if (vault_contents_lower_limit /*window_h - font_size*/ < offset_h + font_size) {
                        selected_idx = i > 0 ? i-1 : 0;
                        vertical_offset_idx = 0;
                        break;
                    }
                    offset_h += print_these_textures[i]->rect->h;
                }
            // END WINDOW RESIZE
            }

            end = SDL_GetTicks64();
            elapsed = end - start;
            if (elapsed >= 0.8*FRAME_DELAY) { // NOTE: if loop taken 80% time, break from handling events to render
                break;
            }
        }

        if (!any_changes) {
            goto vault_loop_delay;
        }

vault_loop_render:
        SDL_RenderClear(renderer);
        for (int i=vertical_offset_idx-1; i>=0; i--) {
            SDL_Rect *corrected_rect = print_these_textures[i]->rect;
            corrected_rect->y = -(print_these_textures[i]->rect->h * (vertical_offset_idx-i)); 
        }
        int offset_h = 0;
        for (int i=vertical_offset_idx; i<print_textures_count; i++) {
            bool should_print = true;
            SDL_Rect *corrected_rect = print_these_textures[i]->rect;
            corrected_rect->y = offset_h;

            if (print_these_textures[i]->rect->y < 0 ||
                vault_contents_lower_limit /*window_h - font_size*/ < offset_h + print_these_textures[i]->rect->h) {
                should_print = false;
            }
            if (should_print) {
                if (i == selected_idx) {
                    SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
                    SDL_RenderFillRect(renderer, &(SDL_Rect){
                        .x=print_these_textures[i]->rect->x, 
                        .y=offset_h,
                        .w=window_w, 
                        .h=print_these_textures[i]->rect->h
                    });
                    SDL_SetRenderDrawColor(renderer, prev_renderer_color.r, prev_renderer_color.g, prev_renderer_color.b, prev_renderer_color.a); 
                }
                SDL_RenderCopy(renderer, print_these_textures[i]->t, NULL, corrected_rect);
            }
            offset_h += corrected_rect->h;
        }
        

        {
            // horizontal border
            SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
            SDL_RenderFillRect(renderer, &(SDL_Rect){.x=0, .y=vault_contents_lower_limit/*window_h - font_size*/, .w=window_w, .h=1}); // NOTE: border to sep vault content from input
            SDL_RenderCopy(renderer, input_texture->t, NULL, input_texture->rect);
            SDL_SetRenderDrawColor(renderer, prev_renderer_color.r, prev_renderer_color.g, prev_renderer_color.b, prev_renderer_color.a);
        }
        
        SDL_RenderPresent(renderer);

vault_loop_delay:
        end = SDL_GetTicks64();
        elapsed = end - start;
        if (elapsed <= FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - elapsed);
        }
    }

    SDL_StopTextInput();

    for (int i=0; i<line_count; i++) {
        SDL_DestroyTexture(vault_contents_textures[i]->t);
    }
    SDL_DestroyTexture(input_texture->t);
    if (selected_idx < 0 || print_textures_count == 0) {
        return -1;
    }
    return print_these_textures[selected_idx]->idx;
}

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


    SDL_Window *window = SDL_CreateWindow("pwgui", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (window == NULL) {
        fprintf(stderr, "failed to create window: '%s'\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "failed to create renderer: '%s'\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    int logical_w, logical_h;
    SDL_GetWindowSize(window, &logical_w, &logical_h);
    int renderer_w, renderer_h;
    SDL_GetRendererOutputSize(renderer, &renderer_w, &renderer_h);
    float scale_w = renderer_w/logical_w;
    float scale_h = renderer_h/logical_h;

    scale = (scale_w + scale_h) / 2.0f;
    font_size *= scale;
    padding = 0.20f*font_size;
    vault_contents_lower_limit = renderer_h - font_size - padding;

    TTF_Font *font = TTF_OpenFont(FONT, font_size);
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
    SDL_GetRendererOutputSize(renderer, &window_w, &window_h);

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
    pw_data->length = 0;
    if (vault_contents[idx][3] != NULL) {
        pw_data->length = atoi(vault_contents[idx][3]);
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

    char *password = pw(pw_data->master_key, pw_data->salt, pw_data->pepper, pw_data->domain, pw_data->length);
    printf("domain %s; password %s\n", pw_data->domain, password); // REMOVEME:

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
