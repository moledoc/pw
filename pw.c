#include <stdio.h>
#include <errno.h>
#include <limits.h>

#define var_name(var) #var

// ascii char range to use 32-126: (0-94)+32
#define OFFSET 32
#define MOD 95

#define LCG_M_MAX 50000
#define SEED_SIZE_MAX 4096
#define PW_SIZE_MIN 12
#define PW_SIZE_MAX 128

#define PW_SIZE 32

enum mode {
	MODE_SYM,
	MODE_NUM,
	MODE_UPP,
	MODE_LOW
};

int MODE = MODE_SYM;
int LCG_A = 0;
int LCG_B = 0;
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
	if (src_len > SEED_SIZE_MAX) {
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
	
	return (LCG_A*seed+LCG_B)%LCG_M;
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
	int nseed = gen_seedling(seed, seed_len);

	int failsafe = 0;
	size_t i = 0;
	for (;i<pw_size;) {
		if (failsafe >= 1000) {
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

char *validate_lcg(int a, int c, int m) {
	if (!(0 < m && m <= LCG_M_MAX)) {
		errno = EINVAL;
		return ", invalid modulo";
	}
	if (!(0 < a && a < LCG_M)) {
		errno = EINVAL;
		return ", invalid multiplier";
	}
	if (!(0 <= c && c < LCG_M)) {
		errno = EINVAL;
		return ", invalid constant";
	}
	return "";
}

void help() {
	printf("NAME\n\tpw - vaultless password manager\n");
	printf("\nSYNOPSIS\n\tpw [-h] [-l LENGTH] [-m MODE] -lcg A B M -s SEED DOMAIN\n");
	printf("\nOPTIONS\n");
	printf("\t%s\n\t\tprint help\n", "-h, -help, --help, help");
	printf("\t%s\n\t\tpassword domain, i.e. for what; eg 'github.com/moledoc'\n", "DOMAIN");
	printf("\t%s\n\t\tpassword seed\n", "-s SEED");
	printf("\t\t - if points to file, seed is file content\n");
	printf("\t\t - else is seed\n");
	printf("\t\tmax length of seed is %s(%d)\n", var_name(SEED_SIZE_MAX), SEED_SIZE_MAX);
	printf("\t%s\n\t\tLCG modifiers\n", "-lcg A B M");
	printf("\t\t - M (modulo), 0 < M <= %s(%d)\n", var_name(LCG_M_MAX), LCG_M_MAX);
	printf("\t\t - A (multiplier), 0 < A < M\n");
	printf("\t\t - B (constant), 0 <= B < M\n");
	printf("\t\tall modifiers are equal, some are just more equal\n");
	printf("\t%s\n\t\tlength of password; %s(%d) <= LENGTH <= %s(%d) (default=%d)\n", "-l LENGTH", var_name(PW_SIZE_MIN), PW_SIZE_MIN, var_name(PW_SIZE_MAX), PW_SIZE_MAX, PW_SIZE);
	printf("\t%s\n\t\tpassword mode (default=%d)\n", "-m MODE", MODE_SYM);
	printf("\t\t %d (%s) - alphanumeric characters and symbols\n", MODE_SYM, var_name(MODE_SYM));
	printf("\t\t %d (%s) - alphanumeric characters\n", MODE_NUM, var_name(MODE_NUM));
	printf("\t\t %d (%s) - alphabet characters\n", MODE_UPP, var_name(MODE_UPP));
	printf("\t\t %d (%s) - lowercase characters\n", MODE_LOW, var_name(MODE_LOW));
	printf("\t\tanything else defaults to %s\n", var_name(MODE_SYM));
	printf("\nEXAMPLES\n");
	printf("\t./pw -s test -lcg 65 678 12345 example@example.com/exampleuser\n");
	printf("\t./pw -s ./README.md -lcg 122 12 300 example@example.com/exampleuser\n");
	printf("\t./pw -m 1 -s seed -lcg 12 0 345 example@example.com/exampleuser\n");
	printf("\t./pw -l 16 -m 2 -s test-test -lcg 99 86 100 example@example.com/exampleuser # not good modifiers with this seed, non-terminating loop detected\n");
	printf("\t./pw -l 24 -s test-test-test -lcg 255 1000 25000 example@example.com/exampleuser\n");
	printf("\nAUTHOR\n");
	printf("\tMeelis Utt (meelis.utt@gmail.com)\n");
}

int main(int argc, char **argv) {
	char seed[SEED_SIZE_MAX];
	zero(seed, SEED_SIZE_MAX);
	char *domain = 0;
	size_t pw_size = PW_SIZE;

	// parse args
	for (int i=1; i<argc; ++i) {
		char *flag = argv[i];
		char hint[128];
		if (eq("-h", flag)|| eq("help", flag) || eq("-help", flag) || eq("--help", flag)) {
			help();
			return 0;
		} else if (eq("-s", flag) && i+1 < argc && *seed == '\0') {
			FILE *fptr = fopen(argv[i+1], "r");
			if (fptr) {			
				fseek(fptr, 0, SEEK_END);
				long sf_size = ftell(fptr);
				rewind(fptr);
				if (sf_size > SEED_SIZE_MAX) {
					snprintf(hint, 128, ", file content(%d) > %s(%d)", sf_size, var_name(SEED_SIZE_MAX), SEED_SIZE_MAX);
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
			if (!(PW_SIZE_MIN <= pw_size && pw_size <= PW_SIZE_MAX)) {
				snprintf(hint, 128, ", invalid length(%d)", pw_size);
				errno = EINVAL;
			}
		} else if (eq("-m", flag) && i+1 < argc) {
			MODE = a_i(argv[i+1]);
		} else if (eq("-lcg", flag) && i+3 < argc) {
			LCG_A = a_i(argv[i+1]);
			LCG_B = a_i(argv[i+2]);
			LCG_M = a_i(argv[i+3]);
			snprintf(hint, 128, validate_lcg(LCG_A, LCG_B, LCG_M));
			i+=2;
		} else if (i == argc - 1) {
			domain = flag;
		} else {
			errno = EINVAL;
		}

		if(errno) {
			fprintf(stderr, "invalid argument at %s%s\nusage: pw [-h] [-l length] [-m mode] -lcg a b m -s seed domain \nmore with `pw help`\n", flag, hint);
			return errno;
		}
		++i;
	}

	// check required args
	if (domain == NULL || *seed == '\0' || LCG_A == 0 || LCG_M == 0) {
		char hint[20];
		int hint_offset = 0;
		if (domain == NULL) {
			cpy(hint, "domain");
			hint_offset = slen("domain");
		}
		if (*seed == '\0') {
			if (hint_offset) {
				cpy(hint+hint_offset, ", ");
				hint_offset += slen(", ");
			}
			cpy(hint+hint_offset, "seed");
			hint_offset += slen("seed");
		}
		if (LCG_A == 0 || LCG_M == 0) {
			if (hint_offset) {
				cpy(hint+hint_offset, ", ");
				hint_offset += slen(", ");
			}
			cpy(hint+hint_offset, "lcg");
			hint_offset += slen("lcg");
		}
		fprintf(stderr, "missing at least 1 required argument: %s\nusage: pw [-h] [-l length] [-m mode] -lcg a b m -s seed domain \nmore with `pw help`\n", hint);
		return EINVAL;
	}

	// calc pw
	char pw[pw_size+1];
	pw[pw_size] = '\0';
	calc_pw(seed, domain, pw, pw_size);
	if (errno) {
		fprintf(stderr, "password calculation cancelled, non-terminating loop detected. Please try other lcg modifiers\n");
		return errno;
	}
	puts(pw);
	return 0;		
}
