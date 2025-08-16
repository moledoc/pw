#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef __APPLE__
#include <unistd.h>
#elif __linux__
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif

#define MD5_IMPLEMENTATION
#include "./md5.h"
#include "version.h"


#define SLEEP_FOR 10 // in seconds

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540
#define FONT_SIZE 20
#define FRAME_DELAY 16 // in milliseconds; ~60FPS

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

// MAYBE: TODO: use calloc instead of malloc
// NOTE: while deving
// clear && clang -g -fsanitize=address vault.c && ./a.out -f ./tests/inputs.txt
// clang pw.c -o verify && ./verify -k test -s salt -p pepper domain

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

// TODO: GUI for selecting domain
PwData *gui(char ***vault_contents, int line_count) {
    int master_key_max_len = 1024;
    char *master_key = mmalloc(sizeof(char)*(master_key_max_len+1)); // TODO: ask master key

    char *master_key_prompt = "Provide Master Key";

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
                                        SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                        SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);

    if (window == NULL) {
        fprintf(stderr, "failed to create window: '%s'\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        fprintf(stderr, "failed to create renderer: '%s'\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 1);

    TTF_Font *font = TTF_OpenFont(GUI_FONT, FONT_SIZE);
    if (font == NULL) {
        fprintf(stderr, "failed to load font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    return NULL;
    }

    SDL_RenderClear(renderer);
    
    SDL_Surface *text_surface = TTF_RenderUTF8_Solid(font, master_key_prompt, rgb_to_sdl_color(0x00000));
    if (text_surface == NULL) {
        fprintf(stderr, "failed to create text surface: %s\n", TTF_GetError());
        return NULL;
    }

    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (text_texture == NULL) {
        fprintf(stderr, "failed to create text texture: %s\n", SDL_GetError());
        return NULL;
    }
    int t_width = text_surface->w;
    int t_height = text_surface->h;
    SDL_FreeSurface(text_surface);

    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_Rect text_rect = {w/2, h/2, t_width, t_height};
    
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
 
    SDL_RenderPresent(renderer);

    float elapsed = 0;
    bool keep_window_open = true;
    Uint32 start = SDL_GetTicks64();
    Uint32 end = SDL_GetTicks64();
    while (keep_window_open) {
        start = SDL_GetTicks64();
        SDL_Event sdl_event = {0};
        while (SDL_PollEvent(&sdl_event) > 0) {
            // QUIT START
            if (sdl_event.type == SDL_QUIT ||
                    (sdl_event.type == SDL_KEYDOWN &&
                        sdl_event.key.state == SDL_PRESSED &&
                        sdl_event.key.keysym.sym == SDLK_c &&
                        sdl_event.key.keysym.mod & KMOD_CTRL) || 
                        (sdl_event.type == SDL_KEYDOWN &&
                            sdl_event.key.state == SDL_PRESSED &&
                            sdl_event.key.keysym.sym == SDLK_d &&
                            sdl_event.key.keysym.mod & KMOD_CTRL)) {
                keep_window_open = false;
            return NULL;
            // QUIT END
            }
        }

        SDL_RenderClear(renderer);

        SDL_GetWindowSize(window, &w, &h);
        SDL_Rect text_rect = {(w-t_width)/2, (h-t_height)/2, t_width, t_height};
        
        SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
        
        SDL_RenderPresent(renderer);
        end = SDL_GetTicks64();
        elapsed = end - start;
        if (elapsed <= FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - elapsed);
        }
    }
    TTF_CloseFont(font);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    ///////////////////////////

    
    int idx = 0;// TODO: select domain

    PwData *pw_data = mmalloc(sizeof(PwData)*1);
    
    pw_data->master_key = master_key;

    pw_data->salt = vault_contents[idx][0];
    pw_data->pepper = vault_contents[idx][1];
    pw_data->domain = vault_contents[idx][2];
    pw_data->digest_len = 16;
    if (vault_contents[idx][3] != NULL) {
        int tmp_digest_len = atoi(vault_contents[0][3]);
        if (0 < tmp_digest_len && tmp_digest_len <= 16) {
            pw_data->digest_len = tmp_digest_len;
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
        goto fastexit;
    }
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL) {
        goto exit;
    }

    char *clipboard = (char *)GlobalLock(hData);
    if (clipboard == NULL) {
        CloseClipboard();
        goto exit;
    }
    size_t clipboard_len = strlen(clipboard);
    size_t size = clipboard_len;
    if (buf_size < clipboard_len) {
        size = buf_size;
    }
    memcpy(buf, clipboard, size);

    GlobalUnlock(hData);
exit:
    CloseClipboard();
fastexit:
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
        return 1;
    }

    int line_count = 0;
    char ***vault_contents = read_vault_contents(arg_filename, &line_count);
    vault_contents_printer(vault_contents, line_count);
    if (line_count == 0) {
        mfree();
        fprintf(stdout, "vault is empty\n");
        return 0;
    }

    PwData *pw_data = gui(vault_contents, line_count);
    if (pw_data == NULL) { // NULL doesn't mean an error, maybe we cancelled the selection
        return 0;
    }

    char *password = pw(pw_data->master_key, pw_data->salt, pw_data->pepper, pw_data->domain, pw_data->digest_len);
    printf("%s\n", password);

    char *prev = read_from_clipboard();
    write_to_clipboard((const char *)password);

    sleep_for(arg_sleep); // TODO: add note to README that recommended to run this program in background because of this line

    char *cur = read_from_clipboard();
    if (strcmp(cur, password) == 0) {
        write_to_clipboard(prev);
    }
    mfree();
}