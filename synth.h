/*
 *	ss/src/syn32.h
 *	(c) 1994-97 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
 *	(c) 1997 Jirka Hanika, geo@ff.cuni.cz
 *
 *	This file has not been fully integrated yet.
 */

#ifndef SYN32_H
#define SYN32_H

//#define  maxmodel		24
#define  minsynt		23

#define  sample_t		short

const long DEEM=29184;	// constant of deemphasis = 0.89 * 32768
const long HPF=26542;   // constant of high-pass output filter
const int UPRAV=5120;
const nhilb=15;         // number of Hilbert coefficients of excitation impulse
const rad=8;            // order of LPC

const long hilbk[15]={4608,0,3584,0,6656,0,20992,0,-20992,0,-6656,0,-3584,0,-4608};
                        // Hilbert impulse for voiced excitation
const int lroot=10054;

//const int SizeBuff=8192;


typedef struct
{
	int hlas;
	int znel;
	int inf0;
	int adre;
	int adrrc;
} vqimodel;

/*	typedef struct
	{
	int znel;
	int inf0;
	int adre;
	int adrrc;
	} pphlas;
*/

typedef struct
{
	short int rc[8];
	short int ener;
	short int incrl;
} cmodel;               // model for integer synthesis

typedef struct
{
	float rc[8];
	float ener;
	float incrl;
} fcmodel;              // model for float synthesis

typedef struct
{
	short int adrrc;
	short int adren;
	short int incrl;
} vqmodel;              // model for vector quantised synthesis

typedef struct
{
	int rc[8];
	int nsyn;
	int lsyn;
	int esyn;
	int lsq;
} model;                //internal representation of model

/*
typedef struct
{
	int hlaska;
	int lproz;
	int eproz;
	int nproz;
} difon;
*/ /*

typedef struct
{
	int kod;
	int rychlost;
	int f0 ;
	int intenzita;
} hlaska;

typedef struct
{
	int Difon;
	int DDelka;
	int DPitch;
	int DEnergy;
} TDifon;

typedef struct
{
	long DifCount;
	TDifon *Dif;
} TDifony;

*/

typedef struct
{
	char string1[4];
	long flen;
	char string2[8];
	long xnone;
	short int  datform, numchan, sf1, sf2, avr1, avr2, wlenB, wlenb;
	char string3[4];
	long dlen;
 } wavehead;            // wav file header

class synteza
{
	long NumSample, iyold,ifilt[8],kyu;
	int ipitch,i,lastl;
	unsigned int crc;
	unsigned int pocet_vzorku;
	int wavout; 		// raw file handle (buffering is home made,
				// we use ioctl's, so FILE would be clumsy...)
	sample_t *wave_buffer;
	int buffer_size;
	int buff_idx;
	wavehead wavh;
   public:
	synteza(int l);
	~synteza(void);
	void synmod(model mod);
};

class dsynth //:synteza
{
	vqimodel vqmodels[4000];	// array of models for vq synthesis
	cmodel cmodely[4000];  		// array of models for vq synthesis
	fcmodel fcmodely[4000];    	// array of models for vq synthesis
	short int kor_t[445], kor_i[445];	//corrections of doc. Ptacek on WS97
	float fkor_t[445], fkor_i[445];		//used for his vacation inventory
	short int ener[64];        	// tabula energiae
	int i,lold,nvyrov;
	unsigned char ldelky[441]; 	// diphone lengths
	short int kodk[256][8];    	// codebook for vector quantisation
	int pzac[441];             	// index table of diphones in model array
	SYNTH_TYPE use_syn;        	// type of the synthesis
	synteza *syn;
   public:
	char tdiph[441][4];        	//table of text names of the diphones
	dsynth(int sw_syn, int f0);
	dsynth(SYNTH_TYPE type, const char *count, const char *models, const char *book,
		const char *dpt, int f0, int t0, int i0, int Hz);
	~dsynth(void);
	void syndif(diphone d);
};

void play_diphones(unit *root, dsynth *ds);
void show_diphones(unit *root, dsynth *ds);

#endif		// ifndef SYN32_H
