# vault

Convenience MacOS app for `pw`.

## Install

Download `vault.app` into `/Applications`

## Dependencies

* [pw](https://github.com/moledoc/pw)
* fzf
* BSD `sed`/`grep` to exist in `/usr/bin`
* file named `vault.contents` in `/Applications/vault/Contents/MacOS` that contains salt, pepper, domain and optionally digest length in format:
	* `salt pepper domain (digest_length)`

## Author

Meelis Utt
