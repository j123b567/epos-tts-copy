/*
 *	epos/src/unit.cc
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

#define QUESTION_MARK      '?'    // ignored context character (in diphone names)

#define SSEG_QUESTION_LEN 32	        // see unit::sseg()
#define OMEGA          10000            // Random infinite number, see unit::diph()


unit EMPTY;

/****************************************************************************
 This constructor will construct a unit at the "layer" level, 
 using the input from the parser
 ****************************************************************************/

unit::unit(UNIT layer, parser *parser)
{
	next = prev = firstborn = lastborn = father = NULL;
	depth = layer;
	cont = NO_CONT;
	f = i = t = 0;
	scope=false;
	if((signed int) layer <0) shriek(861, fmt("Can't use this point of view: %d", layer));
	DEBUG(1,2,fprintf(STDDBG,"New unit %u, parser %u\n", layer, parser->level);)

	while (parser->level < layer) insert_end(new unit((UNIT) (layer-1), parser),NULL);
	if    (parser->level == layer) {
		f = parser->f;
		i = parser->i;
		t = parser->t;
		cont = parser->gettoken();
	}

	DEBUG(1,2,fprintf(STDDBG,"Finished a unit with %c\n",cont);)
}

/****************************************************************************
 This one constructs a unit with the content and level specified
 ****************************************************************************/

unit::unit(UNIT layer, int content)
{
	next = prev = firstborn = lastborn = father = NULL;
	depth = layer;
	cont = content;
	f = i = t = 0;
	scope = false;
	DEBUG(1,2,fprintf(STDDBG,"New unit %u, content %d\n", layer, content);)
}

/****************************************************************************
 This one constructs an invalid, empty unit
 ****************************************************************************/

unit::unit()   
{
	next = prev = firstborn = lastborn = father = NULL;
	depth = U_ILL;
	cont = JUNCTURE;
}

/****************************************************************************
 The destructor
 ****************************************************************************/

unit::~unit()
{
	DEBUG(0,2,fprintf(STDDBG,"Disposing %c\n",cont);)
	if (next) delete next;
	if (firstborn) delete firstborn;
//	if (father && father->firstborn == NULL) delete father;	// FIXME
}

/****************************************************************************
 Dumps the diphones to an array of "struct diphone". Returns how many dumped.
 ****************************************************************************/


int
unit::write_diphs(diphone *whither, int first, int n)
{
	bool tmpscope = scope;
	int m;
	unit *tmpu;
	static unit *iucache; static int ifcache; static unit *ocache;
	 
	if (!whither) shriek (861, fmt("NULL ptr passed to write_diphs() n=%d", n));
	scope = true;
	if (first == ifcache && iucache == this) tmpu=ocache;
	else for (m = first, tmpu = LeftMost(U_DIPH); m--; tmpu = tmpu->Next(U_DIPH));
	for (m=0; m<n && tmpu != &EMPTY; m++, tmpu = tmpu->Next(U_DIPH)) {
		tmpu->sanity();
		whither[m].code = tmpu->cont;
		whither[m].f = tmpu->effective(Q_FREQ);
		whither[m].e = tmpu->effective(Q_INTENS);
		whither[m].t = tmpu->effective(Q_TIME);
	}
	scope = tmpscope;
	iucache = this; ifcache = first+m; ocache = tmpu;
	return m;
}

void
unit::show_phones()
{
	unit *tmpu;
	for (tmpu = LeftMost(U_PHONE); tmpu != &EMPTY; tmpu = tmpu->Next(U_PHONE))
		printf("%c %d %d %d\n", tmpu->cont,
			tmpu->effective(Q_FREQ),
			tmpu->effective(Q_INTENS),
			tmpu->effective(Q_TIME));
}

/****************************************************************************
 Dumps the structure to a file or to the screen, somewhat configurable
 ****************************************************************************/


void
unit::fout(char *filename)      //NULL means stdout
{
	FILE *outf;
	file *tmp;
	outf = filename ? fopen(filename,"wt") : cfg->stddbg;

	tmp = claim(cfg->header_xscr, cfg->ini_dir, "", "rt", "transcription header");
	fputs(tmp->data, outf);
	unclaim(tmp);

	fdump(outf);

	tmp = claim(cfg->footer_xscr, cfg->ini_dir, "", "rt", "transcription footer");
	fputs(tmp->data, outf);
	unclaim(tmp);

	if (filename) fclose(outf);
}

/****************************************************************************
 unit::fdump
 ****************************************************************************/

inline char *
fmtchar(char c)
{
	static char b[2];
	switch (c) {
		case DOTS:	return "...";
		case DECPOINT:	return ".";
		case RANGE:	return "-";
		case MINUS:	return "-";
	}
	b[0] = c;
	b[1] = 0;
	return b;
}

void
unit::fdump(FILE *handle)        //this one does the real job
{
	unit *tmpu;
    
	sanity();
	if (depth == U_PHONE) {
		colorize (depth, handle);
		if (cont != NO_CONT || !cfg->out_swallow__) fputs(fmtchar(cont), handle);
		colorize(-1, handle);
		return;
	}
	if (cfg->out_prefix && !(cont == NO_CONT && cfg->out_swallow__)) {
		colorize(depth, handle);   //If you wanna disable this, go to interf.cc::colorize()   
		fputs(fmtchar(cont), handle);
		colorize(-1, handle);
	}
	if (cfg->out_verbose) {
		colorize(depth, handle);   //If you wanna disable this, go to interf.cc::colorize()   
		fputs(cfg->out_opening[depth], handle);
		colorize(-1,handle);
	}
	if ((tmpu = firstborn)) {
		tmpu->fdump(handle);
		tmpu = tmpu->next;
		while (tmpu) {
			if (cfg->out_verbose) {
				colorize(depth-1, handle);	
				fputs(cfg->out_separ[depth-1], handle);
				colorize(-1, handle);
			}
			tmpu->fdump(handle);
			tmpu = tmpu->next;
		}
	}
	if (cfg->out_verbose) {
		colorize(depth,handle);
		fputs(cfg->out_closing[depth],handle);
		colorize(-1,handle);
	} else fputc(' ', handle);	
	if (cfg->out_postfix && !(cont == NO_CONT && cfg->out_swallow__)) { 
		colorize(depth, handle);   //If you wanna disable this, go to interf.cc::colorize()   
		fputs(fmtchar(cont), handle);
		colorize(-1, handle);
	}
}

/****************************************************************************
 unit::set_father
 ****************************************************************************/

void
unit::set_father(unit *new_fath)
{ 
	father = new_fath;
	if (next) next->set_father(new_fath);
}

/****************************************************************************
 unit::insert
 ****************************************************************************/

void
unit::insert(UNIT target, bool backwards, char what, bool *left, bool *right)
{
	unit *tmpu;
	static unit *baby = NULL;	//check against infinite spawning

	if (depth == target) {
		DEBUG(1,4,fprintf(STDDBG,"inner unit::insert %c %c %c\n",Prev(depth)->inside(), cont, Next(depth)->inside());)
		DEBUG(1,4,fprintf(STDDBG,"   env is %c %c\n",left[Prev(depth)->inside()]+'0',right[Next(depth)->inside()]+'0');)
		if (this == baby) return;
		if (left[cont] && right[Next(depth)->cont]) {
			tmpu = new unit(depth, what);
			tmpu->prev = this;
			if (next) {
				next->prev = tmpu;
				tmpu->next = next;
			} else father->lastborn = tmpu;
			next = tmpu;
			tmpu->father = father;
			tmpu->f = f;
			tmpu->i = i;
			tmpu->t = t;
		}
		if (!prev && right[cont] && left[Prev(depth)->cont]) {
			baby = tmpu = new unit(depth, what);
			tmpu->next = this;
			father->firstborn = tmpu;
			prev = tmpu;
			tmpu->father = father;
			tmpu->f = f;
			tmpu->i = i;
			tmpu->t = t;
		}
		DEBUG(1,4,fprintf(STDDBG,"New contents: %c\n",cont);)
		return;
	}
	baby = NULL;
	if (depth > target) 
		for(tmpu=(backwards?lastborn:firstborn);tmpu;tmpu=(backwards?tmpu->prev:tmpu->next))
			tmpu->insert(target,backwards,what,left,right);
	else shriek (861, "Out of turtles");
}


/****************************************************************************
 unit::insert_begin/end
 ****************************************************************************/

void
unit::insert_begin(unit*from, unit*member)
{
	sanity();
	if(!member) shriek(861, fmt("I am Sorry. Nice to meet ya at %d",depth));
	if (firstborn) {
		firstborn->prev=member;
		member->next=firstborn;
	} else {
		lastborn=member;
	}
	firstborn=from;
	from->set_father(this);         //sibblings also affected
	sanity();
}

void
unit::insert_end(unit *member, unit*to)
{
	sanity();
	if(!to) to=member;
	if(!member) shriek (861, fmt("I am Sorry. Nice to meet ya at %d", depth));    //this may not be the reason
	if (lastborn) {
		lastborn->next=member;
		member->prev=lastborn;
	} else {
		firstborn=member;
	}
	lastborn=to;
	member->set_father(this);    
	sanity();
}

/****************************************************************************
 unit::gather
 ****************************************************************************/

char *
unit::gather(char *buffer_now, char *buffer_end, bool suprasegm)
{    
	unit *tmpu;
//	DEBUG(0,3,fprintf(STDDBG,"Chroust! %d %s\n", buffer_end - buffer_now, buffer_now - 3);)
	for(tmpu = firstborn;tmpu && buffer_now < buffer_end; tmpu = tmpu->next) {
		buffer_now = tmpu->gather(buffer_now, buffer_end, suprasegm);
		if (!buffer_now) return NULL;	// FIXME: slow and ugly
	}
	if (cont != NO_CONT && (depth == U_PHONE || suprasegm)) {
		if(buffer_now >= buffer_end) 
			if (cfg->paranoid) shriek(461, "unit::gather buffer overflow");
			else return NULL;
		if (depth < U_PHONE) 	// FIXME CHECKME!
			// shriek(811, "Cannot gather diphonized units (reorder rules)");
			return buffer_now;	// FIXME
		*(buffer_now++)=(char)cont; 
	}
	return buffer_now;
}

/****************************************************************************
 unit::subst  (innermost - does the substitution proper)
 	      returns: whether the substition really occured
 ****************************************************************************/

char *_subst_buff=NULL;

inline void
unit::subst(char *subst_buff)
{
	parser *parsie;
	unit   *tmpu;

	parsie=new parser(subst_buff);
	DEBUG(0,3,fprintf(STDDBG,"innermost unit::subst - parser is ready\n");)
	tmpu=new unit(depth,parsie);
	if (cfg->paranoid) parsie->done();
	sanity();
	if (firstborn && !tmpu->firstborn) {
		unlink(M_DELETE);
		return;
	}
	delete firstborn;
	DEBUG(0,3,fprintf(STDDBG,"innermost unit::subst - gonna relink after subst\n");)
	firstborn = tmpu->firstborn;
	lastborn  = tmpu->lastborn;
	firstborn->set_father(this);
	sanity();
	tmpu->firstborn=NULL;      //lest they delete out our newly adopted children
	tmpu->next=NULL;           //Paranoid
	delete tmpu;
	delete parsie;
	DEBUG(0,3,fprintf(STDDBG,"innermost unit::subst - return to caller\n");)
}

/****************************************************************************
 unit::subst  (inner - checks one substring and prepares the substitution)
 	      returns: whether the substition really occured
 ****************************************************************************/

inline bool
unit::subst(hash *table, unsigned int safe_grow, char *prefix, char *prefix_end, char *body, char *suffix, char *suffix_end)
{
	char *resultant;

	DEBUG(0,3,fprintf(STDDBG,"innermost unit::subst called with PREFIX %s %s MAIN %s SUFFIX %s %s\n",prefix,prefix_end,body,suffix,suffix_end);)
	sanity();
	resultant=table->translate(body);
	if (!resultant) return false;
	if (strlen(resultant) > safe_grow && strlen(resultant) - strlen(body) > safe_grow)
		shriek (463, fmt("Huge or infinitely iterated substitution %30s...", resultant));
	if (prefix && prefix_end-prefix>0) {
		strncpy(_subst_buff, prefix, prefix_end-prefix);
		*(_subst_buff+(prefix_end-prefix))=0;
	} else *_subst_buff=0;
	strcat (_subst_buff, resultant);
	DEBUG(1,3,fprintf(STDDBG,"innermost unit::subst result: %s\n",_subst_buff);)
    
	if (suffix && suffix_end-suffix>0) strncat(_subst_buff, suffix, suffix_end-suffix);
	DEBUG(1,3,fprintf(STDDBG,"innermost unit::subst - subst found: %s Resultant: %s\n", _subst_buff, resultant);)
	subst(_subst_buff);
	return true;
}

/****************************************************************************
 unit::subst  (outer - implements the various subst "methods")
 ****************************************************************************/

bool
unit::subst(hash *table, SUBST_METHOD method)
{
	char    gatbuff[MAX_GATHER+2];
	char   *tail;
	char   *strend;
	char   *strrealend;
	
	unsigned int safe_grow;		/* Bytes that can be added in situ */
	
	sanity();
	if ((method & M_LEFT) && !prev) return false;
	if ((method & M_RIGHT) && !next) return false;
	for (int i=cfg->multi_subst; i; i--) {
		*gatbuff='^';
		strend=gather(gatbuff+1, gatbuff+MAX_GATHER, (method&M_PROPER)!=M_EXACT);
		if (!strend) return false;	// gather overflow
		*strend=0;
		safe_grow = gatbuff - strend + MAX_GATHER;
		// if (safe_grow>300) shriek("safe_grow");	// fixme
		DEBUG(1,3,fprintf(STDDBG,"inner unit::subst %s, method %d\n", gatbuff+1, method);)
		sanity();
		if ((method & M_PROPER) == M_EXACT) {
			if (subst(table, safe_grow, NULL,NULL,gatbuff+1,NULL,NULL)) goto break_home;
		}
		if (strend[-1]==cont) --strend;
		*strend='$';*++strend=0;
		if (method & M_END)
			for(tail=gatbuff;*tail && tail-gatbuff<MAX_GATHER;tail++)
				if(subst(table, safe_grow, gatbuff+1,tail, tail, NULL,NULL)) goto break_home;
		if (method & M_BEGIN)
			for(tail=strend;tail>gatbuff;tail--) {
				tail[0]=tail[-1];tail[-1]=0;
				if(subst(table, safe_grow, NULL,NULL,gatbuff, tail, strend)) goto break_home;
			}
		if (method & M_SUBSTR) {
			for(strrealend=strend;strend!=gatbuff; strend--) {
				strend[1]=*strend;
				*strend=0;
				tail=gatbuff<strend-table->longest?strend-table->longest:gatbuff;
				for(;tail<strend;tail++) 
					if(subst(table,safe_grow, gatbuff+1,tail,tail,strend+1,strrealend)) 
						goto break_home;
			}
		}
		return i != cfg->multi_subst;
    
		break_home:
		DEBUG(1,3,fprintf(STDDBG,"inner unit::subst has made the subst, relinking l/r, method %d\n", method);)
		sanity();
		if (method & (M_LEFT | M_RIGHT)) 
			return unlink(method&M_LEFT ? M_LEFTWARDS : M_RIGHTWARDS), true;
		if (method & M_ONCE) return true;
	}
	shriek(463, fmt("Infinite substitution loop detected on \"%s\"", gatbuff));
	sanity();
	return true;
}

/****************************************************************************
 unit::relabel	implements subst for other targets than phones.
		This is a completely different job than subst does, because
		we don't get (and don't want to specify) any structure below
		this level. We are therefore restricted to equal-sized 
		substitutions and we assume that we can just "relabel" existing
		units, never creating or deleting any.

		On the other hand, this limitation makes the implemetation
		easier and faster.

		M_END and M_BEGIN are not implemented and I may remove
		them also from subst. No use for them.
 ****************************************************************************/

bool
unit::relabel(hash *table, SUBST_METHOD method, UNIT target)
{
	if (target == U_PHONE) return subst(table, method);

	char    buff[MAX_GATHER+2];
	char	*r;
	unit	*u;
	unit	*v;
	int len, i, j , n;
	
	*buff='^';
	u = v = LeftMost(target);
	for (i = 1; u != &EMPTY; i++) {
		buff[i] = u->cont;
		u = u->Next(target);
		if (i == MAX_GATHER && cfg->paranoid) shriek(863, "Too huge word relabelled");
	}
	buff[i]='$'; buff[++i]=0; len=i;

	sanity();
	if ((method & M_LEFT) && !prev) return false;
	if ((method & M_RIGHT) && !next) return false;
	for (n=cfg->multi_subst; n; n--) {
		DEBUG(1,3,fprintf(STDDBG,"unit::relabel %s, method %d\n", buff+1, method);)
		sanity();
		if ((method & M_PROPER) == M_EXACT) {
			buff[--len] = 0; len--;
			if ((r = table->translate(buff + 1))) {
				if (cfg->paranoid && strlen(r) - len)
					shriek(462, fmt("Substitute length differs: '%s' to '%s'", buff + 1, r));
				strcpy(buff + 1, r);
				goto commit;
			}
		}
		if (method & M_SUBSTR) {
			for (i = len; i; i--) {
				char tmp = buff[i];
				buff[i] = 0;
				j = i > table->longest ? i - table->longest : 0;
				for (; j<i; j++) {
					if ((r = table->translate(buff+j))) {
						j += !j;
						buff[i] = tmp;
						if (cfg->paranoid && strlen(r) - i + j + !tmp)
							shriek(462, fmt("Substitute length differs: '%s' to '%s'", buff+j, r));
						memcpy(buff+j, r, strlen(r));
						goto break_home;
					}
				}
				buff[i] = tmp;
			}
		}
		if (n == cfg->multi_subst) return false; else goto commit;

		break_home:
		if (method & M_ONCE) goto commit;
	}
	shriek(463, fmt("Infinite substitution loop detected on \"%s\"", buff));
	sanity();

commit:
	DEBUG(1,3,fprintf(STDDBG,"inner unit::relabel has made the subst, relinking l/r, method %d\n", method);)    
	for (i = 1; v != &EMPTY; i++) {
		v->cont = buff[i];
		v = v->Next(target);
	}
	sanity();
	if (method & (M_LEFT | M_RIGHT))
		unlink(method&M_LEFT ? M_LEFTWARDS : M_RIGHTWARDS);
	return true;
}



#ifdef WANT_REGEX

#ifdef __QNX__			// FIXME! Hacked to death.
#define rm_so  rm_sp - gatbuff
#define rm_eo  rm_ep - gatbuff
#endif

/****************************************************************************
 unit::regex
 ****************************************************************************/

void
unit::regex(regex_t *regex, int subexps, regmatch_t *subexp, const char *repl)
{
	char    gatbuff[MAX_GATHER+2];
	char   *strend;
	int	i,j,k,l;
	
	sanity();
	for (i=cfg->multi_subst; i; i--) {
		strend=gather(gatbuff, gatbuff+MAX_GATHER, true);
		if (!strend) return;	// gather overflow
		*strend=0;
		DEBUG(1,3,fprintf(STDDBG,"unit::regex %s, subexps=%d\n", gatbuff, subexps);)
		if (regexec(regex, gatbuff, subexps+1, subexp, 0)) return;
		sanity();
		DEBUG(1,3,fprintf(STDDBG,"unit::regex matched %s\n", gatbuff);)
		
		for (k = 0; k < subexp[0].rm_so; k++) _subst_buff[k] = gatbuff[k];
		
		for (j = 0; ; j++) {
			if (repl[j]==ESCAPE && repl[j+1]>='0' && repl[j+1]<='9') {
				int index = repl[j+1] - '0';
				if (index >= subexps) 
					shriek(463, fmt("Index %d too big in regex replacement", index));
				for (l = subexp[index].rm_so; l < subexp[index].rm_eo; l++) {
					if (l<0) shriek(463, "regex - alternative not taken, sorry");
					_subst_buff[k++] = gatbuff[l];
				}
				j++;
				continue;
			}
			_subst_buff[k] = repl[j];
			if (!repl[j]) break;
			k++;
		}

		for (l = subexp[0].rm_eo; (_subst_buff[k] = gatbuff[l]); k++,l++);

		subst(_subst_buff);
	}
	shriek(463, fmt("Infinite regex replacement loop detected on \"%s\"", gatbuff));
	sanity();
	return;
}

#endif // WANT_REGEX

/****************************************************************************
 unit::assim
 ****************************************************************************/

void 
unit::assim(UNIT target, bool backwards, char *fn, bool *left, bool *right)
{
	unit *tmpu;

	sanity();
	if (depth == target) {
		DEBUG(1,4,fprintf(STDDBG,"inner unit::assim %c %c %c\n",Prev(depth)->inside(), cont, Next(depth)->inside());)
		DEBUG(1,4,fprintf(STDDBG,"   env is %c %c\n",left[Prev(depth)->inside()]+'0',right[Next(depth)->inside()]+'0');)
		if (right[Next(depth)->inside()] && left[Prev(depth)->inside()]) {
			if (cont == DELETE_ME) return;
			cont = (unsigned char)fn[(unsigned char)cont];     // Digital UNIX once had trouble here
			if (cont == DELETE_ME) unlink(M_DELETE);
			DEBUG(1,4,fprintf(STDDBG,"New contents: %c\n",cont);)
		}
		return;
	}
	if (depth>target) 
		for(tmpu = (backwards?lastborn:firstborn);tmpu;) {
			unit *tmp_next = backwards ? tmpu->prev : tmpu->next;
			tmpu->assim(target,backwards,fn,left,right);
			tmpu = tmp_next;
		}
	else shriek (861, "Out of vodka");
}

/****************************************************************************
 unit::split     splits this unit just before the parametr
 ****************************************************************************/

void 
unit::split(unit *before)
{
	unit *clone=new unit(*this);

	DEBUG(1,5,fprintf(STDDBG,"Splitting in %c before %c, clone is %c\n", cont, before->inside(), clone->cont);
		fout(NULL);)
	clone->scope=false;
	if(!(lastborn=before->prev))
		shriek (463, "Attempted splitting a unit just at its boundary");
	before->prev=NULL;
	lastborn->next=NULL;
	clone->firstborn=before;
	before->set_father(clone);
	if (next) next->prev=clone;
	clone->prev=this;
	next=clone;
	if (father && this==father->lastborn) father->lastborn=clone; 
	DEBUG(1,5,fprintf(STDDBG,"is split in %c before %c, clone is %c\n", cont, before->inside(), clone->cont);
		father->fout(NULL);)
	sanity();
} 

/****************************************************************************
 unit::syllabify   breaks the syllables according to the sonority table
                   We try to split the syllable locally thus: VCV -> V|CV
                   and if there is no clear dividing point: VCCCRV -> VC|CCRV
                   where C is the least and V the most sonant element
                   Other examples: V|CRV, VC|CV, VR|CV, V|CR|CV
                   Bugs: Sanskrit-like R|CVC 
                   
		   Mhm. We've been using some quadratic or worse algorithm
		   by accident, so I decided to rewrite it completely.
		   
		   syll_break() is called to break the syllable before 
		   the place given. Here the optional side-syllable 
		   hack is implemented.
 ****************************************************************************/

char syll_pending;

void
unit::syll_break(char *sonor, unit *before)
{
	if (this_lang->syll_hack && prev==father->firstborn 
		&& sonor[prev->inside()]<sonor[this_lang->syll_thr]) return;
	father->split(before);
	syll_pending=0;
}

void 
unit::syllabify(char *sonor)
{
	if (sonor[inside()]<sonor[Next(depth)->inside()])
		if (sonor[inside()]<sonor[Prev(depth)->inside()]) syll_break(sonor,this);
		else syll_pending+=(sonor[inside()]==sonor[Prev(depth)->inside()]);
	else if (syll_pending && sonor[inside()]<sonor[Prev(depth)->inside()]) {
		syll_break(sonor,next);
		syll_pending=0;
	}
}

void
unit::syllabify(UNIT target, char *sonor)
{
	unit *tmpu;
	
	DEBUG(0,5,fprintf(STDDBG,"unit::syllabify in level %d cont %c %c %c \n", depth,Prev(depth)->cont,cont,Next(depth)->cont);)
	syll_pending=0;
	for(tmpu=RightMost(target);tmpu!=&EMPTY;tmpu=tmpu->Prev(tmpu->depth))
		tmpu->syllabify(sonor);
}

/****************************************************************************
 unit::sseg    suprasegmentalia are found here
 ****************************************************************************/

#define SSEG_QUESTIONS 5
static char _sseg_question[SSEG_QUESTIONS][SSEG_QUESTION_LEN];

void
unit::sseg(hash *templts, char symbol, int *quant)
{
	int adj;
	
	for (int i=0; i<SSEG_QUESTIONS; i++) {
		*_sseg_question[i]=symbol;
		adj=templts->translate_int(_sseg_question[i]);
		if (adj==INT_NOT_FOUND) continue;

		DEBUG(0,2,fprintf(STDDBG,"unit::sseg adjusts %c by %d in level %d\n", symbol, adj, depth);)
		*quant+=adj;
		return;	
	}
}

void
unit::sseg(UNIT target, hash *templts)
{
	unit *tmpu;
	int  j,n;
	
	DEBUG(1,2,fprintf(STDDBG,"unit::sseg in level %d\n", depth);)
	n=0;
	for(tmpu=RightMost(target);tmpu!=&EMPTY;tmpu=tmpu->Prev(target)) n++;
	for(j=n, tmpu=RightMost(target); j>0; j--, tmpu=tmpu->Prev(target)) {
		DEBUG(0,5,fprintf(STDDBG,"unit::sseg question n=%d j=%d\n", n, j);)
		sprintf(_sseg_question[0], " /%d:%d", j, n);
		sprintf(_sseg_question[1], " /%d:*", j);
		sprintf(_sseg_question[2], " /%dlast:*", n-j+1);
		sprintf(_sseg_question[3], " /*:%d", n);
		sprintf(_sseg_question[4], " /*:*");
		sseg(templts, 'f', &tmpu->f);
		sseg(templts, 'i', &tmpu->i);
		sseg(templts, 't', &tmpu->t);
	}
}

/****************************************************************************
 unit::contour	Distributes a prosodic contour over a linear string of units.
		The last two arguments are the prosodic quantity code and
			a flag whether to add the contour to the current
			values or to set it absolutely.
		The contour is applied left-to-right.
 ****************************************************************************/

#define FIT(x,y) ((y==Q_FREQ) ? (x->f) : (y==Q_INTENS) ? (x->i) : (x->t) )

void
unit::contour(UNIT target, int *recipe, int rec_len, int padd_start, FIT_IDX what, bool additive)
{
	unit *u;
	int i;
	int padd_count;
	for (u=LeftMost(target), i = (padd_start > -1);
			i<rec_len && u != &EMPTY;
			u=u->Next(target), i++)  /* just count'em */ ;
	if (u->Next(target) != &EMPTY && padd_start == -1) {
		shriek(463, fmt("recipe too short: %d items", rec_len));
	}
	if (i < rec_len) {
		shriek(463, "recipe too long");
	}
	for (padd_count = 0; u != &EMPTY; u = u->Next(target)) padd_count++;

		/* the following happens to work even if (padd_start == -1) */

	for (u=LeftMost(target), i=0;  i < padd_start;  u=u->Next(target), i++)
		FIT(u, what) = recipe[i] + (additive ? FIT(u, what) : 0);
	for ( ;  padd_count;  u=u->Next(target), padd_count--)
		FIT(u, what) = recipe[i] + (additive ? FIT(u, what) : 0);
	if (padd_start > -1) i++;
	for ( ;  i < rec_len;  u=u->Next(target), i++)
		FIT(u, what) = recipe[i] + (additive ? FIT(u, what) : 0);
}


/****************************************************************************
 unit::smooth	Smoothens one of the suprasegmental quantities.	The new 
 		value for the quantity is computed as a weighted average
 		of the weights to the left and to the right. The pointer
 		argument points to the array of weights (starting with
 		the leftmost one). The integer arguments are the index
 		of the weight of the unit being processed itself and the
 		total number of weights in the array.
 		
 		We cannot simply change the value sequentially for each
 		target, because that would influence the part of its
 		neighbourhood that would get processed later. We therefore
 		keep the old values in a circular queue (cq), which is big
 		enough not to let new data influence the process.
 ****************************************************************************/

int smooth_cq[SMOOTH_CQ_SIZE];

void
unit::smooth(UNIT target, int *recipe, int n, int rec_len, FIT_IDX what)
{
	unit *u, *v;
	int cq_wrap, j, k, avg;
	
	DEBUG(1,2,fprintf(STDDBG, "unit::smooth (%d:%d) %d %d ...\n", n, rec_len, recipe[0], recipe[1]);)
	u=v=LeftMost(target);
	for (j=0; j<n; j++) smooth_cq[j] = FIT(u, what);
	for (; j<rec_len && u->Next(target)!=&EMPTY; j++, u=u->Next(target)) smooth_cq[j] = FIT(u, what);
	for (; j<rec_len; j++) smooth_cq[j] = FIT(u, what);

	cq_wrap=0;

	for (; v!=&EMPTY; v=v->Next(target)) {
		avg=0;
		for (k=0; k<rec_len; k++) avg+=recipe[k]*smooth_cq[(k + cq_wrap) % rec_len];
		FIT(v, what) = avg/RATIO_TOTAL;
		smooth_cq[cq_wrap]=FIT(u, what);
		if (++cq_wrap==rec_len) cq_wrap=0;
		if (u->Next(target) != &EMPTY) u = u->Next(target);
	}
}

#undef FIT

void
unit::project(UNIT target, int adjf, int adji, int adjt)
{
	unit *u;
	if (depth > target) {
		for (u=firstborn; u; u=u->next)
			u->project(target, adjf+f, adji+i, adjt+t);
		f=i=t=0;
	} else {
		f+=adjf; i+=adji; t+=adjt;
	}
}

/****************************************************************************
 unit::raise    moves the characters given to the level given
 		The move occurs iff the char moved is contained in "what"
 		and the character to be replaced is contained in "when".
 		The other two parameters specify the levels involved.
 ****************************************************************************/

void 
unit::raise(bool *whattab, bool*whentab, UNIT whither, UNIT whence)
{
	if (whither!=depth) shriek(861, "raise bad");
	unit *tmpu, *tmpbig;
	DEBUG(1,2,fprintf(STDDBG,"unit::raise moving from %d to %d\n",whence,whither);)
	if  (whither<=whence) shriek(462, "Raising downwards...huh...");
	for (tmpbig=LeftMost(whither); tmpbig!=&EMPTY; tmpbig=tmpbig->Next(whither)) {
		if (whentab[tmpbig->cont]) {
			DEBUG(1,2,fprintf(STDDBG,"unit::raise searching %c\n",tmpbig->cont);)
			bool tmpscope=scope; scope=true;
			for(tmpu=LeftMost(whence); tmpu!=&EMPTY; tmpu=tmpu->Next(whence)) {
				if (whattab[tmpu->cont]) {
					DEBUG(0,2,fprintf(STDDBG,"unit::raise found %c\n",tmpu->cont);)
					tmpbig->cont=tmpu->cont;
				}
			}
			scope=tmpscope;
		} else DEBUG(1,2,fprintf(STDDBG,"unit::raise NOT searching %c\n",tmpbig->cont););

	}
}

/****************************************************************************
 unit::diph    (innermost - creates a single "diphone" unit, if possible)
 		The for(;;) cycle will normaly execute exactly once, if
 		a diphone is found; otherwise, nothing happens, because
 		a negative value is returned by translate_int(). 

 		You may, however, add a multiply of OMEGA to the diphone
 		number to have this diphone repeated a few times, whenever
 		it occurs in a given environment, in any .dph .
 ****************************************************************************/

char _d_descr[4];       //this buffer is an implicit parameter to diph()

void 
unit::diph(hash *dinven)   //_d_descr should contain a diphone name
{
	int n;
	for (n=dinven->translate_int(_d_descr);n>=0;n-=OMEGA) {
		DEBUG(1,5,fprintf(STDDBG,"Diphone number %d born\n", n%OMEGA);)
		insert_end(new unit(U_DIPH, n%OMEGA),NULL);
		sanity();
		DEBUG(1,5,fprintf(STDDBG,"...born and inserted\n");)
	}
}

/****************************************************************************
 unit::diphs     (outer - public)    creates the diphones' layer
                 Basically, we'll try out "LX?" "?X?" "?XR" (in _d_descr),
                 respectively, as the possible subdiphones of this phone
 ****************************************************************************/
 
void
unit::diphs(UNIT target, hash *dinven) 
{
	unit *tmpu;

	DEBUG(0,5,fprintf(STDDBG,"Entering outer unit::diphs in depth %d cont %c tar %d\n", depth, cont,target);)
	sanity();    
	if (depth==target) {
		DEBUG(1,5,fprintf(STDDBG,"unit::diphs %c %c %c\n",Prev(depth)->inside(), cont, Next(depth)->inside());)
		if (!dinven->items) {
			delete firstborn, firstborn=lastborn=NULL;
			return;
		}
		_d_descr[3]=0;
		_d_descr[2]=QUESTION_MARK;
		_d_descr[1]=inside();
		_d_descr[0]=Prev(depth)->inside();
		diph (dinven);
		_d_descr[0]=QUESTION_MARK;
		diph (dinven);
		_d_descr[2]=Next(depth)->inside();
		diph (dinven);
		_d_descr[0]=Prev(depth)->inside();
		diph (dinven);
		DEBUG(1,5,fprintf(STDDBG,"unit::diphs is done in: %c\n", cont);)
	}
	else for(tmpu=firstborn;tmpu;tmpu=tmpu->next) tmpu->diphs(target,dinven);
	DEBUG(1,5,fprintf(STDDBG,"Exiting outer unit::diphs tar %d (finished)\n", target);)
	sanity();
}

/****************************************************************************
 unit::unlink    removes this one from the neighborhood, reparents children
 ****************************************************************************/

unit *_unit_just_unlinked=NULL;		//used by unlink(), ::epos_done(), sanity()

void
unit::unlink(REPARENT rmethod)
{
	DEBUG(1,2,fprintf(STDDBG,"unlinking depth=%d\n",depth);)
	sanity();
	if (next) next->prev=prev;
		else if (father) father->lastborn=prev;
	if (prev) prev->next=next;
		else if (father) father->firstborn=next;
	switch (rmethod) {    
	case M_DELETE:
		if (firstborn) delete firstborn;
		break;
	case M_RIGHTWARDS:
		DEBUG(0,2,fprintf(STDDBG,"unlinking rightwards at level %d\n",depth);)
		if (next) next->insert_begin(firstborn, lastborn);
		else shriek(861, "reparent impossible in unit::unlink");
		break;
	case M_LEFTWARDS:    
		DEBUG(0,2,fprintf(STDDBG,"unlinking leftwards at level %d\n",depth);)
		if (prev) prev->insert_end(firstborn, lastborn);
		else shriek(861, "reparent impossible in unit::unlink");
		break;
	}
	DEBUG(0,2,fprintf(STDDBG," ...successfully unlinked\n");)
	firstborn=NULL;lastborn=NULL;            //"this" may be involved in a for-cycle
	if (_unit_just_unlinked) {                      //  up the stack, so we can't delete it
	        _unit_just_unlinked->next=NULL;        //  right now. We always keep the last one.
	        delete _unit_just_unlinked;
	}
	_unit_just_unlinked=this;

	if (father && !father->firstborn) {
		DEBUG(4,2,fprintf(STDDBG, "Unsafe unlink - may need more fixing\n"););
		father->unlink(M_DELETE);
	}
	DEBUG(0,2,fprintf(STDDBG,"unit deleted, farewell\n");)
}

/****************************************************************************
 unit::forall
 ****************************************************************************/

int
unit::forall(UNIT target, bool userfn(unit *patiens))
{
	unit *tmpu;
	int n = 0;

	if (!this) return shriek(862, "NULL->forall"),0;
	if (target == depth) n += (int) userfn(this);
	else for (tmpu = firstborn; tmpu; tmpu = tmpu->next)
	{
		tmpu->sanity();
		n += (tmpu->forall(target, userfn));
	}       //for(;;)
	return n;
}
/****************************************************************************
 unit::effective    sum up either F,I,T for this unit
		if cfg->sseg_mul[(quantity)] is true, multiply up.
 ****************************************************************************/

int
unit::effective(FIT_IDX which)
{
	DEBUG(0,2,fprintf(STDDBG,"computing unit::effective, level %d, f=%d\n", depth, f);)

	if (cfg->pros_mul[which]) {
		int product = 1 << 10;
		for (unit *u = this; u; u = u->father) {
			int w = cfg->pros_weight[u->depth];
			if (w > 10) shriek(462, "Weight absurd in unit::effective");
			int x = which  ?  which==2 ? u->t : u->i  :  u->f;
			for ( ; w; w--) product += product * x / cfg->pros_neutral[which];
		}
		return product * cfg->pros_neutral[which] >> 10;
	} else {
		int sum = 0;
		for (unit *u = this; u; u = u->father) {
			int x = which  ?  which==2 ? u->t : u->i  :  u->f;
			sum += x * cfg->pros_weight[depth];
		}
		return sum + cfg->pros_neutral[which];
	}
} 

/****************************************************************************
 Some bogus relicts
 ****************************************************************************/
/*
void
unit::fprintln(FILE *outf)
{
	fprintf(outf,"%c,%u,%u,%u\n",cont,f,i,t);
}*/

inline unsigned char
unit::inside()
{
	sanity();
	return((unsigned char) cont);
}

unit *
unit::ancestor(UNIT level)
{
	if (level == depth) return this;
	return father ? father->ancestor(level) : (unit *)NULL;
}

int
unit::index(UNIT what, UNIT where)
{
	int i=0;
	if (what > where) shriek(862, "Bad sequence of arguments to unit::index");
	if (what < depth) shriek(862, fmt("Underindexing in unit::index %d",what));
	unit *lookfor = ancestor(what);
	unit *lookin = ancestor(where);
	unit *tmpu;
	lookin->scope = true;
	for (tmpu = lookin->LeftMost(what); tmpu != lookfor; tmpu = tmpu->Next(what)) i++;
	lookin->scope = false;
	return i;
}

int
unit::count(UNIT what)
{
	int i;
	unit *tmpu;
	bool tmpscope = scope;
	scope = true;
	for (i=0, tmpu = LeftMost(what); tmpu && tmpu != &EMPTY; tmpu = tmpu->Next(what)) i++;
	scope = tmpscope;
	return  i;
}

/****************************************************************************
 unit::Right/LeftMost
 ****************************************************************************/

unit*
unit::RightMost(UNIT target)
{
	sanity();
	if(target == depth || this == &EMPTY) return this;
	if(!lastborn) return scope ? &EMPTY : Prev(depth)->RightMost(target);
	return(lastborn->RightMost(target));
}

unit*
unit::LeftMost(UNIT target)
{
	sanity();
	if(target == depth || this == &EMPTY) return this;
	if(!firstborn) return scope ? &EMPTY : Next(depth)->LeftMost(target);
	return(firstborn->LeftMost(target));
}

/****************************************************************************
 unit::Next/Prev
 ****************************************************************************/
	
unit *
unit::Next(UNIT target)
{
	sanity();
	if (scope)  return &EMPTY;                //never cross the scope
	if (next)   return next->LeftMost(target);  //next exists, but is not the target
	if (father) return(father->Next(target));
	return &EMPTY;
}

unit *
unit::Prev(UNIT target)
{
	sanity();
	if (scope)  return &EMPTY;
	if (prev)   return prev->RightMost(target);
	if (father) return father->Prev(target);
	else return &EMPTY;
}

/****************************************************************************
 unit::sanity   Sanity checks (trying to detect bad or wrong pointers) 
 ****************************************************************************/


void
unit::sanity()
{
	if (cfg->trusted) return;    
	if (this == NULL)			  EMPTY.insane ("this non-NULL");
//	if (!firstborn && depth > U_PHONE)		insane ("having content");
	if (this == _unit_just_unlinked)		return;
	if (depth > U_TEXT && this != &EMPTY)		insane("depth");
	if ((firstborn && 1) != (lastborn && 1))	insane("first == last");
	if (firstborn && firstborn->depth+1 != depth)	insane("firstborn->depth");
	if (lastborn && lastborn->depth+1 != depth)	insane("lastborn->depth");
	if (firstborn && firstborn->prev)		insane("firstborn->prev");
	if (lastborn && lastborn->next)			insane("lastborn->next");
	if (prev && prev->next != this)			insane("prev->next");
	if (next && next->prev != this)			insane("next->prev");
	if (depth==U_TEXT && father)			insane("TEXT.father");
	if (cont < 0 || cont > 255 && depth > U_DIPH)	insane("content"); 

        if (cfg->allpointers) return;
	if (prev && (unsigned long int) prev<0x8000000) insane("prev");
	if (next && (unsigned long int) next<0x8000000)  insane("next");
	if (firstborn && (unsigned long int) firstborn<0x8000000) insane("firstborn");
	if (lastborn && (unsigned long int) lastborn<0x8000000)  insane("lastborn");
}

void
unit::insane(const char *token)
{
	if (cfg->colored) fprintf(cfg->stdshriek,"\033[00;32mSanity check of \033[01;32m%s\033[00;32m failed. cont=%d depth=%d\033[37m. This is a bug; contact the authors.\n", token,cont,depth); 
	else fprintf(cfg->stdshriek,"Sanity check of %s failed. cont=%d depth=%d. This is a bug; contact the authors.\n", token,cont,depth); 
	user_pause();
    
    	if (father) father->fout(NULL);
}

#include "nnet.cc"

#include "slab.h"

slab<sizeof(unit)> unit_slab;

void *
unit::operator new(size_t size)
{
	return unit_slab.alloc();
}

void
unit::operator delete(void *ptr)
{
	unit_slab.release(ptr);
}

void shutdown_units()
{
	unit_slab.shutdown();
}

