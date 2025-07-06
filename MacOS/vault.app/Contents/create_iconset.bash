#!/bin/bash

file="$1"
test -z "$file" && echo "empty filename" && exit 1

mkdir -p vault.iconset

sips -z 16 16 "$file" --out Vault.iconset/icon_16x16.png
sips -z 16 16 "$file" --out Vault.iconset/icon_16x16@2x.png

sips -z 32 32 "$file" --out Vault.iconset/icon_32x32.png
sips -z 32 32 "$file" --out Vault.iconset/icon_32x32@2x.png

sips -z 128 128 "$file" --out Vault.iconset/icon_128x128.png
sips -z 128 128 "$file" --out Vault.iconset/icon_128x128@2x.png

sips -z 256 256 "$file" --out Vault.iconset/icon_256x256.png
sips -z 256 256 "$file" --out Vault.iconset/icon_256x256@2x.png

sips -z 512 512 "$file" --out Vault.iconset/icon_512x512.png 
sips -z 512 512 "$file" --out Vault.iconset/icon_512x512@2x.png 

file vault.iconset/*.png
sips -g pixelWidth -g pixelHeight vault.iconset/*.png

iconutil -c icns vault.iconset
mv vault.icns Resources
