/*
 *	ss/src/rule.cc
 *	(c) 1996-98 geo@ff.cuni.cz
 *
 *	This file includes block.cc at the very last line.
 *
 */

#include "common.h"

#define DEFAULT_SCOPE    U_WORD
#define DEFAULT_TARGET   U_PHONE

/*
 * Syntax of an assimilation rule proper: "ptk>bdg(aeiou_aeiou)" is defined here
 * The delimiters need not be different, but they shouldn't collide with the delimitees
 */
 
#define ASSIM_DELIM1    '>'
#define ASSIM_DELIM2    '('
#define ASSIM_DELIM3    '_'
#define ASSIM_DELIM4    ')'
#define RAISE_DELIM	':'

#define LESS_THAN       '<'    //to delimit the sonority groups in parse_syll() 

#define UNDIPHONE "~"			//OP_DIPH only, means "delete diphones if any"

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
	inline void load_hash();
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
	DEBUG(0,1,fprintf(stddbg, "rule::~rule, raw=%s, dbg_tag=%s\n", raw, dbg_tag););
	if (raw) free(raw);
#ifdef DEBUGGING
	if (dbg_tag) free(dbg_tag);
#endif
}

void
rule::set_level(UNIT scp, UNIT trg)
{
	scope =  scp == U_DEFAULT ? DEFAULT_SCOPE : scp;
	target = trg == U_DEFAULT ? DEFAULT_TARGET : trg;
	if (scope <= target) shriek ("Scope must be wider than target%s", debug_tag());
}

void
rule::set_dbg_tag(text *file)
{
#ifdef DEBUGGING
	DEBUG(0,1,fprintf(stddbg,"Debugging tag %s\n",file->current_file));
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
			shriek("Scope of rule exceeds scope of its block%s", debug_tag());
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
	char *wholetag=(char *)FOREVER(malloc(cfg.max_line));

#ifdef DEBUGGING
	char *tmp;
	if (dbg_tag) {
		tmp=strchr(dbg_tag, ':');
		if (!tmp) tmp=" not specified";
		*tmp++=0;
		sprintf(wholetag, " in file %s line %s", dbg_tag, tmp);
		tmp[-1]=':';
	} else
#endif
	{
		if (!global_current_file) return ". Debug tag is corrupted as well.";
		sprintf(wholetag, " in file %s line %d", global_current_file, global_current_line);
	}
	return wholetag;
}

void
rule::apply(unit *root)
{
	unuse(root);
	shriek("rule::apply() called (inherited or not)");		/* Abstract method */
}


hashing_rule::hashing_rule(char *param) : rule(NULL)
{
	raw = compose_pathname(param, cfg.hash_dir);
	dict = NULL;
	allow_id = false;
}


hashing_rule::~hashing_rule()
{
	if (dict) delete dict;
}

inline void
hashing_rule::load_hash()
{
	if (!dict) dict = new hash(raw, cfg.hash_full,
		0, 200, 3, (char *) allow_id, false, "dictionary %s not found");
}

void 
rule::debug()
{
	fprintf(stddbg," rule: '%s' ",enum2str(code(),OPCODEstr));
	fprintf(stddbg,"par %s scope '%s' ", raw, enum2str(scope,UNITstr));
	fprintf(stddbg,"target '%s'\n", enum2str(target,UNITstr));
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
	if (target != U_PHONE) shriek("Cannot substitute for non-phones %s",debug_tag());
}

/************************************************
 r_subst::apply
 ************************************************/

void
r_subst::apply(unit *root)
{
	load_hash();
	root->subst(dict, method);
	if (cfg.lowmemory) {
		DEBUG(2,2,fprintf(stddbg,"Hash table caching is disabled.\n");) //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
}

/************************************************
 r_diph   The following rule class constructs
 	  the diphone layer according to diphone
 **	  numbers found in the dictionary
 ************************************************/


class r_diph: public hashing_rule
{
	virtual OPCODE code() {return OP_DIPH;};
   public:
		r_diph(char *param);
	virtual void apply(unit *root);
};

r_diph::r_diph(char *param) : hashing_rule(param)
{
	if (!strcmp(param, UNDIPHONE)) {
		free(raw);
		raw=strdup(NULL_FILE);
	}
}

/************************************************
 r_diph::apply()
 ************************************************/

void
r_diph::apply(unit *root)
{
	if (!cfg.use_diph) return;
	load_hash();
	root->diphs(target, dict);
	if (cfg.lowmemory) {
		DEBUG(2,2,fprintf(stddbg,"Hash table caching is disabled.\n");) //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict=NULL;
	}

	DEBUG(2,2,fprintf(stddbg,"Diphones w%s be dumped just after the DIPHONES rule\n", cfg.imm_diph?"ill":"on't");)
	if (cfg.imm_diph) {
		static diphone d[DIPH_BUFF_SIZE];   //every item is 16 bytes long
		
		int i=DIPH_BUFF_SIZE;
		for (int k=0; i==DIPH_BUFF_SIZE; k+=DIPH_BUFF_SIZE) {
			i=root->write_diphs(d,k,DIPH_BUFF_SIZE);
			for (int j=0;j<i;j++) 
				fprintf(stddbg,"diphone number=%3d f=%d t=%d i=%d\n", d[j].code, d[j].f, d[j].t, d[j].e);
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
	load_hash();
	DEBUG(1,1,fprintf(stddbg,"entering rules::sseg()\n");)
	root->sseg(target, dict);
	if (cfg.lowmemory) {
		DEBUG(2,2,fprintf(stddbg,"Hash table caching is disabled.\n");) //hashtabscache[rulist[i].param]->debug();
		delete dict;
		dict = NULL;
	}
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
	
	list=(int *)malloc(strlen(param)*sizeof(int));
	list[0]=n=l=0;
	for (p=param+1+(param[1]=='/'); *p; p++) {
		switch (*p) {
		case '/':  n++;		/* and fall through */
		case '\\': list[l++] += tmp*sgn; total += tmp*sgn; tmp=0;
			   sgn=1; list[l]=0;  break;
		case '-':
		case '+':  list[l]+=tmp*sgn; sgn=(*p=='+' ?+1:-1); break;
		default:   if (*p<'0' || *p>'9') shriek("Expected a number, found \"%s\"%s",
					p, debug_tag());
			   else tmp = tmp*10 + *p-'0';
		};
	};
	if (tmp) list[l++]+=tmp*sgn, total+=tmp*sgn;
	if (total!=RATIO_TOTAL)
		shriek ("Smooth percentages don't add up to 100%% (%d%%)", total);
	if (cfg.paranoid) {
		for (tmp=max=0; tmp<l; tmp++)
			if (list[tmp]>max) max=list[tmp];
		if (max!=list[n]) warn("Oversmooth, max weight is given elsewhere");
	}

	if (l>=SMOOTH_CQ_SIZE) shriek("unit::smooth circular queue too small, increase SMOOTH_CQ_SIZE");
	
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
	
	DEBUG(1,1,fprintf(stddbg,"entering rules::smooth()\n");)
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

	eff=strchr(aff=strdup(param),ASSIM_DELIM1);if(!eff) shriek("Bad param%s", debug_tag());
	*eff++=0;
	left=strchr(eff,ASSIM_DELIM2);if(!left) shriek("Bad param%s", debug_tag());
	*left++=0;
	right=strchr(left,ASSIM_DELIM3);if(!right) shriek("Bad param%s", debug_tag());
	*right++=0;
	fn=strchr(right,ASSIM_DELIM4);if(!fn) shriek("Bad param%s", debug_tag());
	*fn++=0;if(*fn) shriek("Strange appendix to param%s", debug_tag());
	DEBUG(2,4,fprintf(stddbg,"Parsed assim param \"%s>%s(%s_%s)\"\n",aff,eff,left,right);)
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
	virtual void set_level(UNIT scope, UNIT target);
	virtual void apply(unit *root);
};

#define MIN_SONORITY	1
#define NO_SONORITY	1

r_syll::r_syll(char *param) : rule(param)
{
	int lv = MIN_SONORITY;
	char *tmp;
	int i;

	son=(char *) malloc(256);
	for (i = 0; i < 256; i++) son[i] = NO_SONORITY;
	for (tmp = param; *tmp && lv; tmp++) {
		if (*tmp==LESS_THAN) lv++;
		else son[(Char)(*tmp)]=lv;
		DEBUG(0,1,fprintf(stddbg,"Giving to %c sonority %d\n", *tmp, lv);)
	};
	DEBUG(1,1,fprintf(stddbg,"rules::parse_syll going to call syllablify()\n");)
}

r_syll::~r_syll()
{
	free(son);
}

void
r_syll::set_level(UNIT scp, UNIT trg)
{
	if (scp == U_DEFAULT) scp = U_SYLL;
	rule::set_level(scp, trg);
}	


/********************************************************
 r_syll::apply
 ********************************************************/

void
r_syll::apply(unit *root)
{
	root->syllablify(target, son);
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
	if ((cond=strchr(raw,RAISE_DELIM))) *cond++=0; else cond="!";
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

#define rshr(x) shriek("Regex invalid: %s%s", x, debug_tag());

r_regex::r_regex(char *param) : rule(param)
{
	char separator = *param++;
	char *tmp = strchr(param, separator);
	if (!tmp) shriek("regex param should be separated thus: /regex/replacement/ %s", debug_tag());
	*tmp++=0;
	parens = 0;
	int result;
	
	for(int i=0, j=0; ; i++,j++) {
		if (param[i]==PAREN_OPEN || param[i]==PAREN_CLOS) {
			if (scratch[j-1]==ESCAPE && j) j--;
			else {
				scratch[j++] = ESCAPE;
				parens++;
			}
		}
		scratch[j] = param[i];
		if (!param[i]) break;
	}
	
	param = strdup(scratch);

	matchbuff = (regmatch_t *)malloc((parens+2)*sizeof(regmatch_t));
	DEBUG(2,1,fprintf(stddbg, "Compiling regex %s\n", param);)
	result = regcomp(&regex, param, 0);
	DEBUG(0,1,fprintf(stddbg, "...hm...\n");)
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
		case REG_EEND:
			rshr("too ugly");
		case REG_ESIZE:
		case REG_ESPACE:
			rshr("too huge");
		default: shriek("Bad regex%s, regcomp returns %d", debug_tag(), result);
	}
	free(param);
	repl = tmp;
	tmp = strchr(tmp, separator);
	if (!tmp) shriek("regex param should be separated thus: /regex/replacement/ %s", debug_tag());
	if (tmp[1]) shriek("garbage follows replacement%s", debug_tag());
	*tmp=0;
	repl=strdup(repl);
	DEBUG(1,1,fprintf(stddbg,"regex%s is okay\n", debug_tag());)
}

#undef rshr

r_regex::~r_regex()
{
	free(matchbuff);
	regfree(&regex);
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

class r_if: public cond_rule
{
	virtual OPCODE code() {return OP_IF;};
	bool result;
   public:
		r_if(char *param, text *file, hash *vars);
	virtual void apply(unit *root);
};

r_if::r_if(char *param, text *file, hash *vars) : cond_rule(param, file, vars)
{
	char *tmp = strchr(raw, '=');
	if (!tmp) shriek("Conditional expression must contain '='%s", debug_tag());
	if (strchr(++tmp, '=')) shriek("Too many eq's%s", debug_tag());
	result =  raw+strlen(tmp) == tmp-1  &&  !strncmp(raw, tmp, strlen(tmp));
	DEBUG(1,1,fprintf(stddbg, "r_if result is constant and %s\n", result?"true":"false");)
}

void
r_if::apply(unit *root)
{
	if (result) then->apply(root);
}

#include "block.cc"
