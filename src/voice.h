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
};

#define STstr "none:internet:::lpc-float:lpc-int:lpc-vq::ktd:"
#define ST_MAX (S_KTD+1)

enum CHANNEL_TYPE {CT_MONO, CT_FIRST, CT_SECOND, CT_BOTH};
#define CHANNEL_TYPEstr "mono:first:second:both:"

struct cowabilium
{
	void *cow;
	cowabilium() {cow = NULL; };
};

struct voice;

struct lang : public cowabilium
{
   #define CONFIG_LANG_DECLARE
   #include "options.cc"

	char *lang_name;
	rules *ruleset;
	hash_table<char, option> *soft_options;
	void *soft_defaults;
	int   n_voices;
	voice **voices;
	voice *default_voice;

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
   #include "options.cc"

//	int samp_size_bytes;
//	int buffer_idx;
//	int written_bytes;
//	char *buffer;
//	int fd;		/* This is a file descriptor to write the samples to.
			/* Can also be an open device or a socket.
			 * -1 means "busy, don't use" (daemon code sets this)
			 */

//	void skip_header();	// for writing into .wav files
//	void write_header();

   public:
//   	char diphone_names[441][4];
//	char (*diphone_names)[5];
	file *diphone_names;
	
	synth *syn;
   
	voice(const char *filename, const char *dirname, lang *parent_lang);
	~voice();

//	void attach();
//	void detach();
//	void flush();
/*	inline void put_sample(unsigned int sample)
	{
		switch (samp_size_bytes)
		{
			case 1:	*(unsigned char *) (buffer + buffer_idx) = sample; break;
			case 2:	*(unsigned short *)(buffer + buffer_idx) = sample; break;
		}
		buffer_idx += samp_size_bytes;
		if (buff_size <= buffer_idx) flush();
	}

	inline void sample(unsigned int sample)
	{
		switch(channel)
		{
			case CT_MONO:	put_sample(sample); break;
			case CT_FIRST:	put_sample(sample); put_sample(0); break;
			case CT_SECOND:	put_sample(0); put_sample(sample); break;
			case CT_BOTH:	put_sample(sample); put_sample(sample); break;
		}
//	}
*/
	void *operator new(size_t);
	void  operator delete(void *);
};

extern lang  *this_lang;
extern voice *this_voice;

struct option;

extern option langoptlist[];
extern option voiceoptlist[];

