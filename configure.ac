
# configure.ac script for md5deep/hashdeep family of programs.
#

AC_PREREQ(2.57)
AC_INIT([MD5DEEP],[4.0.0_alpha_007],[research@jessekornblum.com])
AC_CONFIG_FILES([Makefile src/Makefile man/Makefile tests/Makefile tests/testfiles/Makefile ])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])
AC_DEFINE([_FILE_OFFSET_BITS],64,[Make sure we are using 64-bit offsets])

################################################################
### I am a glutten for punishment and this is security-critical software
CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2 -Wall -g"
CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 -Wall -g"

AC_GNU_SOURCE
AC_CANONICAL_HOST

# This magic is from PhotoRec; see which options we can use. 
# These I don't like:
# -Wdeclaration-after-statement -Wconversion
# doesn't work: -Wunreachable-code 
# causes configure to crash on gcc-4.2.1: -Wsign-compare-Winline 
# causes warnings with unistd.h:  -Wnested-externs 
for option in -Wall -MD -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wshadow -Wwrite-strings -W -Wcast-align -Waggregate-return -Wbad-function-cast -Wcast-qual -Wundef -Wredundant-decls -Wdisabled-optimization -Wfloat-equal -Wmissing-format-attribute -Wmultichar -Wc++-compat -Wmissing-noreturn -fast
do
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $option"
  AC_MSG_CHECKING([whether gcc understands $option])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
      [has_option=yes],
      [has_option=no; CFLAGS="$SAVE_CFLAGS"])
  AC_MSG_RESULT([has_option=$has_option; CFLAGS=$CFLAGS])
  unset has_option
  unset SAVE_CFLAGS
done
unset option
# end photorec magic



# http://osdir.com/ml/gnu.mingw.devel/2003-09/msg00040.html
# Note: Windows 95 WINVER=0x400
# Windows 98 WINVER=0x400  _WIN32_WINDOWS=0x0410
# Windows Me WINVER=0x400  _WIN32_WINDOWS=0x0490
# Windows NT 4.0 WINVER=0x0400  _WIN32_WINNT=0x0400
# Windows NT 4.0 SP3 WINVER=0x0400 _WIN32_WINNT=0x0403
# Windows 2000 WINVER=0x500 _WIN32_WINNT=0x0500
# Windows XP WINVER=0x501 _WIN32_WINNT=0x0501
# Windows Server 2003 WINVER=0x502 _WIN32_WINNT=0x0502

# mingw32 includes  i686-w64-mingw32 and  x86_64-w64-mingw32

mingw32="no"
case $host in
  *-*-*linux*-*) 
     AC_DEFINE([__LINUX__],1,[Linux operating system functions])
     ;;

  *-*-mingw32*)   
     LIBS="-lws2_32 -lgdi32 $LIBS"  # previously had -liberty 
     CPPFLAGS="-DUNICODE -D_UNICODE -D__MSVCRT_VERSION__=0x0601 -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -DHAVE_STRUCT_TIMESPEC $CPPFLAGS"
     CXXFLAGS="$CXXFLAGS -Wno-format "  # compiler mingw-4.3.0 is broken on I64u formats
     CXXFLAGS="$CXXFLAGS --static"
     mingw="yes"
     ;;		 		     

esac

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL


# Bring additional directories where things might be found into our
# search path. I don't know why autoconf doesn't do this by default
for spfx in /usr/local /opt/local /sw ; do
    echo checking ${spfx}/include
    if test -d ${spfx}/include; then
        CFLAGS="-I${spfx}/include $CFLAGS"
        CPPFLAGS="-I${spfx}/include $CPPFLAGS"
        LDFLAGS="-L${spfx}/lib $LDFLAGS"
    fi
done

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([libgen.h fcntl.h limits.h inttypes.h malloc.h stdint.h stdlib.h string.h \
			   sys/cdefs.h sys/types.h sys/ioctl.h sys/mmap.h sys/mman.h sys/param.h \
			   sys/resource.h wchar.h unistd.h sys/stat.h sys/disk.h\
			   CommonCrypto/CommonDigest.h
			   ])

# These functions not available everywhere
AC_CHECK_FUNCS([_gmtime64_s _gmtime64 gmtime_r mmap usleep mkstemp vasprintf getrusage getprogname])

# This is for Apple's new CommonCrypto (which is FIPS validated)
AC_CHECK_FUNCS([CC_MD5_Init CC_SHA1_Init CC_SHA256_Init])

# These includes are required on FreeBSD
AC_CHECK_HEADERS([sys/mount.h],[],[],
[#ifdef HAVE_SYS_TYPES_H
 # include <sys/types.h>
 #endif
 #ifdef HAVE_SYS_PARAM_H                                                     
 # include <sys/param.h>                           
 #endif])                    

# http://www.gnu.org/s/hello/manual/autoconf/Particular-Headers.html
#
# If a program may include both time.h and sys/time.h, define
# TIME_WITH_SYS_TIME. On some ancient systems, sys/time.h included
# time.h, but time.h was not protected against multiple inclusion, so
# programs could not explicitly include both files. This macro is
# useful in programs that use, for example, struct timeval as well as
# struct tm. It is best used in conjunction with HAVE_SYS_TIME_H,
# which can be checked for using AC_CHECK_HEADERS([sys/time.h]).

AC_CHECK_HEADERS([time.h sys/time.h])
AC_HEADER_TIME


# Don't think we need this anymore.
# AC_C_BIGENDIAN

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FSEEKO
AC_SYS_LARGEFILE
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_VPRINTF

# More permutations of fseek/ftell lossage
AC_CHECK_FUNCS([fseeko64 ftello64])

# See if program_invocation_name is defined on this system
AC_TRY_COMPILE([#include <errno.h>],
			[const char *progname = program_invocation_name;],
			extern_program_invocation_name=yes,
			extern_program_invocation_name=no)
if test x"${extern_program_invocation_name}" = x"yes"; then
  AC_MSG_NOTICE([extern program_invocation_name in errno.h])
  AC_DEFINE(HAVE_PROGRAM_INVOCATION_NAME,1,[define to 1 if program_invocation_name is available])
fi




################################################################
# PTHREAD support
# With special nods to compiling under mingw

if test  x"$mingw" = x"yes";  then
  AC_DEFINE([HAVE_STRUCT_TIMESPEC],1,[Required for mingw])
  CFLAGS="$CFLAGS -mthreads "
  CPPFLAGS="-DPTW32_STATIC_LIB $CPPFLAGS"
  CXXFLAGS="$CXXFLAGS -mthreads "
fi

AC_CHECK_HEADERS([pthread.h])
m4_include([ax_pthread.m4])
AX_PTHREAD()

AC_CHECK_LIB([pthreadGC2],[pthread_create])

if test x"$pthread_done" != x"yes" ; then
  # And finally, run the m4 script
  echo Using PTHREAD output
  LIBS="$PTHREAD_LIBS $LIBS"
  CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
  CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS "
  CC="$PTHREAD_CC"
fi

AC_DEFINE([INCLUDE_NP],1,[Include non-portable functions for statically linking pthreads in mingw])

LIBS="$LIBS $PTHREAD_LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CPPFLAGS="$CPPFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# On mingw, be sure to use the static version and be sure we are using mthread option
# (which should be a no-op on later version of G++ anyway)

AC_CHECK_FUNCS([pthread_win32_process_attach_np pthread_win32_process_detach_np pthread_win32_thread_attach_np pthread_win32_thread_detach_np])
						


################################################################

# This allows us to easily disable optimizations for debugging
AC_ARG_WITH([noopt], AC_HELP_STRING([--with-noopt],[Drop -O C flags]))
if test x"${AFF_NOOPT}" != "x" ; then
   with_noopt="yes";
fi

if test "${with_noopt}" = "yes" ; then
  AC_MSG_NOTICE([XXXXXXXX DROPPING OPTIMIZATION FLAGS XXXXXXXX])
  CFLAGS=`echo "$CFLAGS" | sed s/-O[[0-9]]// | sed s/-fast//`             # note the double quoting! 
  CXXFLAGS=`echo "$CXXFLAGS" | sed s/-O[[0-9]]// | sed s/-fast//`
fi

AC_OUTPUT
