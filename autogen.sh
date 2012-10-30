#! /bin/sh
#

set -x
libtoolize --force --copy --automake
aclocal
autoheader
automake --force-missing --add-missing --copy
autoconf

