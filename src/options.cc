/*
 *	epos/src/options.cc
 *	(c) 1996-99 geo@ff.cuni.cz
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

#include "slab.h"
SLABIFY(configuration, cfg_slab, 2, shutdown_cfgs)

#define CMD_LINE_OPT	"-"
#define CMD_LINE_VAL	'='

#define INDEXER		"@"

inline void *memdup(void *p, int size)
{
	void *r = xmalloc(size);
	memcpy(r,p,size);
	return r;
}

/*
 *	cow - this routine should detect whether a piece of memory used
 *		for global/language/voice configuration is shared with
 *		supposedly logical copies, and, if so, copy it physically
 *		before changing it. A nice space-saving technique.
 *
 *	cow_claim - claims all current configuration
 *	cow_unclaim - releases all current configuration; cowabilia without other
 *		previous claimers are freed
 *
 *	??Semantics: cow == number of claimers minus one
 *
 *	The implementation is terrible (though fast). If I knew how to design this
 *	correctly, I would do that. I have done it twice wrong and it grew moderately
 *	complex during the process of fixing.
 *
 */

void cow(cowabilium **p, int size, int extraoffset, int extrasize)
{
	cowabilium *src = *p;
	cowabilium **ptr = p;

	if (!*p) {
//		if (p == (cowabilium **)&this_lang) shriek(814, "Language-specific option not supported on the cmd line");
//		if (p == (cowabilium **)&this_voice) shriek(814, "Voice-specific option not supported on the cmd line");
		shriek(861, "cow()ing a NULL");
	}

	if (src->cow) {
//		*ptr = (cowabilium *)xmalloc(size);
//		printf("Copying %p(%d) to %p\n", src, src->cow, *ptr);
//		memcpy(*ptr, src, size);

		*ptr = (cowabilium *)memdup(src, size);
		(*ptr)->cow = 0;
		(*ptr)->parent = src;
		src->cow--;

		if (extraoffset) {
			*(void **)((char *)*ptr + extraoffset)
				= memdup(*(void **)((char *)src + extraoffset), extrasize);
		}
	}
}

#define  LANGS_OFFSET   ((int)&((configuration *)NULL)->langs)
#define  LANGS_LENGTH   ((*cfg)->n_langs * sizeof(void *))

void cow_configuration(configuration **cfg)
{
	cow((cowabilium **)cfg, sizeof(configuration), LANGS_OFFSET, LANGS_LENGTH);
}

void cow_claim()
{
//	printf("Claiming %p, was %d\n", cfg, cfg->cow);
	for (int i=0; i < cfg->n_langs; i++) {
		lang *l = cfg->langs[i];
//		printf("   lang %s(%p) was %d\n", l->name, l, l->cow);
		l->cow++;
		for (int j=0; j < l->n_voices; j++) {
//			printf("      voice %s(%p) was %d\n", l->voices[j]->name, l->voices[j], l->voices[j]->cow);
			l->voices[j]->cow++;
		}
	}
	cfg->cow++;
}

static inline void cow_free(cowabilium *p, option *opts, void *extra)
{
	for (option *o = opts; o->optname; o++) {
		if (o->opttype != O_STRING) continue;
		if (*(char **)((char *)p->parent + o->offset)
		 != *(char **)((char *)p + o->offset)) {
//			printf("Freeing %s\n", o->optname);
			free(*(char **)((char *)p + o->offset));
		}
	}
	free(p);
	if (extra) free(extra);
}

extern option optlist[];

void cow_unclaim(configuration *that_cfg)
{
//	printf("Unclaiming %p, was %d\n", that_cfg, that_cfg->cow);
	for (int i=0; i < that_cfg->n_langs; i++) {
		lang *l = that_cfg->langs[i];
//		printf("   lang %s(%p) was %d\n", l->name, l, l->cow);
		for (int j=0; j < l->n_voices; j++) {
//			printf("      voice %s(%p) was %d\n", l->voices[j]->name, l->voices[j],l->voices[j]->cow);
			if (!l->voices[j]->cow--) cow_free(l->voices[j], voiceoptlist, NULL);
			/* * * fortunately, soft options are never strings * * */
		}
		if (!l->cow--) cow_free(l, langoptlist, l->voices);
	}
	if (!that_cfg->cow--) cow_free(that_cfg, optlist, that_cfg->langs), that_cfg = NULL;
}

#define CONFIG_INITIALIZE
inline configuration::configuration() : cowabilium()
{
	#include "options.lst"

	n_langs = 0;
	langs = NULL;

	stdshriek = stderr;
	stddbg = stdout;

	current_stream = NULL;
}

#define EO {NULL, O_INT, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, -1},
#define TWENTY_EXTRA_OPTIONS	EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO EO 

configuration master_cfg;

configuration *cfg = &master_cfg;

#define CONFIG_DESCRIBE
option optlist[]={
        #include "options.lst"

	{"C:language" + 2, O_LANG, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, 0},
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	TWENTY_EXTRA_OPTIONS
	{NULL, O_INT, OS_CFG, A_PUBLIC, A_PUBLIC, false, false, -2}
};

#undef EO
#undef TWENTY_EXTRA_OPTIONS

hash_table<char, option> *option_set = NULL;

void configuration::shutdown()
{
	int i;
	for (i=0; i < n_langs; i++) delete langs[i];
	free(langs);
	langs = NULL;
//	this_lang = NULL; this_voice = NULL;
	n_langs = 0;
}

/*
 * Some string trickery below. option->name is initialised (in options.lst)
 *	to   "X:" "string"+2. In C, it means the same as "X:string"+2,
 *	which means "string" with "X:" immediately preceding it.
 * Now we add to our option_set both normal option names ("string")
 *	and their prefixed spellings ("X:string", for X being either
 *	C, L or V for global/language/voice configuration items,
 *	respectively), eating up minimum memory space only.
 */

inline void put_into_option_set(option *o)
{
	if (*o->optname) {
		option_set->add(o->optname, o);
		option_set->add(o->optname-2, o);
	}
}

void make_option_set()
{
	option *o;

	if (option_set) return;

	option_set = new hash_table<char, option>(300);
	option_set->dupkey = option_set->dupdata = 0;

	for (o = optlist; o->optname; o++) 	put_into_option_set(o);
	for (o = langoptlist; o->optname; o++)	put_into_option_set(o);
	for (o = voiceoptlist; o->optname; o++)	put_into_option_set(o);

	option_set->remove("languages");	// FIXME
	option_set->remove("voices");

	text *t = new text(cfg->allow_file, cfg->ini_dir, "", NULL, true);
	if (!t->exists()) {
		delete t;
		return;		/* if no allow file, ignore it */
	}
	char *line = (char *)xmalloc(cfg->max_line);
	while (t->getline(line)) {
		char *status = split_string(line);
		o = option_set->translate(line);
		if (!o) {
			if (cfg->paranoid)
				shriek(812, fmt("Typo in %s:%d\n", t->current_file, t->current_line));
			continue;
		}
		o->readable = o->writable = A_NOACCESS;
		ACCESS level = A_PUBLIC;
		for (char *p = status; *p; p++) {
			switch(*p) {
				case 'r': o->readable = level; break;
				case 'w': o->writable = level; break;
				case '#': level = A_ROOT; break;
				case '$': level = A_AUTH; break;
				default : shriek(812, fmt("Invalid access rights in %s line %d",
						t->current_file, t->current_line));
			}
		}
	}
	free(line);
	delete t;
}

option *alloc_option(option *optlist, OPT_STRUCT os)
{
	DEBUG(1,10,fprintf(STDDBG, "Allocating an extra option, level %d\n", os);)

	option *o;
	for (o = optlist; ; o++) {
		if (o->offset == -1) return o;
		if (o->offset == -2) shriek(864, "No more options to allocate, add extra options");
	}
	return NULL;	// to keep the compiler happy
}

#define MAX_TOTAL_OPTION_NAME 64

void alloc_level_options(option *optlist, OPT_STRUCT os, cowabilium *base, int levnum, char *levname)
{
	char b[MAX_TOTAL_OPTION_NAME];

	option *o;
	for (o=optlist; o->optname; o++) {
		if (o->per_level) {
			option *p = alloc_option(optlist, os);
			b[0] = "CLV"[os];
			b[1] = ':';
			strcpy(b+2, o->optname);
			strcat(b+2, INDEXER);
			strcat(b+2, levname);
			p->optname = FOREVER(strdup(b));
			p->opttype = o->opttype;
			p->readable = o->readable;
			p->writable = o->writable;
			option_set->add(p->optname, p);
			option_set->add(p->optname+2, p);

			int size;
			switch (o->opttype) {
				case O_STRING:	size = sizeof(char *); break;
				case O_INT:	size = sizeof(int); break;
				case O_BOOL:	size = sizeof(bool); break;
				default: shriek(861, "strange type size_of-ed"); break;
			}
			p->offset = o->offset + levnum * size;
			if (base)
				memcpy((char *)base + p->offset, (char *)base + o->offset, size);
					// CHECKME cow
		}
	}
}


#define LEVNAME_SEGMENT	"segment"
#define LEVNAME_PHONE	"phone"
#define LEVNAME_TEXT	"text"

char *pre_set_unit_levels(char *value)
{
	if ((str2enum(LEVNAME_SEGMENT, value, U_DEFAULT) == U_ILL) ||
		(str2enum(LEVNAME_PHONE, value, U_DEFAULT) == U_ILL) ||
		(str2enum(LEVNAME_TEXT, value, U_DEFAULT) == U_ILL))
			return NULL;		// required level missing

	char b[MAX_TOTAL_OPTION_NAME];
	int n;

	for (n=0; enum2str(n, value); n++) ;
	if (n >= UNIT_MAX) return NULL;		// too many levels
	for (int i=0; i<n; i++) {
		strcpy(b, enum2str(i, value));
		for (int j=0; j<n; j++) {
			if (i!=j && !strcmp(b, enum2str(j, value))) 
				return NULL;	// two level names collide
		}
	}
	return value;
}

void post_set_unit_levels(char *value)
{
	cfg->segm_level = str2enum(LEVNAME_SEGMENT, cfg->unit_levels, U_DEFAULT);
	cfg->phone_level = str2enum(LEVNAME_PHONE, cfg->unit_levels, U_DEFAULT);
	cfg->text_level = str2enum(LEVNAME_TEXT, cfg->unit_levels, U_DEFAULT);
	cfg->default_scope = str2enum("word", cfg->unit_levels, U_DEFAULT);
	if (cfg->default_scope == U_ILL) cfg->default_scope = cfg->text_level;
	cfg->default_target = cfg->phone_level;

	int n;
	for (n=0; enum2str(n, cfg->unit_levels); n++) ;

	for (int i=0; i<n; i++) {
		char *tmp = enum2str(i, cfg->unit_levels);
		alloc_level_options(optlist, OS_CFG, cfg, i, tmp);
		alloc_level_options(langoptlist, OS_LANG, NULL, i, tmp);
	}
}

/*
 *	invoke_set_action gets invoked twice for options whose action bit
 *		is set.  The pre-set call receives and returns a value,
 *		the post-set call receives a NULL and returns anything.
 *
 *		If a pre-set call returns a NULL, the option will not
 *		be changed and no post-set call occurs.
 *
 *		All options have initially the action bit set, and
 *		invoke_set_action shall disable the action bit for
 *		options it doesn't handle specially.
 */

char *invoke_set_action(option *o, char *value)
{
	if (!strcmp(o->optname, "unit_levels") || !strcmp(o->optname, "C:unit_levels")) {
		if (value) {
			return pre_set_unit_levels(value);
		} else {
			post_set_unit_levels(value);
			return value;
		}
	}
	o->action = false;
	return value;
}

option *option_struct(const char *name, hash_table<char, option> *softopts)
{
	option *o;

	if (!option_set) make_option_set();
	if (!name || !*name) return NULL;
	o = option_set->translate(name);
	if (!o && softopts) o = softopts->translate(name);
	return o;
}

void unknown_option(char *name, char *)
{
	shriek(442, fmt("Never heard about \"%s\", option ignored", name));
}

void parse_cfg_str(char *val)
{
	char *brutto, *netto;
	bool tmp;
	
	tmp=0;
	if (*val==DQUOT && *(brutto=val+strlen(val)-1)==DQUOT && brutto[-1]!=ESCAPE) 
		tmp=1, *brutto=0;       //If enclosed in double quotes, kill'em.
	for (netto=val, brutto=val+tmp; *brutto; brutto++,netto++) {
		*netto=*brutto;
		if (*brutto==ESCAPE) *netto=esctab[*++brutto];
	}				//resolve escape sequences
	*netto=0;
}

bool set_option(option *o, char *val, void *base)
{
	if (!o) return false;
	if (o->action && !invoke_set_action(o, val)) return false;
	char *locus = (char *)base + o->offset;
	switch(o->opttype) {
		case O_BOOL:
			int tmp;
			tmp = str2enum(val, BOOLstr, false);
			*(bool *)locus = tmp & 1;
			if (cfg->paranoid && (!val || tmp == U_ILL)) 
				shriek(447, fmt("%s is used as a boolean value for %s", val, o->optname));
			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to %s\n",
				o->optname,enum2str(*(bool*)locus, BOOLstr));)
			break;
		case O_DBG_AREA:
			*(_DEBUG_AREA_ *)locus=(_DEBUG_AREA_)str2enum(val, DEBUG_AREAstr,-1);
			DEBUG(1,10,fprintf(STDDBG,"Debug focus set to %i\n",*(int*)locus);)
			break;
		case O_MARKUP:
			if((*(OUT_ML *)locus=(OUT_ML)str2enum(val, OUT_MLstr, U_ILL))==(int)U_ILL)
				shriek(447, fmt("Can't set %s to %s", o->optname, val));
			DEBUG(1,10,fprintf(STDDBG,"Markup language option set to %i\n",*(int*)locus);)
			break;
		case O_SYNTH:
			if((*(SYNTH_TYPE *)locus=(SYNTH_TYPE)str2enum(val, STstr, U_ILL))==(int)U_ILL)
				shriek(447, fmt("Can't set %s to %s", o->optname, val));
			DEBUG(1,10,fprintf(STDDBG,"Synthesis type option set to %i\n",*(int*)locus);)
			break;
		case O_CHANNEL:
			if((*(CHANNEL_TYPE *)locus=(CHANNEL_TYPE)str2enum(val, CHANNEL_TYPEstr, U_ILL))==(int)U_ILL)
				shriek(447, fmt("Can't set %s to %s", o->optname, val));
			DEBUG(1,10,fprintf(STDDBG,"Channel type option set to %i\n",*(int*)locus);)
			break;
		case O_UNIT:
			if((*(UNIT *)locus=str2enum(val, cfg->unit_levels, U_ILL))==U_ILL) 
				shriek(447, fmt("Can't set %s to %s", o->optname, val));
			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to %d\n",o->optname,*(int *)locus);)
			break;
		case O_INT:
			*(int *)locus=0;
			if (!sscanf(val,"%d",(int*)locus)) shriek(447, "Unrecognized numeric parameter");
			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to %d\n",o->optname,*(int *)locus);)
			break;
		case O_STRING: 
			parse_cfg_str(val);
			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to \"%s\"\n",o->optname,val);)
			*(char**)locus = strdup(val);	// FIXME: should be forever if monolith etc. (maybe)
			break;
		case O_CHAR:
			parse_cfg_str(val);
			if (val[1]) shriek(447, fmt("Multiple chars given for a CHAR option %s", o->optname));
			else *(char *)locus=*val;
//			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to \"%s\"\n",o->optname,val);)
			break;
		case O_LANG:
			if (!lang_switch(val)) shriek(443, "unknown language");
			break;
		case O_VOICE:
			if (!voice_switch(val)) shriek(443, "unknown voice");
			break;
		default: shriek(462, fmt("Bad option type %d", (int)o->opttype));
	}
	if (o->action) invoke_set_action(o, NULL);

	return true;
}

/*
 *	C++ is evil with respect to double indirection.
 *	The casts to cowabilium below are a braindamage,
 *	unless the idea of a typed language is a braindamage.
 *	C++ tries to force us to pass it by reference,
 *	but struct cowabilium should not have formed a basis for
 *	inheritance in the first place.
 *	
 *	The order of cowing cfg, lang and voice, is important.
 */

#define  VOICES_OFFSET  ((int)&((lang *)NULL)->voices)
#define  VOICES_LENGTH  (this_lang->n_voices * sizeof(void *))

bool set_option(option *o, char *value)
{
	if (!o) return false;
	switch(o->structype) {
		case OS_CFG:	cow_configuration(&cfg);
				return set_option(o, value, cfg);
		case OS_LANG:	cow_configuration(&cfg);
				cow((cowabilium **)&this_lang, sizeof(lang),   VOICES_OFFSET, VOICES_LENGTH);
				return set_option(o, value, this_lang);
		case OS_VOICE:	cow_configuration(&cfg);
				cow((cowabilium **)&this_lang, sizeof(lang),   VOICES_OFFSET, VOICES_LENGTH);
				cow((cowabilium **)&this_voice, sizeof(voice) + (this_lang->soft_options->items * sizeof(void *) >> 1), 0, 0);
				return set_option(o, value, this_voice);
	}
	return false;
}

static inline bool set_option(char *name, char *value)
{
//	option *o = option_struct(name, NULL);
//	... tvorba indexu ...
//	char *tmp;
//	if ((tmp = strchr(name, INDEXER)) {
//		UNIT x = str2enum(tmp + 1, cfg->unit_levels, U_ILL);
//		if (x == U_ILL) 
//	}
	return set_option(option_struct(name, NULL), value);
}

static inline void set_option_or_die(char *name, char *value)
{
	option *o = option_struct(name, NULL);
	if (!o) shriek(814, fmt("Unknown option %s", name));
	if (!cfg->langs && o->structype != OS_CFG) return;

	if (set_option(o, value)) return;
	shriek(814, fmt("Bad value %s for option %s", value, name));
}

/*
 *	For the following one, make sure that base is the correct type
 */

static inline bool set_option(char *name, char *value, void *base, hash_table<char, option> *softopts)
{
	return set_option(option_struct(name, softopts), value, base);
}

bool lang_switch(const char *value)
{
	for (int i=0; i < cfg->n_langs; i++)
		if (!strcmp(cfg->langs[i]->name, value)) {
			if (!cfg->langs[i]->n_voices)		// FIXME
				shriek(462, "Switch to a mute language unimplemented");
			cfg->default_lang = i;
			return true;
		}
	return false;
}

bool voice_switch(const char *value)
{
	cow((cowabilium **)&(this_lang), sizeof(lang), VOICES_OFFSET, VOICES_LENGTH);	//new

	for (int i=0; i < this_lang->n_voices; i++)
		if (!strcmp(this_lang->voices[i]->name, value)) {
			this_lang->default_voice = i;
			return true;
		}
	return false;
}

char *format_option(option *o, void *base)
{
	char *locus = (char *)base + o->offset;
	switch(o->opttype) {
		case O_BOOL:
			return *(bool *)locus ? "on" : "off";
		case O_DBG_AREA:
			return enum2str(*(int *)locus, DEBUG_AREAstr);
		case O_MARKUP:
			return enum2str(*(int *)locus, OUT_MLstr);
		case O_SYNTH:
			return enum2str(*(int *)locus, STstr);
		case O_UNIT:
			return enum2str(*(UNIT *)locus, cfg->unit_levels);
		case O_INT:
			sprintf(scratch, "%d", *(int *)locus);
			return scratch;
		case O_STRING: 
			return *(char **)locus;
		case O_CHAR:
			scratch[0] = *(char *)locus;
			scratch[1] = 0;
			return scratch;
		case O_LANG:
			return (char *)this_lang->name;
		case O_VOICE:
			return (char *)this_voice->name;
		default: shriek(462, fmt("Bad option type %d", (int)o->opttype));
	}
	return NULL; /* unreachable */
}

char *format_option(option *o)
{
	switch(o->structype) {
		case OS_CFG:   return format_option(o, cfg);
		case OS_LANG:  return format_option(o, this_lang);
		case OS_VOICE: return format_option(o, this_voice);
		default: shriek(861, "Bad option class");
	}
	return NULL; /* unreachable */
}

char *format_option(const char *name)
{
	option *o = option_struct(name, this_lang->soft_options);
	if (!o) {
		shriek(442, fmt("Not returning empty string for nonexistant option %s", name));	//FIXME?
		return NULL; /* unreachable */
	}
	return format_option(o);
}

void parse_cmd_line()
{
	char *ar;
	char *j;
	register char *par;

	for(int i=1; i<argc; i++) {
		j = ar = argv[i];
		switch(strspn(ar, CMD_LINE_OPT)) {
		case 3:
			ar+=3;
			if (strchr(ar, CMD_LINE_VAL) && cfg->warnings) 
				shriek(814, "Thrice dashed options have an implicit value");
			set_option_or_die(ar, "0");
			break;
		case 2:
			ar+=2;
			par=strchr(ar, CMD_LINE_VAL);
			if (par) {					//opt=val
				*par=0;
				set_option_or_die(ar, par+1);
				*par=CMD_LINE_VAL;
			} else	if (i+1==argc || strchr(CMD_LINE_OPT, *argv[i+1])) 
					set_option_or_die(ar, "");
				else set_option_or_die(ar, argv[++i]);
			break;
		case 1:
			for (j = ar+1; *j; j++) switch (*j) {
				case 'b': cfg->out_verbose=false; break;
				case 'd': cfg->show_diph=true; break;
				case 'e': cfg->show_phones=true; break;
				case 'f': cfg->forking=false; break;
				case 'H': cfg->long_help=true;	/* fall through */
				case 'h': cfg->help=true; break;
				case 'n': cfg->rules_file="nnet.rul";
					  if (this_lang)
						this_lang->rules_file = cfg->rules_file;
					  cfg->neuronet=true; break;
				case 'p': cfg->pausing=true; break;
				case 's': cfg->play_diph=true; break;
				case 'v': cfg->version=true; break;
				case 'D':
					if (!cfg->use_dbg) cfg->use_dbg=true;
					else if (cfg->warnings)
						cfg->always_dbg--;
					break;
				default : shriek(442, fmt("Unknown option -%c, ignored", *j));
			}
			if (!ar[1]) {
				cfg->input_file = "";   	//dash only
				if (this_lang) this_lang->input_file = "";
			}
			break;
		case 0:
			if (cfg->input_text && cfg->input_text!=ar) {
				if (!cfg->warnings) break;
				if (cfg->paranoid) shriek(814, "Quotes forgotten on the command line?");
				scratch = (char *) xmalloc(strlen(ar)+strlen(cfg->input_text)+2);
				sprintf(scratch, "%s %s", cfg->input_text, ar);
				ar = FOREVER(strdup(scratch));
				free(scratch);
			}
			cfg->input_text = ar;
			break;
		default:
			if (cfg->warnings) shriek(814, "Too many dashes");
		}
	}
}

void load_config(const char *filename, const char *dirname, const char *what,
		OPT_STRUCT type, void *whither, lang *parent_lang)
{
	int i;

	if (!filename || !*filename) return;
	DEBUG(3,10,fprintf(STDDBG,"Loading %s from %s\n", what, filename);)
	char *line = (char *)xmalloc(cfg->max_line + 2) + 2;
	line[-2] = "CLV"[type];
	line[-1] = ':';
	text *t = new text(filename, dirname, "", what, true);
	while (t->getline(line)) {
		char *value = split_string(line);
		if (value && *value) {
			for (i = strlen(value)-1; strchr(WHITESPACE, value[i]) && i; i--);
			if (value[i-1] == ESCAPE && value[i] && i) i++;
			if (value[i]) i++;
			value[i] = 0;		// clumsy: strip off trailing whitespace
		}
		if (!set_option(line - 2, value, whither, parent_lang ? parent_lang->soft_options : (hash_table<char, option> *)NULL))
			shriek(812, fmt("Bad option %s %s in %s", line, value, compose_pathname(filename, dirname)));
	}
	free(line - 2);
	delete t;
}

void load_config(const char *filename)
{
	load_config(filename, cfg->ini_dir, "config", OS_CFG, cfg, NULL);
}

static inline void add_language(const char *lng_name)
{
	char *filename = (char *)xmalloc(strlen(lng_name) + 6);
	char *dirname = (char *)xmalloc(strlen(lng_name) + 6);

	DEBUG(3,10, fprintf(STDDBG, "Adding language %s\n", lng_name);)
	sprintf(filename, "%s.ini", lng_name);
	sprintf(dirname, "%s%c%s", cfg->lang_base_dir, SLASH, lng_name);
	if (*lng_name) {
		if (!cfg->langs) cfg->langs = (lang **)xmalloc(8 * sizeof (void *));
		else if (!(cfg->n_langs-1 & cfg->n_langs) && cfg->n_langs > 4)	    // n_langs == 8, 16, 32...
			cfg->langs = (lang **)xrealloc(cfg->langs, cfg->n_langs << 1);
		cfg->langs[cfg->n_langs++] = new lang(filename, dirname);
	}
	free(filename);
	free(dirname);
}

static inline void load_languages(const char *list)
{
	int i;
	int j=0;
	char *tmp = (char *)xmalloc(strlen(list)+1);


	for (i=0; (tmp[j] = list[i]); i++) {
		if (tmp[j] == ':' ) {
			tmp[j] = 0;
			add_language(tmp);
			j = 0;
		} else j++;
	}
	add_language(tmp);
	free(tmp);
}

static inline void version()
{
	fprintf(cfg->stdshriek, "This is Epos version %s, bug reports to \"%s\" <%s>\n", VERSION, MAINTAINER, MAIL);
}

static inline void dump_help()
{
	int i,j,k;

	printf("usage: %s [options] ['Text to be processed']\n", argv[0]);
	printf(" -b  bare format (no frills)\n");
	printf(" -d  show diphones\n");
	printf(" -e  show phones\n");
	printf(" -f  disable forking (detaching) the daemon\n");
	printf(" -n  same as --neuronet --rules_file nnet.rul\n");
	printf(" -p  pausing - show every intermediate state\n");
	printf(" -s  speak it\n");
	printf(" -v  show version\n");
	printf(" -D  debugging info (more D's - more detailed)\n");
	printf(" -   keyboard input\n");
	printf(" --long_options    ...see src/options.lst or 'epos -H' for these\n");
	if (!cfg->long_help) exit(0);

	printf("Long option types: (b) boolean,    (c) character, (e) special enumerated\n");
	printf("                   (n) int number, (s) string,    (u) unit level\n");
	k = 0;
	for (i=0; optlist[i].optname;i++) {
		if (*optlist[i].optname) {
			printf("--%s(%c)", optlist[i].optname, "bueeeencs"[optlist[i].opttype]);
			for (j = -(signed int)strlen(optlist[i].optname)-5; j <= 0; j += 26) k++;
			for (; j>0; j--) printf(" ");
			if (k >= 3) printf("\n"), k=0;
		}
	}
	printf("\n");
	exit(0);
}

void check_cfg_version(const char *filename)
{
	file *f = claim(filename, "", "", "r", NULL);
	if (!f) shriek(843, "Configuration files not installed or very old");
	if (strncmp(f->data, VERSION, strlen(VERSION)) && cfg->paranoid) {
		DEBUG(3,10,fprintf(STDDBG, "Expected version %s, found version %s\n",
			VERSION, f->data);)
		shriek(843, "Configuration version bad");
	}
	unclaim(f);
}

void config_init()
{
	const char *mlinis[] = {"","ansi.ini","rtf.ini", NULL};

	make_option_set();
	parse_cmd_line();	/* this ordering forbids --base_dir for allowed.ini on the cmd line */
	DEBUG(3,10,fprintf(STDDBG,"Base directory is %s\n", cfg->base_dir);)
	DEBUG(2,10,fprintf(STDDBG,"Using configuration file %s\n", cfg->inifile);)

	load_config(cfg->fixedfile);
	parse_cmd_line();

	check_cfg_version("version");

	load_config(mlinis[cfg->ml]);
	load_config(cfg->inifile);
	parse_cmd_line();
	load_languages(cfg->languages);

	if (!this_voice) shriek(842, "No voices configured");

	cfg->warnings = true;
	parse_cmd_line();
	cfg->loaded=true;
	
	cfg->use_diph = cfg->show_diph | cfg->play_diph | cfg->imm_diph;
	
	hash_max_line = cfg->max_line;

	if (cfg->version) version();
	if (cfg->help || cfg->long_help) dump_help();
}

void config_release()
{
	delete option_set;
	option_set = NULL;
}
