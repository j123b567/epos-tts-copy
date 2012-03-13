/*
 *	epos/src/parser.h
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
 *	This file defines how an input file or string is to be parsed. 
 *	It's sole function is to tell the constructor of "unit"
 *	which UNIT (depth) should be the next unit at any given moment.
 *	The constructor will then either return to parent, descend to
 *	a new-born child, or advance to the next character in the parser.
 *
 */

class parser
{
	unsigned char *text;	// allocated dynamically
	unsigned char *current;	// the byte to be parsed next
	unsigned char token;	// the current token
	unsigned int txtlen;	// length of text (not counting \0)
	UNIT CHRLEV[256];	// to be initialized in the constructor
	unsigned char TRANSL_INPUT[256];
			  //to be initialized in the constructor
	void initables(SYMTABLE to_load);    // fills in CHRLEV
	void alias(const char *canon, const char *alias);
					     // fills in TRANSL_INPUT
	void regist(UNIT assigned_function, const char* list);
  public:
	parser(const char *s);         // s is a raw string, use ST_RAW
	parser(const char *s, int mode);
	void init(SYMTABLE symtab);          // the common constructor
	~parser();
	UNIT level;		// contains the UNIT level of the next symbol
	int f, i, t;		// f, i, t of the next symbol (usually zero)
	unsigned char gettoken(); // gets the next symbol
	UNIT chrlev(unsigned char c);  // what level c is to be analysed at
	void done();		// shriek if some input left
};

#define NO_CONT            '_'    // null contents of a unit

/*
 *	Special tokens added to ASCII instead of some absurd ctrl chars:
 *	DOTS	 ...	\~
 *	DECPOINT 1.3	\.
 *	RANGE	 1-3	\-
 *	MINUS	  -3	\m
 *	
 *	_INTERNAL	\X	(never generated by the parser)
 *
 *	Two or more '-''s are always output as a single '-' token
 *
 *	These tokens are generated by parser::gettoken() and reverted
 *	to their original form by fmtchar(). It is possible to refer to
 *	them in rule files and .ini files by the escape sequences just
 *	shown.
 *
 *	Adding special tokens: parser::gettoken(), fmtchar(), .ini
 *	files must recognize the token (probably as a perm_phone), for
 *	which you have to add some intuitive escape sequence to option.cc,
 *	namely to token_esc and value_esc defaults.
 *	Then add some appropriate handling within the rules.
 */

#define DOTS	 1
#define DECPOINT 2
#define	RANGE	 3
#define MINUS	 4

#define _INTERNAL	31
