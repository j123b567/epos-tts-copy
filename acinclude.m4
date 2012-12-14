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

AC_DEFUN([AC_DEST_DF], 
	[AC_MSG_CHECKING([for $1], [epos_cv_$2])
	 AC_CACHE_VAL([epos_cv_$2],
		[
		epos_cv_$2="empty"
		outdir=''
		outfil=''
		sedscript="'s:./$2::; s:/::; s:$:\t\\\:; \$s:\\\\$::; '"
		dirs=`find ./$2 -type d | sed '1i\\t'`
		outdir=`find ./$2 -type d | eval sed -r $sedscript`
		
		sedscript="'s:^./$2$::'"
		dirs=`find ./$2 -type d | eval sed -r $sedscript`
		sedscript="'s:^./$2/::; s:$:\t\\\:; \$s:\\\\$::'"
		for d in $dirs; do
			fls=`find $d -type f | eval sed -r $sedscript`
			if test -n "$fls"; then
				if test -z "$outfil"; then
					outfil=$fls
				else
					outfil="$outfil
$fls"
				fi
	 		fi
		done
		if test -n "$outdir"; then 
			if test -n "$outfil"; then
				epos_cv_$2="checked"
			fi
		fi
		])
	 AC_MSG_RESULT(${epos_cv_$2})
	 if test "x${epos_cv_$2}" = "xchecked"; then
		echo "DATA_DIRS=$outdir" > $2/data_dirs.am
		echo "DATA_FILES=$outfil" > $2/data_files.am
	 fi
])

syscmd(`rm ./cfg/data_dirs.am`)
syscmd(`rm ./cfg/data_files.am`)
syscmd(`rm ./arch/data_dirs.am`)
syscmd(`rm ./arch/data_files.am`)

syscmd(`./dirs.sh cfg arch`)
