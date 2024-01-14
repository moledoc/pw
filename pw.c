#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// ascii char range to use 32-126: (0-94)+32
#define OFFSET 32
#define SIZE 32
#define SEED_SIZE 32
#define MOD 95
#define var_name(var) #var

enum mode {
	MODE_SYM,
	MODE_NUM,
	MODE_UPP,
	MODE_LOW
};

int MODE = MODE_SYM;

size_t slen(const char *s) {
	int i=0;
	while (*(s++) != '\0') {
		++i;
	}
	return i;
}

int eq(const char *s1, const char *s2) {
	size_t s1len = slen(s1);
	size_t s2len = slen(s2);
	if (s1len != s2len) {
		return 0;
	}
	for (int i=0; i<s1len; ++i) {
		if (s1[i] != s2[i]) {
			return 0;
		}
	}
	return 1;
}

unsigned long long calc_rand_seed(char *seed) {
	size_t seed_len = slen(seed);
	int rs = 1;
	for (int i=0; i<seed_len; ++i) {
		rs += (i+1)*seed[i];
	}
	return rs;
}

int fit_mode(char c) {
	int low = 'a' <= c && c <= 'z';
	int up = 'A' <= c && c <= 'Z';
	int num = '0' <= c && c <= '9';
	int sym = ' ' <= c && c <= '~';

	switch (MODE) {
	case MODE_SYM:
		return sym;
	case MODE_NUM:
		return low || up || num;
	case MODE_UPP:
		return low || up;
	case MODE_LOW:
		return low;
	default:
		return sym;
	}
}

void calc_pw(unsigned long long seed, char *pw, size_t size) {
	srand(seed);
	int i=0;
	for (;i<size;) {
		char c = rand()%MOD+OFFSET;
		if (!fit_mode(c)) {
			continue;
		}
		pw[i] = c;
		++i;
	}
}

void cpy(char *dest, const char *src) {
	size_t src_size = slen(src);
	if (src_size+1 > SEED_SIZE) {
		dest = realloc(dest, (src_size+1)*sizeof(char));
	}
	for (int i=0; i<src_size; ++i) {
		dest[i] = src[i];
	}
	dest[src_size] = '\0';
}

void help() {
	printf("NAME\n\tpw - vaultless password manager\n");
	printf("\nSYNOPSIS\n\tpw [-h] [-l LENGTH] [-m MODE] [-s SEED | -f SEEDFILE] DOMAIN\n");
	printf("\nOPTIONS\n");
	printf("\t%-20s - print this help\n", "-h, --help, help");
	printf("\t%-20s - specify length of password (default=%d)\n", "-l LENGTH", SIZE);
	printf("\t%-20s - specify password mode (default=%d)\n", "-m MODE", MODE_SYM);
	printf("\t%-20s - specify password seed; only the first -s/-f is recognized\n", "-s SEED");
	printf("\t%-20s - specify password seedfile; only the first -s/-f is recognized\n", "-f SEEDFILE");
	printf("\t%-20s - specify the domain of the password; eg 'github.com/moledoc'\n", "DOMAIN");
	printf("\nMODES\n");
	printf("\t%d (%s) - alphanumeric characters and symbols\n", MODE_SYM, var_name(MODE_SYM));
	printf("\t%d (%s) - alphanumeric characters\n", MODE_NUM, var_name(MODE_NUM));
	printf("\t%d (%s) - alphabet characters\n", MODE_UPP, var_name(MODE_UPP));
	printf("\t%d (%s) - lowercase characters\n", MODE_LOW, var_name(MODE_LOW));
	printf("\tanything else defaults to %s\n", var_name(MODE_SYM));
	printf("\nAUTHOR\n");
	printf("\tMeelis Utt\n");
}

int main(int argc, char **argv) {
	char *seed = calloc(SEED_SIZE, sizeof(char));
	char *domain;
	int free_seed = 0;
	size_t size = SIZE;
	for (int i=1; i<argc; ++i) {
		if (eq("-h", argv[i])|| eq("help", argv[i]) || eq("--help", argv[i])) {
			help();
			free(seed);
			return 0;
		} else if (eq("-s", argv[i]) && i+1 < argc && eq("", seed)) {
			cpy(seed, argv[i+1]);
			++i;
		} else if (eq("-f", argv[i]) && i+1 < argc && eq("", seed)) {
			FILE *fptr = fopen(argv[i+1], "r");
			if (!fptr) {
				fprintf(stderr, "[ERROR]: file '%s' does not exist\n", argv[i+1]);
				return ENOENT;
			}
			fseek(fptr, 0, SEEK_END);
			long seed_size = ftell(fptr);
			rewind(fptr);
			seed = realloc(seed, (seed_size+1)*sizeof(char));
			seed[seed_size] = '\0';
			fread(seed, sizeof(char), seed_size, fptr);
			fclose(fptr);
			++i;
		} else if (eq("-l", argv[i]) && i+1 < argc) {
			// MAYBE: FIXME: eg 10a1 returns 10
			size = atoi(argv[i+1]);
			if (size <= 0) {
				fprintf(stderr, "[ERROR]: invalid length provided\n");
			}
			++i;
		} else if (eq("-m", argv[i]) && i+1 < argc) {
			MODE = atoi(argv[i+1]); // atoi unsafeness is ok for us - MODE_SYM is default for any invalid input
			++i;
		} else if (i == argc - 1) {
			domain = argv[i];
			break;
		} else {
			fprintf(stderr, "[ERROR]: invalid flag '%s'\n", argv[i]);
			return EINVAL;
		}
	}
	if (eq("", domain)) {
		fprintf(stderr, "[ERROR]: no domain provided\n");
	}
	unsigned long long rs = calc_rand_seed(seed);
	free(seed);
	unsigned long long rd = calc_rand_seed(domain);
	char pw[size+1];
	pw[size] = '\0';
	calc_pw(rs*rd, pw, size);
	puts(pw);
	return 0;
}