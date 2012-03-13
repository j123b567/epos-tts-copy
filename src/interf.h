/*
 *	epos/src/interf.h
 *	(c) geo@cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *	
 *	This file contains the few things that don't fit elsewhere, 
 *	including nearly everything that should be ever printf'ed.
 */

#ifndef EPOS_INTERF_H
#define EPOS_INTERF_H

#define CFG_FILE_ENVIR_VAR	"EPOSCFGFILE"

enum OUT_ML { ML_NONE, ML_ANSI, ML_RTF};
#define OUT_MLstr "none:ansi:rtf:"
enum _DEBUG_AREA_ {_INTERF_, _RULES_, _ELEM_, _SUBST_, _ASSIM_, _SPLIT_,
			_PARSER_=7, _SYNTH_=9, _CFG_=10, _DAEMON_=11, _NONE_=127}; 
           //Don't touch this enum. We're gonna mix these identifiers and raw integer constants;
           // this way, _INTERF_ will be treated as equal to zero, _RULES_ equal to one etc.
#define DEBUG_AREAstr "interf:rules:elem:subst:assim:split::parser::synth:cfg:daemon:"


void epos_init(int argc, char**argv);
void epos_init();
void epos_reinit();
void epos_done();		// No real need to call, ever. Just to be 100% dmalloc correct.

#define color(stream, seq) if (cfg->colored) fprintf(stream, seq)
void colorize(int level, FILE *handle);  // See this function in interf.cc for various #defines

#define MAX_ERR_LINE 320	// No error nor warning message may be longer
				// than that. We need to have it on the stack,
				// as we dare not allocate it if in trouble

// void check_lib_version(const char *s);

char *fmt(const char *s, int userval);
char *fmt(const char *s, int userval, int anotherval);
char *fmt(const char *s, const char *t, int userval, const char *u);
char *fmt(const char *s, const char *t, int userval);
char *fmt(const char *s, const char *t);
char *fmt(const char *s, const char *t, const char *u);
char *fmt(const char *s, const char *t, const char *u, int userval);
char *fmt(const char *s, const char *t, const char *u, const char *v);
char *fmt(const char *s, int userval, const char *u);
char *fmt(const char *s, int userval, const char *u, const char *v);

void user_pause();

void shriek(int code, const char *msg)
#ifdef __GNUC__
	__attribute__((__noreturn__))
#endif
					;
char *split_string(char *string);	// 0-terminate the first word, return the rest

FILE *fopen(const char *filename, const char *flags, const char *reason);

extern void *xmall_ptr_holder;
#define OOM_HANDLER	(shriek(422, "Out of memory"), (void *)NULL)
#define xmalloc(x)	(((xmall_ptr_holder = malloc((x)))) ? xmall_ptr_holder : OOM_HANDLER)
#define xrealloc(x,y)	((((xmall_ptr_holder = realloc((x),(y))))) && (x) ? xmall_ptr_holder : OOM_HANDLER)
#define xcalloc(x,y)	(((xmall_ptr_holder = calloc((x),(y)))) ? xmall_ptr_holder : OOM_HANDLER)

// inline void memhack(int size)
// {
// 	if (size > 8192) printf("memhack %d\n", size);
// }

// void *xmalloc(size_t);
// void *xcalloc(size_t, size_t);
// void *xrealloc(void *, size_t);

void call_abort();

#ifndef HAVE_STRDUP
char *strdup(const char *src);   //Ultrix lacks it. Otherwise, we're just superfluous.
#endif

extern char *scratch;

FIT_IDX fit(char c);		 // converts 'f', 'i' or 't' to 0, 1 or 2, respectively
UNIT str2enum(const char *item, const char *list, int dflt);
const char *enum2str(int item, const char *list);
// hash *str2hash(const char *list, unsigned int max_item_len);
unit *str2units(const char *text);
//char *fntab(const char *s, const char *t); //will calloc and return 256 bytes not freeing s,t
                                       //if len(s)!=len(t), ignore the rest if not cfg.paranoid
//bool *booltab(const char *s);          //will calloc and return 256 bytes not freeing s

char *compose_pathname(const char *filename, const char *dirname, const char *treename);
char *compose_pathname(const char *filename, const char *dirname);
char *limit_pathname(const char *filename, const char *dirname);

struct file
{
	char *data;
	char *filename;
	int ref_count;
	int timestamp;
	~file();
};

file *claim(const char *filename, const char *dirname, const char *treename, const char *flags, const char *description, void oven(char *buff, int len));
// bool reclaim(file *);	// false...unchanged, true...changed
void unclaim(file *);

// void list_languages();
// void list_voices();

class unit;
void process_segments(unit *root);

struct segment {
	short code;
	char nothing;
	char ll;
	int f,e,t;
};


#define DQUOT		'"'            //used when parsing the .ini file
#define ESCAPE		'\\'
#define EXCLAM		'!'
#define PSEUDOSPACE	'\377'


extern charxlat *esctab;

// extern FILE *stdshriek;
// extern FILE *stdwarn;
// extern FILE *stddbg;

// extern int session_uid;

#define	UID_ANON	-1
#define	UID_ROOT	 0
#define UID_SERVER	 1

#ifndef HAVE_FORK
	int fork();
#endif

// bool privileged_exec();		// true if suid or sgid

#define DEBUGGING     

#ifdef DEBUG	
	#ifdef DEBUGGING
		#undef DEBUG	/* this is tricky, will be fixed in 2.5 */
	#endif
#endif

#ifdef DEBUGGING

extern char *current_debug_tag;
bool debug_wanted(int lev, /*_DEBUG_AREA_*/ int area);
void debug_prefix(int lev, int area);

#define DEBUG(xxx,yyy,zzz) {if(debug_wanted(xxx,yyy)) {debug_prefix(xxx,yyy);zzz;fflush(STDDBG);};}
#define STDDBG  ::cfg->stddbg

#else       // ifndef DEBUGGING
#define DEBUG(xxx,yyy,zzz) ; 
#endif      // ifdef DEBUGGING


#ifdef WANT_DMALLOC

char *forever(void *heapptr);
void end_of_eternity();
#define FOREVER(allocated) forever(allocated)
#define ETERNAL_ALLOCS	1024
#define END_OF_ETERNITY  end_of_eternity()

#else       // ifndef WANT_MALLOC
#define FOREVER(allocated) (allocated)
#define END_OF_ETERNITY  /**/
#endif      // ifdef WANT_MALLOC


#endif      // ifndef EPOS_INTERF_H
