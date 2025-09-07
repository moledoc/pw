# pw

`pw` is a vaultless password manager written in C.
There is a cli version `pwcli` as well as graphical version `pwgui`.

## CLI

`pwcli` calculates the password and writes it to stdout.

## Dependencies

* N/A

## Quick Start

```sh
make release && ./bin/pwcli -k test -d example@example.com
```

### Synopsis

```text
pwcli [-h] [-s SALT] [-p PEPPER] -k KEY -d DOMAIN
```

### Options

```text
-h, -help, --help, help
		print help
-v, -version, --version, version
		print version
-k KEY, --key KEY
		key file or literal (required)
-d DOMAIN, --domain DOMAIN
		password domain (required)
-s SALT, --salt SALT
		salt the password - add something-something to the digestable message
-p PEPPER, --pepper PEPPER
		pepper the password - add something-something to the digested message
-l PW_LENGTH, --length PW_LENGTH
		length of the password (default: 32+pepper_length);
		NOTE: if length is specified out of range [1, 32+pepper_length] then default password length is used.
		NOTE: if specified length is less than pepper_length, then only the first character of pepper is used.
```

### Description

`pw` is vaultless password manager as it doesn't store any password - it will calculate the password based on provided input.
Password is constructed in the following way: `md5(domain+key+salt)+pepper`, where `+` denotes string concatenation.

### Examples

```text
* pw -h
* pw help
* pwcli -k test -d example@example.com/exampleuser                                   -> 75cc68dab5da4189421a97ee647ea733
* pwcli -k ./README.md -d example@example.com/exampleuser                            -> ac3b20b32085c5eb7bbcff173c47abac
* pwcli -k test -s test_test -d example@example.com/exampleuser                      -> 07f8995ba12e9b40e9041b31d2b0dded
* pwcli -k test -p _TEST_TEST -d example@example.com/exampleuser                     -> 75cc68dab5da4189421a97ee647ea733_TEST_TEST
* pwcli -k test -s test_test -p _TEST_TEST -d example@example.com/exampleuser        -> 07f8995ba12e9b40e9041b31d2b0dded_TEST_TEST
* pwcli -k ./README.md -s test_test -p _TEST_TEST -d example@example.com/exampleuser -> 57f7842236a12ca35b691cd24b6beb8d_TEST_TEST
* pwcli -k test -s test_test -p _TEST_TEST -d example@example.com/exampleuser -l -1  -> 07f8995ba12e9b40e9041b31d2b0dded_TEST_TEST
* pwcli -k test -s test_test -p _TEST_TEST -d example@example.com/exampleuser -l 0   -> 07f8995ba12e9b40e9041b31d2b0dded_TEST_TEST
* pwcli -k test -s test_test -p _TEST_TEST -d example@example.com/exampleuser -l 32  -> 07f8995ba12e9b40e9041b_TEST_TEST
* pwcli -k test -s test_test -p _TEST_TEST -d example@example.com/exampleuser -l 41  -> 07f8995ba12e9b40e9041b31d2b0dde_TEST_TEST
* pwcli -k test -s test_test -p _TEST_TEST -d example@example.com/exampleuser -l 100 -> 07f8995ba12e9b40e9041b31d2b0dded_TEST_TEST
* pwcli -k test -s test_test -p @TEST_TEST -d example@example.com/exampleuser -l 7   -> 07f899@
```

## GUI

`pwgui` calculates the password and sets it into clipboard for certain amount of time.
Clipboard is reverted, if there is nothing new copied to clipboard.
If key file/literal is not provided as an argument, then master password is asked.
When master password is asked, then the following colors are used as background: white - empty master password, blue - non-zero master password, red - master password has filled the buffer, no further characters are stored.
Salt/pepper/domain/lengths from a space separated file.

## Dependencies

* required: SDL2 and SDL2_ttf
```sh
# mac
brew install sdl2 sdl2_ttf
# linux (debian based)
apt install libsdl2-dev libsdl2-ttf-dev
# windows
# TODO:
```

## Quick Start

```sh
make release && ./bin/pwgui -f ./tests/inputs.txt &
```

### Synopsis

```text
pwgui [-h] [-v] [-k KEY] -f VAULT_CONTENTS
```

### Options

```text
-h, -help, --help, help
		print help
-v, -version, --version, version
		print version
-k KEY, --key KEY
		key file or literal (optional)
		key is asked when not provided
-f VAULT_CONTENTS, --filename VAULT_CONTENTS
		vault contents (required)
		expected format (space separated list): salt pepper domain [length (1-32) default: 32]
```

### NOTEs

* VAULT_CONTENTS (salt pepper domain length) behave the same way as in `pwcli`
* if `pepper` and `length` are provided, then
	* if `0 < length < 32`, then `len(digested password+pepper) = length`.
	* if `length <= 0 || 32 < length`, then `len(digested password+pepper) = 32+len(pepper)`

## TODOs

- [ ] compile and test `pwgui` on windows
- [ ] MAYBE: write tests

## Author

Meelis Utt
