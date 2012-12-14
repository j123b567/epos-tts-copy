// definitions for clambr.cpp


#ifndef _CLAMBR_H_
	#define _CLAMBR_H_
#include "epos.h"

struct mbr_format {		//needed by TDPMBR a LPC-FEST
	char	label[5];
	int16_t	length, perc, f0;
	mbr_format	*next;
};

#define shit 0x00U

class mbrtdp {

public:
	mbr_format	*mbr;		//mbr to tdp format
	int16_t		*cds;		//segs codes
	float		*lengths;
	char		*triph;		//segs triphones
	char		*st;
	int16_t num_phnm, num_segs;
	bool trans;				//sampa alternates
	voice *vce;				//voice setup
	int16_t alloc;			//chars allocated
	int16_t maxlen;			//len
    bool classic;

	//function
    int get_count(){ return num_phnm; }
	void write_mbr();
	void write_b();
	void write_b(float *f);
	void clear_mem(mbr_format *m, bool flag);
	//void cds_extract();
	void timing();	//forward/backward time processing

	// class constructor/desturctor
	mbrtdp( char *b, voice *v, bool tr);
	mbrtdp();//FIXME chybi - bude potreba?
	~mbrtdp();

protected:
	char		*t;
};

#endif