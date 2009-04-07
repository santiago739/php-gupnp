dnl $Id$
dnl config.m4 for extension gupnp

PHP_ARG_WITH(gupnp, for gupnp support,
[  --with-gupnp             Include gupnp support])

if test "$PHP_GUPNP" != "no"; then
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR_GUPNP="/include/gupnp-1.0/libgupnp/gupnp.h" 
  SEARCH_FOR_GSSDP="/include/gssdp-1.0/"
  SEARCH_FOR_LIBSOUP="/include/libsoup-2.4/"
  SEARCH_FOR_GLIB="/include/glib-2.0/"
  SEARCH_FOR_GLIB_LIB="/lib/glib-2.0/include/"

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
  
  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR_GSSDP; then
      GSSDP_DIR=$i
      AC_MSG_RESULT($SEARCH_FOR_GSSDP found in $i)
    fi
  done

  if test -z "$GSSDP_DIR"; then
    AC_MSG_RESULT([$SEARCH_FOR_GSSDP not found])
    AC_MSG_ERROR([Please reinstall the gssdp distribution])
  fi

  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR_LIBSOUP; then
      LIBSOUP_DIR=$i
      AC_MSG_RESULT($SEARCH_FOR_LIBSOUP found in $i)
    fi
  done

  if test -z "$LIBSOUP_DIR"; then
    AC_MSG_RESULT([$SEARCH_FOR_LIBSOUP not found])
    AC_MSG_ERROR([Please reinstall the libsoup distribution])
  fi

  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR_GLIB; then
      GLIB_DIR=$i
      AC_MSG_RESULT($SEARCH_FOR_GLIB found in $i)
    fi
  done

  if test -z "$GLIB_DIR"; then
    AC_MSG_RESULT([$SEARCH_FOR_GLIB not found])
    AC_MSG_ERROR([Please reinstall the glib distribution])
  fi

  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR_GLIB_LIB; then
      GLIB_LIB_DIR=$i
      AC_MSG_RESULT($SEARCH_FOR_GLIB_LIB found in $i)
    fi
  done

  if test -z "$GLIB_LIB_DIR"; then
    AC_MSG_RESULT([$SEARCH_FOR_GLIB_LIB not found])
    AC_MSG_ERROR([Please reinstall the glib distribution])
  fi

  PHP_ADD_INCLUDE($GUPNP_DIR/include/gupnp-1.0/)
  PHP_ADD_INCLUDE($GSSDP_DIR/include/gssdp-1.0/)
  PHP_ADD_INCLUDE($LIBSOUP_DIR/include/libsoup-2.4/)
  PHP_ADD_INCLUDE($GLIB_DIR/include/glib-2.0/)
  PHP_ADD_INCLUDE($GLIB_LIB_DIR/lib/glib-2.0/include/)

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
