#!/bin/sh
WANT_AUTOMAKE=1.8
export WANT_AUTOMAKE
   aclocal \
&& autoheader \
&& automake -a \
&& autoconf \
&& ./configure "$@"
