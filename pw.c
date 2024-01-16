#include <stdio.h>
#include <errno.h>

// ascii char range to use 32-126: (0-94)+32
#define OFFSET 32
#define PW_SIZE 32
#define MAX_SEED_SIZE 4096
#define MOD 95
#define var_name(var) #var

#define LCG_M_MAX 100000

enum mode {
	MODE_SYM,
	MODE_NUM,
	MODE_UPP,
	MODE_LOW
};

int MODE = MODE_SYM;

// lcg variables
int LCG_A = 0;
int LCG_C = 0;
int LCG_M = 0;

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

void cpy(char *dest, char *src) {
	size_t src_len = slen(src);
	if (src_len > MAX_SEED_SIZE) {
		errno = EINVAL;
		return;
	}
	for (int i=0; i<src_len; ++i) {
		dest[i] = src[i];
	}
	return;
}

int a_i(char *ascii) {
	size_t ascii_len = slen(ascii);
	if (ascii_len == 0 || !('0' <= ascii[ascii_len-1] && ascii[ascii_len-1] <= '9')) {
		errno = EINVAL;
		return 0;
	}
	int until = 0;
	int sign = 1;
	if (ascii[0] == '-') {
		until = 1;
		sign = -1;
	}
	int tmp = ascii[ascii_len-1]-'0';
	int mag = 1;
	for (int i=ascii_len-1-1; i>=until; --i) {
		if (!('0' <= ascii[i] && ascii[i] <= '9')) {
			errno = EINVAL;
			return 0;
		}
		tmp += 10*mag*(ascii[i]-'0');
		mag *= 10;
	}
	return sign*tmp;
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
	int nseed = rs*rd;

	int failsafe = 0;
	size_t i = 0;
	for (;i<pw_size;) {
		if (failsafe >= 1000000) {
			errno = ECANCELED;
			return;
		}
		char sc = seed[i%seed_len];
		nseed = lcg(nseed);
		char c = sc*(nseed+sc)%MOD+OFFSET;
		if (!fit_mode(c)) {
			++failsafe;
			continue;
		}
		pw[i] = c;
		++i;
	}
}

void validate_lcg(int a, int c, int m) {
	if (!(0 < m && m <= LCG_M_MAX)) {
		errno = EINVAL;
		return;
	}
	if (!(0 < a && a < LCG_M)) {
		errno = EINVAL;
		return;
	}
	if (!(0 < c && c < LCG_M)) {
		errno = EINVAL;
		return;
	}
}

void help() {
	printf("NAME\n\tpw - vaultless password manager\n");
	printf("\nSYNOPSIS\n\tpw [-h] [-l LENGTH] [-m MODE] -lcg A C M -s SEED DOMAIN\n");
	printf("\nOPTIONS\n");
	printf("\t%s\n\t\tprint this help\n", "-h, -help, --help, help");
	printf("\t%s\n\t\tspecify the domain of the password; eg 'github.com/moledoc'\n", "DOMAIN");
	printf("\t%s\n\t\tspecify password seed:\n", "-s SEED");
	printf("\t\t - if points to file, seed inside file\n");
	printf("\t\t - else is seed\n");
	printf("\t%s\n\t\tspecify LCG modifiers A, C, M:\n", "-lcg");
	printf("\t\t - M (modulo), 0 < M < %s (%d)\n", var_name(LCG_M_MAX), LCG_M_MAX);
	printf("\t\t - A (multiplier), 0 < A < M\n");
	printf("\t\t - C (constant), 0 < C < M\n");
	printf("\t%s\n\t\tspecify length of password (default=%d)\n", "-l LENGTH", PW_SIZE);
	printf("\t%s\n\t\tspecify password mode (default=%d)\n", "-m MODE", MODE_SYM);
	printf("\nMODES\n");
	printf("\t%d (%s) - alphanumeric characters and symbols\n", MODE_SYM, var_name(MODE_SYM));
	printf("\t%d (%s) - alphanumeric characters\n", MODE_NUM, var_name(MODE_NUM));
	printf("\t%d (%s) - alphabet characters\n", MODE_UPP, var_name(MODE_UPP));
	printf("\t%d (%s) - lowercase characters\n", MODE_LOW, var_name(MODE_LOW));
	printf("\tanything else defaults to %s\n", var_name(MODE_SYM));
	printf("\nEXAMPLES\n");
	printf("\tpw -s test-test-test example@example.com/exampleuser\n");
	printf("\nAUTHOR\n");
	printf("\tMeelis Utt (meelis.utt@gmail.com)\n");
}

int main(int argc, char **argv) {
	char seed[MAX_SEED_SIZE];
	zero(seed, MAX_SEED_SIZE);
	char *domain = 0;
	size_t pw_size = PW_SIZE;

	// parse args
	for (int i=1; i<argc; ++i) {
		char *flag = argv[i];
		if (eq("-h", flag)|| eq("help", flag) || eq("-help", flag) || eq("--help", flag)) {
			help();
			return 0;
		} else if (eq("-s", flag) && i+1 < argc && *seed == '\0') {
			FILE *fptr = fopen(argv[i+1], "r");
			if (fptr) {			
				fseek(fptr, 0, SEEK_END);
				long sf_size = ftell(fptr);
				rewind(fptr);
				if (sf_size > MAX_SEED_SIZE) {
					errno = EINVAL;
				} else {
					fread(seed, sizeof(char), sf_size, fptr);
				}
				fclose(fptr);
			} else {
				errno = 0;
				cpy(seed, argv[i+1]);
			}
		} else if (eq("-l", flag) && i+1 < argc) {
			pw_size = (size_t)a_i(argv[i+1]);
		} else if (eq("-m", flag) && i+1 < argc) {
			MODE = a_i(argv[i+1]);
		} else if (eq("--lcg", flag) && i+3 < argc) {
			LCG_A = a_i(argv[i+1]);
			LCG_C = a_i(argv[i+2]);
			LCG_M = a_i(argv[i+3]);
			validate_lcg(LCG_A, LCG_C, LCG_M);
			i+=2;
		} else if (i == argc - 1) {
			domain = flag;
		} else {
			errno = EINVAL;
		}

		if(errno) {
			fprintf(stderr, "invalid argument at %s\nusage: pw [-h] [-l] [-m] -lcg a c m -s seed domain \nmore with `pw help`\n", flag);
			return errno;
		}
		++i;
	}

	// check required args
	if (domain == NULL || *seed == '\0' || LCG_A == 0 || LCG_C == 0 || LCG_M == 0) {
		fprintf(stderr, "required argument invalid or not provided\n");
		return EINVAL;
	}
		
	char pw[pw_size+1];
	pw[pw_size] = '\0';
	calc_pw(seed, domain, pw, pw_size);
	if (errno) {
		fprintf(stderr, "password calculation cancelled, loop detected. Please try other lcg values\n");
		return errno;
	}
	puts(pw);
	return 0;		
}
