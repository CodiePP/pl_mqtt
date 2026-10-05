#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# 1. run 'aclocal'
# 2. run 'autoheader'
# 3. run 'autoconf'
# 4. run './configure'  (optionally with parameters)
# 5. run 'make swi'
#
# subsequently: 'autoreconf -vfi'

AC_PREREQ([2.69])
AC_INIT(plmqtt, 1.0.0)

AC_CONFIG_AUX_DIR([config])
#AC_CONFIG_SRCDIR([test/test.cpp])
AC_CONFIG_HEADERS([src/config.h])

# preset definitions
if test -z "$CFLAGS"; then
  CFLAGS="-g -O2 -fPIC -Wall"
fi

ARCH=$(uname -s)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for libraries.
AC_SUBST(ARCH)
AC_SUBST(SWI_CFLAGS)
AC_SUBST(SWI_LIBS)
AC_SUBST(SWI_BASE)
AC_SUBST(SWI_ARCH)
AC_SUBST(SWI_SOLIBDIR)
AC_SUBST(SWI_PLLIBDIR)
AC_SUBST(CJSON_CFLAGS)
AC_SUBST(CJSON_LIBS)
AC_SUBST(MOSQ_CFLAGS)
AC_SUBST(MOSQ_LIBS)
AC_SUBST(GI_CFLAGS)
AC_SUBST(GI_LIBS)
AC_SUBST(GI_DATADIR)
AC_SUBST(SOTARGET)
AC_SUBST(PLTARGET)

PKG_PROG_PKG_CONFIG()

if [[ $ARCH = "Darwin" ]]; then
  # you likely need to adjust these paths:
  export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${HOME}/.local/share/pkgconfig:/usr/local/opt/openssl:/opt/homebrew/opt/openssl/lib/pkgconfig:/opt/homebrew/opt/mosquitto/lib/pkgconfig:/opt/homebrew/opt/cjson/lib/pkgconfig:/usr/local/opt/mosquitto/lib/pkgconfig:/usr/local/opt/cjson/lib/pkgconfig"
fi

PKG_CHECK_MODULES([SWI], [swipl libmosquitto])
PKG_CHECK_MODULES([MOSQ], [libmosquitto])

# mosquitto >= 2.1 installs headers that include <cjson/cJSON.h>, so cJSON is
# required. libcjson.pc points at <includedir>/cjson, but the headers are
# included as <cjson/cJSON.h>, hence we add <includedir> itself.
PKG_CHECK_MODULES([CJSON], [libcjson], [],
  [AC_MSG_ERROR([cJSON (libcjson) is required by mosquitto's headers: install it, e.g. 'brew install cjson' or 'apt install libcjson-dev'])])
CJSON_INCDIR=$($PKG_CONFIG --variable=includedir libcjson)
CJSON_CFLAGS="-I$CJSON_INCDIR $CJSON_CFLAGS"
SWI_CFLAGS="$SWI_CFLAGS $CJSON_CFLAGS"
MOSQ_CFLAGS="$MOSQ_CFLAGS $CJSON_CFLAGS"

# verify that the mosquitto header can be used (with cJSON where needed)
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $MOSQ_CFLAGS"
AC_CHECK_HEADER([mosquitto.h], [],
  [AC_MSG_ERROR([cannot use mosquitto.h; mosquitto >= 2.1 also needs cJSON (libcjson with pkg-config, e.g. 'brew install cjson' or 'apt install libcjson-dev')])])
CPPFLAGS="$save_CPPFLAGS"

SWI_BASE=$($PKG_CONFIG --variable=PLBASE swipl)
SWI_ARCH=$($PKG_CONFIG --variable=PLARCH swipl)
SWI_SOLIBDIR=$SWI_BASE/lib/$SWI_ARCH
SWI_PLLIBDIR=$SWI_BASE/library

# Checks for header files.
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([pthread.h], [], [AC_MSG_ERROR([pthread.h is required])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_SIZEOF(long)

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strdup])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

