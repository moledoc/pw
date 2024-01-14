# pw

`pw` is a vaultless password manager written in C.

## Synopsis

```text
pw [-h] [-l LENGTH] [-m MODE] [-s SEED | -f SEEDFILE] DOMAIN
```

## Options

```text
	-h, --help, help     - print help
	-l LENGTH            - specify length of password (default=32)
	-m MODE              - specify password mode (default=0)
	-s SEED              - specify password seed; only the first -s/-f is recognized
	-f SEEDFILE          - specify password seedfile; only the first -s/-f is recognized
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