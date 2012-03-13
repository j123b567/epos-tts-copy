/*
 *	ss/src/text.cc
 *	(c) 1997 geo@ff.cuni.cz
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

#define DIRECTIVEstr "#include:#warn:#error:"
#define MAX_DIRECTIVE_LEN 16		//Keep in sync with text::getline's format string
#define MAX_INCL_EMBED	32

#define SPECIAL_CHARS "#;\n\\"

hash *_directive_prefices=NULL;

void doneprefices()
{
	if (!cfg->lowmemory) return;	//will result in a small memory leak
	delete _directive_prefices;
	_directive_prefices=NULL;
};

void strip(char *s)
{
	char *r;
	char *t;

	for (r=t=s + strcspn(s, SPECIAL_CHARS); *t; t++, r++) {
		switch (*r = *t) {
			case ';':
			case '#':
			case '\n':
//				while (r>s && strchr(WHITESPACE, *--r));
				*r = 0;
				return;
			case '\\':
				if (!*t) shriek("text.cc still cannot split lines");
				if (strchr(cfg->token_esc, t[1]))
				*r = esctab[*++t];
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

text::text(const char *filename, const char *dirname, bool warnings)
{
	if (!cfg->loaded) ss_init(0, NULL);
	if (!_directive_prefices) 
		_directive_prefices = str2hash(DIRECTIVEstr, MAX_DIRECTIVE_LEN);
	dir = dirname;
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
	if (!current) shriek("text::subfile out of memory");
	if (!filename || !*filename) {
		current->f=stdin;
	} else { 
		char *pathname=compose_pathname(filename, dir);
		DEBUG(1,1,fprintf(stddbg,"Text preprocessor opening %s\n", pathname);)
		current->f=fopen(pathname, "r", "rules");
		free(pathname);
	};
	current->prev=parent;
	current->filename=current_file;
	current->line=current_line;
	current_file=strdup(filename);
	current_line=0;
	if (++embed>MAX_INCL_EMBED) shriek("infinite #include cycle");
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
text::getline(char *buffer)
{
	static char wordbuff[MAX_DIRECTIVE_LEN+1];
	char *tmp1;
	char *tmp2;

	if (!current) return false;	// EOF, again
	
	while (true) {
		while(!fgets(buffer,cfg->max_line,current->f)) {
			superfile();
			if (!current) return false;
		};
		current_line++;
		global_current_line = current_line;
		global_current_file = current_file;
		
		DEBUG(1,1,fprintf(stddbg,"text::getline processing %s",buffer);)
		*(int *)wordbuff = 0; sscanf(buffer,"%16s",wordbuff);
		strip(buffer);
//		buffer[strcspn(buffer, COMMENT_LINES)]=0;
		switch(_directive_prefices->translate_int(wordbuff)) {
			case DI_INCL:
				tmp1=strchr(buffer+1, DQUOT);
				if (!tmp1) shriek ("Forgotten quotes in file %s line %d", current_file, current_line);
				tmp2=strchr(++tmp1,DQUOT);
				if (!tmp2) shriek ("Forgotten quotes in file %s line %d", current_file, current_line);
				*tmp2=0;
				subfile(tmp1);
				continue;
			case DI_WARN:
				if (warn) fprintf(stdwarn,"%s\n",buffer+1+strcspn(buffer+1, WHITESPACE));
				continue;
			case DI_ERROR:
				tmp1=strchr(buffer+1, DQUOT);
				if (!tmp1) shriek ("Forgotten quotes in file %s line %d", current_file, current_line);
				tmp2=strchr(++tmp1,DQUOT);
				if (!tmp2) shriek ("Forgotten quotes in file %s line %d", current_file, current_line);
				*tmp2=0;
				shriek(tmp1);
			default:
				DEBUG(1,1,fprintf(stddbg,"text::getline default is %s\n",buffer);)
				if (!buffer[strspn(buffer,WHITESPACE)]) continue;
				return true;
		};
	};
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
	doneprefices();
};

void
text::done()
{
	if (current) shriek("File %s was left prematurely at line %d", base, current_line);
}
