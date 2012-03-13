/*
 *	epos/src/synth.h
 *	(c) 1998-99 Jirka Hanika, geo@cuni.cz
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

/*
 *	Read this if you ever derive a new synthesis from class synth.
 *
 *	Every synthesis must provide either a syndiph() or syndiphs()
 *	method. The easiest approach is to override syndiph() with
 *	code to synthesize a single diphone. It will only be called
 *	by synth::syndiphs() - it adjusts the prosody by the values
 *	specified for the current voice for you.
 *
 *	On the other hand, if you override syndiphs(), you'll have
 *	to implement these adjustments yourself. In this case you
 *	should also override syndiph() with a dummy function (it
 *	would never be called except by common code, but it the
 *	compiler needs it at least declared).
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
	virtual void syndiph(voice *v, diphone d, wavefm *w) = 0;
	virtual void syndiphs(voice *v, diphone *d, int count, wavefm *w);
};

class voidsyn: public synth
{
   public:
	voidsyn() : synth() {};
	virtual ~voidsyn() {};
	virtual void syndiph(voice *v, diphone d, wavefm *w);
};

void play_diphones(unit *root, voice *v);
void show_diphones(unit *root);
synth *setup_synth(voice *v);
