AC_INIT(blis++, 1.0, dmatthews@utexas.edu, tensor, http://www.github.com/devinamatthews/blisplusplus)
AM_INIT_AUTOMAKE([foreign silent-rules subdir-objects])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([disable])

AC_CONFIG_MACRO_DIR([m4])

#AM_CONFIG_HEADER(include/blis++_config.h)
AH_TOP([
#ifndef _BLISPP_CONFIG_H_
#define _BLISPP_CONFIG_H_
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
AQ_WITH_PACKAGE([blis], [https://github.com/flame/blis],
                [blis/blis.h], [],
                [bli_init], [-lblis])

#
# Check for Memkind
#
AQ_WITH_PACKAGE([memkind], [],
                [memkind.h], [],
                [memkind_get_version], [-lmemkind])
                
mkdir -p bin

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])
AX_PREFIX_CONFIG_H([include/blis++_config.h], [BLISPP])
AC_OUTPUT
