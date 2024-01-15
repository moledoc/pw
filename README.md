# pw

`pw` is a vaultless password manager written in C.

## Synopsis

```text
pw [-h] [-l LENGTH] [-m MODE] [--lng-a LCG_A] [--lng-c LCG_C] [--lng-m LCG_M] [-s SEED | -f SEEDFILE] DOMAIN
```

## Options

```text
	-h, --help, help     - print this help
	-l LENGTH            - specify length of password (default=32)
	-m MODE              - specify password mode (default=0)
	-s SEED              - specify password seed; only the first -s/-f is recognized
	-f SEEDFILE          - specify password seedfile; only the first -s/-f is recognized
	--lng-a LCG_A        - specify LCG multiplier (default=127); 0<LCG_A<=1000
	--lng-c LCG_C        - specify LCG constant (default=5700); 0<LCG_C<=10000
	--lng-m LCG_M        - specify LCG modulus (default=58595); 95<LCG_M<=10000
	DOMAIN               - specify the domain of the password; eg 'github.com/moledoc'
```

## Modes

```text
	0 (MODE_SYM) - alphanumeric characters and symbols
	1 (MODE_NUM) - alphanumeric characters
	2 (MODE_UPP) - alphabet characters
	3 (MODE_LOW) - lowercase characters
	anything else defaults to MODE_SYM
```

## Author

Meelis Utt