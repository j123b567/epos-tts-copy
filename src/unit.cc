/*
 *	epos/src/unit.cc
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
 */

#include "common.h"
#include "nnet/neural.h"

#define QUESTION_MARK      '?'    // ignored context character (in segment names)

#define SSEG_QUESTION_LEN 32	        // see unit::sseg()
#define OMEGA          10000            // Random infinite number, see unit::seg()


unit EMPTY;

#include "slab.h"

#include "marker.cc"

SLABIFY(unit, unit_slab, 2048, shutdown_units)
SLABIFY(marker, marker_slab, 2048, shutdown_unit_markers)

/****************************************************************************
 This constructor will construct a unit at the "layer" level, 
 using the input from the parser
 ****************************************************************************/

unit::unit(UNIT layer, parser *parser)
{
	next = prev = firstborn = lastborn = father = NULL;
	depth = layer;
	cont = NO_CONT;
//	f = i = t = 0;
	t = 1;
	m = NULL;
	scope = false;
	D_PRINT(1, "New unit %u, parser %u\n", layer, parser->level);

	while (parser->level < layer) insert_end(new unit((UNIT) (layer-1), parser),NULL);
	if    (parser->level == layer) {
//		shriek(899, "unit const");
//		m =
//		f = parser->f;
//		i = parser->i;
		t = parser->t;
		cont = parser->gettoken();
	}

	D_PRINT(1, "Finished a unit with %c\n",cont);
}

/****************************************************************************
 This one constructs a unit with the content and level specified
 ****************************************************************************/

unit::unit(UNIT layer, int content)
{
	next = prev = firstborn = lastborn = father = NULL;
	depth = layer;
	cont = content;
//	f = i = t = 0;
	t = 0;
	m = NULL;
	scope = false;
	D_PRINT(1, "New unit %u, content %d\n", layer, content);
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
//	D_PRINT(0, "Disposing %c\n",cont);
	if (firstborn) delete_children();
}

void
unit::delete_children()
{
	unit *v;
	for (unit *u = firstborn; u; u = v) {
		v = u->next;
		delete u;
	}
	firstborn = lastborn = NULL;
}

/****************************************************************************
 Dumps the segments to an array of "struct segment". Returns how many dumped.
 
 Note:	iucache,ifcache and ocache is a primitive one-line cache of
 	where to start dumping the segments.  It can speed up
 	the dumping of a very very long utterance (it can be slow
 	to find the "first" segment in general, but it is likely to be the
 	one following the last one dumped in the previous run).
 	So the cache hits if we just want to continue the dump.
 	
 	There's a catch in this scheme.  The iucache may be equal to "this"
 	also if iucache's storage has been freed and reused.  To avoid
 	very funny problems, YOU MUST ALWAYS DUMP WITH first==0 FIRST
 	(with any "this" you want ever to dump).  That clears the cache.
 	The cache is safe wrt multitasking, if you keep the above rule
 	and don't blatantly reallocate units during their lifetime.

 	If you can't keep the rule, just comment out the cache.
 ****************************************************************************/


int
unit::write_segs(segment *whither, int first, int n)
{
	bool tmpscope = scope;
	int m;
	unit *tmpu;
	static unit *iucache; static int ifcache; static unit *ocache;
	 
	if (!whither) shriek (861, "NULL ptr passed to write_segs() n=%d", n);
	scope = true;
	if (first && first == ifcache && iucache == this) tmpu=ocache;
	else for (m = first, tmpu = LeftMost(scfg->segm_level); m--; tmpu = tmpu->Next(scfg->segm_level));
	for (m=0; m<n && tmpu != &EMPTY; m++, tmpu = tmpu->Next(scfg->segm_level)) {
		tmpu->sanity();
		whither[m].code = tmpu->cont;
		whither[m].f = tmpu->effective(Q_FREQ);
		whither[m].e = tmpu->effective(Q_INTENS);
		whither[m].t = tmpu->effective(Q_TIME);
		whither[m].nothing = 0;
		whither[m].ll = 0;
		if (cfg->label_sseg) {
			unit *z;
			int prelevel = 0;
			for (z = tmpu; z && !z->next && z->father; z = z->father)
				prelevel++;
			int postlevel = 0;
			for (z = tmpu; z && !z->prev && z->father; z = z->father)
				postlevel++;
			whither[m].ll = prelevel /* > postlevel ? prelevel : postlevel */;
		}
	}
	scope = tmpscope;
	iucache = this; ifcache = first+m; ocache = tmpu;
	return m;
}

/****************************************************************************
 Dumps the structure to SSIF. Returns how many bytes written.
 ****************************************************************************/
 
int
unit::write_ssif_head(char *whither) {
	return sprintf(whither, "%.3s %d ", encode_to_sampa(cont, this_voice->sampa_alt), effective(Q_TIME));
}


#define WSSIF_SAFETY 100		// FIXME

int
unit::write_ssif(char *whither, int first, int len)
{
	bool tmpscope = scope;
	marker *m;
	unit *tmpu;
	int n = 0;		// how many dumped
	static unit *iucache; static int ifcache; static unit *ocache;
	 
	if (!whither) shriek (861, "NULL ptr passed to write_ssif()");
	char *current = whither;
	scope = true;
	if (first == ifcache && iucache == this) tmpu=ocache;
	else for (n = first, tmpu = LeftMost(scfg->phone_level); n--; tmpu = tmpu->Next(scfg->phone_level));

	for (; (current < whither + len - WSSIF_SAFETY) && tmpu != &EMPTY; tmpu = tmpu->Next(scfg->phone_level)) {
		int l = tmpu->write_ssif_head(scratch);
		memcpy(current, scratch, l);
		current += l;
		for (m = tmpu->m; m; m = m->next) {
			int l = m->write_ssif(scratch);
			memcpy(current, scratch, l);
			current += l;
		}
		*current++ = '\n';
		n++;
	}
	*current = 0;

	scope = tmpscope;
	iucache = this; ifcache = first + n; ocache = tmpu;
	return n;
}

void
unit::show_phones()
{
	unit *tmpu;
	for (tmpu = LeftMost(scfg->phone_level); tmpu != &EMPTY; tmpu = tmpu->Next(scfg->phone_level))
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
	outf = filename ? fopen(filename, "wt", "unit dump file") : cfg->stddbg;

	tmp = claim(scfg->header_xscr, scfg->ini_dir, "", "rt", "transcription header", NULL);
	fputs(tmp->data, outf);
	unclaim(tmp);

	fdump(outf);

	tmp = claim(scfg->footer_xscr, scfg->ini_dir, "", "rt", "transcription footer", NULL);
	fputs(tmp->data, outf);
	unclaim(tmp);

	if (filename) fclose(outf);
}

/****************************************************************************
 unit::fdump
 ****************************************************************************/

inline const char *
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
	if (depth == scfg->phone_level) {
		colorize (depth, handle);
		if (cont != NO_CONT || !scfg->out_swallow__) fputs(fmtchar(cont), handle);
		colorize(-1, handle);
		return;
	}
	if (scfg->out_prefix && !(cont == NO_CONT && scfg->out_swallow__)) {
		colorize(depth, handle);   //If you wanna disable this, go to interf.cc::colorize()   
		fputs(fmtchar(cont), handle);
		colorize(-1, handle);
	}
	if (scfg->out_verbose && scfg->out_opening[depth]) {
		colorize(depth, handle);   //If you wanna disable this, go to interf.cc::colorize()   
		fputs(scfg->out_opening[depth], handle);
		colorize(-1,handle);
	}
	if ((tmpu = firstborn)) {
		tmpu->fdump(handle);
		tmpu = tmpu->next;
		while (tmpu) {
			if (scfg->out_verbose && scfg->out_separ[depth-1]) {
				colorize(depth-1, handle);	
				fputs(scfg->out_separ[depth-1], handle);
				colorize(-1, handle);
			}
			tmpu->fdump(handle);
			tmpu = tmpu->next;
		}
	}
	if (scfg->out_verbose && scfg->out_closing[depth]) {
		colorize(depth,handle);
		fputs(scfg->out_closing[depth],handle);
		colorize(-1,handle);
	} else fputc(' ', handle);	
	if (scfg->out_postfix && !(cont == NO_CONT && scfg->out_swallow__)) { 
		colorize(depth, handle);   //If you wanna disable this, go to interf.cc::colorize()   
		fputs(fmtchar(cont), handle);
		if (m) fputs("[", handle);
		if (m && m->quant == Q_FREQ) fputs("f", handle);
		if (m && m->quant == Q_INTENS) fputs("i", handle);
		if (m && m->quant == Q_TIME) fputs("t", handle);
		if (m) {
			sprintf(scratch, "%d", m->par);
			fputs(scratch, handle);
		}
		if (m) fputs("]", handle);
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
unit::insert(UNIT target, bool backwards, char what, charclass *left, charclass *right)
{
	unit *tmpu;

	if (depth == target) {
		D_PRINT(1, "inner unit::insert %c %c %c\n",Prev(depth)->inside(), cont, Next(depth)->inside());
		D_PRINT(1, "   env is %c %c\n",
				left->ismember(Prev(depth)->inside())+'0',
				right->ismember(Next(depth)->inside())+'0');
		if (left->ismember(cont) && right->ismember(Next(depth)->cont)) {
			tmpu = new unit(depth, what);
			tmpu->prev = this;
			if (next) {
				next->prev = tmpu;
				tmpu->next = next;
			} else father->lastborn = tmpu;
			next = tmpu;
			tmpu->father = father;
//			tmpu->f = f;
//			tmpu->i = i;
			tmpu->t = t;
//			tmpu->m = m->derived();
		}
		if (!prev && right->ismember(cont) && left->ismember(Prev(depth)->cont)) {
			tmpu = new unit(depth, what);
			tmpu->next = this;
			father->firstborn = tmpu;
			prev = tmpu;
			tmpu->father = father;
//			tmpu->f = f;
//			tmpu->i = i;
			tmpu->t = t;
//			tmpu->m = m->derived();
		}
		D_PRINT(1, "New contents: %c\n",cont);
		return;
	}

	if (depth > target)  {
		tmpu = backwards ? lastborn : firstborn; 
		while (tmpu) {
			unit *todo = backwards ? tmpu->prev : tmpu->next;
			tmpu->insert(target,backwards,what,left,right);
			tmpu = todo;
		}
	}			
	else shriek (861, "Out of turtles");
}


/****************************************************************************
 unit::insert_begin/end
 ****************************************************************************/

void
unit::insert_begin(unit*from, unit*member)
{
	sanity();
	if(!member) shriek(861, "I am Sorry. Nice to meet ya at %d",depth);
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
	if(!member) shriek (861, "I am Sorry. Nice to meet ya at %d", depth);    //this may not be the reason
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

void
unit::done()
{
	gbsize = sbsize = 0;
	if (gb) free(gb);
	if (sb) free(sb);
	gb = sb = NULL;
	shutdown_units();
	shutdown_unit_markers();
}

/****************************************************************************
 unit::gather
 ****************************************************************************/

#define INIT_GB_SIZE  8
#define MAX_GB_SIZE   4096


char *
unit::gather(char *buffer_now, char *buffer_end, bool suprasegm)
{    
	unit *tmpu;
//	D_PRINT(0, "Chroust! %d %s\n", buffer_end - buffer_now, buffer_now - 3);
	for (tmpu = firstborn;tmpu && buffer_now < buffer_end; tmpu = tmpu->next) {
		buffer_now = tmpu->gather(buffer_now, buffer_end, suprasegm);
		if (!buffer_now) return NULL;
	}
	if(buffer_now >= buffer_end) 
		return NULL;
	if (cont != NO_CONT && (depth == scfg->phone_level
				|| suprasegm && depth > scfg->phone_level)) {
		*(buffer_now++) = (char)cont; 
	}
	return buffer_now;
}

char *
unit::gather(int *l, bool delimited, bool suprasegm)
{
	char *r;
//	if (!gb) {
//		gbsize = INIT_GB_SIZE;
//		gb = (char *)xmalloc(gbsize);
//	}
	do {
		char *b = gb;
		if (delimited) *b++ = '^';
		r = gather(b, gb + gbsize - 2, suprasegm);
		if (!r) {
			if (gbsize >= scfg->maxtext) shriek(456, "buffer grown too long");
			gbsize <<= 1;
			gb = (char *)xrealloc(gb, gbsize);
		}
	} while (!r);
	if (delimited) *r++ = '$';
	*r = 0;
	*l = r - gb;
	return gb;
}

/****************************************************************************
 unit::subst  (innermost - does the substitution proper)
 	      returns: whether the substition really occured
 ****************************************************************************/

// char *_subst_buff = NULL;
// char *_gather_buff = NULL;

char *unit::gb = (char *)xmalloc(INIT_GB_SIZE);
int unit::gbsize = INIT_GB_SIZE;
char *unit::sb = (char *)xmalloc(INIT_GB_SIZE);
int unit::sbsize = INIT_GB_SIZE;

inline void
unit::assert_sbsize(int k)
{
	if (sbsize <= k + 1) {
		while (sbsize <= k + 1) sbsize <<= 1;
		sb = (char *)xrealloc(sb, sbsize);
	}
}


inline void
unit::subst()
{
	parser *parsie;
	unit   *tmpu;

	parsie=new parser(sb, PARSER_MODE_RAW);
	D_PRINT(0, "innermost unit::subst - parser is ready\n");
	tmpu=new unit(depth,parsie);
	if (cfg->paranoid) parsie->done();
	sanity();
	if (firstborn && !tmpu->firstborn) {
		unlink(M_DELETE);
		return;
	}
	delete_children();
	D_PRINT(0, "innermost unit::subst - gonna relink after subst\n");
	firstborn = tmpu->firstborn;
	lastborn  = tmpu->lastborn;
	firstborn->set_father(this);
	sanity();
	tmpu->firstborn = NULL;      //FIXME - paranoid
	tmpu->next = NULL;           //Paranoid
	delete tmpu;
	delete parsie;
	D_PRINT(0, "innermost unit::subst - return to caller\n");
}

/****************************************************************************
 unit::subst  (inner - checks one substring and prepares the substitution)
 	      returns: whether the substition really occured
 ****************************************************************************/

inline bool
unit::subst(hash *table, int l, char *prefix, char *prefix_end, char *body, char *suffix, char *suffix_end)
{
	char *resultant;
	int safe_grow;

	D_PRINT(0, "innermost unit::subst called with PREFIX %s %s MAIN %s SUFFIX %s %s\n",prefix,prefix_end,body,suffix,suffix_end);
	sanity();
	resultant = table->translate(body);
	if (!resultant) return false;
	if (!sb) {
		sbsize = INIT_GB_SIZE;
		sb = (char *)xmalloc(sbsize);
	}
	safe_grow = sbsize - l;
	while ((int)strlen(resultant) - (int)strlen(body) > safe_grow) {
		safe_grow += sbsize;
		sbsize <<= 1;
		sb = (char *)xrealloc(sb, sbsize);
		D_PRINT(1, "Had to realloc subst buffer to %d bytes\n", sbsize);
	}
//		shriek (463, "Huge or infinitely iterated substitution %30s...", resultant);
	if (prefix && prefix_end - prefix > 0) {
		strncpy(sb, prefix, prefix_end - prefix);
		*(sb + (prefix_end - prefix)) = 0;
	} else *sb = 0;
	strcat (sb, resultant);
	D_PRINT(1, "innermost unit::subst result: %s\n",sb);
    
	if (suffix && suffix_end - suffix > 0) strncat(sb, suffix, suffix_end - suffix);
	D_PRINT(1, "innermost unit::subst - subst found: %s Resultant: %s\n", sb, resultant);
	subst();
	return true;
}

/****************************************************************************
 unit::subst  (outer - implements the various subst "methods")
 ****************************************************************************/

bool
unit::subst(hash *table, SUBST_METHOD method)
{
	char   *tail;
	char   *strend;
	char   *strrealend;

	int l;

	char *b;
	
	
	sanity();
	if ((method & M_LEFT) && !prev) return false;
	if ((method & M_RIGHT) && !next) return false;
	bool exact = ((method & M_PROPER) == M_EXACT);
	char separ = cont; cont = NO_CONT;
	for (int i = cfg->multi_subst; i; i--) {
		b = gather(&l, !exact, !exact);
		strend = gb + l;

		// if (safe_grow>300) shriek("safe_grow");	// fixme
		D_PRINT(1, "inner unit::subst %s, method %d\n", gb + 1, method);
		sanity();
		if (exact) {
			if (subst(table, l, NULL, NULL, gb, NULL, NULL))
				goto break_home;
		}
		if (b[l-1] == cont) --l;
		if (method & M_END)
			for(tail = gb; *tail && tail - gb < gbsize; tail++)
				if (subst(table, l, gb + 1, tail, tail, NULL, NULL))
					goto break_home;
		if (method & M_BEGIN)
			for (tail = strend;tail > gb; tail--) {
				tail[0] = tail[-1]; tail[-1] = 0;
				if (subst(table, l, NULL, NULL, gb, tail, gb + gbsize))
					goto break_home;
			}
		if (method & M_SUBSTR) {
			for (strrealend = strend; strend != gb; strend--) {
				strend[1] = *strend;
				*strend = 0;
				tail = gb < strend - table->longest ? strend-table->longest : gb;
				for( ; tail < strend; tail++) 
					if (subst(table, l, gb + 1, tail, tail, strend+1, strrealend)) 
						goto break_home;
			}
		}

		if (method & M_SEQ) {
			// shriek(462, "I reached rule mysubst!\n", 50);

			// algorithm similar to M_BEGIN, but continue through the rest of the string, until nothing found and string processed
			char* real_tail = strend;
			char* real_begin = gb;
			char* temp_tail;

			int subst_amount;

			char* temp_buffer = NULL;
			
			// stop if the whole string was processed
			while (real_begin < real_tail) {
				for (tail = real_tail;tail > real_begin; tail--) {
					bool subst_result;
					
					if (real_begin == gb) {
						subst_result = subst(table, l, gb, real_begin, real_begin, tail, strend);
					}
					else {
						subst_result = subst(table, l, NULL, NULL, real_begin, tail, strend);
					}
						
					if (subst_result) {
						D_PRINT(1, "inner unit::subst has substituted %s\n", real_begin);
						break;
					}
					else {
						D_PRINT(1, "inner unit::%s could not be substituted\n", real_begin);
					}
					tail[0] = tail[-1]; tail[-1] = 0;
				}

				// undo the changes made to the string
				for (temp_tail = tail; temp_tail <= real_tail; temp_tail++) {
					temp_tail[-1] = temp_tail[0];
					temp_tail[0] = 0;
				}
				
				subst_amount = (int) ((tail) - (real_begin));

				D_PRINT(1, "inner unit::subst has substituted %d letters\n", subst_amount);

				if (temp_buffer == NULL) {
					temp_buffer = (char *) xmalloc (1000);
					memset (temp_buffer, 0, 1000);
				}
				if (subst_amount > 0) {
					strcat (temp_buffer, sb);
				}
				else {
					char tmp[2];
					tmp[0] = real_begin[0];
					tmp[1] = 0;
					strcat (temp_buffer, tmp);
				}

				D_PRINT(1, "inner unit::subst temp_buffer contains %s \n", temp_buffer);
				

				if (tail == real_begin) {
					// nothing was found
					real_begin++;
				}
				else {
					// found sth., skip the substituted string
					real_begin = tail;
				}
			}
			
			if (sb != NULL) free (sb);
			sb = temp_buffer;
			subst();
			goto break_home;
	
		}
		cont = separ;
		if (method & (M_LEFT | M_RIGHT) && method & M_NEGATED) {
			unlink(method&M_LEFT ? M_LEFTWARDS : M_RIGHTWARDS);
		}
		return i != cfg->multi_subst;
    
		break_home:
		cont = separ;
		D_PRINT(1, "inner unit::subst has made the subst, relinking l/r, method %d\n", method);
		sanity();
		if (method & (M_LEFT | M_RIGHT)) {
			if (!(method & M_NEGATED))
				unlink(method&M_LEFT ? M_LEFTWARDS : M_RIGHTWARDS);
			return true;
		}
		if (method & M_ONCE) return true;
	}
	shriek(463, "Infinite substitution loop detected on \"%s\"", gb);
	cont = separ;
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
	if (target == scfg->phone_level) return subst(table, method);

	char	*r;
	unit	*u;
	unit	*v;
	int len, i, j , n;
	
	*gb='^';
	u = v = LeftMost(target);
	for (i = 1; u != &EMPTY; i++) {
		gb[i] = u->cont;
		u = u->Next(target);
//		if (i == MAX_GATHER && cfg->paranoid) shriek(863, "Too huge word relabelled");
		if (i == gbsize) {
			gbsize <<= 1;
			gb = (char *)xrealloc(gb, gbsize);
		}
	}
	gb[i]='$'; gb[++i]=0; len=i;

	sanity();
	if ((method & M_LEFT) && !prev) return false;
	if ((method & M_RIGHT) && !next) return false;
	for (n=cfg->multi_subst; n; n--) {
		D_PRINT(1, "unit::relabel %s, method %d\n", gb + 1, method);
		sanity();
		if ((method & M_PROPER) == M_EXACT) {
			gb[--len] = 0; len--;
			if ((r = table->translate(gb + 1))) {
				if (cfg->paranoid && strlen(r) - len)
					shriek(462, "Substitute length differs: '%s' to '%s'", gb + 1, r);
				strcpy(gb + 1, r);
				goto commit;
			}
		}
		if (method & M_SUBSTR) {
			for (i = len; i; i--) {
				char tmp = gb[i];
				gb[i] = 0;
				j = i > table->longest ? i - table->longest : 0;
				for (; j<i; j++) {
					if ((r = table->translate(gb+j))) {
						j += !j;
						gb[i] = tmp;
						if (cfg->paranoid && strlen(r) - i + j + !tmp)
							shriek(462, "Substitute length differs: '%s' to '%s'", gb+j, r);
						memcpy(gb+j, r, strlen(r));
						goto break_home;
					}
				}
				gb[i] = tmp;
			}
		}
		if (n == cfg->multi_subst) return false; else goto commit;

		break_home:
		if (method & M_ONCE) goto commit;
	}
	shriek(463, "Infinite substitution loop detected on \"%s\"", gb);
	sanity();

commit:
	D_PRINT(1, "inner unit::relabel has made the subst, relinking l/r, method %d\n", method);
	for (i = 1; v != &EMPTY; i++) {
		v->cont = gb[i];
		v = v->Next(target);
	}
	sanity();
	if (method & (M_LEFT | M_RIGHT))
		unlink(method&M_LEFT ? M_LEFTWARDS : M_RIGHTWARDS);
	return true;
}



#ifdef WANT_REGEX

#ifndef HAVE_RM_SO			// __QNX__ is slightly incompatible
#define rm_so  rm_sp - _gather_buff	
#define rm_eo  rm_ep - _gather_buff	// nota bene associvitatem!
#endif

/****************************************************************************
 unit::regex
 ****************************************************************************/

// FIXME: more efficient strcpys, please

void
unit::regex(regex_t *regex, int subexps, regmatch_t *subexp, const char *repl)
{
//	char    _gather_buff[MAX_GATHER+2];
	char   *strend;
	int	i,j,k,l;
	
	sanity();
	for (i = cfg->multi_subst; i; i--) {
		gather(&l, false, true);
		strend = gb + l;
//		if (!strend) return;	// gather overflow
//		*strend=0;
		D_PRINT(1, "unit::regex %s, subexps=%d\n", gb, subexps);
		if (regexec(regex, gb, subexps + 1, subexp, 0)) return;
		sanity();
		D_PRINT(1, "unit::regex matched %s\n", gb);
		
//		for (k = 0; k < subexp[0].rm_so; k++) sb[k] = gb[k];
		k = subexp[0].rm_so;
		assert_sbsize(k);
		strncpy(sb, gb, k);
		
		for (j = 0; ; j++) {
			if (repl[j]==ESCAPE && repl[j+1]>='0' && repl[j+1]<='9') {
				int index = repl[j+1] - '0';
				if (index >= subexps) 
					shriek(463, "Index %d too big in regex replacement", index);
				assert_sbsize(k + subexp[index].rm_eo - subexp[index].rm_so);
				for (l = subexp[index].rm_so; l < subexp[index].rm_eo; l++) {
					if (l<0) shriek(463, "regex - alternative not taken, sorry");
					sb[k++] = gb[l];
				}
				j++;
				continue;
			}
			sb[k] = repl[j];
			if (!repl[j]) break;
			k++;
			assert_sbsize(k);
		}

		int len = strlen(gb + subexp[0].rm_eo) + 1;
		assert_sbsize(k + len);
		strncpy(sb + k, gb + subexp[0].rm_eo, len);
		k += len;
//		for (l = subexp[0].rm_eo; (sb[k] = gb[l]); k++,l++);

		subst();
	}
	shriek(463, "Infinite regex replacement loop detected on \"%s\"", gb);
	sanity();
	return;
}

#endif // WANT_REGEX

/****************************************************************************
 unit::assim
 ****************************************************************************/

void 
unit::assim(UNIT target, bool backwards, charxlat *fn, charclass *left, charclass *right)
{
	unit *tmpu;

	sanity();
	if (depth == target) {
		D_PRINT(1, "inner unit::assim %c %c %c\n",Prev(depth)->inside(), cont, Next(depth)->inside());
		D_PRINT(1,"   env is %c %c\n",
				left->ismember(Prev(depth)->inside())+'0',
				right->ismember(Next(depth)->inside())+'0');
		if (right->ismember(Next(depth)->inside()) && left->ismember(Prev(depth)->inside())) {
			if (cont == DELETE_ME) return;
			cont = (unsigned char)fn->xlat(cont);
			if (cont == DELETE_ME) unlink(M_DELETE);
			D_PRINT(1, "New contents: %c\n", cont);
		}
		return;
	}
	if (depth > target) 
		for (tmpu = (backwards?lastborn:firstborn);tmpu;) {
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
	if (before->father != this) {
		before->father->split(before);
		split(before->father);
		return;
	}

	unit *clone=new unit(*this);

	D_PRINT(1, "Splitting in %c before %c, clone is %c\n", cont, before->inside(), clone->cont);
	DBG(1, fout(NULL);)
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
	D_PRINT(1, "is split in %c before %c, clone is %c\n", cont, before->inside(), clone->cont);
	DBG(1, father->fout(NULL);)
	sanity();
} 

/****************************************************************************
 unit::syllabify   breaks the syllables according to the sonority table
                   We try to split the syllable locally thus: VCV -> V|CV
                   and if there is no clear dividing point: VCCCRV -> VC|CCRV
                   where C is the least and V the most sonant element
                   Other examples: V|CRV, VC|CV, VR|CV, V|CR|CV
                   Bugs: Sanskrit-like R|CVC 
                   
		   syll_break() is called to break the syllable before 
		   the place given. 
 ****************************************************************************/

char syll_pending;

void
unit::syll_break(char *sonor, unit *before)
{
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
	unit *tmpu, *tmpu_prev;
	
	D_PRINT(0, "unit::syllabify in level %d cont %c %c %c \n", depth,Prev(depth)->cont,cont,Next(depth)->cont);
	syll_pending=0;
	for (tmpu=RightMost(target);tmpu!=&EMPTY; ) {
		tmpu_prev = tmpu->Prev(tmpu->depth);
		tmpu->syllabify(sonor);
		tmpu = tmpu_prev;
	}
}

const int INFINITY = 300000000;

void
unit::analyze(UNIT target, hash *table, int unanal_unit_penalty, int unanal_part_penalty)
{
	static struct vb_struct
	{
		int last_len;
		int badness;
	} *vb = 0;

	static int vb_size = 0;

	if (target != scfg->phone_level)
		shriek(462, "Analyze with target other than phone unimplemented");	// FIXME

	int l;
	char *b = gather(&l, false, false);
	if (++l > vb_size) {
		vb_size = l;
		D_PRINT(1, "Growing Viterbi buffer to %d items\n", vb_size);
		if (vb) free(vb);
		vb = (vb_struct *)xmalloc(vb_size * sizeof(vb_struct));
	}

	vb[0].last_len = vb[0].badness = 0;
	for (int i = 1; i < l; i++) {
		char tmp = b[i];
		b[i] = 0;
		vb[i].badness = INFINITY;
		for (int j = 0; j < i; j++) {
			char *r = j >= i - table->longest ? table->translate(b + j) : 0;
			int badness = r ? atoi(r)
				: (i - j) * unanal_unit_penalty + unanal_part_penalty;
			badness += vb[j].badness;
			if (badness < vb[i].badness) {
				vb[i].badness = badness;
				vb[i].last_len = i - j;
			}
		}
		b[i] = tmp;
	}

	int n = 0;	
	int last_k = l - 1;
	for (int k = l - 1; k; k -= vb[k].last_len) {
		n++;
		vb[k].badness = last_k;
		last_k = k;
	}

	D_PRINT(1, "Morphoanalyzed %s into %d components\n", b, n);

	int next_split = last_k;
	int count = 0;

	unit *top = firstborn;
	for (unit *u = LeftMost(target); u != &EMPTY; u = u->Next(target)) {
		if (count == next_split) {
			D_PRINT(0, "Morphoanalyzer splits at %d\n", next_split);
			if (top->next) shriek(899, "Hu.");
			top->split(u);
			top = top->next;
			next_split = vb[count].badness;
		}
		count++;
	}
}


/****************************************************************************
 unit::contains    returns whether a unit of certain content is contained
 ****************************************************************************/
 
bool
unit::contains(UNIT target, charclass *set)
{
	for (unit *u = LeftMost(target); u != &EMPTY; u = u->Next(target)) {
		if (set->ismember((unsigned char)u->cont)) return true;
	}
	return false;
}

void
unit::absol(UNIT target)
{
	if (target == depth) {
		if (strchr("aeiou", cont)) t *= 1.3;
		if (strchr("�����AEO", cont)) t *= 1.7;
		if (strchr("ptkbdg", cont)) t *= 0.9;
	} else if (target < depth) {
		for (unit *u = firstborn; u; u = u->next)
			u->absol(target);
	} else shriek(899, "absolutize upwards");
	
}

/****************************************************************************
 unit::sseg    suprasegmentalia are found here
 ****************************************************************************/

//#define SSEG_QUESTIONS 5
//static char _sseg_question[SSEG_QUESTIONS][SSEG_QUESTION_LEN];

#ifdef REALLY_NEED_SSEG

void
unit::sseg(hash *templts, char symbol, int *quant)
{
	int adj;
	
	for (int i=0; i<SSEG_QUESTIONS; i++) {
		*_sseg_question[i]=symbol;
		adj=templts->translate_int(_sseg_question[i]);
		if (adj==INT_NOT_FOUND) continue;

		D_PRINT(0, "unit::sseg adjusts %c by %d in level %d\n", symbol, adj, depth);
		*quant+=adj;
		return;	
	}
}

void
unit::sseg(UNIT target, hash *templts)
{
	unit *tmpu;
	int  j,n;
	
	D_PRINT(1, "unit::sseg in level %d\n", depth);
	n=0;
	for (tmpu = RightMost(target); tmpu != &EMPTY; tmpu = tmpu->Prev(target)) n++;
	for (j=n, tmpu = RightMost(target); j>0; j--, tmpu = tmpu->Prev(target)) {
		D_PRINT(0, "unit::sseg question n=%d j=%d\n", n, j);
		sprintf(_sseg_question[0], " /%d:%d", j, n);
		sprintf(_sseg_question[1], " /%d:*", j);
		sprintf(_sseg_question[2], " /%dlast:*", n-j+1);
		sprintf(_sseg_question[3], " /*:%d", n);
		sprintf(_sseg_question[4], " /*:*");
//		sseg(templts, 'f', &tmpu->f);
//		sseg(templts, 'i', &tmpu->i);
		sseg(templts, 't', &tmpu->t);
		shriek(899, "no sseg");
	}
}

#endif

/****************************************************************************
 unit::contour	Distributes a prosodic contour over a linear string of units.
		The last two arguments are the prosodic quantity code and
			a flag whether to add the contour to the current
			values or to set it absolutely.
		The contour is applied left-to-right.
 ****************************************************************************/

#define FIT(x,y) ((y==Q_FREQ) ? (x->f) : (y==Q_INTENS) ? (x->i) : (x->t) )

#define UGLY_POSITION  0.99

void
unit::prospoint(FIT_IDX what, int value, float position)
{
	D_PRINT(1, "Adding prosody point at %d\n", depth);
	if (what == Q_TIME) t += ((float)value * 0.01);
	else m = new marker(what, true /* extent */, value, m, position);
}

void
unit::contour(UNIT target, int *recipe, int rec_len, int padd_start, FIT_IDX what, bool additive)
{					// FIXME! take additive into account
	unit *u;
	int i;
	int padd_count;

	D_PRINT(1, "unit::contour (%d) %d %d ...\n", rec_len, recipe[0], recipe[1]);

	for (u = LeftMost(target), i = (padd_start > -1);
			i < rec_len && u != &EMPTY;
			u = u->Next(target), i++)  /* just count'em */ ;
	if (u->Next(target) != &EMPTY && padd_start == -1) {
		shriek(463, "recipe too short: %d items", rec_len);
	}
	if (i < rec_len) {
		shriek(463, "recipe too long");
	}
	for (padd_count = 0; u != &EMPTY; u = u->Next(target)) padd_count++;

		/* the following happens to work even if (padd_start == -1) */

	for (u = LeftMost(target), i=0;  i < padd_start;  u = u->Next(target), i++)
		u->prospoint(what, recipe[i], UGLY_POSITION);
	for ( ;  padd_count;  u = u->Next(target), padd_count--)
		u->prospoint(what, recipe[i], UGLY_POSITION);
	if (padd_start > -1) i++;
	for ( ;  i < rec_len;  u = u->Next(target), i++)
		u->prospoint(what, recipe[i], UGLY_POSITION);
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
#ifdef HAVE_SMOOTH
	unit *u, *v;
	int cq_wrap, j, k, avg;
	
	D_PRINT(1, "unit::smooth (%d:%d) %d %d ...\n", n, rec_len, recipe[0], recipe[1]);
	u=v=LeftMost(target);
	for (j = 0; j < n; j++) smooth_cq[j] = FIT(u, what);
	for ( ; j < rec_len && u->Next(target) != &EMPTY; j++, u = u->Next(target))
		smooth_cq[j] = FIT(u, what);
	for ( ; j < rec_len; j++) smooth_cq[j] = FIT(u, what);

	cq_wrap=0;

	for (; v != &EMPTY; v = v->Next(target)) {
		avg = 0;
		for (k = 0; k < rec_len; k++)
			avg += recipe[k] * smooth_cq[(k + cq_wrap) % rec_len];
		FIT(v, what) = avg / RATIO_TOTAL;
		smooth_cq[cq_wrap] = FIT(u, what);
		if (++cq_wrap == rec_len) cq_wrap=0;
		if (u->Next(target) != &EMPTY) u = u->Next(target);
	}
#else
//	shriek(899, "no smooth");
	D_PRINT(3, "No smooth!\n");	// FIXME
#endif
}

#undef FIT



void
unit::project_extents()
{
	D_PRINT(1, "Projecting an extent from %d\n", depth);	// FIXME
	sanity();
	for (marker *n = m; n; n = n->next)
		if (!n->extent)
			shriek(862, "Extent followed by a non-extent");

	for (unit *u = firstborn; u; u = u->next) {
		D_PRINT(3, "Moving prosody point to %d; q=%d, val=%d\n", u->depth, m->quant, m->par);
		sanity();
		u->sanity();
		m->derived()->merge(u->m);
		sanity();
		u->sanity();
	}
	sanity();
	delete m;
	m = NULL;
	sanity();
}

inline void
unit::project_one_level(float sum)
{
	marker *tmp;
	float c = 0;	// relative current time position within the unit
	unit *u = firstborn;
	while (m && !m->extent) {
		sanity();
		shriek(861, "non-extents not implemented");

		tmp = m;
		if (tmp->pos < c) shriek(862, "unsorted markers");
		while (tmp->pos > c + u->t / sum) {
			c += u->t / sum;
			u = u->next;
		}
		m = tmp->next;
		tmp->next = NULL;
		tmp->pos = (tmp->pos - c) / u->t * sum;
		marker **ml;
		for (ml = &u->m; *ml; ml = &(*ml)->next) ;
		*ml = tmp;
	}
	if (m) project_extents();
	sanity();
}


void
unit::project(UNIT target)			// FIXME optimize
{
	sanity();
	if (target == depth) return;
	if (target < depth) {
		float sum = 0;
		for (unit *u = firstborn; u; u = u->next) {
			sum += u->t;
		}

		project_one_level(sum);
		for (unit *u = firstborn; u; u = u->next)
			u->project(target);
	} else shriek(899, "project upwards");
}

/****************************************************************************
 unit::raise    moves the characters given to the level given
 		The move occurs iff the char moved is contained in "what"
 		and the character to be replaced is contained in "when".
 		The other two parameters specify the levels involved.
 ****************************************************************************/

void 
unit::raise(charclass *whattab, charclass *whentab, UNIT whither, UNIT whence)
{
	if (whither != depth) shriek(861, "raise bad");
	unit *tmpu, *tmpbig;
	D_PRINT(1, "unit::raise moving from %d to %d\n",whence,whither);
	if  (whither<=whence) shriek(462, "Raising downwards...huh...");
	for (tmpbig = LeftMost(whither); tmpbig != &EMPTY; tmpbig = tmpbig->Next(whither)) {
		if (whentab->ismember(tmpbig->cont)) {
			D_PRINT(1, "unit::raise searching %c\n",tmpbig->cont);
			bool tmpscope = scope; scope = true;
			for (tmpu = LeftMost(whence); tmpu != &EMPTY; tmpu = tmpu->Next(whence)) {
				if (whattab->ismember(tmpu->cont)) {
					D_PRINT(0, "unit::raise found %c\n",tmpu->cont);
					tmpbig->cont = tmpu->cont;
				}
			}
			scope = tmpscope;
		} else D_PRINT(1, "unit::raise NOT searching %c\n",tmpbig->cont);

	}
}

/****************************************************************************
 unit::seg     (innermost - creates a single "segment" unit, if possible)
 		The for (;;) cycle will normaly execute exactly once, if
 		a segment is found; otherwise, nothing happens, because
 		a negative value is returned by translate_int(). 

 		You may, however, add a multiply of OMEGA to the segment
 		number to have this segment repeated a few times, whenever
 		it occurs in a given environment, in any .dph .
 ****************************************************************************/

char _d_descr[4];       //this buffer is an implicit parameter to seg()

inline void 
unit::seg(hash *dinven)   //_d_descr should contain a segment name
{
	int n;
	for (n = dinven->translate_int(_d_descr); n >= 0; n -= OMEGA) {
		D_PRINT(1, "Diphone number %d born\n", n % OMEGA);
		insert_end(new unit(scfg->segm_level, n % OMEGA), NULL);
		sanity();
		D_PRINT(1, "...born and inserted\n");
	}
}

/****************************************************************************
 unit::segs      (outer - public)    creates the segments' layer
                 Basically, we'll try out "LX?" "?X?" "?XR" (in _d_descr),
                 respectively, as the possible subsegments of this phone
 ****************************************************************************/
 
void
unit::segs(UNIT target, hash *dinven) 
{
	unit *tmpu;

	D_PRINT(0, "Entering outer unit::segs in depth %d cont %c tar %d\n", depth, cont,target);
	sanity();    
	if (depth==target) {
		D_PRINT(1, "unit::segs %c %c %c\n",Prev(depth)->inside(), cont, Next(depth)->inside());
		if (!dinven->items) {
			delete_children();
			return;
		}
		_d_descr[3]=0;
		_d_descr[2]=QUESTION_MARK;
		_d_descr[1]=inside();
		_d_descr[0]=Prev(depth)->inside();
		seg (dinven);
		_d_descr[0]=QUESTION_MARK;
		seg (dinven);
		_d_descr[2]=Next(depth)->inside();
		seg (dinven);
		_d_descr[0]=Prev(depth)->inside();
		seg (dinven);
		D_PRINT(1, "unit::segs is done in: %c\n", cont);
	}
	else for (tmpu = firstborn; tmpu; tmpu = tmpu->next) tmpu->segs(target, dinven);
	D_PRINT(1, "Exiting outer unit::segs tar %d (finished)\n", target);
	sanity();
}

/****************************************************************************
 unit::unlink    removes this one from the neighborhood, reparents children
 ****************************************************************************/

unit *_unit_just_unlinked=NULL;		//used by unlink(), ::epos_done(), sanity()

void
unit::unlink(REPARENT rmethod)
{
	D_PRINT(1, "unlinking depth=%d\n",depth);
	sanity();
	if (next) next->prev=prev;
		else if (father) father->lastborn=prev;
	if (prev) prev->next=next;
		else if (father) father->firstborn=next;
	switch (rmethod) {    
	case M_DELETE:
		if (firstborn) delete_children();
		break;
	case M_RIGHTWARDS:
		D_PRINT(0, "unlinking rightwards at level %d\n",depth);
		if (next) next->insert_begin(firstborn, lastborn);
		else shriek(861, "reparent impossible in unit::unlink");
		break;
	case M_LEFTWARDS:    
		D_PRINT(0, "unlinking leftwards at level %d\n",depth);
		if (prev) prev->insert_end(firstborn, lastborn);
		else shriek(861, "reparent impossible in unit::unlink");
		break;
	}
	D_PRINT(0, " ...successfully unlinked\n");
	firstborn = NULL;lastborn = NULL;
	if (_unit_just_unlinked) {			// this could probably be simplified
	        _unit_just_unlinked->next=NULL;		// to "delete this"
	        delete _unit_just_unlinked;
	}
	_unit_just_unlinked=this;

	if (father && !father->firstborn) {
		D_PRINT(3, "Unsafe unlink - may need more fixing\n");	// as above
		father->unlink(M_DELETE);
	}
	D_PRINT(0, "unit deleted, farewell\n");
}

/****************************************************************************
 unit::forall
 ****************************************************************************/

int
unit::forall(UNIT target, bool userfn(unit *patiens))
{
	unit *tmpu, *tmpu_next;
	int n = 0;

	if (!this) return shriek(862, "NULL->forall"),0;
	if (target == depth) n += (int) userfn(this);
	else for (tmpu = firstborn; tmpu; ) {
		tmpu_next = tmpu->next;
		n += (tmpu->forall(target, userfn));
		tmpu->sanity();
		tmpu = tmpu_next;
	}
	return n;
}
/****************************************************************************
 unit::effective    sum up either F,I,T for this unit
		if cfg->sseg_mul[(quantity)] is true, multiply up.
 ****************************************************************************/

int
unit::effective(FIT_IDX which)
{
	marker *ma;
	unit *u = this;
	if (which == Q_TIME) {
		if (cfg->pros_mul[which]) {
			float product = 1;
			for (unit *u = this; u; u = u->father) {
				int w = cfg->pros_weight[u->depth];
				if (w > 10) shriek(462, "Weight absurd in unit::effective");
//				float x = u->t;
//				for ( ; w; w--) product += product * x / cfg->pros_neutral[which];
//				printf("Multiplying %d times by %f at level %d\n", w, u->t, u->depth);
				for ( ; u->t > 0 && w; w--) product *= u->t;
			}
			return product * cfg->pros_neutral[which];
		} else {
			int sum = 0;
			for (unit *u = this; u; u = u->father) {
				int x = (int)u->t;
				sum += x * cfg->pros_weight[depth];
			}
			return sum + cfg->pros_neutral[which];
		}
	}
	int r = cfg->pros_neutral[which];		// FIXME
	for (u = this; u; u = u->father) {
		for (ma = u->m; ma; ma = ma->next)
		{
			if (ma->quant == which && ma->extent) {
//				printf(" += %d\n", ma->par);
				r += ma->par;
			}
		}
	}
//	printf("sto\n");
	return r;
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
	if (what > where) shriek(862, "Wrong order of arguments to unit::index");
	if (what < depth) shriek(862, "Underindexing in unit::index %d",what);
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
	if (scfg->trusted) return;    
	if (this == NULL)			  EMPTY.insane ("this non-NULL");
//	if (!firstborn && depth > scfg->phone_level)	insane ("having content");
	if (this == _unit_just_unlinked)		return;
	if (depth > scfg->text_level && this != &EMPTY)	insane("depth");
	if ((firstborn && 1) != (lastborn && 1))	insane("first == last");
	if (firstborn && firstborn->depth+1 != depth)	insane("firstborn->depth");
	if (lastborn && lastborn->depth+1 != depth)	insane("lastborn->depth");
	if (firstborn && firstborn->prev)		insane("firstborn->prev");
	if (lastborn && lastborn->next)			insane("lastborn->next");
	if (prev && prev->next != this)			insane("prev->next");
	if (next && next->prev != this)			insane("next->prev");
	if (depth==scfg->text_level && father)		insane("TEXT.father");
	if (cont < -128 || cont > 255 && depth > scfg->segm_level)	insane("content"); 

	if (next && !m->disjoint(next->m)) insane("disjointness problem");
	if (prev && !m->disjoint(prev->m)) insane("disjointness problem");
	if (father && !m->disjoint(father->m)) insane("disjointness problem");
	if (firstborn && !m->disjoint(firstborn->m)) insane("disjointness problem");
	if (lastborn && !m->disjoint(lastborn->m)) insane("disjointness problem");
	if (lastborn && lastborn->prev && !m->disjoint(lastborn->prev->m)) insane("disjointness problem");

        if (scfg->allpointers) return;
	if (prev && (unsigned long int) prev<0x8000000) insane("prev");
	if (next && (unsigned long int) next<0x8000000)  insane("next");
	if (firstborn && (unsigned long int) firstborn<0x8000000) insane("firstborn");
	if (lastborn && (unsigned long int) lastborn<0x8000000)  insane("lastborn");
}

void
unit::insane(const char *token)
{
	if (scfg->colored) fprintf(cfg->stdshriek,"\033[00;32mSanity check of \033[01;32m%s\033[00;32m failed. cont=%d depth=%d\033[37m. This is a bug; contact the authors.\n", token,cont,depth); 
	else fprintf(cfg->stdshriek,"Sanity check of %s failed. cont=%d depth=%d. This is a bug; contact the authors.\n", token,cont,depth); 
	shriek(461, "TSR sanity check failed");
//	user_pause();
    
    	if (father) father->fout(NULL);
}

void 
unit::neural (UNIT target, CNeuralNet *neuralnet)
{
	D_PRINT (0, "applying neuralnet");
	sanity ();

	neuralnet->target = target;
	neuralnet->run (this);
}

// #include "nnet.cc"

