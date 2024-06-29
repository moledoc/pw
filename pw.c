#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD5_IMPLEMENTATION
#include "md5.h"

#define VERSION "0.5.1"

void help() {
  printf("NAME\n\tpw - vaultless password manager\n");
  printf("\nSYNOPSIS\n\tpw [-h] [-s SALT] [-p PEPPER] -k KEY DOMAIN\n");
  printf("\nOPTIONS\n");
  printf("\t%s\n\t\tprint help\n", "-h, -help, --help, help");
  printf("\t%s\n\t\tprint version\n", "-h, -version, --version, version");
  printf("\t%s\n\t\tkey file or literal (required)\n", "-k KEY, --key KEY");
  printf("\t%s\n\t\tsalt the password - add something-something to the "
         "digestable message\n",
         "-s SALT, --salt SALT");
  printf("\t%s\n\t\tpepper the password - add something-something to the "
         "digested message\n",
         "-p PEPPER, --pepper PEPPER");
  printf("\t%s\n\t\thow many digest elements are printed (default: 16, "
         "allowed: 1-16);\n",
         "-l LENGTH, --length LENGTH");
  printf("\t\tNOTE: each digest element is 2 characters long\n");
  printf("\t%s\n\t\tpassword domain (required)\n", "DOMAIN");
  printf("\nEXAMPLES\n");
  printf("\t* pw -h\n");
  printf("\t* pw help\n");
  printf("\t* pw -k test example@example.com/exampleuser\n");
  printf("\t* pw -k ./README.md example@example.com/exampleuser\n");
  printf("\t* pw -k test -s test_test example@example.com/exampleuser\n");
  printf("\t* pw -k test -p TEST_TEST example@example.com/exampleuser\n");
  printf("\t* pw -k test -s test_test -p TEST_TEST "
         "example@example.com/exampleuser\n");
  printf("\t* pw -k ./README.md -s test_test -p TEST_TEST "
         "example@example.com/exampleuser\n");
  printf("\nAUTHOR\n");
  printf("\tMeelis Utt (meelis.utt@gmail.com)\n");
}

int main(int argc, char **argv) {
  char *domain = 0;
  char *key = 0;
  char *salt = 0;
  char *pepper = 0;
  size_t length = 16;

  for (int i = 1; i < argc; ++i) {
    char *flag = argv[i];
    if (strcmp("-h", flag) == 0 || strcmp("help", flag) == 0 ||
        strcmp("-help", flag) == 0 || strcmp("--help", flag) == 0) {
      help();
      return 0;
    } else if (strcmp("-v", flag) == 0 || strcmp("version", flag) == 0 ||
        strcmp("-version", flag) == 0 || strcmp("--version", flag) == 0) {
      printf("version: %s\n", VERSION);
      return 0;
    } else if ((strcmp("-k", flag) == 0 || strcmp("--key", flag) == 0) &&
               i + 1 < argc) {
      FILE *fptr = fopen(argv[i + 1], "r");
      if (fptr) {
        fseek(fptr, 0, SEEK_END);
        long sf_size = ftell(fptr);
        rewind(fptr);
        key = malloc((sf_size+1) * sizeof(char));
        key[sf_size] = '\0';
        fread(key, sizeof(char), sf_size, fptr);
        fclose(fptr);
        if (key[sf_size-1] == '\n') {
          key[sf_size-1] = '\0';
        }
      } else {
        size_t len = strlen(argv[i + 1]);
        key = malloc((len+1) * sizeof(char));
        key[len] = '\0';
        memcpy(key, argv[i+1], strlen(argv[i+1]));
      }
      ++i;
    } else if ((strcmp("-s", flag) == 0 || strcmp("--salt", flag) == 0) &&
               i + 1 < argc) {
      salt = argv[i + 1];
      ++i;
    } else if ((strcmp("-p", flag) == 0 || strcmp("--pepper", flag) == 0) &&
               i + 1 < argc) {
      pepper = argv[i + 1];
      ++i;
    } else if ((strcmp("-l", flag) == 0 || strcmp("--length", flag) == 0) &&
               i + 1 < argc) {
      length = atoi(argv[i + 1]);
      ++i;
    } else if (i == argc - 1) {
      domain = flag;
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
  if (length <= 0 || 16 < length) {
    fprintf(stderr, "invalid length provided, needs to be in [1, 16].\n");
    return 1;
  }

  size_t domain_len = strlen(domain);
  size_t key_len = strlen(key);
  size_t salt_len = strlen(salt);
  size_t message_len = domain_len + key_len + salt_len;
  char message[message_len + 1];
  message[message_len] = '\0';
  memcpy(message, domain, domain_len);
  memcpy(message + domain_len, key, key_len);
  memcpy(message + domain_len + key_len, salt, salt_len);

  unsigned char digest[16];
  md5(message, digest);
  md5_print(digest, length);
  printf("%s", pepper);
  putchar('\n');
  free(key);
}
