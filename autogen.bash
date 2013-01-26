#!/bin/bash
( cd "$(dirname "$0")" \
&& mkdir -p aux.d \
&& aclocal \
&& autoheader \
&& automake -a \
&& autoconf \
&& "$(dirname "$0")"/configure "$@" \
)
