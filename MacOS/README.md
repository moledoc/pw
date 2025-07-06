# vault

Convenience MacOS app for `pw`.

## Install

Download `vault.app` into `/Applications`

## Dependencies

* [pw](https://github.com/moledoc/pw)
* fzf
* BSD `sed`/`grep` to exist in `/usr/bin`
* file named `vault.contents` in `/Applications/vault/Contents/MacOS` that contains salt, pepper, domain and extra flags in format:
	* `salt pepper domain ("extra args")`; **NOTE:** extra args are optional and surrounded by double-quotes (")

## Author

Meelis Utt
