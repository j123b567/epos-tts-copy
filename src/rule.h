/*
 *	epos/src/rules.h
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
 *	This file defines how a rules file should be parsed. 
 *
 *	Note that you may receive quite random behavior if
 *	you call rules::apply() for any other rule set than that
 *	of this_lang->ruleset. this_voice must be set to a voice
 *	of the same language as well. This is because r_if may
 *	refer to a soft option and the set of available soft options
 *	is language dependent.
 */

#define DIPH_BUFF_SIZE  1000 //unimportant

#define ESCAPE		'\\'
#define EXCL		'!'
#define PSEUDOSPACE	'\377'

#define OPCODEstr "subst:regex:postp:prep:diphones:prosody:contour:progress:regress:insert:syll:smooth:raise:debug:if:inside:with:{:}:[:]:<:>:nothing:error:"
enum OPCODE {OP_SUBST, OP_REGEX, OP_POSTP, OP_PREP, OP_DIPH, OP_PROSODY, OP_CONTOUR, OP_PROGRESS, OP_REGRESS, 
	OP_INSERT, OP_SYLL, OP_SMOOTH, OP_RAISE, OP_DEBUG, OP_IF, OP_INSIDE, OP_WITH,
	OP_BEGIN, OP_END, OP_CHOICE, OP_CHOICEND, OP_SWITCH, OP_SWEND, OP_NOTHING, OP_ERROR};
		/* OP_BEGIN, OP_END and other OP's without parameters should come last
		   OP_ERROR would abort the compilation (never used)			*/
enum RULE_STATUS {RULE_OK, RULE_IGNORED, RULE_BAD=-1};

extern char * _rules_buff;
extern char * _next_rule_line;
extern char * _resolve_vars_buff;

extern char * global_current_file;
extern int    global_current_line;

class rule;
class r_block;

class rules
{
    public:
	int  current_rule;      //currently processed rule (in apply())
	r_block *body;		//contains pointers to the individual rules
	
	rules(const char *filename, const char *dirname);
	~rules();
	void apply(unit *root);
	void debug();       //dumps all rules
};

rule *next_rule(text *file, hash *vars, int *count);	// count is an out-parameter
