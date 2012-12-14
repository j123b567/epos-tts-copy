AC_DEFUN([EPOS_CXX_OPTION],
[AC_MSG_CHECKING([for $2], [epos_cv_cxx_opt_$1])
AC_CACHE_VAL(epos_cv_cxx_opt_$1,
[ cat > conftest.cc <<EOF
#include <stdio.h>
int main(int argc, char **argv)
{
	if (argc < 0 || ((int)argv) == 0) printf("");
	return 0;
}
EOF
epos_cv_cxx_opt_$1="unknown"
for opt in $3; do
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${opt} -o conftest conftest.cc 2>conftest2 1>&5 || continue
		msg=`cat conftest2`
		if test -z "$msg"; then
			epos_cv_cxx_opt_$1=$opt
			break
		fi
done
rm -f conftest conftest2 conftest.cc])
AC_MSG_RESULT(${epos_cv_cxx_opt_$1})
if test "x${epos_cv_cxx_opt_$1}" = "xunknown"; then
	$1=
else
	$1=${epos_cv_cxx_opt_$1}
fi])

AC_DEFUN([AC_CHECK_32_64b],
	[dnl
	AC_MSG_CHECKING([for 32 bit version of the library $1], [epos_cv_lib_$1])
	AC_CACHE_VAL([epos_cv_lib_$1],
		[dnl
		epos_cv_lib_$1='yes'
		$1=''
		adrs=(32 64)
		for i in ${adrs[[@]]}; do
			check_err=`file /usr/lib$i | sed '/ERROR/!d'`
			if test -z "$check_err"; then
				#num=`objdump -a /usr/lib$1.so | sed '/elf64*/!d'`
				$1+="$i"
			fi
		done
		if test -z "$1"; then epos_cv_lib_$1="no"
			$1="missing"
		else
			if test "$1" = "64"; then
				epos_cv_lib_$1='no'
			fi
		fi		
])
	AC_MSG_RESULT(${epos_cv_lib_$1})
]
)

syscmd(`rm ./cfg/data_dirs.am`)
syscmd(`rm ./cfg/data_files.am`)
syscmd(`rm ./arch/data_dirs.am`)
syscmd(`rm ./arch/data_files.am`)

syscmd(`./dirs.sh cfg arch`)
