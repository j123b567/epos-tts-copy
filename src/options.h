/*
 *	epos/src/options.h
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
 *	This file handles the configuration options in general,
 *	that is, class configuration, lang and voice. It doesn't
 *	handle individual options; these are declared in options.lst
 *	and scattered all around the code base.
 */

enum OPT_STRUCT { OS_CFG, OS_LANG, OS_VOICE };
enum ACCESS { A_PUBLIC, A_AUTH, A_ROOT, A_NOACCESS };
enum OPT_TYPE { O_BOOL, O_UNIT, O_MARKUP, O_SYNTH, O_CHANNEL, O_DBG_AREA, O_INT, O_CHAR, O_STRING, O_LANG, O_VOICE, O_CHARSET };
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

	void *operator new(size_t size);
	void operator delete(void *ptr);
};

 //void cow(cowabilium **p, int size, int, int);	/* copy **p if shared and adjust *p, see options.cc */
void cow_claim();				/* claim all current global cfg */
void cow_unclaim(configuration *);		/* unclaim the cfg specified */

void cow_configuration(configuration **);

struct option
{
	const char *optname;
  	OPT_TYPE opttype;
	OPT_STRUCT structype;
	ACCESS 	readable;
	ACCESS 	writable;
	bool action;
	bool per_level;
	short int offset;
};

void config_init();
void config_release();

// void process_options(hash *tab, option *list, void *base);
// char *get_named_cfg(const char *option_name);
option *option_struct(const char *name, hash_table<char, option> *softopts);

/* For the following two functions, the value MAY get changed by set_option()
   (in-place), if o->opttype is O_STRING or O_CHAR and value contains
   backslashes or double quotes  		*/
bool set_option(option *o, const char *value);			// the const qualifier IS A LIE
bool set_option(option *o, const char *value, void *whither);	// the const qualifier IS A LIE


const char *format_option(option *name);	// may return scratch etc.
const char *format_option(const char *name);  // ditto

bool lang_switch(const char *name);
bool voice_switch(const char *name);

extern configuration master_cfg;
extern configuration *cfg;

void load_config(const char *filename);
void load_config(const char *filename, const char *dirname, const char *what,
		OPT_STRUCT type, void *whither, lang *parent_lang);

void list_languages();
void list_voices();

void shutdown_cfgs();
void shutdown_langs();

extern int argc_copy;
extern char **argv_copy;

#define DQUOT          '"'            //used when parsing the .ini file
