/*
 *	epos/src/interf.cc
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
#include "exc.cc"
#include <time.h>	// used to initialize the rand number generator

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_WINSOCK2_H
	#include <winsock2.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#if (('a'-'A')!=32 || '9'-'0'!=9)	//Well, we rely on these a few times I think
#error If this machine doesn't believe in ASCII, I wouldn't port it hither.
#error Hmmm... but you will manage it.
#endif

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

	command_failed *xcf;

	switch (code / 100) {
		case 4 :
//			printf("Just a command will fail.\n");
			xcf = new command_failed (code, s);
			throw xcf;

		case 6 :
			throw new connection_lost (code, s);

		case 8 :
			printf("Abnormal condition: %s (code %d)\n", s, code);
			if (errno) printf("Current errno value: %d (%s)\n", errno, strerror(errno));
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

char *fmt(const char *s, int t, const char *u)
{
	sprintf(error_fmt_scratch, s, t, u);
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

unit *str2units(const char *text)
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

char *compose_pathname(const char *filename, const char *dirname, const char *treename)
{
	register int tmp=0;
	char *pathname;

	if (!filename || !dirname || !treename) return strdup("");
	if (!*dirname) dirname = ".";
	pathname = (char *)malloc(strlen(filename) + strlen(dirname) + strlen(treename) + strlen(cfg->base_dir) + 4);
	if (IS_NOT_SLASH(*filename) && (filename[0]!='.' || IS_NOT_SLASH(filename[1]))) {
		if (IS_NOT_SLASH(*dirname)) {
			if (IS_NOT_SLASH(*treename)) {
				strcpy(pathname, cfg->base_dir);
				tmp = strlen(pathname);
				if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++] = SLASH;
			}
			if (*treename) {	// just optimizing away the non-typical branch
				strcpy(pathname+tmp, treename);
				tmp = strlen(pathname);
				if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++] = SLASH;
			}
		}
		strcpy(pathname+tmp, dirname);
		tmp = strlen(pathname);
		if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++] = SLASH;
	}
	strcpy(pathname+tmp, filename);

#if SLASH!='/'
	for (tmp = 0; pathname[tmp]; tmp++)
		if (pathname[tmp]=='/') pathname[tmp] = SLASH;
#endif
	return pathname;
}

char *compose_pathname(const char *filename, const char *dirname)
{
	return compose_pathname(filename, dirname, "");
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

file *claim(const char *filename, const char *dirname, const char *treename, const char *flags, const char *description)
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

	if (!filename) shriek(445, fmt("Failed to read %s (no file name)", description ? description : "??"));

	pathname = compose_pathname(filename, dirname, treename);
	ff = file_cache->translate(pathname);
	if (ff) {
		reclaim(ff, flags, description);
		DEBUG(1,0,fprintf(STDDBG, "cache hit on %s\n", pathname);)
		free(pathname);
		ff->ref_count++;
		return ff;
	}

	f = fopen(pathname, flags, description);
	if (fseek(f, 0, SEEK_END)) tmp = cfg->dev_txtlen;
	else tmp = ftell(f) + 1;
	data = (char *)malloc(tmp);
	rewind(f);
	tmp = fread(data, 1, tmp, f);
	if (tmp == -1) {
		if (!description) return NULL;
		shriek(445, fmt("Failed to read %s from %s (non-readable)", description, pathname));
	}
	data[tmp] = 0;	  // remember DOS crlfs. tmp suits here.
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

void uncache_file(char *, file *ff, void *)	// the first argument is ignored
{
	delete file_cache->remove(ff->filename);
}

void unclaim(file *ff)
{
	ff->ref_count--;
	if (!ff->ref_count) {
		if (cfg->lowmemory) uncache_file(NULL, ff, NULL);
	}
}

void shutdown_file_cache()
{
	if (!file_cache) return; // shriek("No file cache to shutdown");
	if (file_cache->items)
		file_cache->forall(uncache_file, NULL);
	delete file_cache;
	file_cache = NULL;
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
	register int optlen=strlen(CFG_FILE_OPTION);
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

	if (!cfg->loaded)	cfg->stddbg = stdout,
				cfg->stdshriek = stderr;
	if (sizeof(int)<4*sizeof(char) || sizeof(int *)<4*sizeof(char)) 
		shriek (862, fmt("You dwarf! I want at least 32 bit arithmetic & pointery [%d]", sizeof(int)));
	if (sizeof(int) > sizeof(void *)) shriek(862, "Your integers are longer than pointers!\n"
				"Turn hash_table::forall() fns in hash.h the other way round.");

	srand(time(NULL));	// randomize

	if (!esctab) esctab = FOREVER(fntab(cfg->token_esc, cfg->value_esc));

	config_init();

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

//	if (_directive_prefices) delete _directive_prefices;
//	_directive_prefices = NULL;

	// one a_protocol may be lost here, see agent.cc

	cfg->shutdown();

	shutdown_file_cache();
}

void epos_done()
{
	epos_catharsis();
	config_release();
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
	if (cfg->always_dbg > 10 || cfg->always_dbg < 0) shriek(862, "cfg bogus"); //FIXME: hack
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

static int forever_count = 0;
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


