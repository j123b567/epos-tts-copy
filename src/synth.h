/*
 *	ss/src/synth.h
 *	(c) 1998 Jirka Hanika, geo@ff.cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 */

class synth
{
	unsigned int crc;
   public:
/*	synth(SYNTH_TYPE type, const char *counts_file, const char *models_file,
 *			const char *codebook_file, const char *dpt_file,
 ****			int f, int i, int t, int sampling_rate);	*/

	synth();
	virtual ~synth(void);
	virtual void syndiph(voice *v, diphone d) = 0;  /* Any subclass must define syndiph or syndiphs */
	virtual void syndiphs(voice *v, diphone *d, int count);
};

class voidsyn: public synth
{
   public:
	voidsyn() : synth() {};
	virtual ~voidsyn() {};
	virtual void syndiph(voice *v, diphone d);
};

void play_diphones(unit *root, voice *v);
void show_diphones(unit *root);

