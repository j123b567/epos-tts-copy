/*
 *	epos/src/text.cc
 *	(c) 1997-99 geo@ff.cuni.cz
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

enum DIRECTIVE {DI_INCL, DI_WARN, DI_ERROR};

#define PP_ESCAPE	'@'
#define D_INCLUDE	"include"
#define D_WARN		"warn"
#define D_ERROR		"error"

//#define DIRECTIVEstr "@include:@warn:@error:"
//#define MAX_DIRECTIVE_LEN 16		//Keep in sync with text::getline's format string
#define MAX_INCL_EMBED	32

#define SPECIAL_CHARS "#;\n\\"

// hash *_directive_prefices=NULL;

/*

void doneprefices()
{
	if (!cfg->lowmemory) return;	//will result in a small memory leak
	delete _directive_prefices;
	_directive_prefices=NULL;
};

*/

void strip(char *s)
{
	char *r;
	char *t;

	for (r=t=s + strcspn(s, SPECIAL_CHARS); *t; t++, r++) {
		switch (*r = *t) {
			case ';':
			case '#':
				if (s != r && !strchr(WHITESPACE, r[-1])) break;	// ??? nothing happens
			case '\n':
//				while (r>s && strchr(WHITESPACE, *--r));
				*r = 0;
				return;
			case ESCAPE:
				if (!*++t || *t=='\n') shriek(462, "text.cc still cannot split lines");
				if (strchr(cfg->token_esc, *t)) *r = esctab[*t];
				else t--;
//				else r++;
				break;
			default:;
		}
	}
	*r = 0;
}

struct textlink {
	FILE *f;
	textlink *prev;
	char *filename;
	int line;
};

text::text(const char *filename, const char *dirname, const char *treename,
				const char *description, bool warnings)
{
//	if (!_directive_prefices) 
//		_directive_prefices = str2hash(DIRECTIVEstr, MAX_DIRECTIVE_LEN);
	dir = dirname;
	tree = treename;
	tag = description;
	base = strdup(filename);
	warn = warnings;
	embed = 0;
	current = NULL;
	current_file = NULL;
	current_line = 0;
	subfile(base);
}

void
text::subfile(const char *filename)
{
	textlink *parent;


	parent=current;
	current=new textlink;
	if (!current) shriek(422, "text::subfile out of memory");
	if (!filename || !*filename) {
		current->f=stdin;
	} else { 
		char *pathname=compose_pathname(filename, dir, tree);
		DEBUG(2,1,fprintf(STDDBG,"Text preprocessor opening %s\n", pathname);)
		current->f=fopen(pathname, "r", tag);
		free(pathname);
	}
	current->prev=parent;
	current->filename=current_file;
	current->line=current_line;
	current_file=strdup(filename);
	current_line=0;
	if (++embed>MAX_INCL_EMBED) shriek(812, "infinite #include cycle");
}

void
text::superfile()
{
	textlink *tmp;
	free(current_file);
	current_line=current->line;
	current_file=current->filename;
	fclose(current->f);
	tmp=current;
	current=tmp->prev;
	embed--;
	delete tmp;
}

bool
text::exists()
{
	return current && current->f;
}

inline bool begins(char *buffer, char *s)
{
	return !strncasecmp(buffer + strspn(buffer, WHITESPACE) + 1, s, strlen(s));
}

bool
text::getline(char *buffer)
{
//	static char wordbuff[MAX_DIRECTIVE_LEN+1];
	char *tmp1;
	char *tmp2;

	if (!current) return false;	// EOF, again
	
	while (true) {
		while(!fgets(buffer,cfg->max_line,current->f)) {
			superfile();
			if (!current) return false;
		}
		current_line++;
		global_current_line = current_line;
		global_current_file = current_file;
		
		DEBUG(1,1,fprintf(STDDBG,"text::getline processing %s",buffer);)
//		*(int *)wordbuff = 0; sscanf(buffer,"%16s",wordbuff);
		strip(buffer);
//		buffer[strcspn(buffer, COMMENT_LINES)]=0;
//		switch(_directive_prefices->translate_int(wordbuff)) {
//		switch(str2enum(buffer + strspn(buffer, WHITESPACE), DIRECTIVEstr, 0)) {
/*
			case DI_INCL:
				tmp1=strchr(buffer+1, DQUOT);
				if (!tmp1) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
				tmp2=strchr(++tmp1,DQUOT);
				if (!tmp2) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
				*tmp2=0;
				subfile(tmp1);
				continue;
			case DI_WARN:
				if (warn) fprintf(cfg->stdshriek,
					"%s\n",buffer+1+strcspn(buffer+1, WHITESPACE));
				continue;
			case DI_ERROR:
				tmp1=strchr(buffer+1, DQUOT);
				if (!tmp1) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
				tmp2=strchr(++tmp1,DQUOT);
				if (!tmp2) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
				*tmp2=0;
				shriek(801, tmp1);
			default:
				DEBUG(0,1,fprintf(STDDBG,"text::getline default is %s\n",buffer);)
				if (!buffer[strspn(buffer,WHITESPACE)]) continue;
				return true;
*/
		if (buffer[strspn(buffer, WHITESPACE)] != PP_ESCAPE) {
			DEBUG(0,1,fprintf(STDDBG,"text::getline default is %s\n",buffer);)
			if (!buffer[strspn(buffer,WHITESPACE)]) continue;
			return true;
		}
		if (begins(buffer, D_INCLUDE)) {
			tmp1=strchr(buffer+1, DQUOT);
			if (!tmp1) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
			tmp2=strchr(++tmp1,DQUOT);
			if (!tmp2) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
			*tmp2=0;
			subfile(tmp1);
			continue;
		} else if (begins(buffer, D_WARN)) {
			if (warn) fprintf(cfg->stdshriek,
				"%s\n",buffer+1+strcspn(buffer+1, WHITESPACE));
			continue;
		} else if (begins(buffer, D_ERROR)) {
		
			tmp1=strchr(buffer+1, DQUOT);
			if (!tmp1) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
			tmp2=strchr(++tmp1,DQUOT);
			if (!tmp2) shriek (812, fmt("Forgotten quotes in file %s line %d", current_file, current_line));
			*tmp2=0;
			shriek(801, tmp1);
		} else shriek(812, fmt("Bad directive in file %s line %d", current_file, current_line));
	}
}

void
text::rewind()
{
	if (cfg->paranoid) done();
	subfile(base);
}

void
text::rewind(bool warnings)
{
	text::rewind();
	warn=warnings;
}

text::~text()
{
	done();
	free(base);
//	doneprefices();
};

void
text::done()
{
	if (current && exists()) shriek(812, fmt("File %s was left prematurely at line %d", base, current_line));
}
