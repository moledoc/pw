#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MD5_IMPLEMENTATION
#include "./md5.h"

// MAYBE: TODO: use calloc instead of malloc
// NOTE: while deving
// clear && clang -g -fsanitize=address vault.c && ./a.out -f /Applications/vault.app/Contents/MacOS/vault.contents
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
       
        if (c == '"') { // NOTE: naive string parsing, not checking escaped quotes (\")
            *carved_len += 1; // opening quote
            while (offset+(*carved_len) < contents_len && contents[offset+(*carved_len)] != '"') {
                *carved_len += 1;
            }
            *carved_len += 1; // closing quote
            break;
        }

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
            printf("line %d: salt '%s', pepper '%s', domain '%s', extra '%s'\n", 
                i+1,
                vault_contents[i][0],
                vault_contents[i][1],
                vault_contents[i][2],
                vault_contents[i][3]);
        }
    }
}

char *pw(char *key, char *salt, char *pepper, char *domain, int length) {
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
    
    char *pw = mmalloc(sizeof(char)*(length*2+pepper_len+1));
    for (int i=0; i<length; ++i) {
        sprintf(pw+i*2, "%02x", digest[i]);
    }
    sprintf(pw+length*2, "%s", pepper);
    return pw;
}

void help() {
    printf("TODO: help\n");
}

int main(int argc, char **argv) {
    char *arg_filename;
    int arg_time = 0;

    for (int i=0; i<argc; ++i) {
        if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0) && i+1 < argc) {
            size_t argv_len = strlen(argv[i+1]);
            size_t filename_size = sizeof(char)*argv_len+1; // +1 for \0
            arg_filename = mmalloc(filename_size);
            memcpy(arg_filename, argv[i+1], argv_len);
        } else if ((strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) || strcmp(argv[i], "help") == 0) {
            help();
            exit(0);
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--time") == 0)) {
            int atoi_res = atoi(argv[i]);
            if (atoi_res > 0) {
                arg_time = atoi_res;
            }
        }
    }

    int line_count = 0;
    char ***vault_contents = read_vault_contents(arg_filename, &line_count);
    vault_contents_printer(vault_contents, line_count);

    // TODO: GUI for selecting domain
    // domain = gui()

    // TODO: parse length properly
    // TODO: use actual key to calc password
    char *password = pw("test", "salt", "pepper", "domain", 16); // vault_contents[0][3]);
    // char *password = pw("test", vault_contents[0][0], vault_contents[0][1], vault_contents[0][2], 16); // vault_contents[0][3]);
    printf("%s\n", password);

    // TODO: set to clipboard
    // prev = current_clipboard
    // clipboard_set(password)

    // TODO: revert password after x sec from clipboard, if needed
    // if current_clipboard == password: clipboard_set(prev)
    mfree();
}