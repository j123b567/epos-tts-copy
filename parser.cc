/*
 *	ss/src/parser.cc
 *	(c) 1996-98 geo@ff.cuni.cz
 *
 */

#include"common.h"

/*
 *	If mode is 0, filename is truly a filename. If mode is 1, filename is
 *	really the contents, NOT a filename. I may fix this strange scheme later.
 *
 */

simpleparser::simpleparser(const char *filename, int mode)
{
	if (!cfg.loaded) ss_init();
	if (!filename || !*filename) {
		register signed char c;
		int i=0;
		text=(Char *)malloc(cfg.dev_txtlen+1);
		if(!text) shriek("Parser: Out of memory");
		do text[i++]=c=getchar(); 
			while(c!=-1 && c!=cfg.eof_char && i<cfg.dev_txtlen);
		text[--i]=0;
		txtlen=i;
	} else { 
		text = mode
			? (unsigned char *) strdup(filename)
			: (unsigned char *) freadin(filename, cfg.input_dir);
		txtlen=strlen((char *)text);
	}
	DEBUG(2,7,fprintf(stddbg,"Allocated %u bytes for the main parser\n", txtlen);)
	init(ST_ROOT);
};

simpleparser::simpleparser(const char *str)
{
	if (!cfg.loaded) ss_init();
	DEBUG(1,7,fprintf(stddbg,"Parser for %s is to be built\n",str);)
	text=(Char *)strdup(str);
	txtlen=strlen((char *)text);
	init(ST_RAW);
};

void
simpleparser::init(SYMTABLE symtab)
{
	unsigned int i;

	initables(symtab);
	for(i=0;i<txtlen;i++) text[i]=TRANSL_INPUT [text[i]];
	DEBUG(1,7,fprintf(stddbg,"Parser: has set up with %s\n",text);)
		//TRANSL_INPUT should be filled in before, in this constructor.
		//    It's meant to be altered when we later decide to use
		//    another Czech char encoding.
	current=text-1; 
	do level=chrlev(*++current); while (level>U_PHONE && level<U_TEXT);
		//We had to skip any garbage before the first phone
	DEBUG(0,7,fprintf(stddbg,"Parser: initial level is %u.\n",level);)
/*	if (level==U_TEXT) {		// This should go away sooner or later
		DEBUG(2,7,fprintf(stddbg,"Parser: is empty\n");)
		initables(ST_EMPTY);
		*current=NO_CONT;  // return '_' or something instead of quirky '\0'
	}   */
};

simpleparser::~simpleparser()
{
	free(text);
};

Char
simpleparser::getch()
{
	Char retchar=*current++;
	UNIT lastlev=level;
	level=chrlev(*current);   // level of the next character
	while (level<=lastlev && level>U_PHONE) level=chrlev(*++current);
		// (We are skipping any empty units, except for phones.)

	if (level==U_TEXT) {
		DEBUG(2,7,fprintf(stddbg,"Parser: end of text reached, changing the policy\n");)
		initables(ST_EMPTY);
		*current=NO_CONT;  // return '_' or something instead of quirky '\0'
	}
	DEBUG(0,7,fprintf(stddbg,"Parser: char requested, '%c' (level %u), next: '%c' (level %u)\n",retchar, lastlev, *current, level);)

	return(retchar);           // return the old char
};

void
simpleparser::done()
{
	if (level != U_TEXT && CHRLEV[*current] != U_ILL) 
		shriek("Too high level symbol in a dictionary, parser contains %s", (char *)text);
}

UNIT
simpleparser::chrlev(Char c)
{
	if (current>text+txtlen+1)/*(!current[-1] && c))*/ return(U_VOID);
	if (CHRLEV[c]==U_ILL)
	{
		if (c>127) fprintf(stdshriek,"Seems you're mixing two Czech character encodings?\n");
		fprintf(stdshriek,"Fatal: parser dumps core.\n%s\n",(char *)current-2);
		shriek("Parsing an unhandled character - ASCII code %d", (unsigned int) c);
	};
	return(CHRLEV[c]);
};

void
simpleparser::regist(UNIT u, const char *list)
{
	Char *s;
	if (!list) shriek ("Parser configuration: No characters for level %d", u);
	for(s=(Char*)list;*s!=0;s++)
	{
		if (CHRLEV[*s]!=U_ILL && CHRLEV[*s]!=u)
			shriek("Ambiguous syntactic function of %c",*s);
		CHRLEV[*s]=u;
	};
};

void
simpleparser::alias(const char *canonicus, const char *alius)
{
	int i;
	if (!canonicus || !alius) shriek ("Parser configuration: Aliasing NULL");
	for(i=0;(Char*)canonicus[i] && (Char*)alius[i];i++)
		TRANSL_INPUT[((Char*)alius)[i]]=((Char*)canonicus)[i];
	if((Char *)canonicus[i] || (Char*)alius[i]) 
		shriek("Parser configuration: Can't match aliases");
};


void
simpleparser::initables(SYMTABLE table)
{
	int c;
	for(c=0;c<256;c++)TRANSL_INPUT[c]=(Char)c;
	for(c=1;c<256;c++)CHRLEV[c]=U_ILL;*CHRLEV=U_TEXT;
	switch (table) {
	case ST_ROOT:
		alias(cfg.lowercase,cfg.uppercase);
		alias("   ","\n\r\t");
		regist(U_PHONE, cfg.lowercase);
		regist(U_SYLL,"|");
		regist(U_WORD," ~");
		regist(U_COLON,",-");
		regist(U_SENT,":.!?");
		regist(U_PHONE,"'%");
		break;
	case ST_RAW:
		regist(U_PHONE, cfg.lowercase);
		regist(U_PHONE, cfg.uppercase);
		regist(U_PHONE,"'%");
		regist(U_SYLL,"|");
		regist(U_WORD," ~");
		regist(U_COLON,",-");
		regist(U_SENT,":.!?");
		break;
	case ST_EMPTY:
		for(c=1;c<256;c++)CHRLEV[c]=U_VOID;*CHRLEV=U_TEXT;
		break;
	default: shriek("Garbage passed to simpleparser::initables, %d", table);
	};
};
