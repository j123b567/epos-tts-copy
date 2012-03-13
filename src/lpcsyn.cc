/*
 *	epos/src/lpcsyn.cc
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

#include "common.h"
#include "lpcsyn.h"

#include <fcntl.h>

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifndef   O_BINARY
#define   O_BINARY	0
#endif

//#pragma warn -pia

// DEEM, HPF, hilbk  were long int - geo

const int DEEM=29184;	// constant of deemphasis = 0.89 * 32768
const int HPF=26542;   // constant of high-pass output filter
const int UPRAV=5120;
const int nhilb=15;         // number of Hilbert coefficients of excitation impulse
const int rad=8;            // order of LPC

const int hilbk[15]={4608,0,3584,0,6656,0,20992,0,-20992,0,-6656,0,-3584,0,-4608};
                        // Hilbert impulse for voiced excitation
const int lroot=10054;

const int minsynt=23;


struct cmodel
{
	short int rc[8];
	short int ener;
	short int incrl;
};               // model for integer synthesis

struct fcmodel
{
	float rc[8];
	float ener;
	int   incrl;
};              // model for float synthesis

struct vqmodel
{
	short int adrrc;
	short int adren;
	short int incrl;
};              // model for vector quantised synthesis


lpcsyn::lpcsyn(voice *v)
{

	int i; //,delmod;

	kyu = 0;		/* random initial value - geo */

	diph_offs = (int *)xmalloc(sizeof(int) * 441);
//	diph_len = (unsigned char *)xmalloc(sizeof(unsigned char) * 441);

	nvyrov = 0;
	lold = v->init_f;

/* incipit constructor clasae synth */
	ipitch = 0;
	lastl = lold;
	iyold = 0 ;

	for (i=0; i<rad; i++) ifilt[i] = 0;

//	tdiph = (char (*)[4])freadin(v->dptfile, v->inv_dir, "rt", "diphone names");
	diph_len = claim(v->counts, v->loc, cfg->inv_base_dir, "rb", "model counts", NULL);
	diph_offs[0] = 0;
	for (i=1; i<441; i++) diph_offs[i] = diph_offs[i-1] + (int)diph_len->data[i-1];	// cast to unsigned char * before [] if LPC sounds bad
//	models = claim(v->models, v->loc, cfg->inv_base_dir, "rb", "lpc inventory");
//	delmod = diph_offs[440] + diph_len[440];
}

lpcsyn::~lpcsyn(void)
{
	free(diph_offs);
	unclaim(diph_len);
	unclaim(models);
}


/*
 *	FIX the following. If ipitch>0, mod.lsyn>0, and
 *	ipitch+mod.lsyn-lastl!=0, then ihilb is used before inited.
 *
 *	If this can't ever happen, remove ihilb = 0 as well as the shriek.
 *	Else remove ihilb = 0 and fix the warning which results.
 *
 */

inline void lpcsyn::synmod(model mod, wavefm *w)
{
	int i,k,ihilb = 0;
	int gain,finp,iy,jrc[8],kz;	// were long - geo

	if (cfg->paranoid && ipitch>0 && mod.lsyn>0 && ipitch + mod.lsyn - lastl)
		shriek(862, "ihilb is undefined in synteza.synmod! Please contact the authors.\n");
	for(i=0;i<rad;i++) jrc[rad-i-1]=(int)mod.rc[i];		// cast was to long - geo
	gain=(int)mod.lsq;		// cast was to long - geo
	if(mod.lsyn!=0)
	{
		if(ipitch != 0 && ipitch + mod.lsyn - lastl>0) ipitch += mod.lsyn - lastl;
	} else 	ipitch = 0;
	if (lastl == 0 && mod.lsyn != 0) for (i=0; i<rad; i++) ifilt[i]=0;

	for(k=0;k<mod.nsyn;k++)
	{	//buzeni
		if(mod.lsyn==0) finp = rand()%(2*UPRAV)-UPRAV;
		else {
			finp=0;
			if (ipitch==0) {
				ipitch=mod.lsyn; ihilb=0; finp=hilbk[ihilb++];
			} else if (ihilb<nhilb) finp=hilbk[ihilb++];
		}
		//kriz_clanek
		iy=finp*gain-jrc[0]*ifilt[0];
		for(i=1;i<rad;i++) {
			iy=iy-jrc[i]*ifilt[i];
			ifilt[i-1]=(((iy>>15)*jrc[i])>>15)+ifilt[i];
		}
		ifilt[rad-1]=(int)(iy>>15);
		//vyst_uprav
		iy=(iy>>15)*(int)mod.esyn+(DEEM*iyold>>4);	// cast was to long - geo
		iyold=iy>>11;
		kz=kyu+iy;
		kyu=-iy+(kz>>15)*HPF;

		if (mod.lsyn!=0) ipitch--;

		w->sample(kz>>11);
	}
	lastl = mod.lsyn;
}//synmod


void lpcsyn::syndiph(voice *, diphone d, wavefm *w)	// voice not used
{
	int i,imodel,lincr,incrl,numodel,zaklad,znely;
	model m;
//	if (cfg->ti_adj) shriek("Should call lpcvq::adjust(diphone *), but can't"); // twice in this file
	DEBUG(1,9,fprintf(STDDBG, "lpcsyn processing another diphone\n"));
//	d.eproz=(d.eproz-100) / 9 + (cfg->ti_adj ? kor_i[d.hlaska]-15 : 0);
	lincr=0;
	numodel=(int)diph_len->data[d.code];	// cast to unsigned char * before [] if problems
	if(!numodel) {
		DEBUG(4,9,fprintf(STDDBG, "Unknown diphone %d, %3s\n", d.code, d.code<445 ? "in range" : "out of range"));
		return;
	}
	/* nacti_mem_popis(code,numodel) */

	for (imodel=0; imodel<numodel; imodel++) {
		// nacti_k_model(imodel)

		frobmod(imodel, d, &m, incrl, znely);

		// synchr(imodel)
		if(!znely) lincr += incrl;
		if(nvyrov < d.t / 2) {
			if(!znely) m.lsyn=0, m.lsq=12288, m.nsyn=d.t-nvyrov;
			else {
				if(imodel==numodel-1) m.lsyn=d.f;
				else {
					zaklad = (d.f-lold) * 256 / numodel;
					       // Deliberately ^^^ changed associativity - geo
					m.lsyn = lold + zaklad*(imodel+1) / 256;
				}
				m.lsyn=m.lsyn+lincr;
				m.lsq=lroot;
				i = d.t / m.lsyn + 1;
				if(i==1)m.nsyn=m.lsyn; else
					if(abs(nvyrov-d.t+i*m.lsyn)<abs(nvyrov-d.t+(i-1)*m.lsyn))m.nsyn=i*m.lsyn;
					else m.nsyn=(i-1)*m.lsyn;
			}
			nvyrov += m.nsyn - d.t;
		} else {
			if(imodel!=numodel-1) m.nsyn=0, m.lsyn=0, m.lsq=12288;
			else if(!znely) m.lsyn=0, m.lsq=12288, m.nsyn=minsynt;
				else m.lsyn=d.f, m.lsq=lroot, m.nsyn=m.lsyn;
			nvyrov -= d.t;
		}
		DEBUG(1,9,fprintf(STDDBG, "Model %d\n", imodel+1);)
		if(m.nsyn>=minsynt) synmod(m, w);  //proved syntezu modelu
	}
	lold = d.f;
}//hlask_synt


void lpcvq::frobmod(int imodel, diphone d, model *m, int &incrl, int &znely)
{
	int i;
	vqmodel *vqm = (vqmodel *)models->data + diph_offs[d.code] + imodel;
	short int (*cbook)[8] = (short int (*)[8])codebook->data;

	incrl = vqm->incrl == 999 ? 0 : -vqm->incrl;
//	if(vqmodels[diph_offs[d.code]+imodel].incrl == 999) incrl = 0;
//	else incrl = -vqmodels[diph_offs[d.code]+imodel].incrl;
	znely = (vqm->incrl != 999);	// was "=" instead of "!=" (a bug?) - geo
	d.e = (d.e-100) / 9; // + (cfg->ti_adj ? kor_i[d.code]-15 : 0);
	i = vqm->adren-1 + d.e;
	if (i>63) i=63;                    //uprava indexu
	if (i<0) i=0;                      //(tabulka energii ma jen 64 poli)
	DEBUG(1,9,fprintf(STDDBG, "energeia %d\n", i);)
	m->esyn = ener[i];                   //vyber energii z tabulky
	for (i=0; i<rad; i++) m->rc[i] = cbook[vqm->adrrc-1][i];
}


void lpcfloat::frobmod(int imodel, diphone d, model *m, int &incrl, int &znely)
{
	int i;
	fcmodel *fcm = (fcmodel *)models->data + diph_offs[d.code] + imodel;

	incrl = fcm->incrl;
	m->esyn = (int)(32768.0 * fcm->ener);
	m->esyn = m->esyn * d.e / 100;
	for (i=0; i<rad; i++)
		m->rc[i]=(int)(32768.0 * fcm->rc[i]);
	znely = (incrl!=999);
	if (incrl == 999) incrl = 0;
}

void lpcint::frobmod(int imodel, diphone d, model *m, int &incrl, int &znely)
{
	int i;
	cmodel *cm = (cmodel *) models->data + diph_offs[d.code] + imodel;

//  	if (cfg->ti_adj) adjust(d);
	incrl = (int)cm->incrl;
	m->esyn = (int)cm->ener;
	m->esyn = m->esyn * d.e / 140;
	for(i=0; i<rad; i++)
		m->rc[i] = cm->rc[i];
	znely = (incrl != 999);
	if (incrl == 999) incrl = 0;
}

void floatoven(char *p, int l)
{
	if (!cfg->big_endian) return;
	for (fcmodel *tmp = (fcmodel *)p; (char *)tmp < p + l; tmp++) {
		// FIXME - a few floats
	}
}

void intoven(char *, int)
{
	if (!cfg->big_endian) return;
	shriek(462, "no int inventories on big-endians, please");
}

void vqoven(char *, int)
{
	if (!cfg->big_endian) return;
	shriek(462, "no vq inventories on big-endians, please");
}

void bswap(short *p)
{
	*p = ((*p & 255) << 8) | (*p >> 8);
}

void shortoven(char *p, int l)
{
	if (!cfg->big_endian) return;
	for (short *tmp = (short *)p; (char *)tmp < p + l; tmp++) {
		bswap(tmp);
	}
}


lpcfloat::lpcfloat(voice *v) : lpcsyn(v)
{
// 	fcmodely = (fcmodel *)freadin(v->models, v->inv_dir, "rb", "float inventory");
	models = claim(v->models, v->loc, cfg->inv_base_dir, "rb", "lpc inventory", floatoven);
}

lpcint::lpcint(voice *v) : lpcsyn(v)
{
// 	cmodely = (cmodel *)freadin(v->models, v->inv_dir, "rb", "integer inventory");
#if 0
	if (cfg->ti_adj) { //Needs some more hacking before use -- twice in this file
//	    kor_t = (short int *)freadin("korekce.set", v->inv_dir, "rb", "integer inventory");
		shriek(462, "Remove the comment above this message and hack the line to claim()/unclaim(), then remove this shriek()");
	    kor_i = (short int *)(445*2 + (char *)kor_t);
	}
#endif
	models = claim(v->models, v->loc, cfg->inv_base_dir, "rb", "lpc inventory", intoven);
}

lpcvq::lpcvq(voice *v) :lpcsyn(v)
{
//	codebook = (short int (*)[8])freadin(v->book, v->inv_dir, "rb", "vector quant synthesis");
	codebook = claim(v->book, v->loc, cfg->inv_base_dir, "rb", "vector quant synthesis codebook", shortoven);
	ener = (short int *)(256*16 + (char *)codebook->data);
//	vqmodels = (vqmodel *)freadin(v->models, v->inv_dir, "rb", "vector quant synthesis");
	models = claim(v->models, v->loc, cfg->inv_base_dir, "rb", "lpc inventory", vqoven);
}

lpcvq::~lpcvq()
{
	unclaim(codebook);
}

void lpcint::adjust(diphone d)
{
	int pomf;
	pomf=d.t;                         //korekce zatim nepouzivam
	pomf=kor_t[d.code]*pomf;
	d.t=pomf / 100;
	d.e=d.e*(100+(kor_i[d.code]-15))/100;  //Help Ptacek
//	d.e=d.e*(10000-625*(15-kor_i[d.code]));  //Checkme
}

