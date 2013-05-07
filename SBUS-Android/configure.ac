AC_INIT([sbus], [0.85], [dmi1000@cl.cam.ac.uk])

dnl Autoconf >= 2.61 has AC_COMPUTE_INT built-in.
dnl Remove this when we can assume autoconf >= 2.61.
m4_ifdef([AC_COMPUTE_INT], [], [
  AC_DEFUN([AC_COMPUTE_INT], [_AC_COMPUTE_INT([$2],[$1],[$3],[$4])])
])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC gl_EARLY
AC_PROG_CXX
AC_PROG_RANLIB
AC_CHECK_PROG([have_latex], [latex], [true])
AC_CHECK_PROG([have_javac], [javac], [true])
gl_INIT
AC_CHECK_LIB([nsl], [gethostbyname])
AC_CHECK_LIB([socket], [connect])
AC_SEARCH_LIBS([sqlite3_open], [sqlite3], [have_sqlite_l=true], [have_sqlite_l=false])
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_CHECK_HEADER([sqlite3.h], [have_sqlite_h=true], [have_sqlite_h=false])
AC_CHECK_HEADER([pthread.h])
AC_TYPE_SSIZE_T
AC_CHECK_FUNCS([getline], [], [use_gnulib=true])
AC_CHECK_FUNCS([getifaddrs], [], [need_getifaddrs=true])
AC_CONFIG_HEADERS([config.h])
AC_ARG_WITH([electric-fence],
	    [AS_HELP_STRING([--with-electric-fence], [use Electric Fence])],
	    [use_electric_fence=true],
	    [])
AM_CONDITIONAL([HAVE_JAVAC], [test x$have_javac = xtrue])
AM_CONDITIONAL([HAVE_LATEX], [test x$have_latex = xtrue])
AM_CONDITIONAL([USE_GNULIB], [test x$use_gnulib = xtrue])
AM_CONDITIONAL([USE_COMPAT], [test x$need_getifaddrs = xtrue])
AM_CONDITIONAL([USE_EFENCE], [test x$use_electric_fence = xtrue])
AM_CONDITIONAL([NEED_GETIFADDRS], [test x$need_getifaddrs = xtrue])
AM_CONDITIONAL([USE_SQLITE], [test x$have_sqlite_l = xtrue -a x$have_sqlite_h = xtrue])
#AS_IF([test $USE_EFENCE],
      #[AC_CHECK_LIB([efence], [malloc], [], [AC_MSG_ERROR([You asked for Electric Fence but I can't find it.])])], [])
AS_IF([test $USE_COMPAT],
	[AC_SUBST(COMPATLDADD, [../compat/libsbus_compat.a])],
	[AC_SUBST(COMPATLDADD, [ ])] )
AC_CONFIG_FILES([
	Makefile
	gl/Makefile
	library/Makefile
	compat/Makefile
	wrapper/Makefile
	tools/Makefile
	utils/Makefile
	idl/Makefile
])
AS_IF([test x$have_latex = xtrue],
      [AC_CONFIG_FILES([java/Makefile java/tools/Makefile])],
      [AC_MSG_NOTICE([Note that as javac can't be found, the Java bindings and componentswon't be built.]) ] )
AS_IF([test x$have_latex = xtrue],
      [AC_CONFIG_FILES([docs/Makefile])],
      [AC_MSG_NOTICE([Note that as LaTeX can't be found, the documentation won't be built.]) ] )
AC_OUTPUT
AS_IF([test x$have_sqlite_l = xfalse -o x$have_sqlite_h = xfalse],
	[AC_MSG_NOTICE([Note that as sqlite can't be found, the storage component spersist won't be built.]) ] )
