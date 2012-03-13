/*
 *	ss/src/parser.cc
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

#include"common.h"

/*
 *	If mode is 0, filename is truly a filename. If mode is 1, filename is
 *	really the contents, NOT a filename. I may fix this strange scheme later.
 *
 */

parser::parser(const char *filename, int mode)
{
	if (!cfg->loaded) ss_init();
	if (!filename || !*filename) {
		register signed char c;
		int i = 0;
		text = (unsigned char *)malloc(cfg->dev_txtlen+1);
		if(!text) shriek("Parser: Out of memory");
		do text[i++] = c = getchar(); 
			while(c!=-1 && c!=cfg->eof_char && i<cfg->dev_txtlen);
		text[--i] = 0;
		txtlen = i;
	} else { 
		if (mode) text = (unsigned char *) strdup(filename);
		else {
			file *f = claim(filename, this_lang->input_dir, "rt", "input text");
			text = (unsigned char *)strdup(f->data);
			unclaim(f);
		}
		txtlen = strlen((char *)text);
	}
	DEBUG(2,7,fprintf(stddbg,"Allocated %u bytes for the main parser\n", txtlen);)
	init(ST_ROOT);
}

parser::parser(const char *str)
{
	if (!cfg->loaded) ss_init();
	DEBUG(1,7,fprintf(stddbg,"Parser for %s is to be built\n",str);)
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
	DEBUG(1,7,fprintf(stddbg,"Parser: has set up with %s\n",text);)
		//TRANSL_INPUT should be filled in before, in this constructor.
		//    It's meant to be altered when we later decide to use
		//    another Czech char encoding.
	current = text-1; 
	do level = chrlev(*++current); while (level > U_PHONE && level < U_TEXT);
		//We had to skip any garbage before the first phone

	gettoken();		// new - hope this works
//	f = i = t = 0;

	DEBUG(0,7,fprintf(stddbg,"Parser: initial level is %u.\n",level);)
/*	if (level == U_TEXT) {		// This should go away sooner or later
		DEBUG(2,7,fprintf(stddbg,"Parser: is empty\n");)
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
		DEBUG(2,7,fprintf(stddbg,"Parser: end of text reached, changing the policy\n");)
		initables(ST_EMPTY);
		*current = NO_CONT;  // return '_' or something instead of quirky '\0'
	}
	DEBUG(0,7,fprintf(stddbg,"Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n",retchar, lastlev, *current, level);)

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
					token = POINT;
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
			case '<': if (cfg->stml) shriek("STML not implemented"); else break;
			case '&': if (cfg->stml) shriek("STML not implemented"); else break;
			default : ;
		}
		level = chrlev(token);
		f = i = t = 0;
		if (level == U_TEXT) {
			DEBUG(2,7,fprintf(stddbg,"Parser: end of text reached, changing the policy\n");)
			initables(ST_EMPTY);
			*current = NO_CONT;  // return '_' or something instead of quirky '\0'
		}
		current++;
	} while (level <= lastlev && level > U_PHONE);
		// (We are skipping any empty units, except for phones.)
	DEBUG(0,7,fprintf(stddbg,"Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n", ret, lastlev, *current, level);)

	return ret;
}

void
parser::done()
{
	if (current <= text + txtlen) 
		shriek("Too high level symbol in a dictionary, parser contains %s", (char *)text);
}

UNIT
parser::chrlev(unsigned char c)
{
	if (current > text+txtlen+1) /*(!current[-1] && c))*/ return(U_VOID);
	if (CHRLEV[c] == U_ILL)
	{
		if (c>127) fprintf(stdshriek,"Seems you're mixing two Czech character encodings?\n");
		fprintf(stdshriek,"Fatal: parser dumps core.\n%s\n",(char *)current-2);
		shriek("Parsing an unhandled character - ASCII code %d", (unsigned int) c);
	}
	return(CHRLEV[c]);
}

void
parser::regist(UNIT u, const char *list)
{
	unsigned char *s;
	if (!list) shriek ("Parser configuration: No characters for level %d", u);
	for(s=(unsigned char *)list;*s!=0;s++)
	{
		if (CHRLEV[*s] != U_ILL && CHRLEV[*s] != u)
			shriek("Ambiguous syntactic function of %c",*s);
		CHRLEV[*s] = u;
		TRANSL_INPUT[*s] = (unsigned char)*s;
	}
}

void
parser::alias(const char *canonicus, const char *alius)
{
	int i;
	if (!canonicus || !alius) shriek ("Parser configuration: Aliasing NULL");
	for(i=0; (unsigned char *)canonicus[i] && (unsigned char *)alius[i]; i++)
		TRANSL_INPUT[((unsigned char *)alius)[i]] = ((unsigned char *)canonicus)[i];
	if((unsigned char *)canonicus[i] || (unsigned char *)alius[i]) 
		shriek("Parser configuration: Can't match aliases");
}


void
parser::initables(SYMTABLE table)
{
	int c;
	UNIT u = U_PHONE;
//	if (cfg->relax_input) for (c=1; c<256; c++) TRANSL_INPUT[c] = cfg->dflt_char;
	for(c=0; c<256; c++) TRANSL_INPUT[c] = (unsigned char)c;
	for(c=1; c<256; c++) CHRLEV[c] = U_ILL;*CHRLEV = U_TEXT;
	switch (table) {
	case ST_ROOT:
		alias("   ","\n\r\t");
	case ST_RAW:
		for (u = U_PHONE; u < U_TEXT; u = (UNIT)(u+1))
			regist(u, this_lang->perm[u]);
		regist(U_WORD, " ");
		break;
	case ST_EMPTY:
		for(c=1; c<256; c++) CHRLEV[c] = U_VOID; *CHRLEV = U_TEXT;
		break;
	default: shriek("Garbage passed to parser::initables, %d", table);
	}
}
