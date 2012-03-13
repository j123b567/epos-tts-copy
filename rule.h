/*
 *	ss/src/rules.h
 *	(c) geo@ff.cuni.cz
 *
 *	This file defines how a rules file should be parsed. 
 */

#define DIPH_BUFF_SIZE  1000 //unimportant

#define ESCAPE		'\\'
#define EXCL		'!'
#define PSEUDOSPACE	'\377'

#define OPCODEstr "subst:regex:postp:prep:diphones:prosody:progress:regress:insert:syll:smooth:raise:debug:if:inside:{:}:[:]:error:"
enum OPCODE {OP_SUBST, OP_REGEX, OP_POSTP, OP_PREP, OP_DIPH, OP_PROSODY, OP_PROGRESS, OP_REGRESS, 
	OP_INSERT, OP_SYLL, OP_SMOOTH, OP_RAISE, OP_DEBUG, OP_IF, OP_INSIDE,
	OP_BEGIN, OP_END, OP_CHOICE, OP_CHOICEND, OP_ERROR};
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
	
	rules(const char *filename, int to_be_ignored);
	~rules();
	void apply(unit *root);
	void debug();       //dumps all rules
};

rule *next_rule(text *file, hash *vars, int *count);	// count is an out-parameter
