/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#undef _ALL_SOURCE
#endif

/* Define if your compiler incorrectly treats enum bit fields as signed */
#undef BROKEN_ENUM_BITFIELDS

/* Define to empty if the keyword does not work.  */
#undef const

/* Define if your compiler somehow offers a boolean type.  */
#undef HAVE_BOOL

/* Define if you have socklen_t in sys/socket.h or sys/types.h.  */
#undef HAVE_SOCKLEN_T

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#undef HAVE_SYS_WAIT_H

/* Define if your compiler understands explicit class template instantiations
   as in template class templ_type<param_type>; */
#undef HAVE_TEMPL_INST

/* Define if you have the built-in terminate function.  */
#undef HAVE_TERMINATE

/* Define if you have <unistd.h>.  */
#undef HAVE_UNISTD_H

/* Define if you have <vfork.h>.  */
#undef HAVE_VFORK_H

/* Define as __inline if that's what the C compiler calls it.  */
#undef inline

/* Define if int is 16 bits instead of 32.  */
#undef INT_16_BITS

/* Define if long int is 64 bits.  */
#undef LONG_64_BITS

/* Define if your C compiler doesn't accept -c and -o together.  */
#undef NO_MINUS_C_MINUS_O

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef pid_t

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
#undef _POSIX_1_SOURCE

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
#undef size_t

/* Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/* Define if your <sys/time.h> declares struct tm.  */
#undef TM_IN_SYS_TIME

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef uid_t

/* Define vfork as fork if vfork does not work.  */
#undef vfork




/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */
