#include <stdio.h>
#include <errno.h>

// ascii char range to use 32-126: (0-94)+32
#define OFFSET 32
#define PW_SIZE 32
#define MAX_SEED_SIZE 4096
#define MOD 95
#define var_name(var) #var

#define LCG_A_MAX 1000
#define LCG_C_MAX 10000
#define LCG_M_MAX 100000

enum mode {
	MODE_SYM,
	MODE_NUM,
	MODE_UPP,
	MODE_LOW
};

int MODE = MODE_SYM;

// lng variables - arbitrarily selected to produce good enough pseudo-numbers
int LCG_A = 127;
int LCG_C = 5700;
int LCG_M = 58595;

size_t slen(const char *s) {
	int i=0;
	while (*(s++) != '\0') {
		++i;
	}
	return i;
}

void zero(char *arr, size_t size) {
	for (int i=0; i<size; ++i) {
		arr[i] = '\0';
	}
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

int cpy(char *dest, char *src) {
	size_t src_len = slen(src);
	if (src_len > MAX_SEED_SIZE) {
		return 0;
	}
	for (int i=0; i<src_len; ++i) {
		dest[i] = src[i];
	}
	return 1;
}

int a_i(char *ascii, int *integer) {
	size_t ascii_len = slen(ascii);
	if (!('0' <= ascii[ascii_len-1] && ascii[ascii_len-1] <= '9')) {
		 return 0;
	}
	int tmp = ascii[ascii_len-1]-'0';
	int mag = 1;
	for (int i=ascii_len-1-1; i>=0; --i) {
		if (!('0' <= ascii[i] && ascii[i] <= '9')) {
			return 0;
		}
		tmp += 10*mag*(ascii[i]-'0');
		mag *= 10;
	}
	*integer = tmp;
	return 1;
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

int lcg(int seed) {
	return (LCG_A*seed+LCG_C)%LCG_M;
}

int gen_seedling(char *arr, size_t size) {
	int seedling = 1;
	for (int i=0; i<size; ++i) {
		seedling += (i+1)*arr[i];
	}
	return seedling;
}

void calc_pw(char *seed, char *domain, char *pw, size_t pw_size) {
	size_t seed_len = slen(seed);
	int rs = gen_seedling(seed, seed_len);
	size_t domain_len = slen(domain);
	int rd = gen_seedling(domain, domain_len);

	int iseed = rs*rd;
	size_t i = 0;
	for (;i<pw_size;) {
		iseed = lcg(iseed);
		char sc = ' ';
		if (seed_len) {
			sc = seed[i%seed_len];
		}
		char c = (sc*(iseed+sc))%MOD+OFFSET;
		if (!fit_mode(c)) {
			continue;
		}
		pw[i] = c;
		++i;
	}
}

void help() {
	printf("NAME\n\tpw - vaultless password manager\n");
	printf("\nSYNOPSIS\n\tpw [-h] [-l LENGTH] [-m MODE] [--lng-a LCG_A] [--lng-c LCG_C] [--lng-m LCG_M] [-s SEED | -f SEEDFILE] DOMAIN\n");
	printf("\nOPTIONS\n");
	printf("\t%-20s - print this help\n", "-h, --help, help");
	printf("\t%-20s - specify length of password (default=%d)\n", "-l LENGTH", PW_SIZE);
	printf("\t%-20s - specify password mode (default=%d)\n", "-m MODE", MODE_SYM);
	printf("\t%-20s - specify password seed; only the first -s/-f is recognized\n", "-s SEED");
	printf("\t%-20s - specify password seedfile; only the first -s/-f is recognized\n", "-f SEEDFILE");
	printf("\t%-20s - specify LCG multiplier (default=%d); 0<LCG_A<=%d\n", "--lng-a LCG_A", LCG_A, LCG_A_MAX);
	printf("\t%-20s - specify LCG constant (default=%d); 0<LCG_C<=%d\n", "--lng-c LCG_C", LCG_C, LCG_C_MAX);
	printf("\t%-20s - specify LCG modulus (default=%d); %d<LCG_M<=%d\n", "--lng-m LCG_M", LCG_M, MOD, LCG_C_MAX);
	printf("\t%-20s - specify the domain of the password; eg 'github.com/moledoc'\n", "DOMAIN");
	printf("\nMODES\n");
	printf("\t%d (%s) - alphanumeric characters and symbols\n", MODE_SYM, var_name(MODE_SYM));
	printf("\t%d (%s) - alphanumeric characters\n", MODE_NUM, var_name(MODE_NUM));
	printf("\t%d (%s) - alphabet characters\n", MODE_UPP, var_name(MODE_UPP));
	printf("\t%d (%s) - lowercase characters\n", MODE_LOW, var_name(MODE_LOW));
	printf("\tanything else defaults to %s\n", var_name(MODE_SYM));
	printf("\nAUTHOR\n");
	printf("\tMeelis Utt (meelis.utt@gmail.com)\n");
}

int main(int argc, char **argv) {
	char seed[MAX_SEED_SIZE];
	zero(seed, MAX_SEED_SIZE);
	char *domain = 0;
	size_t pw_size = PW_SIZE;
	for (int i=1; i<argc; ++i) {
		if (eq("-h", argv[i])|| eq("help", argv[i]) || eq("--help", argv[i])) {
			help();
			return 0;
		} else if (eq("-s", argv[i]) && i+1 < argc && *seed == '\0') {
			int isvalid = cpy(seed, argv[i+1]);
			if (!isvalid) {
				fprintf(stderr, "[ERROR]: seed size (%ld) greater than allowed (%d)\n", slen(argv[i+1]), MAX_SEED_SIZE);
				return EINVAL;
			}
			++i;
		} else if (eq("-f", argv[i]) && i+1 < argc && *seed == '\0') {
			FILE *fptr = fopen(argv[i+1], "r");
			if (!fptr) {
				fprintf(stderr, "[ERROR]: file '%s' does not exist\n", argv[i+1]);
				return ENOENT;
			}
			fseek(fptr, 0, SEEK_END);
			long sf_size = ftell(fptr);
			rewind(fptr);
			if (sf_size > MAX_SEED_SIZE) {
				fprintf(stderr, "[ERROR]: seed size (%ld) greater than allowed (%d)\n", sf_size, MAX_SEED_SIZE);
				return EINVAL;
			}
			fread(seed, sizeof(char), sf_size, fptr);
			fclose(fptr);
			++i;
		} else if (eq("-l", argv[i]) && i+1 < argc) {
			int size;
			int isvalid = a_i(argv[i+1], &size);
			if (!isvalid) {
				fprintf(stderr, "[ERROR]: invalid length provided\n");
				return EINVAL;
			}
			pw_size = (size_t)size;
			++i;
		} else if (eq("-m", argv[i]) && i+1 < argc) {
			a_i(argv[i+1], &MODE); // if was invalid nr, MODE is not changed and stays default, ie MODE_SYM
			++i;
		} else if (eq("--lng-a", argv[i]) && i+1 < argc) {
			int isvalid = a_i(argv[i+1], &LCG_A);
			if (!isvalid || !(0 < LCG_A && LCG_A <= LCG_A_MAX)) {
				fprintf(stderr, "[ERROR]: invalid LCG multiplier provided\n");
				return EINVAL;
			}
			++i;
		} else if (eq("--lng-c", argv[i]) && i+1 < argc) {
			int isvalid = a_i(argv[i+1], &LCG_C);
			if (!isvalid || !(0 < LCG_C && LCG_C <= LCG_C_MAX)) {
				fprintf(stderr, "[ERROR]: invalid LCG constant provided\n");
				return EINVAL;
			}
			++i;
		} else if (eq("--lng-m", argv[i]) && i+1 < argc) {
			int isvalid = a_i(argv[i+1], &LCG_M);
			if (!isvalid || !(MOD < LCG_M && LCG_M <= LCG_M_MAX)) {
				fprintf(stderr, "[ERROR]: invalid LCG modulo provided\n");
				return EINVAL;
			}
			++i;
		} else if (i == argc - 1) {
			domain = argv[i];
			break;
		} else {
			fprintf(stderr, "[ERROR]: unsupported flag '%s'\n", argv[i]);
			help();
			return EINVAL;
		}
	}


	if (domain == NULL) {
		fprintf(stderr, "[ERROR]: no domain provided\n");
		return EINVAL;
	}

	char pw[pw_size+1];
	pw[pw_size] = '\0';
	calc_pw(seed, domain, pw, pw_size);
	puts(pw);
	return 0;		
}
