/* Please ignore this file. If under Borland C++, compile it instead.*/

#ifndef __MSDOS__
#define __MSDOS__
#endif

#pragma warn -pia
#pragma warn -ccc
#pragma warn -rch

#define bool int

#define inline /* Borland can't handle inline */
#define const  /* Borland can't handle even const properly! */

#ifndef __WATCOM_C__
#define HAVE_TEMPL_INST
#else

#define REGEX_MALLOC

#include <fcntl.h>

#include "monolith.cc"
#include "hash.cc"
#include "interf.cc"
#include "parser.cc"
#include "elements.cc"
#include "rule.cc"
#include "text.cc"
#include "synth.cc"
#include "ktdsyn.cc"
#include "lpcsyn.cc"	// comment out if not necessary
#include "voice.cc"
#include "rx.c"
