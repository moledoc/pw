# pw

`pw` is a vaultless password manager written in C.

## Synopsis

```text
pw [-h] [-s SALT] [-p PEPPER] -k KEY DOMAIN
```

## Options

```text
-h, -help, --help, help
	print help
-k KEY, --key KEY
	key file or literal (required)
-s SALT, --salt SALT
	salt the password - add something-somethig to the digestable message
-p PEPPER, --pepper PEPPER
	pepper the password - add something-something to the digested message
-l LENGTH, --length LENGTH
	how many digest elements are printed (default: 16, allowed: 1-16);
	NOTE: each digest element is 2 characters long
DOMAIN
	password domain (required)
```

## Description

`pw` is vaultless password manager as it doesn't store any password - it will calculate the password based on provided input.
Password is constructed in the following way: `md5(domain+key+salt)+pepper`, where `+` denotes string concatenation.

## Examples

```sh
* pw -h
* pw help
* pw -k test example@example.com/exampleuser
* pw -k ./README.md example@example.com/exampleuser
* pw -k test -s test_test example@example.com/exampleuser
* pw -k test -p TEST_TEST example@example.com/exampleuser
* pw -k test -s test_test -p TEST_TEST example@example.com/exampleuser
* pw -k ./README.md -s test_test -p TEST_TEST example@example.com/exampleuser
```

## Author

Meelis Utt