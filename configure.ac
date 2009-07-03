dnl Process this file with autoconf to produce a configure script.
AC_INIT([EM8300], [0.17.3-hg])
AC_CONFIG_SRCDIR([em8300setup/em8300setup.c])
AC_PREREQ(2.50)
AC_CONFIG_AUX_DIR(autotools)

AC_PROG_CC

AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
AC_CONFIG_HEADERS([include/config.h])

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_INSTALL

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL

AC_SUBST(DEBUG_CFLAGS)
AC_SUBST(GLOBAL_CFLAGS)

dnl Common cflags for all platforms
COMMON_CFLAGS="-Wall -D_FILE_OFFSET_BITS=64"

GLOBAL_CFLAGS="$GLOBAL_CFLAGS $CFLAGS $COMMON_CFLAGS"
DEBUG_CFLAGS="$GLOBAL_CFLAGS $CFLAGS $COMMON_CFLAGS -DDEBUG"

AM_PATH_GTK_2_0(2.0.0, has_gtk=yes, [has_gtk=no
	AC_MSG_RESULT(*** GTK not found ***)])

if test x$has_gtk = xyes; then
    AC_DEFINE(HAVE_GTK, 1, [Defined if you have GTK])
fi
AM_CONDITIONAL(HAVE_GTK, test x$has_gtk = "xyes")

FIRMWARE_DIR=/lib/firmware
AC_ARG_WITH(firmware-dir,
  [AS_HELP_STRING([--with-firmware-dir=DIR],[firmware directory [/lib/firmware]])], 
  if test "x$withval" != xyes; then FIRMWARE_DIR="$withval"; fi)
AC_SUBST(FIRMWARE_DIR)
AC_DEFINE_UNQUOTED(FIRMWARE_DIR, "$FIRMWARE_DIR", [Define the default path for the firmware.])

AC_ARG_WITH(alsa-dir,
  [AS_HELP_STRING([--with-alsa-dir=DIR],[ALSA configuration directory [searched]])], 
  [],
  [with_alsa_dir=search])

ALSA_DIR=
AS_IF([test "x$with_alsa_dir" != xno],
  AS_IF([test "x$with_alsa_dir" = xyes || test "x$with_alsa_dir" = xsearch],
    [AC_MSG_CHECKING([for alsa configuration directory])
     for dir in /usr/local/share/alsa /usr/share/alsa /etc/alsa; do
       if test -f "${dir}/alsa.conf"; then
         ALSA_DIR="$dir"
	 break
       fi
     done
     if test "x$ALSA_DIR" = x; then
       AC_MSG_RESULT([not found])
     else
       AC_MSG_RESULT([$ALSA_DIR])
     fi
     if test "x$ALSA_DIR" = x && test "x$with_alsa_dir" = xyes; then
       AC_MSG_FAILURE(
         [--with-alsa-dir was given but configuration directory was not found])
     fi],
    [ALSA_DIR="$with_alsa_dir"]))
AC_SUBST(ALSA_DIR)

AC_CONFIG_FILES([
	Makefile
	include/Makefile
	include/linux/Makefile
	overlay/Makefile
	dhc/Makefile
	em8300setup/Makefile
	scripts/Makefile
])
AC_OUTPUT
