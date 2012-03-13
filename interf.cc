/*
 *	ss/src/interf.cc
 *	(c) 1996-98 geo@ff.cuni.cz
 *
 */

#include "common.h"
#include <time.h>	// used to initialize the rand number generator

#if (('a'-'A')!=32)	//Well, we rely on this a few times I think
#error If this machine doesn't believe in ASCII, I wouldn't port it hither.
#error Hmmm... but you will manage it.
#endif

#define CMD_LINE_OPT	"-"
#define CMD_LINE_VAL	'='

FILE *stdshriek;
FILE *stdwarn;
FILE *stddbg;

char *scratch;

char *esctab;

void
check_lib_version(const char *version)
{
	if (strcmp(version, VERSION))
		shriek("Version mismatch - shared library is %s, expecting %s", VERSION, version);
	return;
}

void shriek(const char *s) 
{	/* Art (c) David Miller */
	if (cfg.shriek_art == 1) fprintf(stdshriek,
"              \\|/ ____ \\|/\n"
"              \"@'/ ,. \\`@\"\n"
"              /_| \\__/ |_\\\n"
"                 \\__U_/\n");
	if (cfg.shriek_art == 2) fprintf(stdshriek, "\nSuddenly, something went wrong.\n\n");
	color(stdshriek, cfg.shriek_col);
	fprintf(stdshriek, "Fatal: %s\n",s); 
	color(stdshriek, cfg.normal_col);
	exit(-1);
}

void shriek(const char *s, int i) 
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer, s, i);
	shriek(buffer);
}

void shriek(const char *s, const char *t, const char *u) 
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer, s, t, u);
	shriek(buffer);
}

void shriek(const char *s, const char *t, int i)
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer, s, t, i);
	shriek(buffer, i);
}

void shriek(const char *s, const char *t)
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer,s,t);
	shriek(buffer);
}

void shriek(const char *s, const char *t, const char *u, int i)
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer, s, t, u, i);
	shriek(buffer);
}

void shriek(const char *s, const char *t, const char *u, const char *v)
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer, s, t, u, v);
	shriek(buffer);
}


void warn(const char *s)
{
	if (!cfg.warnings) return;
	color(stdwarn,cfg.warn_col);
	fprintf(stdwarn, "Warning: %s\n",s);
	color(stdwarn,cfg.normal_col);
	if (cfg.warnpause) user_pause();
}

void warn(const char *s, int i)
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer,s,i);
	warn(buffer);
}

void warn(const char *s, const char *t)
{
	char buffer[MAX_ERR_LINE];
	sprintf(buffer,s,t);
	warn(buffer);
}

void user_pause()
{
	printf("Press Enter\n");
	getchar();
}

FILE *
fopen(const char *filename, const char *flags, const char *reason)
{
	FILE *f;
	char *message;
	switch (*flags) {
		case 'r': message = "Failed to read %s from %s: %s"; break;
		case 'w': message = "Failed to write %s to %s: %s"; break;
		default : shriek("Bad flags for %s", reason);
	}
	f = fopen(filename, flags);
	if (!f) shriek(message, reason, filename, strerror(errno));
	return f;
};

void colorize(int level, FILE *handle)
{
	if (!cfg.colored) return; 
	if (level==-1) {
		fputs(cfg.normal_col, handle);
		return;
	};
	fputs(cfg.out_color[level],handle);
};

FIT_IDX fit(char c)
{
	switch(c) {
		case 'f':
		case 'F': return Q_FREQ;
		case 'i':
		case 'I': return Q_INTENS;
		case 't':
		case 'T': return Q_TIME;
		default: shriek("Expected f,i or t, found %c",c);
			return Q_FREQ;
	};
}

#define LOWERCASED(x) (((x)<'A' || (x)>'Z') ? (x) : (x)|('a'-'A'))

		//list is colon-separated, case insensitive.  
UNIT str2enum(const char *item, const char *list, int dflt) 
{
	const char *i=list;
	const char *j=item;
	int k=0;
	for(;i&&*i;j++,i++) {
		if(!item)  return (UNIT)dflt;
		if(!*j && (*i==LIST_DELIM)) return UNIT(k);
		if(LOWERCASED(*i) != LOWERCASED(*j)) k++,j=item-1,i=strchr(i,LIST_DELIM);
		DEBUG(0,0,fprintf(stddbg,"str2enum %s %s %d\n",i,j,k);)
	};
	if(i&&j&&!*j) return (UNIT)k;
	DEBUG(3,0,fprintf(stddbg,"str2enum returns ILL for %s %s\n",item, list);)
	return U_ILL;
};

char _enum2str_buff[20];

char *enum2str(int item, const char *list)
{
	const char *i;
	char *b=_enum2str_buff;
	for(i=list;*i && item+1;i++) {
		if (*i==LIST_DELIM) {
			item--;
			*b=0;
			b=_enum2str_buff;
		}
		else *b++=*i;
	};
	if (*i && item>0) shriek("enum2str should stringize %d",item);
	return _enum2str_buff;
};

char *fntab(const char *s, const char *t)
{
	char *tab;
	int tmp;
	tab=(char *)malloc(256); for(tmp=0;tmp<256;tmp++) tab[tmp]=(Char)tmp;  //identity mapping
	if(cfg.paranoid && (tmp=strlen(s)-strlen(t)) && t[1]) 
		shriek(tmp>0?"Not enough (%d) resultant elements":"Too many (%d) resultant elements",abs(tmp));
	if(!t[1]) for (tmp=0;s[tmp];tmp++) tab[(Char)s[tmp]]=*t;
	else for (tmp=0;s[tmp]&&t[tmp];tmp++) tab[(Char)(s[tmp])]=t[tmp];
	return(tab);
}


bool *booltab(const char *s)
{
	bool *tab;
	const char *tmp;
	bool mode=true;
	tab=(bool *)calloc(sizeof(bool),256);           //should always return zero-filled array
	
	DEBUG(0,0,fprintf(stddbg, "gonna booltab out of %s\n", s););
	if (*s==EXCL) memset(tab,true,256*sizeof(bool));
	for(tmp=s;*tmp;tmp++) switch (*tmp) {
		case EXCL: mode=1-mode; break;
		case ESCAPE: if (!*++tmp) tmp--;	// and fall through
		default:  tab[(Char)(*tmp)]=mode;
	};
	return(tab);
}

/*
 * be slow and robust
 * if filename is absolute, ignore the dirname
 * if filename begins with "./", ignore the dirname
 * non-absolute dirnames start from cfg.base_dir
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
	pathname=(char *)malloc(strlen(filename)+strlen(dirname)+strlen(cfg.base_dir)+3);
	if (IS_NOT_SLASH(*filename) && (filename[0]!='.' || IS_NOT_SLASH(filename[1]))) {
		if (IS_NOT_SLASH(*dirname)) {
			strcpy(pathname, cfg.base_dir);
			tmp = strlen(pathname);
			if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++]=SLASH;
		}
		strcpy(pathname+tmp, dirname);
		tmp = strlen(pathname);
		if (IS_NOT_SLASH(pathname[tmp-1])) pathname[tmp++]=SLASH;
	};
	strcpy(pathname+tmp, filename);

#if SLASH!='/'
	for (tmp = 0; pathname[tmp]; tmp++)
		if (pathname[tmp]=='/') pathname[tmp] = SLASH;
#endif
	return pathname;
}


#undef IS_NOT_SLASH

char *freadin(const char *filename, const char *dirname)
{
	char *pathname;		//also used for the return value
	int tmp;
	FILE *file;

	pathname=compose_pathname (filename, dirname);
	file=fopen(pathname, "rt", "a configuration string");
	free(pathname);
	fseek(file, 0, SEEK_END);
	pathname=(char *)malloc(tmp=ftell(file)+1);
	rewind(file);
	pathname[fread(pathname,1,tmp,file)]=0;
	fclose(file);
	return pathname;
}


#define CONFIG_INITIALIZE
configuration cfg = {
	#include "options.cc"
};

struct option {
	const char *optname;
	OPT_TYPE opttype;
	void *optid;
};

#define CONFIG_DESCRIBE
option optlist[]={
        #include "options.cc"
	{NULL}
};

char *configuration::named_item(const char *name)
{
	option *o;
	for (o = optlist; o->optname; o++) if (!strcasecmp(o->optname, name)) {
		if (o->opttype == O_STRING) return *(char **)o->optid;
		if (o->opttype == O_BOOL) return *(bool *)o->optid ? "=" : "!=";
		shriek("cfg::named_item not implemented for %s (not a string)", name);
	}
	warn("Never heard about \"%s\", empty word returned", name);
	return "";
}

void unknown_option(char *name, char *data)
{
	unuse(data);
	warn("Never heard about \"%s\", option ignored", name);
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
	};				//resolve escape sequences
	*netto=0;
	DEBUG(1,0,fprintf(stddbg,"Configuration option \"%s\" set to \"%s\"\n",optname,val);)
#ifndef DEBUGGING
	unuse(optname);
#endif
}


void process_options(hash *tab)
{
	char *val;

	esctab[cfg.slash_esc]=SLASH;

	for (option *o=optlist; o->optname; o++) {
		val=tab->remove(o->optname);
		if (!val) continue;
		switch(o->opttype) {
		case O_BOOL:
			int tmp;
			tmp = str2enum(val, BOOLstr, false);
			*(bool *)o->optid = tmp & 1;
			if (cfg.paranoid && (!val || tmp == U_ILL)) 
				shriek("%s is used as a boolean value for %s", val, o->optname);
			DEBUG(1,0,fprintf(stddbg,"Configuration option \"%s\" set to %s\n",
				o->optname,enum2str(*(bool*)o->optid, BOOLstr));)
			break;
		case O_DBG_AREA:
			*(_DEBUG_AREA_ *)o->optid=(_DEBUG_AREA_)str2enum(val, DEBUG_AREAstr,-1);
			DEBUG(1,0,fprintf(stddbg,"Debug focus set to %i\n",*(int*)o->optid);)
			break;
		case O_MARKUP:
			if((*(OUT_ML *)o->optid=(OUT_ML)str2enum(val, OUT_MLstr, -1))==-1)
				shriek("Can't set %s to %s", o->optname, val);
			DEBUG(1,0,fprintf(stddbg,"Markup language option set to %i\n",*(int*)o->optid);)
			break;
		case O_SYNTH:
			if((*(SYNTH_TYPE *)o->optid=(SYNTH_TYPE)str2enum(val, STstr, -1))==-1)
				shriek("Can't set %s to %s", o->optname, val);
			DEBUG(1,0,fprintf(stddbg,"Synthesis type option set to %i\n",*(int*)o->optid);)
			break;
		case O_UNIT:
			if((*(UNIT *)o->optid=(UNIT)str2enum(val, UNITstr, -1))==-1) 
				shriek("Can't set %s to %s", o->optname, val);
			DEBUG(1,0,fprintf(stddbg,"Configuration option \"%s\" set to %d\n",o->optname,*(int *)o->optid);)
			break;
		case O_INT:
			*(int *)o->optid=0;
			if (!sscanf(val,"%d",(int*)o->optid)) shriek("Unrecognized numeric parameter");
			DEBUG(1,0,fprintf(stddbg,"Configuration option \"%s\" set to %d\n",o->optname,*(int *)o->optid);)
			break;
		case O_STRING: 
			parse_cfg_str(val, o->optname);
			*(char**)o->optid=FOREVER(strdup(val));
			break;
		case O_FILE:
			parse_cfg_str(val, o->optname);
			*(char**)o->optid=FOREVER(freadin(val, cfg.ini_dir));
			break;
		case O_CHAR:
			parse_cfg_str(val, o->optname);
			if (val[1]) shriek("Multiple chars given for a CHAR option %s", o->optname);
			else *(char *)o->optid=*val;
			break;
		default: shriek("Bad option type %d", (int)o->opttype);
		};
		free(val);
	};
	tab->forall(unknown_option);
}

void parse_cmd_line()
{
	char *ar;
	char *j;
	register char *par;
	hash *opts;

	opts=new hash(argc|15);
	for(int i=1; i<argc; i++) {
		ar=argv[i];
		switch(strspn(ar, CMD_LINE_OPT)) {
		case 3:
			ar+=3;
			if (strchr(ar, CMD_LINE_VAL) && cfg.warnings) 
				shriek("Thrice dashed options have an implicit value");
			opts->add(ar, "0");
			break;
		case 2:
			ar+=2;
			par=strchr(ar, CMD_LINE_VAL);
			if (par) {					//opt=val
				*par=0;
				opts->add(ar, par+1);
				*par=CMD_LINE_VAL;
			} else	if (i+1==argc || strchr(CMD_LINE_OPT, *argv[i+1])) 
					opts->add(ar, "");		//opt
				else opts->add(ar, argv[++i]);		//opt val
			break;
		case 1:
			for (j=ar+1; *j; j++) switch (*j) {
				case 'b': cfg.out_verbose=false; break;
				case 'c': cfg.colloquial=true; break;
				case 'd': cfg.show_diph=true; break;
				case 'H': cfg.long_help=true;	/* fall through */
				case 'h': cfg.help=true; break;
				case 'i': cfg.irony=true; break;
				case 'n': cfg.rules_file="nnet.rul";
					  cfg.neuronet=true; break;
				case 'p': cfg.pausing=true; break;
				case 's': cfg.play_diph=true; break;
				case 'v': cfg.version=true; break;
				case 'D':
					if (!cfg.use_dbg) cfg.use_dbg=true;
					else if (cfg.warnings)
						cfg.always_dbg--;
					break;
				default : warn("Unknown option -%c, ignored", *j);
			}
			if (j==ar+1) cfg.input_file="";		//dash only
			break;
		case 0:
			if (cfg.input_text && cfg.input_text!=ar) {
				if (!cfg.warnings) break;
				if (cfg.paranoid) shriek("Quotes forgotten on the command line?");
				scratch = (char *) malloc(strlen(ar)+strlen(cfg.input_text)+2);
				sprintf(scratch, "%s %s", cfg.input_text, ar);
				ar = FOREVER(strdup(scratch));
				free(scratch);
			}
			cfg.input_text = ar;
			break;
		default:
			if (cfg.warnings) shriek("Too many dashes");
		};
	};
	process_options(opts);
	delete opts;
}

void load_config(const char *filename, const char *dirname, const char *not_found)
{
	hash *tab;
	char *pathname;
	bool warn = cfg.warnings;
	cfg.warnings = true;
	
	if (!filename || !*filename) return;

	pathname=compose_pathname(filename, dirname);
	DEBUG(3,0,fprintf(stddbg,"Loading config from %s\n", pathname);)
	tab=new hash(pathname, 40,10,200,6, "", true, not_found);
				//Those 40%, 10%, 200%, height 6 are unimportant.
	free(pathname);
	process_options(tab);	
	delete tab;
	cfg.warnings = warn;
}

static inline void load_language(const char *lng_name)
{
	char *tmp = (char *)malloc(strlen(cfg.lang_dir) + 2 * strlen(lng_name) + 7);
	sprintf(tmp, "%s%c%s%c%s.ini", cfg.lang_dir, SLASH, lng_name, SLASH, lng_name);
	if (*lng_name) {
		load_config(tmp, "", "Unknown language");
	}
	free(tmp);
}

static inline void load_diph_inv(const char *inv_name)
{
	char *tmp = (char *)malloc(strlen(cfg.invent_dir) + strlen(inv_name) + 6);
	sprintf(tmp, "%s%c%s.ini", cfg.invent_dir, SLASH, inv_name);
	if (*inv_name) {
		load_config(tmp, "", "Unknown speaker");
	}
	free(tmp);
}

static inline void version()
{
	fprintf(stdwarn, "This is ss version %s, bug reports to \"%s\" <%s>\n", VERSION, MAINTAINER, MAIL);
}

static inline void dump_help()
{
	int i;
	int j=0;

	printf("usage: ss [options] ['Text to be processed']\n");
	printf(" -b  bare format (no frills)\n");
	printf(" -c  casual pronunciation\n");
	printf(" -d  show diphones\n");
	printf(" -i  say ironically (experimental)\n");
	printf(" -n  same as --neuronet --rules_file nnet.rul\n");
	printf(" -p  pausing - show every intermediate state\n");
	printf(" -s  speak it\n");
	printf(" -v  show version\n");
	printf(" -D  debugging info (more D's - more detailed)\n");
	printf(" -   keyboard input\n");
	printf(" --long_options    ...see options.cc or 'ss -H' for these\n");
#if(0)
	for (option *o = optlist; o->optname; o++) if (*o->optname) printf(" --%s", o->optname);
#endif
	if (!cfg.long_help) exit(0);

	for (i=0; optlist[i].optname;i++) {
		if (*optlist[i].optname) printf("--%-18s%s",
			optlist[i].optname, j++ ? "" : "\n");
		if (j==4) j=0;
	}
	printf("\n");
	exit(0);
}

int argc=0;
char **argv=NULL;

void ss_init(int argc_, char**argv_)	 //Some global sanity checks made here
{
	static const char * CFG_FILE_OPTION = "--cfg_file";
	register optlen=strlen(CFG_FILE_OPTION);
	register char *result;
	
	argc=argc_; argv=argv_;

	if ((result=getenv(CFG_FILE_ENVIR_VAR))) cfg.inifile=result;
	for (int i=1; i<argc-1; i++) if (!strncmp(argv[i], CFG_FILE_OPTION, optlen)) {
		switch (argv[i][optlen]) {
			case 0:	  cfg.inifile=argv[++i]; break;
			case '=': cfg.inifile=argv[i]+optlen+1; break;
			default:  /* another option, most likely a bug */;
		};
	};
	ss_init();
}


void ss_init()	 //Some global sanity checks made here
{
	const char *mlinis[] = {"","ssansi.ini","ssrtf.ini", NULL};

	if (!cfg.loaded) stddbg=stdwarn=stdout,	stdshriek=stderr;
	if (sizeof(int)<4 || sizeof(int *)<4) 
		shriek ("You dwarf! I require 32 bit arithmetic & pointery [%d]", sizeof(int));

	srand(time(NULL));	// randomize

	esctab = FOREVER(fntab(cfg.token_esc, cfg.value_esc));

	parse_cmd_line();
	DEBUG(2,0,fprintf(stddbg,"Using configuration file %s\n", cfg.inifile);)
	DEBUG(1,0,fprintf(stddbg,"The in-memory cfg struct is %d bytes long\n", sizeof(cfg));)

	load_config(cfg.ssfixed, cfg.ini_dir, "Cannot open base config file %s");
	parse_cmd_line();

	load_language(cfg.lng);    parse_cmd_line();
	load_diph_inv(cfg.inventory);
	load_config(mlinis[cfg.ml], cfg.ini_dir, "Cannot open output config file %s");
	load_config(cfg.inifile, cfg.ini_dir, "Cannot open config file %s");

	cfg.warnings = true;
	parse_cmd_line();
	cfg.loaded=true;
	
	cfg.use_diph = cfg.show_diph | cfg.play_diph | cfg.imm_diph;
	
	hash_max_line = cfg.max_line;

	if (cfg.version) version();
	if (cfg.help) dump_help();

#ifdef DEBUGGING
	if (cfg.use_dbg && cfg.stddbg) stddbg = fopen(cfg.stddbg,"w","debugging messages");
#else
	if (cfg.use_dbg) shriek("Either disable debugging in config file, or #define it in interf.h");
#endif
	stdshriek = stderr;
	if (cfg.stdshriek) stdshriek = fopen(cfg.stdshriek, "w", "fatal error messages");
	if (cfg.stdwarn) stdwarn = fopen(cfg.stdwarn, "w", "warning messages");
		
	_subst_buff = FOREVER((char *)malloc(MAX_GATHER+2));
	_next_rule_line = FOREVER((char *)malloc(cfg.max_line+1));
	_resolve_vars_buff = FOREVER((char *)malloc(cfg.max_line+1)); 
	scratch = FOREVER((char *)malloc(cfg.scratch+1));
}

void ss_reinit()
{
	load_config("ssdeflt.ini", cfg.ini_dir, "Cannot open the defaults %s");
	ss_init();
}

void ss_done()
{
	if (_unit_just_unlinked) {  // to avoid a memory leak in unlink()
		_unit_just_unlinked->next=NULL;
		delete _unit_just_unlinked;
		_unit_just_unlinked=NULL;
	};

	if (_directive_prefices) delete _directive_prefices;

	_directive_prefices=(hash *)FOREVER(NULL);	//the right side counts
}

#ifdef DEBUGGING

char *current_debug_tag = NULL;

int  debug_config(int area)
{
	switch (area) {
		case _INTERF_: return cfg.interf_dbg;
		case _RULES_:  return cfg.rules_dbg;
		case _ELEM_:   return cfg.elem_dbg;
		case _SUBST_:  return cfg.subst_dbg;
		case _ASSIM_:  return cfg.assim_dbg;
		case _SPLIT_:  return cfg.split_dbg;
		case _PARSER_: return cfg.parser_dbg;
		case _SYNTH_:  return cfg.synth_dbg;
	};
	shriek("Unknown debug area %d", area);
	return 0;   // keep the compiler happy
}

bool debug_wanted(int lev, /*_DEBUG_AREA_*/ int area) 
{
	if (!cfg.use_dbg) return false;
	if (lev>=cfg.always_dbg) return true;
	if (area==cfg.focus_dbg) return lev>=debug_config(area);
	if (lev<cfg.limit_dbg)   return false;
	return lev>=debug_config(area);
}

void debug_prefix(int lev, int area)
{
	unuse(lev); unuse(area);
	if (current_debug_tag) fprintf(stddbg, "[%s] ", current_debug_tag);
}

#endif   // ifdef DEBUGGING


#ifdef WANT_DMALLOC

char *forever(void *heapptr)
{
	static count=-1;
	static void*ptr_list[ETERNAL_ALLOCS];
	if (!heapptr) {
		DEBUG(3,0,fprintf(stddbg,"Freeing %d permanent heap buffers.\n",count+1);)
		for(;count>=0;count--) {
			DEBUG(0,0,fprintf(stddbg, "pointer number %d was %p\n", count, ptr_list[count]);)
			free(ptr_list[count]);
		};
		return NULL;
	};
	if (++count >= ETERNAL_ALLOCS) {
		warn("Too many eternal allocations (memory leak?)");
		return (char *)heapptr;
	};
	ptr_list[count] = heapptr;
	return (char *)heapptr;
}


void *
operator new(size_t n)
{
	void *ret=malloc(n);
	if (n==176) printf("new, size %d %d %p\n",n, sizeof(unit), ret);
	return ret;
}


/*
void *
operator new[](size_t n)
{
	DEBUG(2,0,fprintf(stddbg,"operator new[] called, size %d\n", n));
	return malloc(n);
}
*/

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

#ifndef HAVE_FORK
int fork()
{
	return -1;
}
#endif   // ifdef HAVE_FORK
