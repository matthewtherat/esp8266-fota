#! /usr/bin/bash

HERE=`dirname "$(readlink -f "$BASH_SOURCE")"`
OUT="${HERE}/public/index.bundle.html"
SDIR="${HERE}/public"

cd $HERE && npm run build

truncate -s0 $OUT

echo -n '<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">' >> $OUT
echo -n '<link rel="icon" type="image/png" ' >> $OUT
echo -n 'href="data:image/png;base64,' >> $OUT
base64 -w0 $SDIR/favicon-32x32.png >> $OUT
echo -n '"><style>' >> $OUT
cat $SDIR/build/bundle.css >> $OUT
echo -n '</style></head><body></body><script>' >> $OUT
head -n1 $SDIR/build/bundle.js | tr -d '\n' >> $OUT
echo -n "</script></html>" >> $OUT

