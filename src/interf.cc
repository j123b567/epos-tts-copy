/*
 *	epos/src/interf.cc
 *	(c) 1996-98 geo@ff.cuni.cz
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
#include "exc.cc"
#include <time.h>	// used to initialize the rand number generator

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <sys/stat.h>

#if (('a'-'A')!=32 || '9'-'0'!=9)	//Well, we rely on these a few times I think
#error If this machine doesn't believe in ASCII, I wouldn't port it hither.
#error Hmmm... but you will manage it.
#endif

#define CMD_LINE_OPT	"-"
#define CMD_LINE_VAL	'='

// FILE *stdshriek;
// FILE *stdwarn;
// FILE *stddbg;

char *scratch = NULL;

char *esctab = NULL;

int unused_variable;


void shriek(int code, const char *s) 
{	/* Art (c) David Miller */
	if (cfg->shriek_art == 1) fprintf(cfg->stdshriek,
"              \\|/ ____ \\|/\n"
"              \"@'/ ,. \\`@\"\n"
"              /_| \\__/ |_\\\n"
"                 \\__U_/\n");
	if (cfg->shriek_art == 2) fprintf(cfg->stdshriek, "\nSuddenly, something went wrong.\n\n");
	color(cfg->stdshriek, cfg->shriek_col);
	fprintf(cfg->stdshriek, "Fatal: %s\n",s); 
	color(cfg->stdshriek, cfg->normal_col);

	FILE *hackfile = fopen("hackfile","w");
	fprintf(hackfile, s);
	fclose(hackfile);

	switch (code / 100) {
		case 4 :
//			printf("Just a command will fail.\n");
			throw new command_failed (code, s);

		case 6 :
			throw new connection_lost (code, s);

		case 8 :
			printf("Abnormal condition: %s (code %d)\n", s, code);
			if (errno) printf("Current errno value: %d (%s)\n", errno, sys_errlist[errno]);
			throw new fatal_error (code, s);

			printf("Your compiler does NOT support exception handling, aborting\n");
			fflush(NULL);
			abort();
		default:  shriek(869, fmt("Bad error class %d", code));
	}
}

char error_fmt_scratch[MAX_ERR_LINE];

char *fmt(const char *s, int i) 
{
	sprintf(error_fmt_scratch, s, i);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, const char *u) 
{
	sprintf(error_fmt_scratch, s, t, u);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, int i)
{
	sprintf(error_fmt_scratch, s, t, i);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t)
{
	sprintf(error_fmt_scratch,s,t);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, const char *u, int i)
{
	sprintf(error_fmt_scratch, s, t, u, i);
	return error_fmt_scratch;
}

char *fmt(const char *s, int i, int j)
{
	sprintf(error_fmt_scratch, s, i, j);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, const char *u, const char *v)
{
	sprintf(error_fmt_scratch, s, t, u, v);
	return error_fmt_scratch;
}

void hash_shriek(const char *s1, const char *s2, int i)
{
	shriek(812, fmt(s1, s2, i));	//FIXME?
}

void user_pause()
{
	printf("Press Enter\n");
	getchar();
}


char *split_string(char *param)
{
	char *value;
	param += strspn(param, WHITESPACE);
	value = param + strcspn(param, WHITESPACE);
	if (*value) *value++ = 0;
	value += strspn(value, WHITESPACE);
	return value;
}

FILE *
fopen(const char *filename, const char *flags, const char *reason)
{
	FILE *f;
	char *message;

	if (!filename || !*filename)
		return *flags == 'r' ? stdin : stdout;
	switch (*flags) {
		case 'r': message = "Failed to read %s from %s: %s"; break;
		case 'w': message = "Failed to write %s to %s: %s"; break;
		default : shriek(861, fmt("Bad flags for %s", reason)); message = NULL;
	}
	f = fopen(filename, flags);
	if (!f && reason) shriek(445, fmt(message, reason, filename, strerror(errno)));
	if (cfg->paranoid && f) {
		if (!fread(&message, 1, 1, f))
			shriek(445, fmt(message, reason, filename, "maybe a directory"));
		rewind(f);
	}
	return f;
}

/*
 *	async_close() function is equivalent to close(), except that
 *	it returns immediately (and without a return value, which may
 *	be still unknown at the moment). Doing the close is left to
 *	a child process; as soon, as the child holds the only file
 *	descriptor copy in question, it is killed, which implies
 *	closing its file descriptors.
 */

void async_close(int fd)
{
	int pid = fork();
	switch(pid)
	{
		case -1:
			if(close(fd)) shriek(665,"Error on close()");
			return;
		case 0:
			sleep(1800);	/* will be killed by the parent soon */
			abort();	/* hopefully impossible */
		default:
			if(close(fd)) shriek(665, "Error on close()");
			kill(pid, SIGKILL);
	}
}

void colorize(int level, FILE *handle)
{
	if (!cfg->colored) return; 
	if (level==-1) {
		fputs(cfg->normal_col, handle);
		return;
	}
	fputs(cfg->out_color[level],handle);
}

FIT_IDX fit(char c)
{
	switch(c) {
		case 'f':
		case 'F': return Q_FREQ;
		case 'i':
		case 'I': return Q_INTENS;
		case 't':
		case 'T': return Q_TIME;
		default: shriek(811, fmt("Expected f,i or t, found %c",c));
			return Q_FREQ;
	}
}

#define LOWERCASED(x) (((x)<'A' || (x)>'Z') ? (x) : (x)|('a'-'A'))

		//list is colon-separated, case insensitive.  
UNIT str2enum(const char *item, const char *list, int dflt) 
{
	const char *i = list;
	const char *j = item;
	int k = 0;
	if (!item)  return (UNIT)dflt;
	for(; i && *i; j++,i++) {
		if (!*j && (*i==LIST_DELIM)) return UNIT(k);
		if (LOWERCASED(*i) != LOWERCASED(*j)) k++, j=item-1, i=strchr(i,LIST_DELIM);
//		DEBUG(0,0,fprintf(STDDBG,"str2enum %s %s %d\n",i,j,k);)
	}
	if(i && j && !*j) return (UNIT)k;
	DEBUG(3,0,fprintf(STDDBG,"str2enum returns ILL for %s %s\n",item, list);)
	return U_ILL;
}

char _enum2str_buff[20];

char *enum2str(int item, const char *list)
{
	const char *i;
	char *b=_enum2str_buff;
	for(i=list; *i && item+1; i++) {
		if (*i==LIST_DELIM) {
			item--;
			*b=0;
			b=_enum2str_buff;
		}
		else *b++ = *i;
	}
	if (*i && item>0) shriek(446, fmt("enum2str should stringize %d",item));
	return _enum2str_buff;
}

hash *str2hash(const char *list, unsigned int max_item_len)
{
	const char *item;
	int i,d;
	hash * h;

	for (i=0,d=0; list[i]; i++) if (list[i]==LIST_DELIM) d++;
	h = new hash(d*2);
	for (; d; d--) {
		item=enum2str(d-1, list);	// sllooww
		h->add_int(item,d-1);
		if (max_item_len && strlen(item) > max_item_len)
			shriek(446, fmt("Directive %s too long", item));
	}
	return h;
}

/*
 *  In a sense, str2units is the main routine responsible for the whole
 *  conversion of input text to the textual structure of the text just
 *  before the rules are to be applied.
 */

unit *str2units(char *text)
{
	unit *root;
	parser *parsie;

	if (text && *text) parsie = new parser(text, 1);
	else parsie = new parser(this_lang->input_file, 0);
	root=new unit(U_TEXT, parsie);
	delete parsie;
	return root;
}



char *fntab(const char *s, const char *t)
{
	char *tab;
	int tmp;
	tab=(char *)malloc(256); for(tmp=0; tmp<256; tmp++) tab[tmp] = (unsigned char)tmp;  //identity mapping
	if(cfg->paranoid && (tmp=strlen(s)-strlen(t)) && t[1]) 
		shriek(811, fmt(tmp>0 ? "Not enough (%d) resultant elements"
					: "Too many (%d) resultant elements",abs(tmp)));
	if(!t[1]) for (tmp=0; s[tmp]; tmp++) tab[(unsigned char)s[tmp]]=*t;
	else for (tmp=0;s[tmp]&&t[tmp];tmp++) tab[(unsigned char)(s[tmp])]=t[tmp];
	return(tab);
}


bool *booltab(const char *s)
{
	bool *tab;
	const char *tmp;
	bool mode = true;
	tab = (bool *)calloc(sizeof(bool), 256);         //should always return zero-filled array
	
	DEBUG(0,0,fprintf(STDDBG, "gonna booltab out of %s\n", s););
	if (*s==EXCL) memset(tab, true, 256*sizeof(bool));
	for(tmp=s; *tmp; tmp++) switch (*tmp) {
		case EXCL: mode = !mode; break;
		case ESCAPE: if (!*++tmp) tmp--;	// and fall through
		default:  tab[(unsigned char)(*tmp)]=mode;
	}
	return(tab);
}

/*
 * be slow and robust
 * if filename is absolute, ignore the dirname
 * if filename begins with "./", ignore the dirname
 * non-absolute dirnames start from cfg->base_dir
 * convert any slashes to backslashes in DOS
 * the compiler will notice if '/'==SLASH and test only once
 */
 
#define IS_NOT_SLASH(x) (x!=SLASH && x!='/')

char *compose_pathname(const char *filename, const char *dirname)
{
	register int tmp=0;
	char *pathname;

	if (!filename || !dirname) return strdup("");
	if (!*dirname) dirname = ".";
	pathname = (char *)malloc(strlen(filename) + strlen(dirname) + strlen(cfg->base_dir) + 3);
	if (IS_NOT_SLASH(*filename) && (filename[0]!='.' || IS_NOT_SLASH(filename[1]))) {
		if (IS_NOT_SLASH(*dirname)) {
			strcpy(pathname, cfg->base_dir);
			tmp = strlen(pathname);
			if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++]=SLASH;
		}
		strcpy(pathname+tmp, dirname);
		tmp = strlen(pathname);
		if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++]=SLASH;
	}
	strcpy(pathname+tmp, filename);

#if SLASH!='/'
	for (tmp = 0; pathname[tmp]; tmp++)
		if (pathname[tmp]=='/') pathname[tmp] = SLASH;
#endif
	return pathname;
}

char *limit_pathname(const char *filename, const char *dirname)
{
	char *p;
	char *r;
	if ((int)strlen(filename) >= cfg->scratch) shriek(864, "File name too long");
	strcpy(scratch, filename);
	for (p = scratch; !IS_NOT_SLASH(*p); p++);
	for (r = scratch; (*r = *p); p++, r++) {
		if (IS_NOT_SLASH(*p) || p[1] != '.' || p[2] != '.'
				|| p[3] && IS_NOT_SLASH(p[3]))  continue;
		while (IS_NOT_SLASH(*--r) && scratch < r) ;
		p += 3;
	}
	return compose_pathname(scratch, dirname);
}

#undef IS_NOT_SLASH

/*
 *	If file does not exist or stat() is broken, return 0 for a timestamp
 */

int get_timestamp(char *filename)
{
	struct stat buff;
	if (stat(filename, &buff)) return 0;
	return buff.st_ctime;
}

file::~file()
{
	if (ref_count) shriek(862, fmt("File %s not unclaimed", filename));
	free(data);
	free(filename);
}

static hash_table<char, file> *file_cache = NULL;

inline bool reclaim(file *ff, const char *flags, const char *description)
{
	FILE *f;
	int tmp;
	int ts = get_timestamp(ff->filename);

	if (ts == ff->timestamp)
		return false;
	ff->timestamp = ts;
	f = fopen(ff->filename, flags, description);
	fseek(f, 0, SEEK_END);
	ff->data = (char *)realloc(ff->data, tmp = ftell(f) + 1);
	rewind(f);
	ff->data[fread(ff->data, 1, tmp, f)] = 0;
	fclose(f);
	DEBUG(1,0,fprintf(STDDBG, "cache update for %s\n", ff->filename);)
	return true;
}

file *claim(const char *filename, const char *dirname, const char *flags, const char *description)
{
	FILE *f;
	file *ff;
	char *pathname;
	char *data;
	int tmp;
	
	if (!file_cache) {
		file_cache = new hash_table<char, file>(30);
		file_cache->dupkey = file_cache->dupdata = false;
	}

	pathname = compose_pathname (filename, dirname);
	ff = file_cache->translate(pathname);
	if (ff) {
		reclaim(ff, flags, description);
		DEBUG(1,0,fprintf(STDDBG, "cache hit on %s\n", pathname);)
		free(pathname);
		ff->ref_count++;
		return ff;
	}

	f = fopen(pathname, flags, description);
	fseek(f, 0, SEEK_END);
	data = (char *)malloc(tmp = ftell(f) + 1);
	rewind(f);
	data[fread(data, 1, tmp, f)] = 0;	  // remember DOS crlfs. tmp suits here.
	fclose(f);

	ff = new file;
	ff->ref_count = 1;
	ff->data = data;
	ff->filename = pathname;
	ff->timestamp = get_timestamp(pathname);
	file_cache->add(pathname, ff);
	DEBUG(1,0,fprintf(STDDBG, "cache miss on %s\n", pathname);)
	return ff;
}

void uncache_file(char *, file *ff)	// the first argument is ignored
{
	delete file_cache->remove(ff->filename);
}

void unclaim(file *ff)
{
	ff->ref_count--;
	if (!ff->ref_count) {
		if (cfg->lowmemory) uncache_file(NULL, ff);
	}
}

void shutdown_file_cache()
{
	if (!file_cache) return; // shriek("No file cache to shutdown");
	if (file_cache->items)
		file_cache->forall(uncache_file);
	delete file_cache;
	file_cache = NULL;
}


/*
 *	cow - this routine should detect whether a piece of memory used
 *		for global/language/voice configuration is shared with
 *		supposedly logical copies, and, if so, copy it physically
 *		before changing it. A nice space-saving technique.
 *
 *	cowprep - declares all current configuration as owned by the parameter.
 *
 *	Semantics: if e.g. cfg->cow, we do not own this cfg.
 */

void cow(cowabilium **p, int size)
{
	void *src;
	cowabilium **ptr = p; 

	if ((*p)->cow) {
		src = *p;
		*ptr = (cowabilium *)malloc(size);
		memcpy(*ptr, src, size);
		(*ptr)->cow = NULL;
	}
}

void cow_claim(void *c)
{

	if (cfg->cow) return;
	for (int i=0; i < cfg->n_langs; i++) {
		lang *l = cfg->langs[i];
		if (l->cow) continue;
		l->cow = c;
		for (int j=0; j < l->n_voices; j++) {
			if (l->voices[j]->cow) continue;
			l->voices[j]->cow = c;
		}
	}
	cfg->cow = c;
}


#define CONFIG_INITIALIZE
inline configuration::configuration() : cowabilium()
{
	#include "options.cc"

	n_langs = 0;
	langs = NULL;

	stdshriek = stderr;
	stddbg = stdout;

	current_stream = NULL;
}

configuration master_cfg;

configuration *cfg = &master_cfg;

#define CONFIG_DESCRIBE
option optlist[]={
        #include "options.cc"
	{NULL}
};

hash_table<char, option> *option_set = NULL;

void configuration::shutdown()
{
	int i;
	for (i=0; i < n_langs; i++) delete langs[i];
	free(langs);
	langs = NULL;
	this_lang = NULL; this_voice = NULL;
	n_langs = 0;
}

/*
 * Some string trickery below. option->name is initialised (in options.cc)
 *	to   "X:" "string"+2. In C, it means the same as "X:string"+2,
 *	which means "string" with "X:" immediately preceding it.
 * Now we add to our option_set both normal option names ("string")
 *	and their prefixed spellings ("X:string", for X being either
 *	C, L or V for global/language/voice configuration items,
 *	respectively), eating up a minimum memory space only.
 */


void make_option_set()
{
	option *o;

	if (option_set) return;

	option_set = new hash_table<char, option>(300);
	option_set->dupkey = option_set->dupdata = 0;

	for (o = optlist; o->optname; o++) option_set->add(o->optname, o);
	for (o = langoptlist; o->optname; o++) option_set->add(o->optname, o);
	for (o = voiceoptlist; o->optname; o++) option_set->add(o->optname, o);

	for (o = optlist; o->optname; o++) option_set->add(o->optname-2, o);
	for (o = langoptlist; o->optname; o++) option_set->add(o->optname-2, o);
	for (o = voiceoptlist; o->optname; o++) option_set->add(o->optname-2, o);

	option_set->remove("languages");	// FIXME
	option_set->remove("language");
	option_set->remove("voices");
	option_set->remove("voices");

	text *t = new text(cfg->allow_file, cfg->ini_dir, NULL, true);
	if (!t->exists()) {
		delete t;
		return;		/* if no allow file, ignore it */
	}
	char *line = (char *)malloc(cfg->max_line);
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

#pragma argsused

void parse_cfg_str(char *val, const char *optname)
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
	DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to \"%s\"\n",optname,val);)
#ifndef DEBUGGING
	unuse(optname);
#endif
}

//FIXME: wild typing follows

bool set_option(option *o, char *val, void *base)
{
	if (!o) return false;
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
			if((*(UNIT *)locus=(UNIT)str2enum(val, UNITstr, U_ILL))==U_ILL) 
				shriek(447, fmt("Can't set %s to %s", o->optname, val));
			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to %d\n",o->optname,*(int *)locus);)
			break;
		case O_INT:
			*(int *)locus=0;
			if (!sscanf(val,"%d",(int*)locus)) shriek(447, "Unrecognized numeric parameter");
			DEBUG(1,10,fprintf(STDDBG,"Configuration option \"%s\" set to %d\n",o->optname,*(int *)locus);)
			break;
		case O_STRING: 
			parse_cfg_str(val, o->optname);
			*(char**)locus=FOREVER(strdup(val));
			break;
//		case O_FILE:
//			parse_cfg_str(val, o->optname);
//			*(char**)locus=freadin(val, cfg->ini_dir, "rt", "config file content");
//			break;
		case O_CHAR:
			parse_cfg_str(val, o->optname);
			if (val[1]) shriek(447, fmt("Multiple chars given for a CHAR option %s", o->optname));
			else *(char *)locus=*val;
			break;
		default: shriek(462, fmt("Bad option type %d", (int)o->opttype));
	}
	return true;
}

/*
 *	C++ is evil, or egcs does not implement C++ correctly.
 *	The casts to cowabilium below are a complete braindamage,
 *	unless the idea of a typed language is a braindamage.
 */

bool set_option(option *o, char *value)
{
	if (!o) return false;
	switch(o->structype) {
		case OS_CFG:	cow((cowabilium **)&cfg, sizeof(configuration));
				return set_option(o, value, cfg);
		case OS_LANG:	cow((cowabilium **)&this_lang, sizeof(lang));
				return set_option(o, value, this_lang);
		case OS_VOICE:	cow((cowabilium **)&this_voice, sizeof(voice) + (this_lang->soft_options->items * sizeof(void *) >> 1));
				return set_option(o, value, this_voice);
	}
	return false;
}

static inline bool set_option(char *name, char *value)
{
	return set_option(option_struct(name, NULL), value);
}

static inline void set_option_or_die(char *name, char *value)
{
	if (set_option(name, value)) return;
	shriek(814, fmt("Unknown option %s", name));
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
			else this_voice = *cfg->langs[i]->voices;
			this_lang = cfg->langs[i];
			return true;
		}
//	shriek("Switch to an unknown language");
	return false;
}

bool voice_switch(const char *value)
{
	for (int i=0; i < this_lang->n_voices; i++)
		if (!strcmp(this_lang->voices[i]->name, value)) {
			this_voice = this_lang->voices[i];
			return true;
		}
//	shriek("Switch to an unknown voice");
	return false;
}

char *format_option(option *o, void *base)
{
	char *locus = (char *)base + o->offset;
	switch(o->opttype) {
		case O_BOOL:
			return strdup(*(bool *)locus ? "on" : "off");
		case O_DBG_AREA:
			return strdup(enum2str(*(int *)locus, DEBUG_AREAstr));
		case O_MARKUP:
			return strdup(enum2str(*(int *)locus, OUT_MLstr));
		case O_SYNTH:
			return strdup(enum2str(*(int *)locus, STstr));
		case O_UNIT:
			return strdup(enum2str(*(int *)locus, UNITstr));
		case O_INT:
			sprintf(scratch, "%d", *(int *)locus);
			return strdup(scratch);
		case O_STRING: 
			return strdup(*(char **)locus);
//		case O_FILE:
//			warn("File option value no more kept");
//			return "no idea";
		case O_CHAR:
			scratch[0] = *(char *)locus;
			scratch[1] = 0;
			return strdup(scratch);
		default: shriek(462, fmt("Bad option type %d", (int)o->opttype));
	}
	return "(impossible value)";
}

char *format_option(option *o)
{
	switch(o->structype) {
		case OS_CFG:   return format_option(o, cfg);
		case OS_LANG:  return format_option(o, this_lang);
		case OS_VOICE: return format_option(o, this_voice);
	}
	return "?!";
}

char *get_named_cfg(const char *name)
{
	void *address;
	option *o = option_struct(name, this_lang && this_lang->soft_options
				? this_lang->soft_options : (hash_table<char, option>*)NULL);
	if (!o) {
		shriek(442, fmt("Not returning empty string for nonexistant option %s", name));	//FIXME?
		return "";
	}

	switch (o->structype) {
		case OS_CFG:	address = cfg;		break;
		case OS_LANG:	address = this_lang;	break;
		case OS_VOICE:	address = this_voice;	break;
		default: shriek(861, "Bad option class");
	}

	*(char **)&address += o->offset;

	if (o->readable == A_NOACCESS) return "nic nebude";	//FIXME

	switch (o->opttype) {
		case O_STRING:	return *(char **)address;
//		case O_BOOL:	return *(bool *)address ? "=" : "!=";
		default:	shriek(861, fmt("cfg::named_item not implemented for %s (not a string)", name));
				return NULL; /* unreachable */
	}
}

void parse_cmd_line()
{
	char *ar;
	char *j;
	register char *par;
//	hash *opts;

//	opts=new hash(argc|15);
	for(int i=1; i<argc; i++) {
		ar=argv[i];
		switch(strspn(ar, CMD_LINE_OPT)) {
		case 3:
			ar+=3;
			if (strchr(ar, CMD_LINE_VAL) && cfg->warnings) 
				shriek(814, "Thrice dashed options have an implicit value");
//			opts->add(ar, "0");
			set_option_or_die(ar, "0");
			break;
		case 2:
			ar+=2;
			par=strchr(ar, CMD_LINE_VAL);
			if (par) {					//opt=val
				*par=0;
//				opts->add(ar, par+1);
				set_option_or_die(ar, par+1);
				*par=CMD_LINE_VAL;
			} else	if (i+1==argc || strchr(CMD_LINE_OPT, *argv[i+1])) 
//					opts->add(ar, "");		//opt
					set_option_or_die(ar, "");
//				else opts->add(ar, argv[++i]);		//opt val
				else set_option_or_die(ar, argv[++i]);
			break;
		case 1:
			for (j=ar+1; *j; j++) switch (*j) {
				case 'b': cfg->out_verbose=false; break;
//				case 'c': cfg->colloquial=true; break;
				case 'd': cfg->show_diph=true; break;
				case 'e': cfg->show_phones=true; break;
				case 'f': cfg->forking=false; break;
				case 'H': cfg->long_help=true;	/* fall through */
				case 'h': cfg->help=true; break;
//				case 'i': cfg->irony=true; break;
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
			if (j==ar+1) {
				cfg->input_file = "";   	//dash only
				if (this_lang) this_lang->input_file = "";
			}
			break;
		case 0:
			if (cfg->input_text && cfg->input_text!=ar) {
				if (!cfg->warnings) break;
				if (cfg->paranoid) shriek(814, "Quotes forgotten on the command line?");
				scratch = (char *) malloc(strlen(ar)+strlen(cfg->input_text)+2);
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
//	process_options(opts, optlist, cfg);
//	delete opts;
}

void load_config(const char *filename, const char *dirname, const char *what,
		OPT_STRUCT type, void *whither, lang *parent_lang)
{
	int i;

	if (!filename || !*filename) return;
	DEBUG(3,10,fprintf(STDDBG,"Loading %s from %s\n", what, filename);)
	char *line = (char *)malloc(cfg->max_line + 2) + 2;
	line[-2] = "CLV"[type];
	line[-1] = ':';
	text *t = new text(filename, dirname, what, true);
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
	char *filename = (char *)malloc(strlen(lng_name) + 6);
	char *dirname = (char *)malloc(strlen(lng_name) + 6);

	DEBUG(3,10, fprintf(STDDBG, "Adding language %s\n", lng_name);)
	sprintf(filename, "%s.ini", lng_name);
	sprintf(dirname, "%s%c%s", cfg->lang_base_dir, SLASH, lng_name);
	if (*lng_name) {
		if (!cfg->langs) cfg->langs = (lang **)malloc(8 * sizeof (void *));
		else if (!(cfg->n_langs-1 & cfg->n_langs) && cfg->n_langs > 4)	    // n_langs == 8, 16, 32...
			cfg->langs = (lang **)realloc(cfg->langs, cfg->n_langs << 1);
		cfg->langs[cfg->n_langs++] = new lang(filename, dirname);
	}
	free(filename);
	free(dirname);
}

static inline void load_languages(const char *list)
{
	int i;
	int j=0;
	char *tmp = (char *)malloc(strlen(list)+1);


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

/****
static inline void load_diph_inv(const char *inv_name)
{
	char *tmp = (char *)malloc(strlen(cfg->invent_dir) + strlen(inv_name) + 6);
	sprintf(tmp, "%s%c%s.ini", cfg->invent_dir, SLASH, inv_name);
	if (*inv_name) {
		load_config(tmp, "Unknown voice");
	}
	free(tmp);
}
********/

static inline void version()
{
	fprintf(cfg->stdshriek, "This is Epos version %s, bug reports to \"%s\" <%s>\n", VERSION, MAINTAINER, MAIL);
}

static inline void dump_help()
{
	int i,j,k;

	printf("usage: %s [options] ['Text to be processed']\n", argv[0]);
	printf(" -b  bare format (no frills)\n");
//	printf(" -c  casual pronunciation\n");
	printf(" -d  show diphones\n");
	printf(" -e  show phones\n");
	printf(" -f  disable forking (detaching) the daemon\n");
//	printf(" -i  say ironically (experimental)\n");
	printf(" -n  same as --neuronet --rules_file nnet.rul\n");
	printf(" -p  pausing - show every intermediate state\n");
	printf(" -s  speak it\n");
	printf(" -v  show version\n");
	printf(" -D  debugging info (more D's - more detailed)\n");
	printf(" -   keyboard input\n");
	printf(" --long_options    ...see options.cc or 'epos -H' for these\n");
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

int argc=0;
char **argv=NULL;

static inline void release(char **buffer)
{
	if (*buffer) free(*buffer);
	*buffer = NULL;
}

/*
 *	epos_init(): to bring up everything (from a "main() {" state)
 *	epos_done(): to release everything just before exit() (no need to
 *		call this one except for debugging)
 *	epos_reinit(): to reread all files, adjust all structures
		Takes nearly as much time as killing and restarting
 *	epos_catharsis(): to release as much as possible, but leave a way back
 */

void epos_init(int argc_, char**argv_)	 //Some global sanity checks made here
{
	static const char * CFG_FILE_OPTION = "--cfg_file";
	register optlen=strlen(CFG_FILE_OPTION);
	register char *result;
	
	argc=argc_; argv=argv_;

	if ((result=getenv(CFG_FILE_ENVIR_VAR))) cfg->inifile=result;
	for (int i=1; i<argc-1; i++) if (!strncmp(argv[i], CFG_FILE_OPTION, optlen)) {
		switch (argv[i][optlen]) {
			case 0:	  cfg->inifile=argv[++i]; break;
			case '=': cfg->inifile=argv[i]+optlen+1; break;
			default:  /* another option, most likely a bug */;
		}
	}
	epos_init();
}


void epos_init()	 //Some global sanity checks made here
{
	int i;

	const char *mlinis[] = {"","ansi.ini","rtf.ini", NULL};

	if (!cfg->loaded)	cfg->stddbg = stdout,
				cfg->stdshriek = stderr;
	if (sizeof(int)<4*sizeof(char) || sizeof(int *)<4*sizeof(char)) 
		shriek (862, fmt("You dwarf! I want at least 32 bit arithmetic & pointery [%d]", sizeof(int)));

	srand(time(NULL));	// randomize

	if (!esctab) esctab = FOREVER(fntab(cfg->token_esc, cfg->value_esc));

	make_option_set();
	parse_cmd_line();	/* this ordering forbids --base_dir for allowed.ini on the cmd line */
	DEBUG(2,10,fprintf(STDDBG,"Using configuration file %s\n", cfg->inifile);)

	load_config(cfg->fixedfile);
	parse_cmd_line();

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

#ifdef DEBUGGING
	if (cfg->use_dbg && cfg->stddbg_file && *cfg->stddbg_file)
		cfg->stddbg = fopen(cfg->stddbg_file,"w","debugging messages");
#else
	if (cfg->use_dbg) shriek(813, "Either disable debugging in config file, or #define it in interf.h");
#endif
	cfg->stdshriek = stderr;
	if (cfg->stdshriek_file && *cfg->stdshriek_file)
		cfg->stdshriek = fopen(cfg->stdshriek_file, "w", "error messages");
		
	if (!_subst_buff) _subst_buff = (char *)malloc(MAX_GATHER+2);
	if (!_resolve_vars_buff) _resolve_vars_buff = (char *)malloc(cfg->max_line+1); 
	if (!scratch) scratch = (char *)malloc(cfg->scratch+1);
	
	_next_rule_line = (char *)malloc(cfg->max_line+1);
	for (i=0; i<cfg->n_langs; i++) cfg->langs[i]->compile_rules();
	free(_next_rule_line); _next_rule_line = NULL;

	DEBUG(1,10,fprintf(STDDBG,"struct configuration is %d bytes\n", sizeof(configuration));)
	DEBUG(1,10,fprintf(STDDBG,"struct lang is %d bytes\n", sizeof(lang));)
	DEBUG(1,10,fprintf(STDDBG,"struct voice is %d bytes\n", sizeof(voice));)
}

void end_of_eternity();

void epos_catharsis()
{
	if (_unit_just_unlinked) {  // to avoid a memory leak in unlink()
		_unit_just_unlinked->next=NULL;
		delete _unit_just_unlinked;
		_unit_just_unlinked=NULL;
	}

	if (_directive_prefices) delete _directive_prefices;
	_directive_prefices = NULL;

	// one a_protocol may be lost here, see agent.cc

	cfg->shutdown();

	shutdown_file_cache();
	delete option_set;
	option_set = NULL;
}

void epos_done()
{
	epos_catharsis();
	shutdown_hashing();
	shutdown_units();

	release(&_subst_buff);
	release(&_resolve_vars_buff);
	release(&scratch);
	
	END_OF_ETERNITY;
}

void epos_reinit()
{
	epos_catharsis();
	make_option_set();
	load_config("default.ini");
	epos_init();
}

#ifdef DEBUGGING

char *current_debug_tag = NULL;

int  debug_config(int area)
{
	switch (area) {
		case _INTERF_: return cfg->interf_dbg;
		case _RULES_:  return cfg->rules_dbg;
		case _ELEM_:   return cfg->elem_dbg;
		case _SUBST_:  return cfg->subst_dbg;
		case _ASSIM_:  return cfg->assim_dbg;
		case _SPLIT_:  return cfg->split_dbg;
		case _PARSER_: return cfg->parser_dbg;
		case _SYNTH_:  return cfg->synth_dbg;
		case _CFG_:    return cfg->cfg_dbg;
		case _DAEMON_: return cfg->daemon_dbg;
	}
	shriek(861, fmt("Unknown debug area %d", area));
	return 0;   // keep the compiler happy
}

bool debug_wanted(int lev, /*_DEBUG_AREA_*/ int area) 
{
	if (!cfg->use_dbg) return false;
	if (lev >= cfg->always_dbg) return true;
	if (area == cfg->focus_dbg) return lev >= debug_config(area);
	if (lev < cfg->limit_dbg)   return false;
	return  lev >= debug_config(area);
}

void debug_prefix(int lev, int area)
{
	unuse(lev); unuse(area);
	if (current_debug_tag) fprintf(STDDBG, "[%s] ", current_debug_tag);
//	if (this_lang && this_lang->name) printf("%s ", this_lang->name);
}

#endif   // ifdef DEBUGGING


#ifdef WANT_DMALLOC

static forever_count = 0;
static void *forever_ptr_list[ETERNAL_ALLOCS];

char *forever(void *heapptr)
{
	if (forever_count >= ETERNAL_ALLOCS) {
		shriek(863, "Too many eternal allocations (memory leak?)");
		return (char *)heapptr;
	}
	forever_ptr_list[forever_count++] = heapptr;
	return (char *)heapptr;
}

void end_of_eternity()
{
	DEBUG(3,0,fprintf(STDDBG,"Freeing %d permanent heap buffers.\n", forever_count);)
	while (forever_count-- > 0) {
		DEBUG(0,0,fprintf(STDDBG, "pointer number %d was %p\n", forever_count, forever_ptr_list[forever_count]);)
		free(forever_ptr_list[forever_count]);
	}
}

void *
operator new(size_t n)
{
	void *ret = malloc(n);
	return ret;
}

void
operator delete(void * cp)
{
	free(cp);
}

#endif   // ifdef WANT_DMALLOC


#ifndef HAVE_STRDUP

char *strdup(const char*src)
{
	return strcpy((char *)malloc(strlen(src)+1), src);
}

#endif   // ifdef HAVE_STRDUP

#ifndef HAVE_TERMINATE

void terminate(void)
{
	abort();
}

#endif

#ifndef HAVE_FORK
int fork()
{
	return -1;
}
#endif   // ifdef HAVE_FORK

