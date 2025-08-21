#!/bin/sh

filename="./inputs.txt"
contents=$(split -p '\n' $filename)

while IFS= read -r line; do
    echo "$line" | /usr/bin/sed -E 's/^([^ ]+)? ?([^ ]+)? ?([^ ]+)? ?([0-9]+)?$/\1|\2|\3|\4/' | while IFS='|' read -r salt pepper domain length; do
        test -n "$salt" && salt="-s $salt"
        test -n "$pepper" && pepper="-p $pepper"
        test -n "$length" && length="-l $length"
        test -n "$domin" && domin="-d $domin"
        # pw_old=$(pw -k "test" $salt $pepper $length $domain | tr -d '\n')
        # pw_new=$(../bin/pwcli -k "test" $salt $pepper $length $domain | tr -d '\n')
        # test "$pw_old" = "$pw_new" && echo "$pw_new" || echo "diff:\n\tline: $line\n\told: $pw_old\n\tnew: $pw_new"
    done
done < $filename



