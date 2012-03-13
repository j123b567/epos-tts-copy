/*
 *	epos/src/ptdsyn.h
 *	(c) 1997-98 Martin Petriska, petriska@decef.elf.stuba.sk
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

#ifndef EPOS_PTD_H
#define EPOS_PTD_H
#define sample_type short
#define pimpMAX 40
#define MAXFILENAME 50
#define WAVEMAX 30000
class kbuffer
{private:
  int N;
  
 public: 
  sample_type *data; 
  int i1,i2,i3,oST;
  void kbaddST(sample_type *x,int ST,int ind);
  kbuffer(int len);
  ~kbuffer();
  sample_type kbread();
  void kbadd(sample_type x);
};
enum WIN {HAMMING, HANN, PARZEN, BLACKMAN};
class DIFON
{ public:
  char *name;
  char *fname;
  int pimp;
  int time;
  int *labels;
  DIFON()
  { name=new char[20];
	 fname=new char[20];
	 labels=new int[pimpMAX];
	 pimp=0;
	 time=0;
  }
  ~DIFON()
  { if(name)delete (name);
    if(fname)delete (fname);
    if(labels) delete (labels);
  }
};
class ptdsyn : public synth
{
 private:
 DIFON *dif;
 kbuffer *K;
 sample_type val;
 sample_type *PSL;
 int ST;
 double Fvz1,Fvz2;
 int npimp;
 double ntime,fn;
 double nperi;
 int nperiN;
 int i,j;
 int *ta;
 public:
 double window(int I,int N, WIN w);
 int analyza(DIFON *dfn, sample_type *waveout, int *tas, voice *v);
 int modifik(sample_type *wave, int pimp, int npimp, int nperiN, int *ta);
 ptdsyn(voice *);
 ~ptdsyn();
 virtual void syndiph(voice *v, diphone d, wavefm *w);
};

#endif