/*
 *	ss/src/text.h
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
 *	This file provides a text file abstraction. It provides the handling of
 *	preprocessing directives, such as #include "..." .
 */

#ifndef SS_TEXT
#define SS_TEXT

extern hash *_directive_prefices;		//called only by ss_done()

struct textlink;

class text
{
	textlink *current;        //the byte to be parsed next
	const char *dir;
	char *base;     	//the filename given to the constructor
	bool warn;		//whether to heed or ignore #warn
	int embed;		//subfiling depth 
	
	void subfile(const char *filename);
	void superfile();
  public:
  	text(const char *filename, const char *dirname, bool warnings);
	~text();
	bool getline(char *line);    // return true on success, false on EOF
	void done();		//shriek if some input left
	void rewind();		//let's start again
	void rewind(bool warnings);
	
	char *current_file;
	int   current_line;
};

#endif                    //#ifndef SS_TEXT
