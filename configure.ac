AC_INIT([iii], [0.3], [iii-bugs@klever.net])
AC_CONFIG_SRCDIR([src/iiid.cc])
AC_CONFIG_AUX_DIR([aux.d])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_INSTALL
AC_PROG_CXX
AC_PROG_CC
PKG_PROG_PKG_CONFIG

AC_HEADER_STDC

AC_PATH_PROG([XSLTPROC],[xsltproc],[true])

PKG_CHECK_MODULES([MODULES],[gsoap++ libcrypto libconfuse])
PKG_CHECK_MODULES([UUID],[uuid],[have_uuid=true],[have_uuid=false])
AM_CONDITIONAL([HAVE_UUID],[$have_uuid])
if $have_uuid ; then
 AC_DEFINE([HAVE_LIBUUID],,[defined in presence of libuuid])
 AC_SUBST([UUID_UUID],[uuid])
fi

AC_PATH_PROG([SOAPCPP2],[soapcpp2],[false])
test "$SOAPCPP2" = "false" && AC_MSG_ERROR([no soapcpp2 tool, part of gsoap package, found.])

notfound=false
AC_CHECK_HEADERS([archive.h],[
 AC_CHECK_LIB([archive],[archive_read_new],,[notfound=true])
],[notfound=true])
$notfound && AC_MSG_ERROR([no required libarchive library found. get one from http://people.freebsd.org/~kientzle/libarchive/])

notfound=false
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([autosprintf.h],[
 AC_CHECK_LIB([asprintf],[main],,[notfound=true])
],[notfound=true])
$notfound && AC_MSG_ERROR([no autosprintf, part of gettext, found])
AC_LANG_POP([C++])

nitpick=false
AC_ARG_ENABLE([nitpicking],
 AC_HELP_STRING([--enable-nitpicking],[make compiler somewhat overly fastidious about the code it deals with]),
 [ test "$enableval" = "no" || nitpick=true ]
)
if $nitpick ; then
 CPP_NITPICK="-pedantic -Wall -Wextra -Wundef -Wshadow \
  -Wunsafe-loop-optimizations -Wconversion -Wmissing-format-attribute \
  -Wredundant-decls -ansi -Wlogical-op -Wmissing-noreturn"
 C_NITPICK="$CPP_NITPICK"
 CXX_NITPICK="$C_NITPICK"

 CPPFLAGS="$CPPFLAGS $CPP_NITPICK"
 CFLAGS="$CFLAGS $C_NITPICK"
 CXXFLAGS="$CXXFLAGS $CXX_NITPICK"
fi

ndebug=true
AC_ARG_ENABLE([debug],
 AC_HELP_STRING([--enable-debug],[enable debugging code]),
 [ test "$enableval" = "no" || ndebug=false ]
)
if $ndebug ; then
 CPPFLAGS_DEBUG="-DNDEBUG"
else
 CPPFLAGS_DEBUG="-DDEBUG"
fi
AC_SUBST([CPPFLAGS_DEBUG])

AC_CONFIG_FILES([
 Makefile
 src/Makefile
 doc/Makefile
  doc/iiid.8
])
AC_OUTPUT
