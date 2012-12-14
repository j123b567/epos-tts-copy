/*
 *	This file must be adjusted manually whenever a new test
 *	is added to configure.in
 *
 *	This file is to be ignored under UNIX. When compiling with
 *	Visual C++ under Windows NT, you should rename this file
 *	to config.h, possibly removing a UNIXy config.h first.
 */

// #define __STDC__ 0	/* Borland does not like this #define */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if your compiler somehow offers a boolean type.  */
#define HAVE_BOOL 1

/* Define if you have socklen_t in sys/socket.h or sys/types.h.  */
/* #undef HAVE_SOCKLEN_T */

/* Define if your compiler understands explicit class template instantiations
   as in template class templ_type<param_type>; */
#define HAVE_TEMPL_INST 1

/* Define if you have the built-in terminate function.  */
#define HAVE_TERMINATE 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you have the fork function.  */
/* #undef HAVE_FORK */

/* Define if you have the strcasecmp function.  */
/* #undef HAVE_STRCASECMP */

/* Define if you have the strdup function.  */
/* #undef HAVE_STRDUP */

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the stricmp function.  */
#define HAVE_STRICMP 1

/* Define if you have the _pipe function.  */
#define HAVE__PIPE 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <io.h> header file.  */
#define HAVE_IO_H 1

/* Define if you have the <process.h> header file.  */
#define HAVE_PROCESS_H 1

/* Define if you have the <linux/kd.h> header file.  */
/* #undef HAVE_LINUX_KD_H */

/* Define if you have the <netdb.h> header file.  */
/* #undef HAVE_NETDB_H */

/* Define if you have the <netinet/in.h> header file.  */
/* #undef HAVE_NETINET_IN_H */

/* Define if you have the <regex.h> header file.  */
/* #undef HAVE_REGEX_H */

/* Define if you have the <rx.h> header file.  */
/* #undef HAVE_RX_H */

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
/* #undef HAVE_STRINGS_H */

/* Define if you have the <sys/audio.h> header file.  */
/* #undef HAVE_SYS_AUDIO_H */

/* Define if you have the <sys/ioctl.h> header file.  */
/* #undef HAVE_SYS_IOCTL_H */

/* Define if you have the <sys/select.h> header file.  */
/* #undef HAVE_SYS_SELECT_H */

/* Define if you have the <sys/socket.h> header file.  */
/* #undef HAVE_SYS_SOCKET_H */

/* Define if you have the <winsock2.h> header file.  */
#define HAVE_WINSOCK2_H 1

/* Define if you have the <sys/soundcard.h> header file.  */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
/* #undef HAVE_SYS_TIME_H */

/* Define if you have the <unistd.h> header file.  */
/* #undef HAVE_UNISTD_H */

#define HAVE_ABORT

/* Define if you have the <wait.h> header file.  */
/* #undef HAVE_WAIT_H */

/* Define if you have the c library (-lc).  */
#define HAVE_LIBC 1

/* Define if you have the regex library (-lregex).  */
/* #undef HAVE_LIBREGEX */

/* Define if you have the rx library (-lrx).  */
/* #undef HAVE_LIBRX */

/* Define if you have the stdc++ library (-lstdc++).  */
#define HAVE_LIBSTDC__ 1

/* Define if using Visual C++ 6.0 or something like that */
#define BROKEN_ENUM_BITFIELDS 1

/* Define at least if using unpatched Visual C++ 6.0   */
#define FORGET_ENUM_BITFIELDS 1

/* Defining this initially may save you some headaches */
/* #undef IGNORE_REGEX_RULES 1 */

/* Define except when using an obscure regex library and having extra luck */
#define HAVE_RM_SO 1

/* Defining this is necessary with any DOS/NT version */
#define FORGET_SOUND_IOCTLS 1

/* Defining this is necessary with any DOS/NT version */
#define FORGET_PORTAUDIO 1

/* Define if using the mmsystem for output */
#define HAVE_MMSYSTEM_H 1

/* Define if Epos should run as an NT service */
#define HAVE_WINSVC_H  1

/* Define if you have this header file */
#define HAVE_WINDOWS_H	1

/* Define if you have this header file */
#define HAVE_DIRECT_H
