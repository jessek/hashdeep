AC_PREREQ(2.57)

AC_INIT([MD5DEEP],[3.1],[md5deep@jessekornblum.com])
AM_INIT_AUTOMAKE
#AM_MAINTAINER_MODE

AC_GNU_SOURCE

AC_CANONICAL_HOST

case $host in
  *-*-*linux*-*) AC_DEFINE([__LINUX__],1,[Linux operating system functions]) ;;
  *-*-mingw32)   LIBS="-liberty $LIBS" && CPPFLAGS="-DUNICODE -D_UNICODE $CPPFLAGS"
esac

AC_PROG_CC
AC_PROG_INSTALL

AC_CONFIG_FILES([Makefile hashdeep/Makefile md5deep/Makefile])
AM_CONFIG_HEADER([config.h])

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
AC_CHECK_HEADERS([libgen.h fcntl.h limits.h stdlib.h string.h sys/types.h sys/ioctl.h sys/param.h wchar.h unistd.h sys/stat.h sys/disk.h])

AC_CHECK_HEADER([inttypes.h],,AC_MSG_ERROR([You must have inttypes.h or some other C99 equivalent]),)

# These includes are required on FreeBSD
AC_CHECK_HEADERS([sys/mount.h],[],[],
[#ifdef HAVE_SYS_TYPES_H
 # include <sys/types.h>
 #endif
 #ifdef HAVE_SYS_PARAM_H                                                     
 # include <sys/param.h>                           
 #endif])                    

AC_HEADER_TIME

AC_C_BIGENDIAN

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

# We use strsep do our string processing, but some operating systems
# don't have it yet (e.g. OpenSolaris). We if don't have strsep
# we compile it ourselves using a local copy.
AC_CHECK_FUNC([strsep],
              [AC_DEFINE([HAVE_STRSEP], [1],	
			 [Define if we have strsep.])],
              EXTRA_HASHDEEP='lib-strsep.$(OBJEXT)')
AC_SUBST([EXTRA_HASHDEEP])

AC_OUTPUT
