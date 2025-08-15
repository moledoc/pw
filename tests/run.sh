#!/bin/sh

filename="./inputs.txt"
contents=$(split -p '\n' $filename)

while IFS= read -r line; do
    echo "$line" | /usr/bin/sed -E 's/^([^ ]+)? ?([^ ]+)? ?([^ ]+)? ?([0-9]+)?$/\1|\2|\3|\4/' | while IFS='|' read -r salt pepper domain digest_len; do
        test -n "$salt" && salt="-s $salt"
        test -n "$pepper" && pepper="-p $pepper"
        test -n "$digest_len" && digest_len="-l $digest_len"
        pw_old=$(pw_old -k "test" $salt $pepper $digest_len $domain | tr -d '\n')
        test -n "$domain" && domain="-d $domain"
        pw_new=$(pw -k "test" $salt $pepper $digest_len $domain | tr -d '\n')
        test "$pw_old" = "$pw_new" && echo "$pw_new" || echo "diff:\n\tline: $line\n\told: $pw_old\n\tnew: $pw_new"
    done
done < $filename



