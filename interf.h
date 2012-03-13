/*
 *	ss/src/interf.h
 *	(c) geo@ff.cuni.cz
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

#ifndef SS_INTERF_H
#define SS_INTERF_H

#define CFG_FILE_ENVIR_VAR	"SSCFGFILE"

enum OPT_STRUCT { OS_CFG, OS_LANG, OS_VOICE };
enum OPT_TYPE { O_BOOL, O_UNIT, O_MARKUP, O_SYNTH, O_CHANNEL, O_DBG_AREA, O_INT, O_CHAR, O_STRING, O_FILE };
								//various types of options
enum OUT_ML { ML_NONE, ML_ANSI, ML_RTF};
#define OUT_MLstr "none:ansi:rtf:"
enum _DEBUG_AREA_ {_INTERF_, _RULES_, _ELEM_, _SUBST_, _ASSIM_, _SPLIT_,
			_PARSER_=7, _SYNTH_=9, _CFG_=10,_NONE_=127}; 
           //Don't touch this enum. We're gonna mix these identifiers and raw integer constants;
           // this way, _INTERF_ will be treated as equal to zero, _RULES_ equal to one etc.
           // Some more info can be found in doc/debug.txt
#define DEBUG_AREAstr "interf:rules:elem:subst:assim:split::parser::synth:cfg:"

#define CONFIG_DECLARE
struct configuration		//Some description & defaults can be found in options.cc
{
	#include "options.cc"

	int n_langs;
	lang **langs;

	void shutdown();	// destructor of some sort
};

struct option {
	const char *optname;
	OPT_TYPE opttype;
	OPT_STRUCT structype;
	short int offset;
};

void process_options(hash *tab, option *list, void *base);
char *get_named_cfg(const char *option_name);
bool set_option(char *name, char *value);
char *format_option(const char *name);	// will malloc some space

extern configuration master_cfg;
extern configuration *cfg;
extern int argc;
extern char** argv;

void ss_init(int argc, char**argv);
void ss_init();
void ss_reinit();
void load_config(const char *filename, const char *not_found_message);
void load_config(const char *filename, const char *dirname, const char *what,
			void *whither, const char *not_found, option *olist);
void ss_done();		// No real need to call, ever. Just to be 100% dmalloc correct.


#define color(stream, seq) if (cfg->colored) fprintf(stream, seq)
void colorize(int level, FILE *handle);  // See this function in interf.cc for various #defines

#define MAX_ERR_LINE 320	// No error nor warning message may be longer
				// than that. We need to have it on the stack,
				// as we dare not allocate it if in trouble

void check_lib_version(const char *s);

void shriek(const char *s);
void shriek(const char *s, int userval);
void shriek(const char *s, const char *t, int userval);
void shriek(const char *s, const char *t);
void shriek(const char *s, const char *t, const char *u);
void shriek(const char *s, const char *t, const char *u, int userval);
void shriek(const char *s, const char *t, const char *u, const char *v);

void warn(const char *s);
void warn(const char *s, const char *t);
void user_pause();
FILE *fopen(const char *filename, const char *flags, const char *reason);

#ifndef HAVE_STRDUP
char *strdup(const char *src);   //Ultrix lacks it. Otherwise, we're just superfluous.
#endif

extern char *scratch;

FIT_IDX fit(char c);		 // converts 'f', 'i' or 't' to 0, 1 or 2, respectively
UNIT str2enum(const char *item, const char *list, int dflt);
char *enum2str(int item, const char *list);
hash *str2hash(const char *list, unsigned int max_item_len);
unit *str2units(char *text);
char *fntab(const char *s, const char *t); //will calloc and return 256 bytes not freeing s,t
                                       //if len(s)!=len(t), ignore the rest if not cfg.paranoid
bool *booltab(const char *s);          //will calloc and return 256 bytes not freeing s

char *compose_pathname(const char *filename, const char *dirname);
char *freadin(const char *filename, const char *dirname, const char *flags, const char *description);  //returns malloced file contents

void list_languages();
void list_voices();

class unit;
void process_diphones(unit *root);

struct diphone {
	int code;
	int f,e,t;
};

struct freadin_file	// used by freadin() (and mentioned in hash.cc) only
{
	char *data;
	int ref_count;
	~freadin_file();
};


#define DQUOT          '"'            //used when parsing the .ini file

extern char *esctab;

extern FILE *stdshriek;
extern FILE *stdwarn;
extern FILE *stddbg;

extern int session_uid;

#define	UID_ANON	-1
#define	UID_ROOT	 0
#define UID_SERVER	 1



#define DEBUGGING     

#ifdef DEBUGGING

extern char *current_debug_tag;
bool debug_wanted(int lev, /*_DEBUG_AREA_*/ int area);
void debug_prefix(int lev, int area);

#define DEBUG(xxx,yyy,zzz) {if(debug_wanted(xxx,yyy)) {debug_prefix(xxx,yyy);zzz;fflush(stddbg);};}

#else       // ifndef DEBUGGING
#define DEBUG(xxx,yyy,zzz) ; 
#endif      // ifdef DEBUGGING


#ifdef WANT_DMALLOC

char *forever(void *heapptr);
#define FOREVER(allocated) forever(allocated)
#define ETERNAL_ALLOCS	256

#else       // ifndef WANT_MALLOC
#define FOREVER(allocated) (allocated)
#endif      // ifdef WANT_MALLOC


#endif      // ifndef SS_INTERF_H
