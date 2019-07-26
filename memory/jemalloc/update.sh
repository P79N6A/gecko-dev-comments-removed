#!/bin/bash

set -e

cd `dirname $0`

source upstream.info

rm -rf src
git clone "$UPSTREAM_REPO" src
cd src
git checkout "$UPSTREAM_COMMIT"
autoconf
git describe --long --abbrev=40 > VERSION
rm -rf .git .gitignore autom4te.cache

patch -p1 < ../0001-Use-a-configure-test-to-detect-whether-to-use-a-cons.patch
patch -p1 < ../0002-Use-ULL-prefix-instead-of-LLU-for-unsigned-long-long.patch
patch -p1 < ../0003-Don-t-use-msvc_compat-s-C99-headers-with-MSVC-versio.patch

cd ..
hg addremove -q src

echo "jemalloc has now been updated.  Don't forget to run hg commit!"
