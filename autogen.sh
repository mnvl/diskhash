#!/bin/sh -ex
libtoolize --automake
aclocal
autoconf
autoheader
automake --foreign --add-missing
./configure

