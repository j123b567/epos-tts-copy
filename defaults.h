/*
 *	ss/src/defaults.h
 *	(c) geo@ff.cuni.cz
 *
 *	This file is the sole thing that hash.* need to include. 
 *	I'm trying to keep it small to make hash.* as independent
 *	of the rest as practical. Furthermore, if you don't have
 *	to construct hash tables out of a file, you don't need
 *	this header file at all.
 */

void shriek (const char *err_mess, const char * filename, int line_number);

#define MAX_WORD_SIZE   256       // in the input or dictionaries (not reported if larger)

extern const char* COMMENT_LINES;    // these characters can start a comment
				     // but text.cc has an independent definition
extern const char* WHITESPACE;       // i.e. space and tab
extern int hash_max_line;		     //maximum line length in i/o files

#define HASH_CAN_READ_FILES      // just to make clear that we've included this file

#ifdef  bool
#define true 	1
#define false	0
#endif
