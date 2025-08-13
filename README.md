# pw

`pw` is a vaultless password manager written in C.

## GUI

### TODOs

- [ ] TODO: read vault.contents
- [ ] TODO: SDL2 gui to display vault contents in fzf-like manner
	- [ ] mouse select
	- [ ] arrow navigation
	- [ ] fuzzy finding (can be naive-ish)
		- [ ] IMPROVEMENT: keep map of lines that match
		- [ ] IMPROVEMENT: tree (trie maybe) struct for more efficiency
- [ ] TODO: asking master password
- [ ] TODO: calculate password
- [ ] TODO: store password in clipboard for x seconds, revert clipboard value (unless it changed in the meantime)

## CLI

### Synopsis

```text
pw [-h] [-s SALT] [-p PEPPER] -k KEY DOMAIN
```

### Options

```text
-h, -help, --help, help
	print help
-k KEY, --key KEY
	key file or literal (required)
-s SALT, --salt SALT
	salt the password - add something-something to the digestable message
-p PEPPER, --pepper PEPPER
	pepper the password - add something-something to the digested message
-l LENGTH, --length LENGTH
	how many digest elements are printed (default: 16, allowed: 1-16);
	NOTE: each digest element is 2 characters long
DOMAIN
	password domain (required)
```

### Description

`pw` is vaultless password manager as it doesn't store any password - it will calculate the password based on provided input.
Password is constructed in the following way: `md5(domain+key+salt)+pepper`, where `+` denotes string concatenation.

### Examples

```sh
* pw -h
* pw help
* pw -v
* pw version
* pw -k test example@example.com/exampleuser
* pw -k ./README.md example@example.com/exampleuser
* pw -k test -s test_test example@example.com/exampleuser
* pw -k test -p TEST_TEST example@example.com/exampleuser
* pw -k test -s test_test -p TEST_TEST example@example.com/exampleuser
* pw -k ./README.md -s test_test -p TEST_TEST example@example.com/exampleuser
```

## Author

Meelis Utt
