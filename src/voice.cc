/*
 *	ss/src/voice.cc
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
lang::lang(const char *filename, const char *dirname)
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
	char *dirname = (char *)malloc(strlen(voice_name) + strlen(cfg->invent_dir) + 6);
	sprintf(filename, "%s.ini", voice_name);
	sprintf(dirname, "%s%c%s", cfg->invent_dir, SLASH, voice_name);
	if (*voice_name) {
		if (!voices) voices = (voice **)malloc(8*sizeof (void *));
		else if (!(n_voices-1 & n_voices) && n_voices > 4)  // if n_voices==8,16,32...
			voices = (voice **)realloc(voices, n_voices << 1);
		voices[n_voices++] = new voice(filename, dirname, this_lang);
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
			shriek("Syntax error in soft option %s in lang %s", optname, name);
		closing[-2] = 0;
		switch(closing[-1]|('a'-'A')) {
			case 'b': o.opttype = O_BOOL; break;
			case 's': o.opttype = O_STRING; break;
			case 'n': o.opttype = O_INT; break;
			case 'c': o.opttype = O_CHAR; shriek("char typed soft options are tricky"); break;
//			case 'f': o.opttype = O_FILE; break;
			default : shriek("Unknown option type in %s in lang %s", optname, name);
		}
	} else if (strchr(optname, OPENING))
		shriek("Unterminated type spec in soft option %s in lang %s", optname, name);
	if (option_struct(optname, NULL))
		shriek("Soft option name conflicts with a built-in option name %s in lang %s", optname, name);
	if (soft_options) {
		if (soft_options->translate(optname))
			shriek("Soft option already exists in lang %s", name);
		soft_defaults = realloc(soft_defaults, soft_options->items * sizeof(void *) >> 1);
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
	DEBUG(3,10,fprintf(stddbg,"Compiling %s language rules, hash dir %s\n", name, hash_dir);)
	ruleset = new rules(rules_file, rules_dir);
	this_lang = tmp;
}


#define CONFIG_INV_INITIALIZE
voice::voice(const char *filename, const char *dirname, lang *parent_lang)
{
	#include "options.cc"

	if (parent_lang->soft_defaults)
		memcpy(this + 1, parent_lang->soft_defaults,
			sizeof(void *) * parent_lang->soft_options->items >> 1);
	
	load_config(filename, dirname, "voice", OS_VOICE, this, parent_lang);
	if (!this_voice) {
		this_voice = this;
		parent_lang->default_voice = this;
	}

	diphone_names = claim(dptfile, inv_dir, "rt", "diphone names");
	
	buffer = 0;
	fd = 0;
	syn = NULL;
}

voice::~voice()
{
	if (diphone_names) unclaim(diphone_names);
	if (buffer) detach();
	delete syn;
}

void
voice::attach()
{
	char *output;
	DEBUG(2,9,fprintf(stddbg,"Attaching voice %s\n", name););
	mark_voice(1);
	samp_size_bytes = samp_size >> 3;
	if (buffer) shriek("Nested voice::attach()");
	if (!cfg->play_diph) cfg->wav_file = NULL_FILE;
	output = compose_pathname(cfg->wav_file, cfg->wav_dir);

#ifdef S_IRGRP
	fd = open(output, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, MODE_MASK);
#else
	fd = open(output, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
#endif
	if (fd == -1) shriek("Failed to %s %s", strncmp(output, "/dev/", 5)
			? "create output file" : "open audio device", output);
	free(output);
	buffer_idx = 0;
	int stereo = channel==CT_MONO ? 0 : 1;
#ifdef SOUND_PCM_GETBLKSIZE
	stereo++;	/* 1 or 2 channels */
	ioctl (fd, SOUND_PCM_WRITE_CHANNELS, &stereo);  // keep it mono (disabled)
	ioctl (fd, SOUND_PCM_WRITE_RATE, &samp_rate);
	ioctl (fd, SOUND_PCM_WRITE_BITS, &samp_size);
	ioctl (fd, SOUND_PCM_GETBLKSIZE, &buff_size);
#else
   #ifdef SNDCTL_DSP_SPEED
	ioctl (fd, SNDCTL_DSP_STEREO, &stereo);		// keep it mono (disabled)
	ioctl (fd, SNDCTL_DSP_SPEED, &samp_rate);

      #ifdef DEBUGGING		/* Badly placed */
	int mask = (unsigned int)-1;
	ioctl (fd, SNDCTL_DSP_GETFMTS, &mask);
	DEBUG(2,9,fprintf(stddbg,"Hardware format mask is 0x%04x\n", mask);)
	if (!(samp_size & mask)) warn("Sampling rate not supported");
      #endif

	ioctl (fd, SNDCTL_DSP_SETFMT, &samp_size);
	ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &buff_size);
   #else
	DEBUG(3,9,fprintf(stddbg, "Sound ioctl's absent\n");)
	unuse(stereo);
   #endif
#endif
	written_bytes = 0;
	if (wav_hdr) skip_header();
	buffer = (char *)malloc(buff_size);
}

void
voice::detach()
{
	flush();
	DEBUG(2,9,fprintf(stddbg,"Detaching voice %s\n", name););
	if (!buffer) shriek("Nested voice::detach()");
#ifdef SOUND_SYNCH
	if (ioctlable) ioctl (fd, SOUND_SYNCH, 0);
#endif
	if (wav_hdr) write_header();
	close(fd);
	mark_voice(-1);
	free(buffer);
	buffer = NULL;
	fd = 0;
}

void
voice::flush()
{
	DEBUG(2,9,fprintf(stddbg, "Flushing the signal\n");)
	write(fd, buffer, buffer_idx);
	written_bytes += buffer_idx;
	buffer_idx = 0;
}


struct wave_header
{
	char string1[4];
	long flen;
	char string2[8];
	long xnone;
	short int  datform, numchan, sf1, sf2, avr1, avr2, wlenB, wlenb;
	char string3[4];
	long dlen;
};			// .wav file header

void
voice::skip_header()
{
	lseek(fd, sizeof(wave_header), SEEK_SET);
}

void
voice::write_header()
{
	static wave_header *wavh = NULL;

	if (lseek(fd, 0, SEEK_SET))
		return;		/* devices incapable of lseek() don't want the header */

	if (!wavh) wavh = new wave_header;
	int stereo = channel==CT_MONO ? 0 : 1;

	strcpy(wavh->string1,"RIFF");
	strcpy(wavh->string2,"WAVEfmt ");
	strcpy(wavh->string3,"data");
	wavh->datform = 1;
	wavh->numchan = 1;
	wavh->sf1 = samp_rate; wavh->sf2 = stereo ? wavh->sf1 : 0;
	wavh->avr1 = 2 * samp_rate; wavh->avr2 = stereo ? wavh->avr1 : 0;
	wavh->wlenB = samp_size_bytes; wavh->wlenb = samp_size;		// FIXME ?
	wavh->xnone = 0x010;
	wavh->dlen = written_bytes;
	wavh->flen = wavh->dlen + 0x24;
	write(fd, wavh, sizeof(wave_header));         //zapsani prazdne wav hlavicky na zacatek souboru
}

void *
voice::operator new(const int size)
{
	int nso;

#ifdef DEBUGGING
	if (size != sizeof(voice)) shriek("I'm missing something");
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
