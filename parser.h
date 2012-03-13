/*
 *	ss/src/parser.h
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
 *	We'll later #define PARSER simpleparser - we might wish to
 *      work with more sophisticated parsers later. Yeah, it's crude.
 */

#ifndef SS_SIMPLE_PARSER
#define SS_SIMPLE_PARSER

class simpleparser
{
	Char *text;           //allocated dynamically
	Char *current;        //the byte to be parsed next
	unsigned txtlen;      //length of text (not counting \0)
	UNIT CHRLEV[256];     //to be initialized in the constructor
	Char TRANSL_INPUT[256];
			  //to be initialized in the constructor
	void initables(SYMTABLE to_load);    //fills in CHRLEV
	void alias(const char *canon, const char *alias);
					     //fills in TRANSL_INPUT
	void regist(UNIT assigned_function, const char* list);
  public:
	simpleparser(const char *s);         // s is a raw string, use ST_RAW
	simpleparser(const char *s, int mode);
	void init(SYMTABLE symtab);          // the common constructor
	~simpleparser();
	UNIT level;           //contains the UNIT level of the next symbol
	Char getch();         //gets the next symbol
	UNIT chrlev(Char c);  //what level c is to be analysed at
	void done();          //shriek if some input left
};

#endif                    //#ifndef SS_SIMPLE_PARSER
