/*
 *	ss/src/ktdsyn.h
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
 */

#define  sample2_t	unsigned char
//#define T_SAMPL unsigned char

typedef struct
  {
    char jm[3];
    char imp;
    int delk;
  } USEK;

typedef struct
{
	char string1[4];
	long flen;
	char string2[8];
	long xnone;
	short int  datform, numchan, sf1, sf2, avr1, avr2, wlenB, wlenb;
	char string3[4];
	long dlen;
 } wavehead2;            // wav file header

class ktdsyn : public synth
{
   public:
	double buf[9];
	int vyska;
	int tempo;
	
	//   etc.    FIXME??

	USEK U[1000];
	int uind, pocimp, peri, pocp;
	double delka, smer, pomr;
	int po_u;			//pocet usekov

	int kw;			// smernik na aktualnu poziciu vo vystupnom poli
	int fr_vz;			//frekvencia vzorkovania

	int dif2psl (char *, char *);
	int op_psl (char *, sample2_t *);

	     ktdsyn (int, int);
	    ~ktdsyn ();
	void syndiph (voice *v, diphone d);
	
//	int sampel2file ();
//	int sampel2wav ();
//	int difon;
//	int melod;
//	int prodl;
//	int inten;
	unsigned long p_b;		//pocet bytov vystupneho suboru
	unsigned int pocet_vzorku;
	int wavout; 		// raw file handle (buffering is home made,
			// we use ioctl's, so FILE would be clumsy...)
	sample2_t *wave_buffer;
	wavehead2 wavh;
	int oldkw, newkw;
//	T_SAMPL *sampel;
};

/*
class Kadlec:public ktdsynteza
{
	private:
	USEK U[1000];
	int uind, pocimp, peri, pocp;
	double delka, smer, pomr;
	int po_u;			//pocet usekov

	int kw;			// smernik na aktualnu poziciu vo vystupnom poli
	int fr_vz;			//frekvencia vzorkovania

	int dif2psl (char *, char *);
	int op_psl (char *, sample2_t *);

	public:
	Kadlec (int, int);		// konstruktor

	void ktdsyndif (diphone d);
	~Kadlec ();

};
*/