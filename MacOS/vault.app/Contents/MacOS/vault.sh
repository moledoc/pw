#!/bin/sh
SHELL=/bin/sh

read -s -p "Master key: " pw_key
echo
echo

cd /Applications/vault.app/Contents/MacOS
contents=$(cat ./vault.contents)

domain=$(printf "$contents" | /usr/bin/sed -E 's/^([^ ]+)? ?([^ ]+)? ?([^ ]+)? ?([0-9]+)?$/\3/' | /opt/homebrew/bin/fzf)
escaped_domain=$(printf '%s' "$domain" | /usr/bin/sed -E 's/[][+/$*.^|\\]/\\&/g')

line=$(printf "$contents" | /usr/bin/grep -E " $escaped_domain(\$| )")

prev=$(pbpaste)
echo "$line" | /usr/bin/sed -E 's/^([^ ]+)? ?([^ ]+)? ?([^ ]+)? ?([0-9]+)?$/\1|\2|\3|\4/' | while IFS='|' read -r salt pepper domain digest_len; do
    test -n "$salt" && salt="-s $salt"
	test -n "$pepper" && pepper="-p $pepper"
	test -n "$domain" && domain="-d $domain"
	test -n "$digest_len" && digest_len="-l $digest_len"
	/usr/local/bin/pw -k $pw_key $salt $pepper $digest_len $domain | tr -d '\n' | pbcopy
done
(sleep 10; printf "$prev" | pbcopy) &
