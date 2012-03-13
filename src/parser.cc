/*
 *	epos/src/parser.cc
 *	(c) 1996-98 geo@ff.cuni.cz
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

/*
 *	If mode is 0, filename is truly a filename. If mode is 1, filename is
 *	really the contents, NOT a filename. I may fix this strange scheme later.
 *
 */

parser::parser(const char *filename, int mode)
{
	if (!cfg->loaded) epos_init();
	if (!filename || !*filename) {
		register signed char c;
		int i = 0;
		text = (unsigned char *)xmalloc(cfg->dev_txtlen+1);
		if(!text) shriek(422, "Parser: Out of memory");
		do text[i++] = c = getchar(); 
			while(c!=-1 && c!=cfg->eof_char && i<cfg->dev_txtlen);
		text[--i] = 0;
		txtlen = i;
	} else { 
		if (mode) text = (unsigned char *) strdup(filename);
		else {
			file *f = claim(filename, this_lang->input_dir, cfg->lang_base_dir, "rt", "input text");
			text = (unsigned char *)strdup(f->data);
			unclaim(f);
		}
		txtlen = strlen((char *)text);
	}
	DEBUG(2,7,fprintf(STDDBG,"Allocated %u bytes for the main parser\n", txtlen);)
	init(ST_ROOT);
}

parser::parser(const char *str)
{
	if (!cfg->loaded) epos_init();
	DEBUG(1,7,fprintf(STDDBG,"Parser for %s is to be built\n",str);)
	text = (unsigned char *)strdup(str);
	txtlen = strlen(str);
	init(ST_RAW);
}

void
parser::init(SYMTABLE symtab)
{
	unsigned int i;

	initables(symtab);
	for(i=0; i<txtlen; i++) text[i]=TRANSL_INPUT [text[i]];
	DEBUG(1,7,fprintf(STDDBG,"Parser: has set up with %s\n",text);)
		//TRANSL_INPUT should be filled in before, in this constructor.
		//    It's meant to be altered when we later decide to use
		//    another Czech char encoding.
	current = text-1; 
	do level = chrlev(*++current); while (level > cfg->phone_level && level < cfg->text_level);
		//We had to skip any garbage before the first phone

	gettoken();		// new - hope this works
//	f = i = t = 0;

	DEBUG(0,7,fprintf(STDDBG,"Parser: initial level is %u.\n",level);)
/*	if (level == U_TEXT) {		// This should go away sooner or later
		DEBUG(2,7,fprintf(STDDBG,"Parser: is empty\n");)
		initables(ST_EMPTY);
		*current = NO_CONT;  // return '_' or something instead of quirky '\0'
	}   */
}

parser::~parser()
{
	free(text);
}

/***************
unsigned char
parser::getchar()
{
	return *current++;
}

***unsigned char
***parser::gettoken()
{
	UNIT lastlev = level;
	unsigned char retchar = *current++;
	f = i = t = 0;
	level = chrlev(*current);   // level of the next character
	if (level == U_TEXT) {
		DEBUG(2,7,fprintf(STDDBG,"Parser: end of text reached, changing the policy\n");)
		initables(ST_EMPTY);
		*current = NO_CONT;  // return '_' or something instead of quirky '\0'
	}
	DEBUG(0,7,fprintf(STDDBG,"Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n",retchar, lastlev, *current, level);)

	return(retchar);           // return the old char
}

***********/

#define IS_DIGIT(x)	((x)>='0' && (x)<='9')

unsigned char
parser::gettoken()
{
	UNIT lastlev = level;
	unsigned char ret = token;
	do {
		switch (token = *current) {
			case '.':
				if (current[1] == '.' && current[2] == '.') {
					current += 2;
					token = DOTS;
					break;
				}
				if (IS_DIGIT(current[1])) {
					token = DECPOINT;
					break;
				}
				break;
			case '-':
				if (IS_DIGIT(current[1])) {
					token = current > text && IS_DIGIT(current[-1]) ? RANGE : MINUS;
					break;
				}
				while (current[1] == '-') current++;
				break;
			case '<': if (cfg->stml) shriek(462, "STML not implemented"); else break;
			case '&': if (cfg->stml) shriek(462, "STML not implemented"); else break;
			default : ;
		}
		level = chrlev(token);
		f = i = t = 0;
		if (level == cfg->text_level) {
			DEBUG(2,7,fprintf(STDDBG,"Parser: end of text reached, changing the policy\n");)
			initables(ST_EMPTY);
			*current = NO_CONT;  // return '_' or something instead of quirky '\0'
		}
		current++;
	} while (level <= lastlev && level > cfg->phone_level);
		// (We are skipping any empty units, except for phones.)
	DEBUG(0,7,fprintf(STDDBG,"Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n", ret, lastlev, *current, level);)

	return ret;
}

void
parser::done()
{
	if (current <= text + txtlen) 
		shriek(463, fmt("Too high level symbol in a dictionary, parser contains %s", (char *)text));
}

UNIT
parser::chrlev(unsigned char c)
{
	if (current > text+txtlen+1) /*(!current[-1] && c))*/ return(U_VOID);
	if (CHRLEV[c] == U_ILL)
	{
		if (cfg->relax_input) return CHRLEV[cfg->dflt_char];
DEBUG(4,7,{	if (c>127) fprintf(cfg->stdshriek,"Seems you're mixing two Czech character encodings?\n");
		fprintf(cfg->stdshriek,"Fatal: parser dumps core.\n%s\n",(char *)current-2);
})
		shriek(431, fmt("Parsing an unhandled character  '%c' - ASCII code %d", (unsigned int) c, (unsigned int) c));
	}
	return(CHRLEV[c]);
}

void
parser::regist(UNIT u, const char *list)
{
	unsigned char *s;
	if (!list) shriek (812, fmt("Parser configuration: No characters for level %d", u));
	for(s=(unsigned char *)list;*s!=0;s++)
	{
		if (CHRLEV[*s] != U_ILL && CHRLEV[*s] != u)
			shriek(812, fmt("Ambiguous syntactic function of %c",*s));
		CHRLEV[*s] = u;
		TRANSL_INPUT[*s] = (unsigned char)*s;
	}
}

void
parser::alias(const char *canonicus, const char *alius)
{
	int i;
	if (!canonicus || !alius) shriek(861, "Parser configuration: Aliasing NULL");
	for(i=0; (unsigned char *)canonicus[i] && (unsigned char *)alius[i]; i++)
		TRANSL_INPUT[((unsigned char *)alius)[i]] = ((unsigned char *)canonicus)[i];
	if((unsigned char *)canonicus[i] || (unsigned char *)alius[i]) 
		shriek(861, "Parser configuration: Can't match aliases");
}


void
parser::initables(SYMTABLE table)
{
	int c;
	UNIT u = cfg->phone_level;
//	if (cfg->relax_input) for (c=1; c<256; c++) TRANSL_INPUT[c] = cfg->dflt_char;
	for(c=0; c<256; c++) TRANSL_INPUT[c] = (unsigned char)c;
	for(c=1; c<256; c++) CHRLEV[c] = U_ILL; *CHRLEV = cfg->text_level;
	switch (table) {
	case ST_ROOT:
		alias("  ","\r\t");
	case ST_RAW:
		for (u = cfg->phone_level; u < cfg->text_level; u = (UNIT)(u+1))
			regist(u, this_lang->perm[u]);
//		regist(U_WORD, " ");
		break;
	case ST_EMPTY:
		for(c=1; c<256; c++) CHRLEV[c] = U_VOID; *CHRLEV = cfg->text_level;
		break;
	default: shriek(861, fmt("Garbage passed to parser::initables, %d", table));
	}
}
