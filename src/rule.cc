/*
 *	epos/src/rule.cc
 *	(c) 1996-99 geo@cuni.cz
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
 *	This file includes block.cc at the very last line.
 *
 */

#include "common.h"

#define DIPH_BUFF_SIZE  1000 //unimportant

#define OPCODEstr "subst:regex:postp:prep:segments:prosody:contour:progress:regress:insert:syll:smooth:raise:debug:if:inside:with:{:}:[:]:<:>:nothing:error:"
enum OPCODE {OP_SUBST, OP_REGEX, OP_POSTP, OP_PREP, OP_DIPH, OP_PROSODY, OP_CONTOUR, OP_PROGRESS, OP_REGRESS, 
	OP_INSERT, OP_SYLL, OP_SMOOTH, OP_RAISE, OP_DEBUG, OP_IF, OP_INSIDE, OP_WITH,
	OP_BEGIN, OP_END, OP_CHOICE, OP_CHOICEND, OP_SWITCH, OP_SWEND, OP_NOTHING, OP_ERROR};
		/* OP_BEGIN, OP_END and other OP's without parameters should come last
		   OP_ERROR would abort the compilation (never used)			*/
enum RULE_STATUS {RULE_OK, RULE_IGNORED, RULE_BAD=-1};



rule *next_rule(text *file, hash *vars, int *count);	// count is an out-parameter
extern char * _rules_buff;



// #define DEFAULT_SCOPE    U_WORD
// #define DEFAULT_TARGET   U_PHONE

/*
 * Syntax of an assimilation rule proper: "ptk>bdg(aeiou_aeiou)" is defined here
 * The delimiters need not be different, but they shouldn't collide with the delimitees
 */
 
#define ASSIM_DELIM1    '>'
#define ASSIM_DELIM2    '('
#define ASSIM_DELIM3    '_'
#define ASSIM_DELIM4    ')'
#define RAISE_DELIM	':'

#define LESS_THAN       '<'	// to delimit the sonority groups in parse_syll() 

#define COMMA		','	// item,value
#define SPACE		' '	// to delimit lists in literal hash tables
#define TILDE		'~'

#define UNDIPHONE "~"			//OP_DIPH only, means "delete segments if any"

class rule
{
   protected:
	const char *debug_tag();
   public:
	virtual OPCODE code() = 0;
	UNIT   scope;
	UNIT   target;
	char  *raw;

	     rule(char *param);
	virtual    ~rule();
	virtual void set_level(UNIT scope, UNIT target);
	virtual void set_dbg_tag(text *file);
	virtual void check_child(rule *r);
	virtual void check_children();
	virtual void verify() {};
	virtual void apply(unit *root) = 0;
	virtual void debug();
#ifdef DEBUGGING
	char *dbg_tag;
#endif
};

class hashing_rule: public rule
{
   protected:
	bool allow_id;
	hash *dict;
   public:
		hashing_rule(char *param);
	virtual ~hashing_rule();
	virtual void verify();
	void load_hash();
};

rule::rule(char *param)
{
	raw = param ? strdup(param) : (char *)NULL;
#ifdef DEBUGGING
	dbg_tag = NULL;
#endif
}

rule::~rule()
{
	DEBUG(0,1,fprintf(STDDBG, "rule::~rule, raw=%s, dbg_tag=%s\n", raw, dbg_tag););
	if (raw) free(raw);
#ifdef DEBUGGING
	if (dbg_tag) free(dbg_tag);
#endif
}

void
rule::set_level(UNIT scp, UNIT trg)
{
	scope =  scp == U_DEFAULT ? cfg->default_scope  : scp;
	target = trg == U_DEFAULT ? cfg->default_target : trg;
	if (scope < target) shriek (811, fmt("Scope must not be more narrow than target%s", debug_tag()));
			// was scope <= target -  consider returning back
}

void
rule::set_dbg_tag(text *file)
{
#ifdef DEBUGGING
	DEBUG(0,1,fprintf(STDDBG,"Debugging tag %s\n",file->current_file));
	sprintf(scratch, "%s:%d", file->current_file, file->current_line);
	dbg_tag=strdup(scratch);
#else
	unuse(file);
#endif
}

void
rule::check_child(rule *r)
{
	if (this->scope < r->scope) {
		if (r->scope != U_INHERIT) {
			printf("this %d child %d\n",scope, r->scope);
			debug();
			shriek(811, fmt("Scope of rule exceeds scope of its block%s", debug_tag()));
		}
		r->scope = this->scope;
	}
}

void
rule::check_children()
{
}

const char *
rule::debug_tag()
{
	char *wholetag=(char *)FOREVER(xmalloc(cfg->max_line));

#ifdef DEBUGGING
	char *tmp;
	if (dbg_tag) {
		tmp=strchr(dbg_tag, ':');
		if (!tmp) {
			sprintf(wholetag, "in unnumbered place %s", dbg_tag);
		} else {
			*tmp++=0;
			sprintf(wholetag, " in file %s line %s", dbg_tag, tmp);
			tmp[-1]=':';
		}
	} else
#endif
	{
		if (!global_current_file) return ". Debug tag is corrupted as well.";
		sprintf(wholetag, " in file %s line %d", global_current_file, global_current_line);
	}
	return wholetag;
}


/* Default value = key */

hash *literal_hash(char *s)
{
	char *p;
	char *last;
	char *comma = NULL;

	hash *dict = new hash((strlen(s) >> 4) + 4);

	p = s;
	last = s + 1;
	while (1) {
		switch(*++p) {
		case COMMA:
			if (comma) shriek(811, "too many commae");
			comma = p;
			*comma = 0;
			break;
		case SPACE:
		case TILDE:
			*p = 0;
			dict->add(last, comma ? comma+1 : last);
			if (comma) *comma = COMMA;
			*p = SPACE;
			last = NULL;
			comma = NULL;
			for (last = p+1; *last == SPACE; last++) ;
			break;
		case DQUOT:
			*p = 0;
			if (last) dict->add(last, comma ? comma+1 : last);
			if (comma) *comma = COMMA;
			*p = DQUOT;
			return dict;
		case 0: return NULL;	// error - unterminated string
		}
	}
}

hashing_rule::hashing_rule(char *param) : rule(NULL)
{
	if (*param == DQUOT) raw = strdup(param);
	else raw = compose_pathname(param, this_lang->hash_dir, cfg->lang_base_dir);
	dict = NULL;
	allow_id = false;
}


hashing_rule::~hashing_rule()
{
	if (dict) delete dict;
}

void
hashing_rule::verify()
{
	if (cfg->paranoid) load_hash();
}

void
hashing_rule::load_hash()
{
	if (dict) shriek(862, "unwanted load_hash");

	if (*raw != DQUOT) {
		dict = new hash(raw, cfg->hash_full, 0, 200, 3,
			(char *) allow_id, false, "dictionary %s not found", esctab);
	} else dict = literal_hash(raw);
	if (!dict) shriek(463, fmt("Unterminated argument%s", debug_tag()));	// or out of memory
}

void 
rule::debug()
{
	fprintf(STDDBG," rule: '%s' ",enum2str(code(),OPCODEstr));
	fprintf(STDDBG,"par %s scope '%s' ", raw, enum2str(scope, cfg->unit_levels));
	fprintf(STDDBG,"target '%s'\n", enum2str(target, cfg->unit_levels));
}




/************************************************
 r_subst  The following rule classes implement
	  substitutions and joining of units
 **	  by enumeration
 ************************************************/


class r_subst: public hashing_rule
{
   protected:
	SUBST_METHOD method;
	virtual OPCODE code() {return OP_SUBST;};
   public:
		r_subst(char *param);
	virtual void set_level(UNIT scope, UNIT target);
	virtual void apply(unit *root);
};

class r_prep: public r_subst
{
	virtual OPCODE code() {return OP_PREP;};
   public:
		r_prep(char *param);
};

class r_postp: public r_subst
{
	virtual OPCODE code() {return OP_POSTP;};
   public:
		r_postp(char *param);
};

r_subst::r_subst(char *param) : hashing_rule(param)
{
	method = M_SUBSTR;
}

r_prep::r_prep(char *param) : r_subst(param)
{
	method = M_RIGHT;
	allow_id = true;
}

r_postp::r_postp(char *param) : r_subst(param)
{
	method = M_LEFT;
	allow_id = true;
}

void
r_subst::set_level(UNIT scp, UNIT trg)
{
	rule::set_level(scp, trg);
//	if (target != U_PHONE) shriek(fmt("Cannot substitute for non-phones %s",debug_tag()));
}

/************************************************
 r_subst::apply
 ************************************************/

void
r_subst::apply(unit *root)
{
	if (!dict) load_hash();

//	if (target == U_PHONE) root->subst(dict, method);

	root->relabel(dict, method, target);

	if (cfg->lowmemory) {
		DEBUG(2,2,fprintf(STDDBG,"Hash table caching is disabled.\n");) //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
}

/************************************************
 r_seg   The following rule class constructs
 	  the segment layer according to segment
 **	  numbers found in the dictionary
 ************************************************/


class r_seg: public hashing_rule
{
	virtual OPCODE code() {return OP_DIPH;};
   public:
		r_seg(char *param);
	virtual void apply(unit *root);
};

r_seg::r_seg(char *param) : hashing_rule(param)
{
	if (!strcmp(param, UNDIPHONE)) {
		free(raw);
		raw=strdup(NULL_FILE);
	}
}

/************************************************
 r_seg::apply()
 ************************************************/

void
r_seg::apply(unit *root)
{
	if (!dict) load_hash();
	root->segs(target, dict);
	if (cfg->lowmemory) {
		DEBUG(2,2,fprintf(STDDBG,"Hash table caching is disabled.\n");) //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict=NULL;
	}

	DEBUG(1,2,fprintf(STDDBG,"Diphones w%s be dumped just after the DIPHONES rule\n", cfg->imm_segs?"ill":"on't");)
	if (cfg->imm_segs) {
		static segment d[DIPH_BUFF_SIZE];   //every item is 16 bytes long
		
		int i=DIPH_BUFF_SIZE;
		for (int k=0; i==DIPH_BUFF_SIZE; k+=DIPH_BUFF_SIZE) {
			i=root->write_segs(d,k,DIPH_BUFF_SIZE);
			for (int j=0;j<i;j++) 
				fprintf(STDDBG,"segment number=%3d f=%d t=%d i=%d\n", d[j].code, d[j].f, d[j].t, d[j].e);
		}
	}
}

/************************************************
 r_prosody The following rule class manipulates
	   suprasegmentalia according to subrules
 **	   in a dictionary
 ************************************************/


class r_prosody: public hashing_rule
{
	virtual OPCODE code() {return OP_PROSODY;};
   public:
		r_prosody(char *param);
	virtual void apply(unit *root);
};

r_prosody::r_prosody(char *param) : hashing_rule(param)
{
}

/************************************************
 r_prosody::apply
 ************************************************/

void
r_prosody::apply(unit *root)
{
	if (!dict) load_hash();
	DEBUG(1,1,fprintf(STDDBG,"entering rules::sseg()\n");)
	root->sseg(target, dict);
	if (cfg->lowmemory) {
		DEBUG(2,2,fprintf(STDDBG,"Hash table caching is disabled.\n");) //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
}
/************************************************
 r_contour The following rule class distributes
	  some prosodic contour over a linear
 **	  sequence of units
 ************************************************/

class r_contour: public rule
{
   protected:
	virtual OPCODE code() {return OP_CONTOUR;};
	int *contour;
	int l;
	int padd_start;
	FIT_IDX quantity;
   public:
		r_contour(char *param);
	virtual ~r_contour();
	virtual void apply(unit *root);
};

r_contour::r_contour(char *param) : rule(param)
{
	char *p;
	short int tmp=0, sgn=1;
	
	contour = (int *)xmalloc(strlen(param)*sizeof(int));
	contour[0] = l = 0;
	padd_start = -1;
	for (p=param+1+(param[1]=='/'); *p; p++) {
		switch (*p) {
		case ':':  contour[l++] += tmp*sgn; tmp=0;
				sgn=1; contour[l] = 0;  break;
		case '*':  if (p[1] && p[1] != ':') shriek(811, fmt("A ':' should follow '*'%s", debug_tag()));
			   if (padd_start > -1) shriek(811, fmt("Ambiguous padding%s", debug_tag()));
				padd_start = l; break;
		case '-':
		case '+':  contour[l]+=tmp*sgn; sgn=(*p=='+' ?+1:-1); break;
		default:   if (*p<'0' || *p>'9') shriek(811, fmt("Expected a number, found \"%s\"%s",
					p, debug_tag()));
			   else tmp = tmp*10 + *p-'0';
		}
	}
	if (tmp) contour[l++] += tmp*sgn;

	quantity = fit(*param);
}

r_contour::~r_contour()
{
	free(contour);
}

void r_contour::apply(unit *root)
{
	root->contour(target, contour, l, padd_start, quantity, false);
}


/************************************************
 r_smooth The following rule class smoothens
	  suprasegmentalia using a given weighted
 **	  averaging
 ************************************************/


class r_smooth: public rule
{
   protected:
	virtual OPCODE code() {return OP_SMOOTH;};
	int *list;
	int n;
	int l;
	FIT_IDX quantity;
   public:
		r_smooth(char *param);
	virtual ~r_smooth();
	virtual void apply(unit *root);
};

r_smooth::r_smooth(char *param) : rule(param)
{
	char *p;
	short int tmp=0, sgn=1, total=0, max;
	
	list=(int *)xmalloc(strlen(param)*sizeof(int));
	list[0]=n=l=0;
	for (p=param+1+(param[1]=='/'); *p; p++) {
		switch (*p) {
		case '/':  n++;		/* and fall through */
		case '\\': list[l++] += tmp*sgn; total += tmp*sgn; tmp=0;
			   sgn=1; list[l]=0;  break;
		case '-':
		case '+':  list[l]+=tmp*sgn; sgn=(*p=='+' ?+1:-1); break;
		default:   if (*p<'0' || *p>'9') shriek(811, fmt("Expected a number, found \"%s\"%s",
					p, debug_tag()));
			   else tmp = tmp*10 + *p-'0';
		}
	}
	if (tmp) list[l++]+=tmp*sgn, total+=tmp*sgn;
	if (total!=RATIO_TOTAL)
		shriek (811, fmt("Smooth percentages don't add up to 100%% (%d%%)%s", total, debug_tag()));
	if (cfg->paranoid) {
		for (tmp=max=0; tmp<l; tmp++)
			if (list[tmp]>max) max=list[tmp];
		if (max!=list[n]) shriek(811, "Oversmooth, max weight is given elsewhere");
	}

	if (l>=SMOOTH_CQ_SIZE) shriek(864, "unit::smooth circular queue too small, increase SMOOTH_CQ_SIZE");
	
	quantity = fit(*param);
}

r_smooth::~r_smooth()
{
	free(list);
}


/************************************************
 r_smooth::apply()
 ************************************************/
 
void
r_smooth::apply(unit *root)
{
	
	DEBUG(1,1,fprintf(STDDBG,"entering rules::smooth()\n");)
	root->project(target,0,0,0);
	root->smooth(target, list, n, l, quantity);
}

/************************************************
 r_*gress The following rule classes implement
	  the traditional "phonetic changes",
 **	  i.e. which phone is changing to which
 **	  one and in what environment
 ************************************************/


class r_regress: public rule
{
   protected:
	virtual OPCODE code() {return OP_REGRESS;};

	bool *ltab;       //callocced by booltab()
	bool *rtab;
	char *fn;
	bool backwards;

   public:
		r_regress(char *param);
	virtual ~r_regress();
	virtual void apply(unit *root);
};

class r_progress: public r_regress
{
	virtual OPCODE code() {return OP_PROGRESS;};
   public:
		r_progress(char *param);
};

r_regress::r_regress(char *param) : rule(param)
{
	char *aff;
	char *eff;
	char *left;      
	char *right;

	eff=strchr(aff=strdup(param),ASSIM_DELIM1);if(!eff) shriek(811, fmt("Bad param%s", debug_tag()));
	*eff++=0;
	left=strchr(eff,ASSIM_DELIM2);if(!left) shriek(811, fmt("Bad param%s", debug_tag()));
	*left++=0;
	right=strchr(left,ASSIM_DELIM3);if(!right) shriek(811, fmt("Bad param%s", debug_tag()));
	*right++=0;
	fn=strchr(right,ASSIM_DELIM4);if(!fn) shriek(811, fmt("Bad param%s", debug_tag()));
	*fn++=0;if(*fn) shriek(811, fmt("Strange appendix to param%s", debug_tag()));
	DEBUG(0,4,fprintf(STDDBG,"Parsed assim param \"%s>%s(%s_%s)\"\n",aff,eff,left,right);)
	fn=fntab(aff,eff);ltab=booltab(left);rtab=booltab(right);
	free(aff);
	
	backwards=true;
}

r_regress::~r_regress()
{
	free(fn);
	free(ltab);
	free(rtab);
}

r_progress::r_progress(char *param) : r_regress(param)
{
	backwards=false;
}

/********************************************************
 r_*gress::apply
 ********************************************************/

void 
r_regress::apply(unit *root)
{
	root->assim(target,backwards,fn,ltab,rtab);

	if (fn[JUNCTURE] != DELETE_ME)
		root->insert(target,backwards,fn[JUNCTURE],ltab,rtab);
}

/************************************************
 r_syll   The following rule class splits words
 	  into syllables using sonority values
 **	  of phones
 ************************************************/


class r_syll: public rule
{
	virtual OPCODE code() {return OP_SYLL;};
	char *son;
   public:
		r_syll(char *param);
	virtual ~r_syll();
//	virtual void set_level(UNIT scope, UNIT target);
	virtual void apply(unit *root);
};

#define MIN_SONORITY	1
#define NO_SONORITY	1

r_syll::r_syll(char *param) : rule(param)
{
	int lv = MIN_SONORITY;
	char *tmp;
	int i;

	son=(char *) xmalloc(256);
	for (i = 0; i < 256; i++) son[i] = NO_SONORITY;
	for (tmp = param; *tmp && lv; tmp++) {
		if (*tmp==LESS_THAN) lv++;
		else son[(unsigned char)(*tmp)]=lv;
		DEBUG(0,1,fprintf(STDDBG,"Giving to %c sonority %d\n", *tmp, lv);)
	}
	DEBUG(0,1,fprintf(STDDBG,"rules::parse_syll going to call syllabify()\n");)
}

r_syll::~r_syll()
{
	free(son);
}

//void
//r_syll::set_level(UNIT scp, UNIT trg)
//{
//	if (scp == U__DEFAULT) scp = U__SYLL;
//	rule::set_level(scp, trg);
//}	


/********************************************************
 r_syll::apply
 ********************************************************/

void
r_syll::apply(unit *root)
{
	root->syllabify(target, son);
}

/************************************************
 r_raise  The following rule class moves
 	  tokens between different levels
 **	  (phones to sentences et c.)
 ************************************************/


class r_raise: public rule
{
	virtual OPCODE code() {return OP_RAISE;};
	bool *whattab;
	bool *whentab;
   public:
		r_raise(char *param);
	virtual ~r_raise();
	virtual void apply(unit *root);
};

r_raise::r_raise(char *param) : rule(param)
{
	char *cond;
	if ((cond = strchr(raw,RAISE_DELIM))) *cond++=0; else cond=(char *)"!";
	whattab = booltab(raw);
	whentab = booltab(cond);
}

r_raise::~r_raise()
{
	free(whattab);
	free(whentab);
}

void
r_raise::apply(unit *root)
{
	root->raise(whattab, whentab, scope, target);
};

#ifdef WANT_REGEX

/************************************************
 r_regex  The following rule class can replace
 	  an arbitrary regular expression with
 **	  a replacement based on it
 ************************************************/


class r_regex: public rule
{
	regex_t regex;
	int parens;
	regmatch_t *matchbuff;
	char *repl;
	virtual OPCODE code() {return OP_REGEX;};
   public:
		r_regex(char *param);
		~r_regex();
	virtual void apply(unit *root);
};

#define PAREN_OPEN  '('
#define PAREN_CLOS  ')'

#define rshr(x) shriek(811, fmt("Regex invalid: %s%s", x, debug_tag()));

r_regex::r_regex(char *param) : rule(param)
{
	char separator = *param++;
	char *tmp = strchr(param, separator);
	if (!tmp) shriek(811, fmt("regex param should be separated thus: /regex/replacement/ %s", debug_tag()));
	*tmp++=0;
	parens = 0;
	int result;
	
	for(int i=0, j=0; ; i++,j++) {
		if (param[i]==PAREN_OPEN || param[i]==PAREN_CLOS) {
			if (j && scratch[j-1]==ESCAPE) j--;
			else {
				scratch[j++] = ESCAPE;
				parens++;
			}
		}
		scratch[j] = param[i];
		if (!param[i]) break;
	}
	
	param = strdup(scratch);

	matchbuff = (regmatch_t *)xmalloc((parens+2)*sizeof(regmatch_t));
	DEBUG(0,1,fprintf(STDDBG, "Compiling regex %s\n", param);)
	result = regcomp(&regex, param, 0);
	switch (result) {
		case 0: break;
		case REG_BADBR:
		case REG_EBRACE:
			rshr("braces should specify an interval");
		case REG_EBRACK:
		case REG_ERANGE:
		case REG_ECTYPE:
			rshr("brackets should enclose a list");
		case REG_EPAREN:
		case REG_ESUBREG:
			rshr("read the docs about () subexps");
		case REG_EESCAPE:
			rshr("badly escaped");
		case REG_BADRPT:
		case REG_BADPAT:
#ifdef HAVE_REG_EEND
		case REG_EEND:
		case REG_ESIZE:
#endif
			rshr("too ugly");
		case REG_ESPACE:
			rshr("too huge");
		default: shriek(811, fmt("Bad regex%s, regcomp returns %d", debug_tag(), result));
	}
	free(param);
	repl = tmp;
	tmp = strchr(tmp, separator);
	if (!tmp) shriek(811, fmt("regex param should be separated thus: /regex/replacement/ %s", debug_tag()));
	if (tmp[1]) shriek(811, fmt("garbage follows replacement%s", debug_tag()));
	*tmp=0;
	repl=strdup(repl);
	DEBUG(0,1,fprintf(STDDBG,"regex%s is okay\n", debug_tag());)
}

#undef rshr

r_regex::~r_regex()
{
	free(matchbuff);
	regfree(&regex);
	free(repl);
}

void
r_regex::apply(unit *root)
{
	root->regex(&regex, parens, matchbuff, repl);
}

#endif


/************************************************
 r_debug  The following rule class can print
 	  various data at the moment the rule
 **	  is applied
 ************************************************/


class r_debug: public rule
{
	virtual OPCODE code() {return OP_DEBUG;};
   public:
		r_debug(char *param);
	virtual void apply(unit *root);
};

r_debug::r_debug(char *param) : rule(param)
{
}

void
r_debug::apply(unit *root)
{
	if(strstr(raw,"elem")) root->fout(NULL);
//	if(strstr(raw,"rules")) ruleset->debug();
//	else if(strstr(raw,"rule") && ruleset->current_rule+1 < ruleset->n_rules)
//		ruleset->rulist[ruleset->current_rule+1]->debug();
	if(strstr(raw,"pause")) user_pause();
}

/************************************************
 cond_rule (abstract class)
 ************************************************/

class cond_rule: public rule
{
   protected:
	rule *then;
   public:
	cond_rule(char *param, text *file, hash *vars);
	~cond_rule();
	virtual void check_children();
};

cond_rule::cond_rule(char *param, text *file, hash *vars) : rule(param)
{
	then = next_rule(file, vars, NULL);
}

cond_rule::~cond_rule()
{
	delete then;
}

void 
cond_rule::check_children()
{
	check_child(then);
	then->check_children();
}

/************************************************
 r_inside The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  inside the units whose contents is
 **	  listed in r_inside parameter
 ************************************************/


class r_inside: public cond_rule
{
	virtual OPCODE code() {return OP_INSIDE;};
	bool *affected;
   public:
		r_inside(char *param, text *file, hash *vars);
		~r_inside();
	virtual void apply(unit *root);
};

r_inside::r_inside(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	affected = booltab(raw);
}

r_inside::~r_inside()
{
	free(affected);
}

void
r_inside::apply(unit *root)
{
	if (affected[(unsigned char)root->cont]) then->apply(root);
}

/************************************************
 r_with   The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  inside the units whose subordinates
 **	  form a string found in the parameter file
 ************************************************/


class r_with: public cond_rule
{
	virtual OPCODE code() {return OP_WITH;};
	hash *dict;
   public:
		r_with(char *param, text *file, hash *vars);
		~r_with();
	virtual void apply(unit *root);
};

r_with::r_with(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	char *old = raw;

	if (*raw == DQUOT) raw = strdup(raw);
	else raw = compose_pathname(raw, this_lang->hash_dir, cfg->lang_base_dir);

	free(old);
	dict = NULL;
}

r_with::~r_with()
{
	if (dict) delete dict;
}

void
r_with::apply(unit *root)
{
	if (!dict) {
		if (*raw == DQUOT) dict = literal_hash(raw);
		else dict = new hash(raw, cfg->hash_full,
			0, 200, 3, DATA_EQUALS_KEY, false, "dictionary %s not found", esctab);
	}
	if (!dict) shriek(811, fmt("Unterminated argument%s", debug_tag()));	// or out of memory

	if (root->subst(dict, M_ONCE)) then->apply(root);
}


/************************************************
 r_if     The following rule class will apply
 	  its subordinated rule (block of rules)
 **	  if a global condition holds
 ************************************************/


class r_if: public cond_rule
{
	virtual OPCODE code() {return OP_IF;};
//	bool result;
	int flag_offs;	// offset for this_voice
 	virtual void set_level(UNIT scp, UNIT trg);
  public:
		r_if(char *param, text *file, hash *vars);
	virtual void apply(unit *root);
};

r_if::r_if(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	option *o = option_struct(raw + (*raw == EXCLAM) , this_lang->soft_options);
	if (!o) shriek(811, fmt("Not an option: %s%s", raw, debug_tag()));
	if (o->opttype != O_BOOL) shriek(811, fmt("Not a truth value option: %s%s", raw, debug_tag()));
	if (o->structype != OS_VOICE) shriek(811, fmt("Not a voice option: %s%s", raw, debug_tag()));
	flag_offs = o->offset;
}

void
r_if::apply(unit *root)		// this_lang must correspond to these rules!
{
	if (this_voice && (*(bool *)((char *)this_voice + flag_offs)) ^ (*raw == EXCLAM))
		then->apply(root);
}

void
r_if::set_level(UNIT scp, UNIT trg)
{
	if (scp == U_DEFAULT) scp = U_INHERIT;
	rule::set_level(scp, trg);
}	


/************************************************
 r_nothing  The following rule class can serve
 	    as a void placeholder, where a rule
 **	    is syntactically required
 ************************************************/


class r_nothing: public rule
{
   protected:
	virtual OPCODE code() {return OP_NOTHING;};
   public:
	r_nothing() : rule(NULL) {};
	virtual void apply(unit *) {};
};

#include "block.cc"

