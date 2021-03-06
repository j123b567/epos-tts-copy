/*
 *	epos/src/encoding.cc
 *	(c) 2001 geo@cuni.cz
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
 
#ifndef FORGET_CHARSETS
  
#include "common.h"

#define UNUSED 0
#define UNDEFINED 0

char *charset_list = NULL;
int charset_list_len = 0;

wchar_t epos_charset[256];		/* Epos to unicode */
wchar_t **charsets = 0;			/* charset to unicode */

unsigned char **encoders = 0;		/* charset to Epos */
unsigned char **decoders = 0;		/* Epos to charset */


void init_enc()
{
	encoders = (unsigned char **)xmalloc(sizeof(unsigned char *));
	decoders = (unsigned char **)xmalloc(sizeof(unsigned char *));
	charsets = (wchar_t **)xmalloc(sizeof(wchar_t *));
	for (int i = 0; i < 256; i++) epos_charset[i] = UNUSED;
}

void shutdown_enc()
{
	for (int i = 0; i < charset_list_len; i++) {
		free(encoders[i]);
		free(decoders[i]);
		free(charsets[i]);
	}
	free(encoders);
	free(decoders);
	free(charsets);
	free(charset_list);
	
	encoders = NULL;
	decoders = NULL;
	charsets = NULL;
	charset_list = NULL;
	charset_list_len = 0;
}

/*
 *	internal_code()  translates a wide character to an internal charcode;
 *		it allocates a new charcode as necessary.
 *		The hint is based on the original encoding and it is used as
 *		the preferred allocation point for the character.
 */

static inline int internal_code(wchar_t c, unsigned char hint)
{
	if (epos_charset[hint] == c) return hint;
	for (int i = 255; i > -1; i--) {
		if (epos_charset[i] == c) return i;
	}
	return UNDEFINED;
}

static inline unsigned char do_alloc_code(wchar_t c, unsigned char hint)
{
	int x = internal_code(c, hint);
	if (x != UNDEFINED) return x;

	if (epos_charset[hint] == UNUSED) return hint;
	for (int i = 255; i ; i--) {
		if (epos_charset[i] == UNUSED) {
			DEBUG(2,10,fprintf(STDDBG, "Allocating charcode unicode %d, original enc %d, internal enc %d\n", c, hint, i);)
			return i;
		}
	}
	DEBUG(2,10,fprintf(STDDBG, "Want to allocate charcode unicode %d, original enc %d\n", c, hint);)
	shriek(447, "Epos internal character table overflow");
	return UNDEFINED;
}

static inline unsigned char alloc_code(wchar_t c, unsigned char hint, int cs)
{
	int orig = hint;
	hint = do_alloc_code(c, hint);

	epos_charset[hint] = c;
	encoders[cs][orig] = hint;
	decoders[cs][hint] = orig;
	return hint;
}

/*
 *	non_unicode_alloc_code() is generally usable for handling any necessary extensions
 *	beyond unicode.  We however use it only to supplement any charset which leaves
 *	character codes 0 to 31 unspecified with an identical mappings at these characters,
 *	which conforms to the unicode standard as well as the way these charsets are used.
 */

static inline wchar_t non_unicode_alloc_code(unsigned char request)
{
	DEBUG(2,10,fprintf(STDDBG, "Allocating charcode, non-unicode character %d\n", request);)
	if (request < ' ' && epos_charset[request] == UNUSED || epos_charset[request] == request) return request;
	else return UNDEFINED;
}

int get_count_allocated()
{
	int result = 0;
	for (int i = 0; i < 256; i++) if (epos_charset[i] != UNUSED) result++;
	return result;
}

void encode_string(unsigned char *s, int cs, bool alloc)
{
	do {
		int t = encoders[cs][*s];
		if (t == UNDEFINED && *s) {
			if (alloc) {
				wchar_t u = charsets[cs][*s];
				if (u == UNDEFINED) u = non_unicode_alloc_code(*s);
				if (u == UNDEFINED)
					shriek(418, fmt(cs ? "Illegal character %c" : "Unspecified charset for character %c", *s));  // FIXME - filename
				alloc_code(u, *s, cs);
				continue;
			} else if (cfg->relax_input) {
				if (encoders[cs][(unsigned char)cfg->dflt_char] == UNDEFINED)
					shriek(431, "You specified relax_input but default_char is undefined");
				else *s = encoders[cs][(unsigned char)cfg->dflt_char];
			} else shriek(431, fmt("Parsing an unhandled character  '%c' - code %d", (unsigned int) *s, (unsigned int) *s));
		} else *s = t;
	} while (*s++);
}

void decode_string(unsigned char *s, int cs)
{
	do {
		int t = decoders[cs][*s];
		if (t == UNDEFINED && *s) shriek(461, fmt("Character '%c', code %d infiltrated internal encoding", *s));
		else *s = t;
	} while (*s++);
}


void alloc_charset(const char *name)
{
	char *p;
	if (charset_list) {
		p = (char *)xrealloc(charset_list, strlen(charset_list) + strlen(name) + 2);
		p[strlen(p) + 1] = 0;
		p[strlen(p)] = LIST_DELIM;
	} else {
		p = (char *)xmalloc(strlen(name) + 1);
		*p = 0;
	}
	strcat(p, name);
	charset_list = p;
	int idx = charset_list_len;
	charset_list_len++;
	
	encoders = (unsigned char **)realloc(encoders, charset_list_len * sizeof(unsigned char *));
	decoders = (unsigned char **)realloc(decoders, charset_list_len * sizeof(unsigned char *));
	charsets = (wchar_t **)realloc(charsets, charset_list_len * sizeof(wchar_t *));

	encoders[idx] = (unsigned char *)xmalloc(256);
	decoders[idx] = (unsigned char *)xmalloc(256);
	charsets[idx] = (wchar_t *)xmalloc(256 * sizeof(wchar_t));
	memset(encoders[idx], UNDEFINED, 256);
	memset(decoders[idx], UNDEFINED, 256);
	for (int i = 0; i < 256; i++) charsets[idx][i] = UNDEFINED;
}

int load_charset(const char *name)
{
	UNIT u = str2enum(name, charset_list, U_ILL);
	if (u != U_ILL) return (int)u;
	
	DEBUG(3,10,fprintf(STDDBG, "Loading charset %s\n", name);)
	char *p = (char *)xmalloc(strlen(name) + 5);
	strcpy(p, name);
	strcat(p, ".txt");
	text *t = new text(p, cfg->unimap_dir, "", NULL, true);
	free(p);
	if (!t->exists()) {
		delete t;
		if (!charsets) shriek(844, fmt("Couldn't find unicode map for initial charset %s", name));
		return CHARSET_NOT_AVAILABLE;
	}
	char *line = (char *)xmalloc(cfg->max_line);
	alloc_charset(name);
	unsigned char *c = encoders[charset_list_len - 1];
	unsigned char *d = decoders[charset_list_len - 1];
	wchar_t *w = charsets[charset_list_len - 1];
	while(t->getline(line)) {
		int b, x, dummy;
		if (sscanf(line, "%i %i %i", &b, &x, & dummy) != 2) continue;
		if (b < 0 || b >= 256) shriek(463, fmt("unicode map file %s maps out of range characters", name));
		int e = internal_code(x, b);
		w[b] = x;
		if (e == UNDEFINED) continue;
		c[b] = e;
		d[e] = b;
	}
	delete t;

//	if (cfg->paranoid) for (int i = 1; i < 256; i++) if (w[i] == UNDEFINED)	FIXME
//		shriek(462, fmt("Character code %d left undefined for charset %s", i, name));
	return charset_list_len - 1;
}

void load_default_charset()
{
	init_enc();
	alloc_charset("default");
	for (int i = 0; i < 128; i++) {		/* all ASCII chars automatically allocated */
		encoders[0][i] = i;
		decoders[0][i] = i;
		charsets[0][i] = i;
	}
}

#endif	// FORGET_CHARSETS
