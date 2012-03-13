/*
 *	epos/src/lpcsyn.h
 *	(c) 1994-2000 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
 *	(c) 1997-2000 Jirka Hanika, geo@cuni.cz
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

#ifndef LPC_H
#define LPC_H

struct cmodel;
struct fcmodel;
struct vqmodel;

struct model
{
	int rc[8];
	int nsyn;
	int lsyn;
	int esyn;
	int lsq;
};                //internal representation of model

class lpcsyn : public synth
{
	int ipitch, lastl;
	long iyold, ifilt[8], kyu;
	int nvyrov, lold;
   protected:
	file *diph_len;
	file *models;
	int *diph_offs;
   public:
//	char (*tdiph)[4];
	lpcsyn(voice *v);
	virtual ~lpcsyn(void);
	inline void synmod(model mod, wavefm *w);
	void syndiph(voice *v, diphone d, wavefm *w);
	virtual void frobmod(int imodel, diphone d, model *m, int &incrl, int &znely) = 0;
};

class lpcint : public lpcsyn
{
//	cmodel *cmodely;
	short int *kor_t;
	short int *kor_i;
   public:
	lpcint(voice *v);
	void adjust(diphone d);
	virtual void frobmod(int imodel, diphone d, model *m, int &incrl, int &znely);
};


class lpcfloat : public lpcsyn
{
//	fcmodel *fcmodely;
	short int *kor_t;
	short int *kor_i;
	float *fkor_t;
	float *fkor_i;
   public:
	lpcfloat(voice *v);
	virtual void frobmod(int imodel, diphone d, model *m, int &incrl, int &znely);
};

class lpcvq : public lpcsyn
{
//	vqmodel *vqmodels;
	short int *ener;
//	short int (*codebook)[8];
	file *codebook;
   public:
	lpcvq(voice *v);
	virtual ~lpcvq();
	virtual void frobmod(int imodel, diphone d, model *m, int &incrl, int &znely);
};

#endif		// LPC_H




