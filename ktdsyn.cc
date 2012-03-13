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

ktdsyn::ktdsyn (int v, int t)
{
	pocet_vzorku=0;
	
	vyska = v;
	tempo = t;
	kw = 100;
	fr_vz = 8000;
	po_u = 0;
	FILE *f;
	if ((f = fopen ("/usr/lib/ss/inv/czech/useky.dat", "rt")) == NULL)
		shriek("Nemozem otvorit subor 'useky.dat'");
	while (!feof (f)) {
		int imp_int;		/* to make scanf() happy */
		fscanf (f, "%s%i%i", U[po_u].jm, &imp_int, &U[po_u].delk);
		U[po_u].imp = imp_int;
		po_u++;
	}
	fclose (f);
	oldkw=0; newkw=0;

/* ---------------- tady se otviral waveout ------------- */

// wave_buffer=(sample2_t*)malloc(buffer_size*(cfg->stereo+1)*sizeof(sample2_t)+2000);
// for(int i=0; i < buffer_size+2000; i++) wave_buffer[i]=0x80;
	
/*	if (cfg->wav_header) {					//naplneni hlavicky wav
		strcpy(wavh.string1,"RIFF");
		strcpy(wavh.string2,"WAVEfmt ");
		strcpy(wavh.string3,"data");
		wavh.datform=1;
		wavh.numchan=1;
		wavh.sf1=cfg->samp_hz; wavh.sf2=0;
		wavh.avr1=2*cfg->samp_hz; wavh.avr2=0;
		wavh.wlenB=2; wavh.wlenb=8;
		wavh.xnone=0x010;
		wavh.dlen=0;
		wavh.flen=0+0x24;
		write(wavout, &wavh, 44);         //zapsani prazdne wav hlavicky na zacatek souboru
	}
*/

	p_b = 0;
	oldkw=0;
	DEBUG(3,9,fprintf(stddbg,"Time Domain synth OK\n");)
}


ktdsyn::~ktdsyn()
{
//	printf("DEBUG: ~ktd\n");	but do fix this destructor!
	pocet_vzorku+=kw;
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
	strcat (m_sub, "/inv/USEKY/");
	dif2psl (U[d.code].jm, m_sub);
	strcat (m_sub, ".PSL");
	op_psl (m_sub, s_psl);
	uind = d.code;
	pocimp = U[d.code].imp;
	delka = ((double) d.t / 100 * (double) U[d.code].delk / 100 * (double) tempo / 10 + 0.5);
	peri = (int) (fr_vz / (((double) d.f / 100 * vyska)) + 0.5);
	pocp = (int) (delka / peri);


// printf("\nU=%d:%s, imp=%d pocp=%i delka=%f peri=%i - ",uind,U[uind].jm,pocimp,pocp,delka,peri); 
// printf("d=%d p=%d t=%d m=%d v=%d - ",U[uind].delk,d.t,tempo,d.f,vyska); 

	if (pocp > 1) {
		smer = (pocimp - 1) / (pocp - 1);
		for (int i = 0; i < pocp; i++) {
//			int iw = kw - peri / 2;   iw bylo nahrazeno v->buffer_idx
			int jw = int (smer * i) * 100;
			for (int j = 0; j < 100; j++) {
				pomr = s_psl[jw] /* - 128 */ ;
				pomr = (pomr /* + v->buffer[v->buffer_idx] */);
				pomr = 0x80 + d.e * (pomr - 0x80) / 100;
				if (pomr > 255) pomr = 255;
				if (pomr < 0) pomr = 0;
				v->sample((int)pomr);
				jw++;
			}
			kw += peri;
		}
	}
/*		FIXME!!!! removed but not understood!
	newkw=kw;
	if (kw>buffer_size+1000) {
		oldkw=kw-buffer_size;
		write(wavout, wave_buffer, sizeof(sample2_t) * buffer_size);
		printf("DEBUG: wrote\n");
		for (int i=0; i<(kw-buffer_size-1); i++)
			wave_buffer[i]=wave_buffer[i+buffer_size];
		for(int i=(kw-buffer_size); i < buffer_size+2000; i++)
			wave_buffer[i]=0x80;
		kw=kw-buffer_size;
		pocet_vzorku+=buffer_size;
	}
*/	
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
