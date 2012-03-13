/*
 *	epos/src/options.h
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
 *	This file handles the configuration options in general,
 *	that is, class configuration, lang and voice. It doesn't
 *	handle individual options; these are declared in options.lst
 *	and scattered all around the code base.
 */

enum OPT_STRUCT { OS_CFG, OS_LANG, OS_VOICE };
enum ACCESS { A_PUBLIC, A_AUTH, A_ROOT, A_NOACCESS };
enum OPT_TYPE { O_BOOL, O_UNIT, O_MARKUP, O_SYNTH, O_CHANNEL, O_DBG_AREA, O_INT, O_CHAR, O_STRING};
								//various types of options
#define CONFIG_DECLARE
struct configuration : public cowabilium	//Some description & defaults can be found in options.lst
{
	#include "options.lst"

	int  n_langs;
	lang **langs;
	int  default_lang;

	FILE *stdshriek;
	FILE *stddbg;

	stream * current_stream;

	configuration();
	void shutdown();	// destructor of some sort
};

/*	Visual C++ 6.0 and Watcom C 10.6 generate incorrect code for
 *	enum bit fields.  The VC thingie even feels syntactically
 *	depressed about them!  There is an autoconf test for
 *	BROKEN_ENUM_BITFIELDS, but not for FORGET_ENUM_BITFIELDS.
 *	Most compilers are OK.
 */

#ifdef  BROKEN_ENUM_BITFIELDS
   #ifdef FORGET_ENUM_BITFIELDS
	#define BIT_FIELD(x) /* alas */
   #else
	#define BIT_FIELD(x) : (x)+1
   #endif
#else
	#define BIT_FIELD(x) : x	
#endif

 //void cow(cowabilium **p, int size, int, int);	/* copy **p if shared and adjust *p, see options.cc */
void cow_claim();				/* claim all current global cfg */
void cow_unclaim(configuration *);		/* unclaim the cfg specified */

void cow_configuration(configuration **);

struct option
{
	const char *optname;
  	OPT_TYPE opttype	BIT_FIELD(5);
//	int reserved		BIT_FIELD(3);
	OPT_STRUCT structype	BIT_FIELD(2);
	ACCESS 	readable	BIT_FIELD(2);
	ACCESS 	writable	BIT_FIELD(2);
//	int reserved		BIT_FIELD(2);
	short int offset;
};

void config_init();
void config_release();

// void process_options(hash *tab, option *list, void *base);
char *get_named_cfg(const char *option_name);
option *option_struct(const char *name, hash_table<char, option> *softopts);
bool set_option(option *o, char *value);
bool set_option(option *o, char *value, void *whither);
char *format_option(option *name);	// will malloc some space
char *format_option(const char *name);	// will malloc some space

bool lang_switch(const char *name);
bool voice_switch(const char *name);

extern configuration master_cfg;
extern configuration *cfg;

void load_config(const char *filename);
void load_config(const char *filename, const char *dirname, const char *what,
		OPT_STRUCT type, void *whither, lang *parent_lang);

void list_languages();
void list_voices();

#define DQUOT          '"'            //used when parsing the .ini file


