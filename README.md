# pw

`pw` is a vaultless password manager written in C.

## Synopsis

```text
pw [-h] [-l LENGTH] [-m MODE] -lcg A B M -s SEED DOMAIN
```

## Options

```text
-h, -help, --help, help
	print help
DOMAIN
	password domain, i.e. for what; eg 'github.com/moledoc'
-s SEED
	password seed
	 - if points to file, seed is file content
	 - else is seed
	max length of seed is SEED_SIZE_MAX(4096)
-lcg A B M
	LCG modifiers
	 - M (modulo), 0 < M <= LCG_M_MAX(50000)
	 - A (multiplier), 0 < A < M
	 - B (constant), 0 <= B < M
	all modifiers are equal, some are just more equal
-l LENGTH
	length of password; PW_SIZE_MIN(12) <= LENGTH <= PW_SIZE_MAX(128) (default=32)
-m MODE
	password mode (default=0)
	 0 (MODE_SYM) - alphanumeric characters and symbols
	 1 (MODE_NUM) - alphanumeric characters
	 2 (MODE_UPP) - alphabet characters
	 3 (MODE_LOW) - lowercase characters
	anything else defaults to MODE_SYM
```

## Examples

```sh
./pw -s test -lcg 65 678 12345 example@example.com/exampleuser
./pw -s ./README.md -lcg 122 12 300 example@example.com/exampleuser
./pw -m 1 -s seed -lcg 12 0 345 example@example.com/exampleuser
./pw -l 16 -m 2 -s test-test -lcg 99 86 100 example@example.com/exampleuser # not good modifiers with this seed, non-terminating loop detected
./pw -l 24 -s test-test-test -lcg 255 1000 25000 example@example.com/exampleuser
```

## Author

Meelis Utt