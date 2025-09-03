#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD5_IMPLEMENTATION
#include "md5.h"

#define MMALLOC_IMPLEMENTATION
#include "mmalloc.h"

#define PW_IMPLEMENTATION
#include "pw.h"

#include "version.h"

void print_example(char *key, char *salt, char *pepper, char *domain, int *length) {
  char *buf = mmalloc(sizeof(char)*(1024+1));
  int buf_offset = 0;

  char *prog_name = "pwcli";
  size_t prog_name_len = strlen(prog_name);

  memcpy(buf, prog_name, prog_name_len);
  buf_offset += prog_name_len;

  size_t key_len = strlen(key);
  size_t salt_len = strlen(salt);
  size_t pepper_len = strlen(pepper);
  size_t domain_len = strlen(domain);
  
  if (key_len > 0) {
    snprintf(buf+buf_offset, key_len+4+1, " -k %s", key);
    buf_offset += key_len+4;
  }
  if (salt_len > 0) {
    snprintf(buf+buf_offset, salt_len+4+1, " -s %s", salt);
    buf_offset += salt_len+4;
  }
  if (pepper_len > 0) {
    snprintf(buf+buf_offset, pepper_len+4+1, " -p %s", pepper);
    buf_offset += pepper_len+4;
  }
  if (domain_len > 0) {
    snprintf(buf+buf_offset, domain_len+4+1, " -d %s", domain);
    buf_offset += domain_len+4;
  }
  if (length != NULL) {
    snprintf(buf+buf_offset, 10+4+1, " -l %d", *length);
  }

  if (length == NULL) {
    printf("%-*s -> %s", 85, buf, pw(key, salt, pepper, domain, 0));
  } else {
    printf("%-*s -> %s", 85, buf, pw(key, salt, pepper, domain, *length));
  }
}

int *i_ptr(int i) {
  int *iptr = mmalloc(sizeof(int)*1);
  *iptr = i;
  return iptr;
}

void help() {
  printf("NAME\n\tpw - vaultless password manager\n");
  printf("\nSYNOPSIS\n\tpw [-h] [-s SALT] [-p PEPPER] [-l PW_LENGTH] -k KEY DOMAIN\n");
  printf("\nOPTIONS\n");
  printf("\t%s\n\t\tprint help\n", "-h, -help, --help, help");
  printf("\t%s\n\t\tprint version\n", "-v, -version, --version, version");
  printf("\t%s\n\t\tkey file or literal (required)\n", "-k KEY, --key KEY");
  printf("\t%s\n\t\tpassword domain (required)\n", "-d DOMAIN, --domain DOMAIN");
  printf("\t%s\n\t\tsalt the password - add something-something to the "
         "digestable message\n",
         "-s SALT, --salt SALT");
  printf("\t%s\n\t\tpepper the password - add something-something to the "
         "digested message\n",
         "-p PEPPER, --pepper PEPPER");
  printf("\t%s\n\t\tlength of the password (default: 32+pepper_length);\n",
         "-l PW_LENGTH, --length PW_LENGTH");
  printf("\t\tNOTE: if length is specified out of range [1, 32+pepper_length] then default password length is used.\n");
  printf("\t\tNOTE: if specified length is less than pepper_length, then only the first character of pepper is used.\n");
  printf("\nEXAMPLES\n");
  printf("\t* pw -h\n");
  printf("\t* pw help\n");
  printf("\t* "); print_example("test", "", "", "example@example.com/exampleuser", NULL); putchar('\n');
  printf("\t* "); print_example("./README.md", "", "", "example@example.com/exampleuser", NULL); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "", "example@example.com/exampleuser", NULL); putchar('\n');
  printf("\t* "); print_example("test", "", "_TEST_TEST", "example@example.com/exampleuser", NULL); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "_TEST_TEST", "example@example.com/exampleuser", NULL); putchar('\n');
  printf("\t* "); print_example("./README.md", "test_test", "_TEST_TEST", "example@example.com/exampleuser", NULL); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "_TEST_TEST", "example@example.com/exampleuser", i_ptr(-1)); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "_TEST_TEST", "example@example.com/exampleuser", i_ptr(0)); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "_TEST_TEST", "example@example.com/exampleuser", i_ptr(32)); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "_TEST_TEST", "example@example.com/exampleuser", i_ptr(32+9)); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "_TEST_TEST", "example@example.com/exampleuser", i_ptr(100)); putchar('\n');
  printf("\t* "); print_example("test", "test_test", "@TEST_TEST", "example@example.com/exampleuser", i_ptr(7)); putchar('\n');
  printf("\nAUTHOR\n");
  printf("\tMeelis Utt (meelis.utt@gmail.com)\n");
}

int main(int argc, char **argv) {
  char *domain = 0;
  char *key = 0;
  char *salt = 0;
  char *pepper = 0;
  size_t length = 0;

  for (int i = 1; i < argc; ++i) {
    char *flag = argv[i];
    if (strcmp("-h", flag) == 0 || strcmp("help", flag) == 0 ||
        strcmp("-help", flag) == 0 || strcmp("--help", flag) == 0) {
      help();
      return 0;
    } else if (strcmp("-v", flag) == 0 || strcmp("version", flag) == 0 ||
               strcmp("-version", flag) == 0 ||
               strcmp("--version", flag) == 0) {
      version();
      return 0;
    } else if ((strcmp("-k", flag) == 0 || strcmp("--key", flag) == 0) &&
               i + 1 < argc) {
      FILE *fptr = fopen(argv[i + 1], "r");
      if (fptr) {
        fseek(fptr, 0, SEEK_END);
        long sf_size = ftell(fptr);
        rewind(fptr);
        key = mmalloc((sf_size + 1) * sizeof(char));
        key[sf_size] = '\0';
        fread(key, sizeof(char), sf_size, fptr);
        fclose(fptr);
        if (sf_size > 0 && key[sf_size - 1] == '\n') {
          key[sf_size - 1] = '\0';
        }
      } else {
        size_t len = strlen(argv[i + 1]);
        key = mmalloc((len + 1) * sizeof(char));
        key[len] = '\0';
        memcpy(key, argv[i + 1], strlen(argv[i + 1]));
      }
      ++i;
    } else if ((strcmp("-d", flag) == 0 || strcmp("--domain", flag) == 0) && i + 1 < argc) {
      domain = argv[i + 1];
      ++i;
    } else if ((strcmp("-s", flag) == 0 || strcmp("--salt", flag) == 0) && i + 1 < argc) {
      salt = argv[i + 1];
      ++i;
    } else if ((strcmp("-p", flag) == 0 || strcmp("--pepper", flag) == 0) && i + 1 < argc) {
      pepper = argv[i + 1];
      ++i;
    } else if ((strcmp("-l", flag) == 0 || strcmp("--length", flag) == 0) && i + 1 < argc) {
      length = atoi(argv[i + 1]);
      ++i;
    } else {
      fprintf(stderr, "unexpected flag '%s'\n", flag);
      return 1;
    }
  }
  if (!key) {
    fprintf(stderr, "missing required argument 'KEY'\n");
    return 1;
  }
  if (!domain) {
    fprintf(stderr, "missing required argument 'DOMAIN'\n");
    free(key);
    return 1;
  }
  if (!salt) {
    salt = "";
  }
  if (!pepper) {
    pepper = "";
  }

  printf("%s", pw(key, salt, pepper, domain, length));
  putchar('\n');
}
