
dnl Process this file with autoconf to produce a configure script

dnl AC_INIT(src/unit.cc)
dnl 

AC_INIT([epos], 2.4-85)
AM_INIT_AUTOMAKE(epos, 2.5.37)

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/unit.cc)
AC_PREFIX_DEFAULT(/usr/local)

dnl CCC="g++"
dnl AC_PROG_CC

AC_PROG_CXX(g++ gcc cc c++)

if test $GXX = yes; then 
	if $CXX -v 2>&1 | grep -q egcs
		then AM_CXXFLAGS="-Wall -Wunused -pipe -gstabs+ -fdefer-pop -O"
		else AM_CXXFLAGS="-Wall -Wunused -gstabs+ -O"
	fi
	else AM_CXXFLAGS=-O
fi

AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(CONFIGURED_CXXFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_ARG_WITH(dmalloc, AC_HELP_STRING([--with-dmalloc], [dmalloc debugging library (developers only)]),
	if test $withval = yes; then
		AM_CXXFLAGS="${AM_CXXFLAGS} -DWANT_DMALLOC"
		LIBS="$LIBS -ldmalloc -L/usr/local/lib"
	fi
)
dnl AC_ARG_WITH(regex, --with-regex       regular expression substitution, , )
AC_ARG_ENABLE(charsets, AC_HELP_STRING([--enable-charsets], [allow automatic conversions between character sets]),
	if test x$enableval = xno; then
		AM_CXXFLAGS="${AM_CXXFLAGS} -DFORGET_CHARSETS"
	fi,
)
dnl AC_ARG_ENABLE(portaudio, AC_HELP_STRING([--enable-portaudio], [alternative sound output method]),
dnl	[case "x$enableval" in
dnl		xno)
dnl			EXTRA_PORTAUDIO_DEFS="-DFORGET_PORTAUDIO"
dnl			EXTRA_PORTAUDIO_LIBS=""
dnl			;;
dnl		xyes)
dnl			EXTRA_PORTAUDIO_DEFS=""
dnl			EXTRA_PORTAUDIO_LIBS="-lportaudio -lpablio -L../libs/portaudio/lib -lpthread"
dnl			;;
dnl		*)	AC_MSG_ERROR(bad value $enableval for --enable-portaudio) ;;
dnl	esac],
dnl	[EXTRA_PORTAUDIO_LIBS=""
dnl	 EXTRA_PORTAUDIO_DEFS="-DFORGET_PORTAUDIO"]
dnl)
dnl AM_CONDITIONAL(PORTAUDIO, test "${EXTRA_PORTAUDIO_DEFS}" = "")

AC_LANG_CPLUSPLUS

AC_CHECK_PROGS(LNK, ld wlink, linker)
AC_CHECK_LIB(stdc++, main)
AC_SEARCH_LIBS(regcomp, c rx regex System, EXTRA_REGEX_OBJ="", EXTRA_REGEX_OBJ="../rx.$(OBJEXT)")
AC_SUBST([EXTRA_REGEX_OBJ])
dnl AC_SUBST([EXTRA_PORTAUDIO_LIBS])
dnl AC_SUBST([EXTRA_PORTAUDIO_DEFS])
AC_CHECK_LIB(socket, main)
AC_CHECK_LIB(lnsl, main)
AC_LANG_CPLUSPLUS
AC_PROG_INSTALL
AC_PROG_YACC
AM_PROG_LEX
AC_STDC_HEADERS
AC_CHECK_HEADERS(strings.h)
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS(io.h)
AC_CHECK_HEADERS(process.h)
AC_CHECK_HEADERS(rx.h)
AC_CHECK_HEADERS(regex.h)
AC_CHECK_HEADERS(errno.h)
AC_CHECK_HEADERS(wait.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(unix.h)
AC_CHECK_HEADERS(signal.h)
AC_CHECK_HEADERS(syslog.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(sys/soundcard.h)
AC_CHECK_HEADERS(sys/audio.h)
AC_CHECK_HEADERS(sys/stat.h)
AC_CHECK_HEADERS(sys/ioctl.h)
AC_CHECK_HEADERS(sys/socket.h)
AC_CHECK_HEADERS(sys/select.h)
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(sys/termios.h)
AC_CHECK_HEADERS(netdb.h)
AC_CHECK_HEADERS(netinet/in.h)
AC_CHECK_HEADERS(linux/kd.h)
AC_CHECK_HEADERS(sys/kernel.h)
AC_CHECK_HEADERS(sys/name.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(direct.h)
AC_CHECK_FUNCS(strerror vsnprintf strdup fork getegid strcasecmp stricmp getcwd gethostname gettimeofday qnx_name_attach abort kill pipe _pipe)

#IMPORTANT - we have to assure that compiling is 32bit

#CXXFLAGS+="-m32"
AC_CHECK_HEADERS(pulse/pulseaudio.h)
AC_CHECK_HEADERS(pulse/simple.h)
if test "x$ac_cv_header_pulse_pulseaudio_h" = "xyes" -a "x$ac_cv_header_pulse_simple_h" = "xyes"; then
	AC_MSG_NOTICE([trying to link PulseAudio])
	AC_SEARCH_LIBS(pa_mainloop_new, pulse, [dnl
		AC_CHECK_32_64b([libpulse])
		case $libpulse in
	   	   64)
			AC_MSG_ERROR([PulseAudio 64bit library will not be working, download the 32bit version...])
			;;
		   32|3264)
			AC_MSG_NOTICE([PulseAudio 32bit library found...])
			;;
		   missing)
			AC_MSG_ERROR([PulseAudio library is missing...])
			;;
		esac], AC_MSG_ERROR([PulseAudio linking failed]))
else
	AC_MSG_ERROR([PulseAudio headers are required...])
fi

AC_CACHE_CHECK([for glibc regex error codes],
   epos_cv_symb_regeend,
   AC_TRY_COMPILE([dnl
	#include <sys/types.h>
	#ifdef HAVE_RX_H
	#include <rx.h>
	#else
	#ifdef HAVE_REGEX_H
	#include <regex.h>
	#else
	#include "rx.h"
	#endif
	#endif]
	, [dnl
	int result = REG_EEND;
	int result2 = REG_ESIZE;
	return 0;],
	epos_cv_symb_regeend=yes,
	epos_cv_symb_regeend=no)
)
if test "$epos_cv_symb_regeend" = yes; then
   AC_DEFINE([HAVE_REG_EEND], [], [ Define if your regex library defines the REG_EEND and REG_ESIZE error codes ])
fi

AC_CACHE_CHECK([for regmatch_t.rm_so],
   epos_cv_struct_rm_so,
   AC_TRY_COMPILE([dnl
	#include <sys/types.h>
	#ifdef HAVE_RX_H
	#include <rx.h>
	#else
	#ifdef HAVE_REGEX_H
	#include <regex.h>
	#else
	#include "rx.h"
	#endif
	#endif]
	,[dnl
	regmatch_t rt;
	rt.rm_so;
	return 0;],
	epos_cv_struct_rm_so=yes,
	epos_cv_struct_rm_so=no)
)
if test "$epos_cv_struct_rm_so" = yes; then
   AC_DEFINE([HAVE_RM_SO], [], [ Define if your regex library defines the REG_EEND and REG_ESIZE error codes ])
fi


AC_CACHE_CHECK([for terminate],
   epos_cv_fn_terminate,
   AC_TRY_COMPILE(
	[ #include <stdlib.h> ],
	[ throw new int; exit(1); terminate() ] ,
	[ epos_cv_fn_terminate=yes ],
	[ epos_cv_fn_terminate=no ])
)
if test "$epos_cv_fn_terminate" = yes; then
   AC_DEFINE([HAVE_TERMINATE], [], [ Define if you have the built-in terminate function. ])
fi

AC_CACHE_CHECK([for explicit template instatiations],
   epos_cv_c_templ_inst,
   AC_TRY_COMPILE(
	[ template <class any_type> class some_type{}; template class some_type<int>; ]	,
	[],
	[ epos_cv_c_templ_inst=yes ],
	[ epos_cv_c_templ_inst=no ])
)
if test "$epos_cv_c_templ_inst" = yes; then
   AC_DEFINE([HAVE_TEMPL_INST], [], [ Define if your compiler understands explicit class template instantiations
   as in template class templ_type<param_type>; ]
)
fi

AC_CACHE_CHECK([for socklen_t],
   epos_cv_type_socklen,
   AC_TRY_COMPILE(
	#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
	#endif
	#include <sys/types.h>
	socklen_t x;,
	,
	epos_cv_type_socklen=yes,
	epos_cv_type_socklen=no)
)
if test "$epos_cv_type_socklen" = yes; then
   AC_DEFINE([HAVE_SOCKLEN_T], [], [ Define if you have socklen_t in sys/socket.h or sys/types.h. ])
fi

broken_enum_bitfields_test='\
	#include <stdlib.h>	\
	enum t { a, b, c };	\
	struct s		\
	{			\
		t m : 2;	\
		t n : 2;	\
		t o : 2;	\
	};			\
	s arr[[2]] = {{a, b, c},{c, c, c}};\
	int main()		\
	{			\
		exit (arr[[1]].n != 2);	\
		return 0;       \
	}\
	'

AC_CACHE_CHECK([for broken enum bitfields],
   epos_cv_c_broken_enum_bitfields,
   AC_TRY_RUN(
	$broken_enum_bitfields_test,
	epos_cv_c_broken_enum_bitfields=no,
	epos_cv_c_broken_enum_bitfields=yes,
	epos_cv_c_broken_enum_bitfields=yes)
)
if test "$epos_cv_c_broken_enum_bitfields" = yes; then
   AC_DEFINE([BROKEN_ENUM_BITFIELDS], [], [ Define if your compiler incorrectly treats enum bit fields as signed ])
fi

EPOS_CXX_OPTION(mafwks,
	[mac audio frameworks],
	["-framework CoreAudio -framework AudioToolbox"])
CONFIGURED_CXXFLAGS="$CONFIGURED_CXXFLAGS $mafwks"


AM_CONFIG_HEADER(config.h)
#AC_CONFIG_SUBDIRS(libs/portaudio)
#AC_CONFIG_FILES([scrpt], [chmod +x scrpt])

#AC_DEST_DF([config destination dirs and files], cfg)
#AC_DEST_DF([architecture destination dirs and files], arch)

AC_OUTPUT(Makefile src/Makefile cfg/Makefile arch/Makefile)

