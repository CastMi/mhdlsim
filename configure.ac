AC_INIT([manager], [0.1])
AC_CONFIG_SRCDIR([managermain.cc])
AM_INIT_AUTOMAKE([no-define -Wall -Werror foreign subdir-objects])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_MACRO_DIR([m4])
dnl Process this file with autoconf to produce a configure script.
LT_INIT

AC_CONFIG_HEADERS( [config.h:config.h.in] )
AC_CONFIG_HEADER(_pli_types.h)
AC_CONFIG_HEADER(vvp/vvp_config.h)
AC_CONFIG_HEADER(vpi/vpi_config.h)
AC_CONFIG_HEADER(libveriuser/config.h)
AC_CONFIG_HEADER(tgt-vvp/tgt_vvp_config.h)

AC_CANONICAL_HOST
dnl Checks for programs.
AC_PROG_CC
# AC_PROG_CC_C99 is only available in autoconf version 2.60 and later.
AC_PREREQ([2.60])
AC_PROG_CC_C99
AC_PROG_CXX
AC_CHECK_TOOL(LD, ld, false)
AC_CHECK_TOOL(AR, ar, false)
AC_CHECK_TOOL(DLLTOOL, dlltool, false)
AC_CHECK_TOOL(STRIP, strip, true)
AC_CHECK_TOOL(WINDRES,windres,false)
AC_CHECK_PROGS(XGPERF,gperf,none)
AC_CHECK_PROGS(MAN,man,none)
AC_CHECK_PROGS(PS2PDF,ps2pdf,none)
AC_CHECK_PROGS(GIT,git,none)
if test "$XGPERF" = "none"
then
	echo ""
        echo "*** Warning: No suitable gperf found. ***"
	echo "    The gperf package is essential for building ivl from"
	echo "    git sources, or modifying the parse engine of ivl itself."
	echo "    You can get away without it when simply building from"
	echo "    snapshots or major releases."
	echo ""
fi

AC_CHECK_PROGS(LEX,flex,none)
if test "$LEX" = "none"
then
	echo "*** Error: No suitable flex found. ***"
        echo "    Please install the 'flex' package."
        exit 1
fi

AC_CHECK_PROGS(YACC,bison,none)
if test "$YACC" = "none"
then
	echo "*** Error: No suitable bison found. ***"
        echo "    Please install the 'bison' package."
        exit 1
fi

AC_EXEEXT
AC_SUBST(EXEEXT)

# Combined check for Microsoft-related bogosities; sets WIN32 if found
AX_WIN32

# Check to see if we are using the Sun compiler. If so then configure
# some of the flags to match the Sun compiler syntax. This is also used
# in the aclocal.m4 file to configure the flags used to build and link
# dynamic libraries
AC_CHECK_DECL(__SUNPRO_C, using_sunpro_c=1, using_sunpro_c=0)
if test ${using_sunpro_c} = 1
then
AC_SUBST(DEPENDENCY_FLAG, [-xMMD])
AC_SUBST(WARNING_FLAGS, [""])
AC_SUBST(WARNING_FLAGS_CXX, [""])
else
# Check to see if -Wextra is supported.
iverilog_temp_cflags="$CFLAGS"
CFLAGS="-Wextra $CFLAGS"
AC_MSG_CHECKING(if gcc supports -Wextra)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
   [[iverilog_wextra_flag="-Wextra";] AC_MSG_RESULT(yes)],
   [[iverilog_wextra_flag="-W";] AC_MSG_RESULT(no)])
CFLAGS="$iverilog_temp_cflags"

AC_SUBST(DEPENDENCY_FLAG, [-MD])
AC_SUBST(WARNING_FLAGS, ["-Wall $iverilog_wextra_flag -Wshadow"])
AC_SUBST(WARNING_FLAGS_CC, ["-Wstrict-prototypes"])
AC_SUBST(WARNING_FLAGS_CXX, [""])
fi
AC_SUBST([AM_CXXFLAGS], ["-Wall -Wextra -g -O0"])

AC_LANG(C++)

AC_ARG_WITH([m32], [AC_HELP_STRING([--with-m32], [Compile 32-bit on x86_64])],
            [ with_m32=yes ],[ with_m32=no ])

AS_IF( [test "x$with_m32" = xyes],
    [ AC_MSG_NOTICE([Compiling for 32-bit environment - needs gcc on x86_64])
      LDTARGETFLAGS="-m elf_i386"
      CTARGETFLAGS="-m32"
    ],
    [])

CFLAGS="$CTARGETFLAGS $CFLAGS"
CXXFLAGS="$CTARGETFLAGS $CXXFLAGS"
LDFLAGS="$CTARGETFLAGS $LDFLAGS"

# Check that we are using either the GNU compilers or the Sun compilers
# but not a mixture of the two (not currently supported).
AC_CHECK_DECL(__SUNPRO_CC, using_sunpro_cc=1, using_sunpro_cc=0)
if test ${using_sunpro_c} = 1
then
  if test ${using_sunpro_cc} = 0
  then
    echo "*** Error: No support for mixing GNU and Sun compilers. ***"
    echo "           Using Sun C compiler and GNU C++ compiler.."
    exit 1
  fi
else
  if test ${using_sunpro_cc} = 1
  then
    echo "*** Error: No support for mixing GNU and Sun compilers. ***"
    echo "           Using GNU C compiler and Sun C++ compiler.."
    exit 1
  fi
fi

iverilog_temp_cxxflags="$CXXFLAGS"
CXXFLAGS="-DHAVE_DECL_BASENAME $CXXFLAGS"

AC_CHECK_HEADERS(getopt.h inttypes.h libiberty.h iosfwd sys/wait.h)
CXXFLAGS="$iverilog_temp_cxxflags"

AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned)

# vvp uses these...
AC_CHECK_LIB(termcap, tputs)
AC_CHECK_LIB(readline, readline)
AC_CHECK_LIB(history, add_history)
AC_CHECK_HEADERS(readline/readline.h readline/history.h sys/resource.h)
case "${host}" in *linux*) AC_DEFINE([LINUX], [1], [Host operating system is Linux.]) ;; esac

# vpi uses these
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(z, gzwrite)
AC_CHECK_LIB(z, gzwrite, HAVE_LIBZ=yes, HAVE_LIBZ=no)
AC_SUBST(HAVE_LIBZ)
if test "$WIN32" = "yes"; then
AC_CHECK_LIB(bz2, main)
AC_CHECK_LIB(bz2, main, HAVE_LIBBZ2=yes, HAVE_LIBBZ2=no)
else
AC_CHECK_LIB(bz2, BZ2_bzdopen)
AC_CHECK_LIB(bz2, BZ2_bzdopen, HAVE_LIBBZ2=yes, HAVE_LIBBZ2=no)
fi
AC_SUBST(HAVE_LIBBZ2)

# The lxt/lxt2 files from GTKWave use these...

AC_FUNC_ALLOCA
AC_FUNC_FSEEKO

# valgrind checks
AC_ARG_WITH([valgrind], [AC_HELP_STRING([--with-valgrind],
                                        [Add valgrind hooks])],
            [], [check_valgrind=yes])

AS_IF([test "x$check_valgrind" = xyes],
      [AC_MSG_NOTICE([Not using valgrind hooks])],
      [AC_CHECK_HEADER([valgrind/memcheck.h],
                       [AC_DEFINE([CHECK_WITH_VALGRIND], [1],
                                  [Define to one to use the valgrind hooks])],
                       [AC_MSG_ERROR([Could not find <valgrind/memcheck.h>])])])

AC_MSG_CHECKING(for sys/times)
AC_TRY_LINK(
#include <unistd.h>
#include <sys/times.h>
,{clock_t a = times(0)/sysconf(_SC_CLK_TCK);},
do_times=yes
AC_DEFINE([HAVE_TIMES], [1], [The times system call is available in the host operating system.]),
do_times=no
)
AC_MSG_RESULT($do_times)

# --
# Look for a dl library to use. First look for the standard dlopen
# functions, and failing that look for the HP specific shl_load function.

AC_CHECK_HEADERS(dlfcn.h dl.h, break)

DLLIB=''
AC_CHECK_LIB(dl,dlopen,[DLLIB=-ldl])
if test -z "$DLLIB" ; then
AC_CHECK_LIB(dld,shl_load,[DLLIB=-ldld])
fi
AC_SUBST(DLLIB)
AC_SUBST(LDRELOCFLAGS)

AC_SUBST(CTARGETFLAGS)
AC_SUBST(LDTARGETFLAGS)

AC_PROG_INSTALL

AC_LANG(C)
AC_C_BIGENDIAN

# $host


# Compiler option for position independent code, needed when making shared objects.
# CFLAGS inherited by cadpli/Makefile?

# may modify LDFLAGS

# Processor specific compile flags
case "${host}" in
    alpha*-*-linux*)
	CPPFLAGS="-mieee $CPPFLAGS"
	CFLAGS="-mieee $CFLAGS"
	;;
    *-*-mingw*)
	CXXFLAGS="-D__USE_MINGW_ANSI_STDIO=1 $CXXFLAGS"
	CFLAGS="-D__USE_MINGW_ANSI_STDIO=1 $CFLAGS"
	;;
esac

# Do some more operating system specific setup. We put the file64_support
# define in a substitution instead of simply a define because there
# are source files (namely lxt support files) that don't include any
# config.h header file.
file64_support=''
case "${host}" in
    *-*-linux*)
	AC_DEFINE([_LARGEFILE_SOURCE], [1], [Indicates LFS (i.e. the ability to create files larger than 2 GiB on 32-bit operating systems).])
	file64_support='-D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64'
	;;
esac
AC_SUBST(file64_support)

# fstapi.c (from GTKWave) needs this define.
AC_CHECK_FUNCS(realpath)

# Check that these functions exist. They are mostly C99
# functions that older compilers may not yet support.
AC_CHECK_FUNCS(fopen64)
# The following math functions may be defined in the math library so look
# in the default libraries first and then look in -lm for them. On some
# systems we may need to use the compiler in C99 mode to get a definition.
# We requested C99 mode earlier with AC_PROG_CC_C99.

# Check to see if an unsigned long and uint64_t are the same from
# a compiler perspective. We can not just check that they are the
# same size since unsigned long and unsigned long long are not the
# same from an overloading perspective even though they could be
# the same size on some 64 bit machines. The result from this test
# is only used if inttypes.h is available, so if the test fails for
# that reason we don't care.
AC_LANG(C++)


# Linker option used when compiling the target

# linker options when building a shared library


#######################
## test for underscores. The vpi module loader needs to know this
## in order to know the name of the start symbol for the .vpi module.
#######################


#######################
## end of test for underscores
#######################

#######################
# Sanity check the configured results
#######################

m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])

AC_MSG_CHECKING(for sanity of prefix)
if test `echo "$prefix" | wc -w` != 1
then
AC_MSG_ERROR(cannot configure white space in prefix: $prefix)
fi
AC_MSG_RESULT(ok)

AC_MSG_CHECKING(for sanity of exec_prefix)
if test `echo "$exec_prefix" | wc -w` != 1
then
AC_MSG_ERROR(cannot configure white space in exec_prefix: $exec_prefix)
fi
AC_MSG_RESULT(ok)

AC_MSG_CHECKING(for sanity of libdir)
if test `echo "$libdir" | wc -w` != 1
then
AC_MSG_ERROR(cannot configure white space in libdir: $libdir)
fi
AC_MSG_RESULT(ok)

AC_OUTPUT(Makefile ivlpp/Makefile tgt-vvp/Makefile vpi/Makefile vvp/Makefile manager/Makefile )
