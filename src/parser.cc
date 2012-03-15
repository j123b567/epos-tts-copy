/*
 *	epos/src/parser.cc
 *	(c) 1996-98 geo@cuni.cz
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

//#ifdef A_HAIRY_FILE_PARSER

/*
 *	If mode is 0, filename is truly a filename. If mode is 1, filename is
 *	really the contents, NOT a filename. I may fix this strange scheme later.
 *
 */

parser::parser(const char *filename, int mode)
{
	if (!filename || !*filename) {
		register signed char c;
		int i = 0;
		text = (unsigned char *)xmalloc(cfg->dev_txtlen+1);
		if(!text) shriek(422, "Parser: Out of memory");
		do text[i++] = c = getchar(); 
			while(c!=-1 && c!=scfg->eof_char && i<cfg->dev_txtlen);
		text[--i] = 0;
		txtlen = i;
	} else { 
		if (mode) text = (unsigned char *) strdup(filename);
		else {
			file *f = claim(filename, this_lang->input_dir, scfg->lang_base_dir, "rt", "input text", NULL);
			text = (unsigned char *)strdup(f->data);
			unclaim(f);
		}
		txtlen = strlen((char *)text);
	}
	D_PRINT(2, "Allocated %u bytes for the main parser\n", txtlen);
	init(ST_ROOT);
}

parser::parser(const char *str)
{
	D_PRINT(1, "Parser for %s is to be built\n",str);
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
	D_PRINT(1, "Parser: has set up with %s\n",text);
	current = text-1; 
	do level = chrlev(*++current); while (level > scfg->phone_level && level < scfg->text_level);
		//We had to skip any garbage before the first phone

	token = '0'; gettoken();		// new - hope this works
	t = 1;

	D_PRINT(0, "Parser: initial level is %u.\n",level);
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
		D_PRINT(2, "Parser: end of text reached, changing the policy\n");
		initables(ST_EMPTY);
		*current = NO_CONT;  // return '_' or something instead of quirky '\0'
	}
	D_PRINT(0, "Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n",retchar, lastlev, *current, level);

	return(retchar);           // return the old char
}

***********/

#define IS_DIGIT(x)	((x)>='0' && (x)<='9')
#define IS_ASCII_LOWER_ALPHA(x) ((x)>='a' && (x)<='z')

unsigned char
parser::gettoken()
{
	if (!token) return NO_CONT;

	UNIT lastlev = level;
	unsigned char ret = token;
	do {
		switch (token = *current) {
			case '.':
				if (IS_DIGIT(current[1])) {
					token = DECPOINT;
					break;
				}
				if (IS_ASCII_LOWER_ALPHA(current[1])) {
					token = URLDOT;
					break;
				}
				if (current[1] == '.' && current[2] == '.') {
					current += 2;
					token = DOTS;
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
//			case '<': if (cfg->stml) shriek(462, "STML not implemented"); else break;
//			case '&': if (cfg->stml) shriek(462, "STML not implemented"); else break;
			default : ;
		}
		if (CHRLEV[token] == U_ILL && cfg->relax_input) token = cfg->dflt_char;
		level = chrlev(token);
//		f = i = t = 0;
		t = 1;
//		if (level == scfg->text_level) {
//			D_PRINT(2, "Parser: end of text reached, changing the policy\n");
//			return ret;
//		}
		current++;
	} while (level <= lastlev && level > scfg->phone_level && level < scfg->text_level);
		// (We are skipping any empty units, except for phones.)
	D_PRINT(0, "Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n", ret, lastlev, *current, level);

	return ret;
}

void
parser::done()
{
	if (current < text + txtlen) 
		shriek(463, fmt("Too high level symbol in a dictionary, parser contains %s", (char *)text));
}

UNIT
parser::chrlev(unsigned char c)
{
	if (current > text+txtlen+1) /*(!current[-1] && c))*/ return(U_VOID);
	if (CHRLEV[c] == U_ILL)
	{
		if (cfg->relax_input && CHRLEV[cfg->dflt_char] != U_ILL) return CHRLEV[cfg->dflt_char];
		DBG(2, fprintf(cfg->stdshriek,"Fatal: parser dumps core.\n%s\n",(char *)current-2);)
		shriek(431, fmt("Parsing an unhandled character  '%c' - ASCII code %d", (unsigned int) c, (unsigned int) c));
	}
	return(CHRLEV[c]);
}

void
parser::regist(UNIT u, const char *list)
{
	unsigned char *s;
	if (!list) return;
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
	for(i=0; canonicus[i] && alius[i]; i++)
		TRANSL_INPUT[((unsigned char *)alius)[i]] = ((unsigned char *)canonicus)[i];
	if(canonicus[i] || alius[i]) 
		shriek(861, "Parser configuration: Can't match aliases");
}


void
parser::initables(SYMTABLE table)
{
	int c;
	UNIT u = scfg->phone_level;
	for(c=0; c<256; c++) TRANSL_INPUT[c] = (unsigned char)c;
	for(c=1; c<256; c++) CHRLEV[c] = U_ILL; *CHRLEV = scfg->text_level;
	switch (table) {
	case ST_ROOT:
		alias(" \n","\t\r");
	case ST_RAW:
		for (u = scfg->phone_level; u < scfg->text_level; u = (UNIT)(u+1))
			regist(u, this_lang->perm[u]);
//		regist(U_WORD, " ");
		break;
//	case ST_EMPTY:
//		for(c=1; c<256; c++) CHRLEV[c] = U_VOID; *CHRLEV = scfg->text_level;
//		break;
	default: shriek(861, fmt("Garbage passed to parser::initables, %d", table));
	}
}
