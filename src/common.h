/*
 *	epos/src/common.h
 *	(c) Jirka Hanika, geo@cuni.cz
 *	(c) Petr Horak, horak@ure.cas.cz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

 *
 *	The GNU General Public License can be found in file doc/COPYING.
 *
 *	This is the main header file. You should only include this one
 *	no matter what do you want to do with this code.
 *
 */


#ifndef EPOS_COMMON_H
#define EPOS_COMMON_H

#define MAINTAINER  		"Jirka Hanika"
#define MAIL        		"geo@cuni.cz"
#define VERSION     		"2.4.85"
#define TTSCP_PORT		 8778

#include "config.h"

#include <stdio.h>
#include <stdlib.h>           // just exit() in shriek(), malloc &co...
#include <stdint.h>
#include <stddef.h>

#ifdef HAVE_STRING_H
	#include <string.h>
#else
   #ifdef HAVE_STRINGS_H
	#include <strings.h>
   #else
	#error String library misconfigured. No.
   #endif
#endif

//char *strdup(const char *);

#ifndef HAVE_STRCASECMP
#ifdef  HAVE_STRICMP
	#define strcasecmp stricmp	// if only stricmp is defined
	#define strncasecmp strnicmp
#endif
#endif

#ifdef WANT_DMALLOC
	#include <dmalloc.h>  // Unimportant debugging hack. Throw it out.
#endif			      // new and delete are overloaded in interf.cc !

#ifndef  IGNORE_REGEX_RULES
	#define WANT_REGEX    // About always, we want to use the regex code
#endif


#ifdef WANT_REGEX
   extern "C" {
	#ifdef HAVE_RX_H
		#include <rx.h>
	#else
	    #ifdef HAVE_REGEX_H
		#include <regex.h>	// in some cases, "rx.h" may do better anyway
	    #else
		#include "rx.h"
	    #endif
	#endif
   }
#endif



enum SYMTABLE {ST_ROOT, ST_RAW, ST_EMPTY};
enum SUBST_METHOD {M_EXACT=0, M_END=1, M_BEGIN=2, M_BEGIN_OR_END=3, M_SUBSTR=4, M_PROPER=7, M_LEFT=8, M_RIGHT=16, M_ONCE=32, M_NEGATED=64};
enum REPARENT {M_DELETE, M_RIGHTWARDS, M_LEFTWARDS};
enum FIT_IDX {Q_FREQ, Q_INTENS, Q_TIME};
#define FITstr	"f:i:t:"
#define BOOLstr "false:true:off:on:no:yes:disabled:enabled:-:+:n:y:0:1:non::"
#define LIST_DELIM	 ':'

typedef char UNIT;
#define UNIT_MAX 12
#define U_ILL		127
#define U_DEFAULT	126
#define U_INHERIT	125
#define U_VOID		120

#define unuse(x) (void)(x);

extern const int is_monolith;

struct file;
struct option;
class  unit;

class stream;

#include "defaults.h"
#include "exc.h"
#include "hash.h"
#include "text.h"
#include "voice.h"
#include "function.h"
#include "interf.h"             //See interf.h and even interf.cc for other headerities
#include "options.h"
#include "parser.h"
#include "unit.h"
#include "rule.h"              //See rules.h for additional #defines and enums
#include "waveform.h"
#include "synth.h"
#include "encoding.h"

#define MAX_PATHNAME       256	  // only load_language uses this

#ifdef HAVE_UNISTD_H
	#define SLASH              '/'
	#define NULL_FILE	   "/dev/null"
	#define MODE_MASK	   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
	#define O_BINARY	   0
#else
	#define SLASH              '\\'
	#define NULL_FILE	   "NUL"
	#define MODE_MASK	   (S_IREAD | S_IWRITE)
#endif

#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
	#define HAVE_WINSOCK
	#define socky unsigned
#else
	#define socky signed
#endif


#endif   //#ifndef EPOS_COMMON_H



