/*
 *	epos/src/voice.h
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
 *	This class represents a single front end diphone inventory.
 */

class rules;
class synth;

enum SYNTH_TYPE {
	S_NONE = 0,
	S_TCP = 1,
	S_LPC_FLOAT = 4,
	S_LPC_INT = 5,
	S_LPC_VQ = 6,
	S_KTD = 8,
	S_PTD = 10,
};

#define STstr "none:internet:::lpc-float:lpc-int:lpc-vq::ktd::ptd:"
#define ST_MAX (S_PTD+1)

enum CHANNEL_TYPE {CT_MONO, CT_FIRST, CT_SECOND, CT_BOTH};
#define CHANNEL_TYPEstr "mono:first:second:both:"

struct cowabilium
{
	int cow;
	cowabilium *parent;
	cowabilium() {cow = 0; parent = this;};
};

struct voice;

struct lang : public cowabilium
{
   #define CONFIG_LANG_DECLARE
   #include "options.lst"

//	char *lang_name;
	rules *ruleset;
	hash_table<char, option> *soft_options;
	void *soft_defaults;
	int   n_voices;
	voice **voices;
	int   default_voice;

   public:
	lang::lang(const char *filename, const char *dirname);
	lang::~lang();
	void add_soft_option(const char  *option_name);
	void add_soft_opts(const char *option_names);
	void add_voice(const char  *voice_name);
	void add_voices(const char *voice_names);
	void compile_rules();		// could be made delayed....
};


struct voice : public cowabilium
{
   #define CONFIG_INV_DECLARE
   #include "options.lst"

   public:
	file *diphone_names;
	
	synth *syn;
   
	voice(const char *filename, const char *dirname, lang *parent_lang);
	~voice();

	void claim_diphone_names();

	void *operator new(size_t, lang *parent_lang);
	void  operator delete(void *);
};

// extern lang  *this_lang;
// extern voice *this_voice;

#define this_lang  (cfg->langs[cfg->default_lang])
#define this_voice (this_lang->voices[this_lang->default_voice])

struct option;

extern option langoptlist[];
extern option voiceoptlist[];

