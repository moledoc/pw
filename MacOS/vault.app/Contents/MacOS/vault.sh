#!/bin/sh
SHELL=/bin/sh

read -s -p "Master key: " pw_key
echo
echo "$pw_key"

cd /Applications/vault.app/Contents/MacOS
contents=$(cat ./vault.contents)

domain=$(printf "$contents" | /usr/bin/sed -E 's/^([^ ]+) ([^ ]+) ([^ ]+)( ".*")?$/\3/' | /opt/homebrew/bin/fzf)
escaped_domain=$(printf '%s' "$domain" | /usr/bin/sed -E 's/[][+/$*.^|\\]/\\&/g')

line=$(printf "$contents" | /usr/bin/grep -E " $escaped_domain(\$| )")
# [ "$(echo "$line" | wc -l)" -ne 1 ] && { echo "conflict"; sleep 5; exit 0}

prev=$(pbpaste)
echo "$line" | /usr/bin/sed -E 's/^([^ ]+) ([^ ]+) ([^ ]+)( ".*")?$/\1|\2|\3|\4/' | while IFS='|' read -r salt pepper domain extra; do
	extra=$(echo $extra | sed 's/"//g')
	/usr/local/bin/pw -k "$pw_key" -s $salt -p $pepper $extra $domain | tr -d '\n' | pbcopy
done
(sleep 10; printf "$prev" | pbcopy) &
