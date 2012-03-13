/*
 *	ss/src/common.h
 *	(c) Jirka Hanika, geo@ff.cuni.cz
 *	(c) Petr Horak, horak@ure.cas.cz
 *
 *	For copyright info see doc/COPYING.
 *
 *	This is the main header file. You should only include this one
 *	no matter what do you want to do with this code.
 *
 */


#ifndef SS_COMMON_H
#define SS_COMMON_H

#define MAINTAINER  "Jirka Hanika"
#define MAIL        "geo@ff.cuni.cz"
#define VERSION     "1.5.1"

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>           // just exit() in shriek(), malloc &co...

#ifndef HAVE_STRCASECMP
#ifdef  HAVE_STRICMP
	#define strcasecmp stricmp	// if only stricmp is defined
#endif
#endif

#ifdef WANT_DMALLOC
	#include <dmalloc.h>  // Unimportant debugging hack. Throw it out.
#endif			      // new and delete are replaced in interf.cc !

#define WANT_REGEX

#ifdef HAVE_LIBRX
   extern "C" {
	#include <rx.h>       // You need to have rx already installed
   }
#else
   #ifdef HAVE_LIBREGEX
   extern "C" {
	#include <regex.h>    // You need to have regex already installed
   }
   #else
      #ifdef WANT_REGEX
      #undef WANT_REGEX
      #endif
   #endif
#endif


enum SYMTABLE {ST_ROOT, ST_RAW, ST_EMPTY};
enum SUBST_METHOD {M_EXACT=0, M_END=1, M_BEGIN=2, M_BEGIN_OR_END=3, M_SUBSTR=4, M_PROPER=7, M_LEFT=8, M_RIGHT=16};
enum REPARENT {M_DELETE, M_RIGHTWARDS, M_LEFTWARDS};
enum FIT_IDX {Q_FREQ, Q_INTENS, Q_TIME};
enum UNIT {U_DIPH, U_PHONE, U_SYLL, U_WORD, U_COLON, U_SENT, U_TEXT, U_INHERIT, U_DEFAULT, U_ILL=255, U_VOID=127};
enum SYNTH_TYPE {S_FLOAT, S_INT, S_VQ};
#define STstr "float:int:vq:"
#define UNITstr "diphone:phone:syll:word:colon:sent:text:inherit:default:"
#define BOOLstr "false:true:off:on:no:yes:disabled:enabled:-:+:n:y:0:1:non::"
#define FITstr	"f:i:t:"
#define LIST_DELIM	 ':'

#define Char unsigned char
#define unuse(x) if (((1&(int)(x))*(1&(int)(x)))<0) shriek("I'm drunk");

#include "defaults.h"
#include "hash.h"
#include "text.h"
#include "interf.h"             //See interf.h and even interf.cc for other headerities
#include "parser.h"
#define PARSER simpleparser
#include "elements.h"
#include "rule.h"              //See rules.h for additional #defines and enums
#include "synth.h"

#define MAX_PATHNAME       256	  // only load_language uses this

#define NO_CONT            '_'    // null contents of a unit
#define JUNCTURE           '0'    // scope boundary in assim environment
#define DELETE_ME     JUNCTURE    // changing cont to this one is fatal
#define QUESTION_MARK      '?'    // ignored context character (in diphone names)
#define RATIO_TOTAL	   100	  // 100 % (unit::smooth percent sum)

#ifdef HAVE_UNISTD_H

#define SLASH              '/'
#define NULL_FILE	   "/dev/null"
#define MODE_MASK	   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#else

#define SLASH              '\\'
#define NULL_FILE	   "NUL"
//#define MODE_MASK	   (S_IREAD | S_IWRITE)	// should investigate

#endif


#endif   //#ifndef SS_COMMON_H



