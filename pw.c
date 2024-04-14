#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MD5_IMPLEMENTATION
#include "md5.h"

void help() {
	printf("NAME\n\tpw - vaultless password manager\n");
	printf("\nSYNOPSIS\n\tpw [-h] [-s SALT] -k KEY DOMAIN\n");
	printf("\nOPTIONS\n");
	printf("\t%s\n\t\tprint help\n", "-h, -help, --help, help");
	printf("\t%s\n\t\tkey file or literal\n", "-k KEY, --key KEY");
	printf("\t%s\n\t\tsalt the password\n", "-s SALT, --salt SALT");
	printf("\t%s\n\t\tpassword domain, i.e. for what; eg 'github.com/moledoc'\n", "DOMAIN");
	printf("\nEXAMPLES\n");
	printf("\tTODO:\n");
	printf("\nAUTHOR\n");
	printf("\tMeelis Utt (meelis.utt@gmail.com)\n");
}

int main(int argc, char **argv) {
	char *domain;
	char *key = malloc(0);
	char *salt = 0;

	for (int i=1; i<argc; ++i) {
		char *flag = argv[i];
		if (strcmp("-h", flag)==0|| strcmp("help", flag)==0 || strcmp("-help", flag)==0 || strcmp("--help", flag)==0) {
			help();
			return 0;
		} else if ((strcmp("-k", flag)==0 || strcmp("--k", flag)==0) && i+1 < argc) {
			FILE *fptr = fopen(argv[i+1], "r");
			if (fptr) {
				fseek(fptr, 0, SEEK_END);
				long sf_size = ftell(fptr);
				rewind(fptr);
				key = malloc(sf_size*sizeof(char));
				fread(key, sizeof(char), sf_size, fptr);
				fclose(fptr);
			} else {
				key = realloc(key, strlen(argv[i+1])*sizeof(char));
				strncpy(key, argv[i+1], strlen(argv[i+1]));
			}
		} else if ((strcmp("-s", flag)==0 || strcmp("--salt", flag)==0) && i+1 < argc) {
			salt = argv[i+1];
		} else if (i == argc - 1) {
			domain = flag;
		}
	}
	if (!domain || !key) {
		fprintf(stderr, "missing at least 1 required argument\n");
		return 1;
	}
	if (!salt) {
		salt = "";
	}

	size_t message_len = strlen(domain)+strlen(key)+strlen(salt);
	char message[message_len];
	snprintf(message, message_len, "%s%s%s", domain, key, salt);

	unsigned char digest[16];
	md5(message, digest);
	md5_print(digest);

	free(key);
}
