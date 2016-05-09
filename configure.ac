AC_INIT(blis++, 1.0, dmatthews@utexas.edu, tensor, http://www.github.com/devinamatthews/blisplusplus)

AC_CONFIG_MACRO_DIR([m4])

AH_TOP([
#ifndef _CONFIG_H_
#define _CONFIG_H_
])
AH_BOTTOM([
#ifndef DEBUG
#define NDEBUG
#endif

#endif
])

AC_PROG_CXX([g++ clang++ icpc])
AX_CXX_COMPILE_STDCXX_11([noext])
AC_LANG([C++])
AC_C_RESTRICT

topdir="\"`(cd $srcdir && pwd)`\""
AC_DEFINE_UNQUOTED([TOPDIR], [$topdir], [The top source directory.])

#
# Check for BLIS
#
AQ_WITH_PACKAGE([memkind], [],
                [memkind.h], [],
                [memkind_get_version], [-lmemkind])
                
mkdir -p bin

AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
