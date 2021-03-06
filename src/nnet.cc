/*
 *	epos/src/nnet.cc
 *	(c) 1997 geo@cuni.cz
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
 *	These procedures provide the input to the MATLAB-implemented 
 *	neural networks by Jana Tuckova. Their corresponding header
 *	file is unit.h and this file gets included by unit.cc.
 *
 *
 *	This file is extremely ugly and non-generic.  Shall go away
 *	sooner or later.
 */

inline UNIT get_level (const char *levname)
{
	return str2enum(levname, cfg->unit_levels, U_DEFAULT);
}

#define NN_VOCAL	'A' 
#define NN_DIPHT	'U' 
#define NN_VOICED	'B' 
#define NN_VOICELESS	'P' 
#define NN_SONOR	'L' 
#define NN_NOTHING	'0' 
#define NN_PUNCTUATION  '%'

#define NN_LONG		'a'
 
/****************************************************************************
 Dumps the structure to a file or to the screen in the format expected
 by MATLAB-implemented neural networks.
 ****************************************************************************/

void
unit::nnet_out(const char *filename, const char *dirname)      //NULL means stdout
{
	FILE *outf;
	if (cfg->neuronet) {
		if (filename) {
			filename = compose_pathname(filename, dirname);
			outf=fopen(filename,"wt");
		} else outf=stdout;

//		fputs(cfg->header_xscr,outf);
		nnet_dump(outf);
//		fputs(cfg->footer_xscr,outf);
		if(filename)fclose(outf);
	} else {
		shriek(463, "Neuronet output is disabled.");	// FIXME, error code
	}
}

/****************************************************************************
 unit::nnet_dump - not optimized for speed
 ****************************************************************************/

void
unit::nnet_dump(FILE *handle)        //this one does the real job
{
	unit *tmpu;

	UNIT phone = cfg->phone_level;
    
	for(tmpu=LeftMost(phone);tmpu!=&EMPTY;tmpu=tmpu->Next(phone)) {
			//lastlong=1, iff vocalic(cont) && next->cont!=':'
		fprintf(handle, "%d %d %d %.2f %.2f 2.5   %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
			tmpu->Next(phone)->cont!=NN_LONG && tmpu->cont==NN_VOCAL,
			tmpu->index(phone, get_level("word"))==0,
			tmpu->index(get_level("syll"), get_level("word"))==0,
			tmpu->ancestor(get_level("word"))->count(get_level("syll"))
				/ (float)tmpu->ancestor(get_level("colon"))->count(get_level("syll")),
			(tmpu->index(get_level("syll"), get_level("colon"))+1)/(float)tmpu->ancestor(get_level("colon"))->count(get_level("syll")),
			tmpu->Prev(phone)->Prev(phone)->Prev(phone)->nnet_type(),
			tmpu->Prev(phone)->Prev(phone)->nnet_type(),
			tmpu->Prev(phone)->nnet_type(),
			tmpu->nnet_type(),
			tmpu->Next(phone)->nnet_type(),
			tmpu->Next(phone)->Next(phone)->nnet_type(),
			tmpu->Next(phone)->Next(phone)->Next(phone)->nnet_type()
		);
	}
}

float
unit::nnet_type()
{
	switch(cont) {
		case NN_LONG:
		case NN_VOCAL:  return 2.5;
		case NN_DIPHT:  return 1.5;
		case NN_VOICED: return 1.0;
		case NN_VOICELESS: return 0.5;
		case NN_SONOR:  return 2.0;
		case NN_PUNCTUATION:
		case NN_NOTHING:return 3.0;
	}
	shriek(463, fmt("Please apply nnet rules before calling nnet_dump() %c", cont));
	return 0;
}
