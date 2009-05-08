dnl $Id$
dnl config.m4 for extension gupnp

PHP_ARG_WITH(gupnp, for gupnp support,
[  --with-gupnp             Include gupnp support])

if test "$PHP_GUPNP" != "no"; then
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR_GUPNP="/include/gupnp-1.0/libgupnp/gupnp.h" 
  PACKAGE_GSSDP="gssdp-1.0"
  PACKAGE_LIBSOUP="libsoup-2.4"
  PACKAGE_GLIB="glib-2.0"

  if test -r $PHP_GUPNP/$SEARCH_FOR; then # path given as parameter
    GUPNP_DIR=$PHP_GUPNP
  else # search default path list
    AC_MSG_CHECKING([for gupnp files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR_GUPNP; then
        GUPNP_DIR=$i
        AC_MSG_RESULT($SEARCH_FOR_GUPNP found in $i)
      fi
    done
  fi
  
  if test -z "$GUPNP_DIR"; then
    AC_MSG_RESULT([$SEARCH_FOR_GUPNP not found])
    AC_MSG_ERROR([Please reinstall the gupnp distribution])
  fi
  
  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test -z "$PKG_CONFIG"; then
    AC_MSG_RESULT([pkg-config not found])
    AC_MSG_ERROR([Please reinstall the pkg-config distribution])
  fi
  
  AC_MSG_CHECKING([for $PACKAGE_GSSDP])
  if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists $PACKAGE_GSSDP; then
    GSSDP_INCS=`$PKG_CONFIG $PACKAGE_GSSDP --cflags`
    GSSDP_LIBS=`$PKG_CONFIG $PACKAGE_GSSDP --libs`
    PHP_EVAL_LIBLINE($GSSDP_LIBS, GSSDP_SHARED_LIBADD)
    PHP_EVAL_INCLINE($GSSDP_INCS)
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the $PACKAGE_GSSDP distribution])
  fi
  
  AC_MSG_CHECKING([for $PACKAGE_LIBSOUP])
  if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists $PACKAGE_LIBSOUP; then
    LIBSOUP_INCS=`$PKG_CONFIG $PACKAGE_LIBSOUP --cflags`
    LIBSOUP_LIBS=`$PKG_CONFIG $PACKAGE_LIBSOUP --libs`
    PHP_EVAL_LIBLINE($LIBSOUP_LIBS, LIBSOUP_SHARED_LIBADD)
    PHP_EVAL_INCLINE($LIBSOUP_INCS)
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the $PACKAGE_LIBSOUP distribution])
  fi
  
  AC_MSG_CHECKING([for $PACKAGE_GLIB])
  if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists $PACKAGE_GLIB; then
    GLIB_INCS=`$PKG_CONFIG $PACKAGE_GLIB --cflags`
    GLIB_LIBS=`$PKG_CONFIG $PACKAGE_GLIB --libs`
    PHP_EVAL_LIBLINE($GLIB_LIBS, GLIB_SHARED_LIBADD)
    PHP_EVAL_INCLINE($GLIB_INCS)
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the $PACKAGE_GLIB distribution])
  fi

  PHP_ADD_INCLUDE($GUPNP_DIR/include/gupnp-1.0/)

  LIBNAME=gupnp-1.0 
  LIBSYMBOL=gupnp_context_new

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $GUPNP_DIR/lib, GUPNP_SHARED_LIBADD)
    AC_DEFINE(HAVE_GUPNPLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong gupnp lib version or lib not found])
  ],[
    -L$GUPNP_DIR/lib -lm -ldl
  ])
  
  PHP_SUBST(GUPNP_SHARED_LIBADD)
  PHP_NEW_EXTENSION(gupnp, gupnp.c, $ext_shared)
fi
