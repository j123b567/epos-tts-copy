/*
 *	epos/src/voice.cc
 *	(c) 1998 geo@ff.cuni.cz
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
 */
 
#include "common.h"
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_AUDIO_H
#include <sys/audio.h>
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#ifdef HAVE_LINUX_KD_H
#include <linux/kd.h>	// too unimportant
#endif

#ifdef HAVE_IO_H
#include <io.h>		/* open, write, (ioctl,) ... */
#endif

#ifndef O_BINARY	/* open */
#define O_BINARY  0
#endif

#pragma hdrstop

#ifdef KDGETLED		// Feel free to disable or delete the following stuff
inline void mark_voice(int a)
{
	static voices_attached = 0;
	voices_attached += a;
	int kbd_flags = 0;
	ioctl(1, KDGETLED, kbd_flags);
	kbd_flags = kbd_flags & ~LED_SCR;
	if (voices_attached) kbd_flags |= LED_SCR;
	ioctl(1, KDSETLED, kbd_flags);
}
#else
inline void mark_voice(int) {};
#endif

#define   EQUALSIGN	'='
#define   OPENING	'('
#define   CLOSING	')'




// int n_langs = 0;
// int allocated_langs = 0;
// lang **langs = NULL;

voice *this_voice = NULL;
lang *this_lang = NULL;

#define CONFIG_LANG_DESCRIBE
option langoptlist[] = {
	#include "options.cc"
	{NULL}
};

#define CONFIG_INV_DESCRIBE
option voiceoptlist[] = {
	#include "options.cc"
	{NULL}
};


#define CONFIG_LANG_INITIALIZE
lang::lang(const char *filename, const char *dirname) : cowabilium()
{
	#include "options.cc"
	lang_name = "(unnamed)";
	ruleset = NULL;
	soft_options = NULL;
	soft_defaults = NULL;
	n_voices = 0;
	voices = NULL;
	default_voice = NULL;
	load_config(filename, dirname, "language", OS_LANG, this, NULL);
	if (!this_lang) this_lang = this;
	add_soft_opts(soft_option_names);
	add_voices(voice_names);
}

lang::~lang()
{
	for (int i=0; i<n_voices; i++) delete voices[i];
	if (voices) free(voices);
	if (ruleset) delete ruleset;
	if (soft_options) delete soft_options;
	if (soft_defaults) free(soft_defaults);
}

void
lang::add_voice(const char *voice_name)
{
	char *filename = (char *)malloc(strlen(voice_name) + 6);
	char *dirname = (char *)malloc(strlen(voice_name) + strlen(inv_dir) + 6);
	sprintf(filename, "%s.ini", voice_name);
	sprintf(dirname, "%s%c%s", inv_dir, SLASH, voice_name);
	if (*voice_name) {
		if (!voices) voices = (voice **)malloc(8*sizeof (void *));
		else if (!(n_voices-1 & n_voices) && n_voices > 4)  // if n_voices==8,16,32...
			voices = (voice **)realloc(voices, n_voices << 1);
		voices[n_voices++] = new voice(filename, dirname, this);
	}
	free(filename);
	free(dirname);
	if (!default_voice && voices) default_voice = voices[0];
}

void
lang::add_voices(const char *voice_names)
{
	int i, j;
	char *tmp = (char *)malloc(strlen(voice_names)+1);

	for (i=0, j=0; voice_names[i]; ) {
		if ((tmp[j++] = voice_names[i++]) == ':' ) {
			tmp[j-1] = 0;
			add_voice(tmp);
			j = 0;
		}
	}
	tmp[j] = 0;
	if (j) add_voice(tmp);
	free(tmp);
}

void
lang::add_soft_option(const char *optname)
{
	char *dflt = strchr(optname, EQUALSIGN);
	if (dflt) *dflt++ = 0;
	else dflt = "";
	char *closing = strchr(optname, CLOSING);

	option o;
	o.opttype = O_BOOL;		// default type
	o.structype = OS_VOICE;	// soft options can only be voice options
	o.readable = o.writable = A_PUBLIC;	// ...no access restrictions on them

	if (closing) {
		if (strchr(optname, OPENING) != closing - 2 || closing[1])
			shriek(812, fmt("Syntax error in soft option %s in lang %s", optname, name));
		closing[-2] = 0;
		switch(closing[-1]|('a'-'A')) {
			case 'b': o.opttype = O_BOOL; break;
			case 's': o.opttype = O_STRING; break;
			case 'n': o.opttype = O_INT; break;
			case 'c': o.opttype = O_CHAR; shriek(812, "char typed soft options are tricky"); break;
//			case 'f': o.opttype = O_FILE; break;
			default : shriek(812, fmt("Unknown option type in %s in lang %s", optname, name));
		}
	} else if (strchr(optname, OPENING))
		shriek(812, fmt("Unterminated type spec in soft option %s in lang %s", optname, name));
	if (option_struct(optname, NULL))
		shriek(812, fmt("Soft option name conflicts with a built-in option name %s in lang %s", optname, name));
	if (soft_options) {
		if (soft_options->translate(optname))
			shriek(812, fmt("Soft option already exists in lang %s", name));
		soft_defaults = realloc(soft_defaults,
				(soft_options->items + 2) * sizeof(void *) >> 1);
	} else {
		soft_options = new hash_table<char, option>(30);
		soft_options->dupkey = 0;
		soft_defaults = malloc(sizeof(void *));
	}

	o.offset = sizeof(voice) + (soft_options->items * sizeof(void *) >> 1);

	char *tmp = (char *)malloc(strlen(optname) + 3);
	strcpy(tmp + 2, optname);
	tmp[0] = 'V';
	tmp[1] = ':';
	o.optname = tmp + 2;

	soft_options->add(o.optname - 2, &o);
	soft_options->add(o.optname, &o);

	set_option(&o, dflt, (void *)((voice *)soft_defaults - 1));
}

void
lang::add_soft_opts(const char *names)
{
	int i, j;
	char *tmp = (char *)malloc(strlen(names)+1);

	for (i=0, j=0; names[i]; ) {
		if ((tmp[j++] = names[i++]) == ':' ) {
			tmp[j-1] = 0;
			add_soft_option(tmp);
			j = 0;
		}
	}
	tmp[j] = 0;
	if (j) add_soft_option(tmp);
	free(tmp);
}

void
lang::compile_rules()
{
	lang *tmp = this_lang;
	this_lang = this;
	DEBUG(3,10,fprintf(STDDBG,"Compiling %s language rules, hash dir %s\n", name, hash_dir);)
	ruleset = new rules(rules_file, rules_dir);
	this_lang = tmp;
}


#define CONFIG_INV_INITIALIZE
voice::voice(const char *filename, const char *dirname, lang *parent_lang) : cowabilium()
{
	#include "options.cc"

	if (parent_lang->soft_defaults)
		memcpy(this + 1, parent_lang->soft_defaults,
			sizeof(void *) * parent_lang->soft_options->items >> 1);
	
	load_config(filename, dirname, "voice", OS_VOICE, this, parent_lang);
	if (!parent_lang->default_voice) {
		parent_lang->default_voice = this;
		if (!this_voice)
			this_voice = this;
	}
	if (parent_lang->name == name) {	/* default the name to the stripped filename */
		if (strrchr(filename, SLASH))
			filename = strrchr(filename, SLASH) + 1;
		int l = strcspn(filename, ".");
		char *nname = (char *)malloc(l + 1);
		nname[l] = 0;
		strncpy(nname, filename, l);
		name = nname;
	}

	diphone_names = claim(dptfile, inv_dir, "rt", "diphone names");
	
//	buffer = 0;
//	fd = 0;
	syn = NULL;
}

voice::~voice()
{
	if (diphone_names) unclaim(diphone_names);
//	if (buffer) detach();
	delete syn;
}

void *
voice::operator new(size_t size)
{
	int nso;

#ifdef DEBUGGING
	if (size != sizeof(voice)) shriek(862, "I'm missing something");
#endif
	
	if (!this_lang || !this_lang->soft_options) nso = 0;
	else nso = this_lang->soft_options->items >> 1;
	return malloc(size + nso * sizeof(void *));
}

void
voice::operator delete(void *ptr)
{
	free(ptr);
}
