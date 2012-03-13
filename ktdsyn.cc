/*
 *	ss/src/ktdsyn.cc
 *	(c) 1996-98 Zdenek Kadlec, kadlec@phil.muni.cz
 *	(c) 1997-98 Martin Petriska, petriska@decef.stuba.sk
 *	(c) 1997-98 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
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
 *
 *
 */

#include "common.h"
#include "ktdsyn.h"

#ifdef HAVE_IO_H
#include <io.h>		/* open, write, (ioctl,) ... */
#endif

#ifndef O_BINARY	/* open */
#define O_BINARY  0
#endif


#pragma hdrstop
#pragma warn -pia


extern double buf[6];

ktdsyn::ktdsyn (voice *v)
{
	FILE *f;
	char * pathname = compose_pathname("useky.dat", v->inv_dir);
	f = fopen (pathname, "rt");
	free(pathname);
	if (!f) shriek("Nemozem otvorit subor 'useky.dat'");
	po_u = 0;
 	while (!feof (f)) {		/* FIXME: consider turning into a freadin() call */
		int imp_int;		/* to make scanf() happy */
		fscanf (f, "%s%i%i", U[po_u].jm, &imp_int, &U[po_u].delk);
		U[po_u].imp = imp_int;
		po_u++;
	}
	fclose (f);
	DEBUG(3,9,fprintf(stddbg,"Time Domain synth OK\n");)
}


ktdsyn::~ktdsyn()
{
	/* zapis wav hlav */
	/* uzavrit wavout */
}

void ktdsyn::syndiph(voice *v, diphone d)
{
	DEBUG(1,9,fprintf(stddbg, "Diphone %d for ktdsyn\n", d.code);)
	if (v->samp_size != 8) shriek("ktd synth still supports only 8bit channels, sorry\n");

	unsigned char *s_psl;
	s_psl = new unsigned char[6000];
	char *m_sub;
	m_sub = (char *)malloc(MAX_PATHNAME);
	strcpy (m_sub, cfg->base_dir);
	strcat (m_sub, "/");
	strcat (m_sub, v->inv_dir);
	strcat (m_sub, "/");
	dif2psl (U[d.code].jm, m_sub);
	strcat (m_sub, ".PSL");
	op_psl (m_sub, s_psl);
	uind = d.code;
	pocimp = U[d.code].imp;
	delka = ((double) d.t / 100 * (double) U[d.code].delk * 40 / 10 + 0.5); // ugly as microsoft code
	peri = (int) (v->samp_rate / (8000/(double) d.f) + 0.5);
	pocp = (int) (delka / peri);

	if (pocp > 1) {                                              \
		smer = (pocimp - 1) / (pocp - 1);
		for (int i = 0; i < pocp; i++) {
			int jw = int (smer * i) * 100;
            if (peri<99) jw = jw+(50-peri/2);
			for (int j = 0; j < peri; j++) {
				if (j < 100) {
                    pomr = s_psl[jw] - 128 ;
				    pomr = 0x80 + d.e * (pomr) / 100;
				    if (pomr > 255) pomr = 255;
				    if (pomr < 0) pomr = 0;
				    v->sample((int)pomr);
                } else
                    v->sample((int)128);
				jw++;
			}
		}
	}
	free(m_sub);
	delete (s_psl);
}

int ktdsyn::dif2psl (char *m_difon, char *m_sub)
{
	for (int i = 0; i < 2; i++)
		if ((m_difon[i] == '/') || (m_difon[i] == '\\'))
			strcat (m_sub, "-");
		else if (m_difon[i] == '[')
			strcat (m_sub, "(");
		else if (m_difon[i] > 96) {
			char f;
			f = (m_difon[i] & 0xDF);
			strncat (m_sub, &f, 1);
			strcat (m_sub, "~");
		} else
		strncat (m_sub, &m_difon[i], 1);
	return 0;
}

int ktdsyn::op_psl (char *msub, sample2_t * psl)
{
	FILE *f_psl;

	if ((f_psl = fopen (msub, "rb")) == NULL) {
		fprintf (stderr, "Nemozem otvorit subor %s\n", msub);
		return -1;
	}
	fread (psl, sizeof (sample2_t), 8000, f_psl);
	fclose (f_psl);
	return 0;
}
