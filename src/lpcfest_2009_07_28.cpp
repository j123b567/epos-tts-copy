/*
 *	epos/src/lpcfest.cc
 *	(c) 2006 Zdenek Chaloupka, chaloupka@ure.cas.cz
 *	(c) 1994-2000 Petr Horak, horak@ure.cas.cz
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


#include "epos.h"
#include "lpcfest.h"
#include "endian_utils.h"

#ifdef _DEBUG
    #include <crtdbg.h>
#endif

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifndef   O_BINARY
#define   O_BINARY	0
#endif

void lpcfest::synseg(voice *v, segment d, wavefm *w)	// voice not used
{
	shriek(442, "Use option -s for correct FESTIVAL synthesis");
}

/*
void floatoven(char *p, int l)
{
	if (!scfg->_big_endian)
		return;
	char*   stop = p + l;
	for (fcmodel* tmp = (fcmodel*)p; (char *)tmp < stop; tmp++){
		for (int i = 0;i < 8; i++) {
			tmp->rc[i] = (float) from_le32u(tmp->rc[i]);
		}
		tmp->ener = (float) from_le32u(tmp->ener);
		tmp->incrl = from_le32s(tmp->incrl);
	}
}
*/

inline int lpcfest::reverse32 (int N) {
	short int B0, B1, B2, B3; 
	B0 = (N & 0x000000FF) >> 0;
	B1 = (N & 0x0000FF00) >> 8;
	B2 = (N & 0x00FF0000) >> 16;
	B3 = (N & 0xFF000000) >> 24;
	return (B0 << 24) | (B1 << 16) | (B2 << 8) | (B3 << 0);
}

inline int lpcfest::ulaw2linear(uchar ulawbyte){
  static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
  int exponent, sample;


  ulawbyte = ~ulawbyte;
  exponent = (ulawbyte >> 4) & 0x07;
  sample = exp_lut[exponent] + ((ulawbyte & 0x0F) << (exponent + 3));
  if(ulawbyte & 0x80) 
	  return -sample;
  else
	  return sample;
}

char *lpcfest::convert_tab(char *chr){
	switch(*chr){
	case '?': return "_";
	case 'x': return "ch";
		//case pro znele ch* zatim neni z duvodu EPOS
	case 't': 
		if (strncmp(chr, "t_S", 3) == 0) 
			return "c~";
		if (strncmp(chr, "t_s", 3) == 0) 
			return "c";
		else
			return "t";
	case 'd': 
		if (strncmp(chr, "d_z", 3) == 0)
			return "dz";
		if (strncmp(chr, "d_Z", 3) == 0)
			return "dz~";
		else
			return "d";
	case 'N':
		return "n*";
	case 'J': 
		if (strncmp(chr, "J\\", 2) == 0)
			return "d~";
		else
			return "n~";
	case 'P':
		if (strncmp(chr, "P\\", 2) == 0)
			return "r~";
	case 'Q':
		if (strncmp(chr, "Q\\", 2) == 0)
			return "r~*";
	case 'h':
		if (strncmp(chr, "h\\", 2) == 0)
			return "h";
	case 'S':
		return "s~";
	case 'c':
		return "t~"; //c je 
	case 'Z':
		return "z~";
	case '_': 
		return "#";
	default: return chr;
		//shriek(CA1A, "SAMPA's and FESTIVAL's notation do not match");
	}
}

void lpcfest::clear_text(mbr_format *mbr, bool flag){
	if (mbr->next)
		clear_text(mbr->next, true);
	else if (flag)
		free(mbr);
}

void lpcfest::change_mbrola(char *b, mbr_format *mbr, int n, voice *v){
	int		i = 0, j = 0;
	char	*unit, num_buff[6] = {0};
	mbr_format	*ths;
	
	//if ( j != i )
	//	shriek(CA1A, "Internal EPOS error - sampa notation failed");

	for (; *b != 0; mbr++){
		while (*b == ';'){
			for (; *b != 0x0A; *b++);
			b++;
		}
		unit = b;
		for (i = 0; *unit != 0x0A; i += *unit++ == ')' ? 1 : 0);
		do {
			for (j = 0; *b != 0x20; num_buff[j++] = *b++);
			num_buff[j] = 0;

			//if (strcmp(v->parent_lang->name, "czech") == 0)
				//strcpy(mbr->label, convert_tab(num_buff));
                strcpy(mbr->label, num_buff);
			//else
				//encode_from_sampa(num_buff, (unsigned char*)mbr->label, v->sampa_alternate);

			for (j = 0; (*(++b) != '(') && (*b != ' '); num_buff[j++] = *b);
			num_buff[j] = 0;
			mbr->length = (v->init_t/100.0)*atoi(num_buff); b++;

			if (i) {	// zde bude muset byt cyklus zavisly na poctu zavorek
				do {
					if (mbr->next)
						ths = mbr->next;
					else
						ths = mbr;
					for (j = 0; *(++b) != ','; num_buff[j++] = *b);
					num_buff[j] = 0;
					ths->perc = atoi(num_buff);

					for (j = 0; *(++b) != ')'; num_buff[j++] = *b);
					num_buff[j] = 0;
					ths->f0 = atoi(num_buff);
					// kvuli zmene pri opicarne s mbrola
					if (mbr->next)
						ths->f0 -= 10;
					b += 2;
					if (--i)
						ths->next = (mbr_format*)calloc(1, sizeof(mbr_format));
				}
				while (ths->next);
			}
			else {
				mbr->perc = 99;
				mbr->f0 = v->init_f;
			}
		}
		while ( *b++ != 0x0A );
	}
}

void lpcfest::get_unit(label_hdr* lpc_unit, char* this_phnm, char* next_phnm){
	char	phonem[16]={0}, *unit, *chbuf, path[256] = {0};
	long int buff = 0, j = 0, a, b, c;

#define NUM_FRM 10
#define EST_H_END 15

    if ( -1 == sprintf(phonem, "\n%s-%s ", this_phnm, next_phnm) )
        shriek(CA1A, "Error occured inside lpcfest::get_unit.");
	while ( !(unit = strstr(lpc_units-1, phonem)) )
    {        
        if ( buff > 3 ) 
        {
            sprintf(path, "FESTIVAL inventory - missing unit %s-%s", this_phnm, next_phnm);
            shriek(CA1A, path);
        }
        
        do 
        {
            a = str2enum( buff ? next_phnm : this_phnm, vc->ph_alternates, 0);
            j = 10.0*((1.0*a/2.0) - (int)(1.0*a/2.0));

            if ( j == 0 )
            {
                unit = (char*)enum2str(++a, vc->ph_alternates);
                if ( buff == 1 )
                    sprintf(phonem, "\n%s-%s ", this_phnm, unit);
                else if ( buff == 2 )
                {
                    sprintf(phonem, "\n%s-%s ", chbuf, unit);
                }
                else
                {
                    sprintf(phonem, "\n%s-%s ", unit, next_phnm);
                    chbuf = unit;
                }
            }
            ++buff;
        }
        while ( j != 0 && buff < 4);
    }

    sscanf(unit, "%s %d %d %d", path, &a, &b, &c);
	if ( !(chbuf = strstr(coeffs + a, "NumFrames ") + NUM_FRM) )
		shriek(CA1A, "FESTIVAL inventory - invalid track header, 'NumFrames' not found.");    
	if ( !(lpc_unit->coeffs = (float*)(strstr(chbuf, "EST_Header_End") + EST_H_END)) )
		shriek(CA1A, "FESTIVAL inventory - invalid track header, 'EST_Header_End' not found.");;
	
    sscanf(chbuf, "%d", &a);
    lpc_unit->frames = a;
	lpc_unit->residual = coeffs + b;
	lpc_unit->length = c;
}


void lpcfest::synssif(voice *v, char *b, wavefm *w)
{	
	int		i, j, k, l, m, p, n, order, segm, data_size, samp_rate, encoding, 
			num_phnm, buffer = 0, un_lngt, frames, skip = 0;
    int iNumSampPV = 0;
	char	*chbuf, delim[] = {10,0}, phonem[7] = {0}, *diphon, *unit, 
			path[60] = {0};
	float	*y, point, repeat, nasob, mbr_lngt, intrpl = 0, rpt_intr, time, fbuff;
	double	*win, dEn1, dEn2;
	int16_t	buff;
	short	*outp;
	bool	rpt = true;
	snd_hdr	*snd_head;
	uchar	*snd_data, *ubuff;
	//FILE	*stream = fopen("d:\\usr\\temp\\lpcfest\\segmentyAdd", "wb");
	label_hdr		nx_unit, df_unit, pv_unit = {0};
	mbr_format		*text;
	lpc_coeff_hdr	*lpc;

    // NEW STYLUS
    char *cdeText; //*unit, int32_t num_phnm;
    mbr_format *mbr;

    mbrtdp *mb;
    mb = new mbrtdp(b, v, true);//constructor b - MBROLA, v - voice info, true - perform fromSAMPA translation;
    unit = mb->st;
	mbr = mb->mbr;
	cdeText = mb->triph;
	num_phnm = mb->num_phnm;
    text = mbr;

    unit = strstr(coeffs, "NumChannels") + 12;
    sscanf(unit, "%d", &order);

#define SEGM 2048
    y = (float*)calloc(SEGM, sizeof(*y));
	outp = (short*)calloc(SEGM, sizeof(*outp));
    win = (double*)malloc(SEGM*sizeof(*win));

    if ( *text[0].label == '_' && text[0].length < 50 ) text[0].length = 80; //the initial pause must be at least 80ms
    get_unit(&df_unit, text[0].label, text[1].label);
	
    //HLAVNI CYKLUS
	for (text = mbr, k = 0; NULL != *text[k+1].label; k++) {
		
        if ( NULL != *text[k+2].label ) 
            get_unit(&nx_unit, text[k+1].label, text[k+2].label);
        else
            memset(&nx_unit, 0, sizeof(nx_unit));
		
		lpc = (lpc_coeff_hdr*)(df_unit.coeffs);
		snd_head = (snd_hdr*)(df_unit.residual);
		data_size = reverse32(snd_head->data_size);
			
		if ( 1 != (encoding = reverse32(snd_head->encoding)) )
			shriek(CA1A, "Inventory invalid sample coding %d - only mu-law (.snd) is supported", encoding);
		if ( v->inv_sampling_rate != (samp_rate = reverse32(snd_head->sample_rate)) )
			shriek(CA1A, "Inventory invalid sampling rate %d - only 44100Hz is supported", samp_rate);
		if ( v->channel != (reverse32(snd_head->channels)-1) )
			shriek(CA1A, "Inventory invalid number of signal channels - only MONO is supported");
		snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));

		// length of segm depends on prosody (f0)
		segm = samp_rate*(2.0/text[k].f0);
        if ( segm/2 - (((segm/2)>>1)<<1) )
        {
            segm = ((segm>>1)+1)<<1;
            //shriek(CA1A, "FUCK");
        }
		//time = segm/(2.0*samp_rate);

		if (segm + order > SEGM)
			shriek(CA1A, "Allocated memory was exceeded"); // y, outp, win are allocated to size 2048B        

		n = 0;        

        //CompRepFact(text[k]&pv_unit, &df_unit, &nx_unit, &repeat);
        //DO compute repetition coeff if needed

        int iNumSampDF, iNumSampNX = 1;
        int iPtch, iRep;
        bool bAlloc = true, bRes = true, bCross = false;

        iNumSampDF = (int)(0.5 + 0.001*text[k].length*samp_rate) - iNumSampPV;
        iNumSampNX = (int)(0.5 + 0.001*text[k+1].length*samp_rate)*(1.0*(df_unit.frames-df_unit.length)/(df_unit.frames-df_unit.length+nx_unit.length));
        iNumSampPV = 0;

        if ( iNumSampDF < 0 && *text[k+1].label == '_' && *text[k+2].label == NULL )
            continue;

        float fRepeatDF = df_unit.length*segm/(2.0*iNumSampDF);
        float fRepeatNX = 1.0*(df_unit.frames - df_unit.length)*samp_rate/(text[k+1].f0*iNumSampNX);
        nasob = fabs(fRepeatDF - (int)(fRepeatDF));
        repeat = fRepeatDF;
        frames = df_unit.frames << 1;

        short int *siIdx = (short int*)calloc(frames, sizeof(*siIdx));        
        float *fSnd = (float*)calloc(segm, sizeof(*fSnd));
        memset(win, 0, sizeof(*win)*segm);
        hann(win, segm);

        i = skip ? df_unit.length : 0;
        p = 0;
        for (int seg=segm; i < df_unit.frames; p++)
        {            
            if ( p == frames )
            {
                frames += df_unit.frames;
                siIdx = (short int*)realloc(siIdx, frames*sizeof(*siIdx));
            }

            siIdx[p] = i;

            if ( i >= df_unit.length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                un_lngt = p;  //save length of the unit
                nasob = fabs(fRepeatNX - (int)(fRepeatNX));
                repeat = fRepeatNX;
                seg = samp_rate*(2.0/text[k+1].f0);
                if ( seg/2 - (((seg/2)>>1)<<1) ) //odd segments are badly moved
                {
                    seg = ((seg>>1)+1)<<1;
                }
                iNumSampPV = 0;
                bRes = false;
                if ( skip ) skip = 0;
                if ( iNumSampNX < seg ) { //the next unit is shorter then one segment - release it
                    skip = 1; 
                    break;
                }
            }

            if ( i >= df_unit.length && iNumSampPV + seg/2 > iNumSampNX ) break; //END when you are almost at the desired length

            iNumSampPV += seg/2;    //we are synthetise this F0

            if ( repeat < 1 )   // releasing or repeating pitchs?
            {   // releasing
                if ( nasob >= 1 )   //we reach the cummulative edge
                {
                    nasob -= (int)nasob;    //restoring
                    i++;
                }
                nasob += repeat;    //increase by the repeating const
            }
            else
            {   //repeating
                if ( nasob >= 1 )   //we reach the cummulative edge (1)
                {
                    nasob = fabs(fRepeatDF - (int)(fRepeatDF)); //restoring
                    i++;
                }
                else
                    nasob += repeat-(int)repeat;    // the reminder of the repeating const
                i += (int)repeat;   //increase by the repeating const
            }
        }
        //DO check the F0 difference between following units
            //FIXME - co bude znit lepe - menit F0 na konci prvni, nebo zacatku druhe casti difonu?
        buffer = abs(text[k].f0 - text[k+1].f0);
        int iF0idx = .5 + un_lngt*(buffer>5 ? 0.65 : 0.75);  //how much the F0 changes and where to begin?
        int iAktF0;

        //DO synthetize all frames from unit
        for (bRes=true, bAlloc=false, i = 0; i < p; i++)
        {
            if ( siIdx[i] >= siIdx[iF0idx] && siIdx[i] < df_unit.length && text[k+1].f0 != text[k].f0) //change the F0 gradually
            {
                iAktF0 = text[k].f0 + (text[k+1].f0-text[k].f0)*1.0*(siIdx[i]-siIdx[iF0idx]+1)/(siIdx[un_lngt]-siIdx[iF0idx]);

                iNumSampPV = segm;
                segm = samp_rate*(2.0/iAktF0);
                if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                {
                    segm = ((segm>>1)+1)<<1;
                }
                free(fSnd);
                fSnd = (float*)calloc(segm, sizeof(*fSnd));
                if ( iNumSampPV != segm )//FIXME - posunout na asymetricky stred take pitch pulse?
                {
                    memset(win, 0, sizeof(*win)*segm);
                    hannAs(win, iNumSampPV, segm);
                    bCross = true;
                }
                bAlloc = true;
            }

            if ( siIdx[i] >= df_unit.length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                nasob = fabs(fRepeatNX - (int)(fRepeatNX));
                repeat = fRepeatNX;
                iNumSampPV = segm;
                segm = samp_rate*(2.0/text[k+1].f0);
                if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                {
                    segm = ((segm>>1)+1)<<1;
                }
                free(fSnd);
                fSnd = (float*)calloc(segm, sizeof(*fSnd));
                if ( iNumSampPV != segm )//FIXME - posunout na asymetricky stred take pitch pulse?
                {
                    memset(win, 0, sizeof(*win)*segm);
                    hannAs(win, iNumSampPV, segm);
                    bCross = true;
                }
                iNumSampPV = 0;
                bRes = false;
            }

            if ( siIdx[i] != siIdx[i-1] || bAlloc) //not to allocate the segment twice
            {
                //DO allocate segm/2 from left side of the pitch .....|.
                iPtch = (int)(.5 + samp_rate*lpc[siIdx[i]*(order+2)].p_mark);
                for ( j=0; j < segm/2; j++ )
                {
                    //out of begining of the sound data or crossing the scope to previous pitch pulse
                    if ( iPtch-j < 0 || (siIdx[i]>0 && iPtch-j < samp_rate*lpc[(siIdx[i]-1)*(order+2)].p_mark) )
                        fSnd[segm/2-j] = 0;
                    else 
                        fSnd[segm/2-j] = (float)ulaw2linear(snd_data[ iPtch-j ]);
                }
                //DO allocate segm/2 from right side of the pitch .|.....
                for ( j=1; j < segm/2; j++ )
                {
                    //crossing the scope to next pitch pulse or out of the sound data
                    if ( (siIdx[i]-1 < df_unit.frames && iPtch+j > samp_rate*lpc[(siIdx[i]+1)*(order+2)].p_mark) || (iPtch+j >= data_size) )
                        fSnd[segm/2+j] = 0;
                    else
                        fSnd[segm/2+j] = (float)ulaw2linear(snd_data[ iPtch+j ]);
                }
            }

            //DO energy correction if crossing F0 - STILL in progress
            /*
            if ( bCross )
            {
                dEn1 = dEn2 = 0;
                for ( l=0; l < segm; l++ )
                {
                    dEn1 += win[l]*fSnd[l];
                    dEn2 += fSnd[l]*0.5*(1.0 - cos(2.0*pii*l/(segm-1)));
                }
            }*/

            //DO synthetize the segment
            for (buff = 0, l = order; l < segm+order; l++) 
            {
                y[l] = win[l-order]*fSnd[l-order];
                for (m = 2; m < order+1; y[l] += 1.0*lpc[siIdx[i]*(order+2)].lpccoff[m]*y[l-m+1], m++);

                if (y[l] > 32767 || y[l] < -32767)
                    shriek(CA1A, "lpcfest::synth> Out of short int scale.");
                else
                    outp[l-order] += (short)y[l];
            }

            w->sample(outp, segm/2);
            memmove(outp, outp+segm/2, sizeof(*outp)*segm/2);
            memset(outp+segm/2, 0, sizeof(*outp)*(SEGM-segm/2-1));
            iNumSampPV += segm/2;
            
            if ( bCross )//asymmetric window
            {
                memset(win, 0, sizeof(*win)*segm);
                hann(win, segm);
                bCross = false;
            }
        }
        free(fSnd);
        free(siIdx);
		pv_unit = df_unit;
		df_unit = nx_unit;
	}

    delete mb;
	free(y);
	free(outp);
    free(win);
}

void festoven(char *p, int l){
	if (!scfg->_big_endian)
		return;
	else 
		shriek(CA1A, "nevim co delat, prisel na radu OVEN");
}

lpcfest::lpcfest(voice *v)
{
	int			i = 0, j = 0, k = 0;
	char		*point, set_delim[] = {10, 0}, *buff, num_buff[10] = {0},
				chbuff[50] = {0};
	bool		yesno;
	//label_hdr	*units;

    vc = v;
	lpc_units = 0;
	models	= claim(v->models, v->location, scfg->inv_base_dir, "rb", "inventory", festoven);
	
	if ( (strncmp(models->data, "EST_File", 8) != 0) && (strncmp(&models->data[9], "index", 5) != 0) ) 
		shriek(CA1A, "FESTIVAL grouped file header expected %S", models->filename);

	point	= models->data + strcspn(models->data, set_delim)+1;
	coeffs	= strstr(point, "EST_File Track");

	do {
		for (j = i = 0, yesno = true; (*point != 0x0A) && (*point != 0); chbuff[i++] = *point++) {
			if ((*point != 0x20) && (yesno) )
				j += *point;
			else
				yesno = false;
		};
		chbuff[i] = 0;

		if (!lpc_units) {
			switch (j) {
			//Version
			case 742: if ( (strcmp(chbuff, "Version 2") != 0) && cfg->paranoid )
						  shriek(CA1A, "FESTIVAL inventory - uncompatible version file type %S", models->filename);
				break;
			//DataFormat
			case 796: if (strcmp(chbuff, "DataType ascii") != NULL)
						  shriek(CA1A, "FESTIVAL inventory - DataType is invalid", point);
				break;
			//IndexName
			case 889: if (strncmp(chbuff, "IndexName", 9) != 0) 
						  shriek(CA1A, "FESTIVAL inventory - item 'IndexName' expected", point);
				index_name = (char*)calloc(strlen(chbuff)-10, sizeof(char));
				strcpy(index_name, chbuff+10);
				///pridat polozku pro index name
				break;
			//DataFormat
			case 995: if (strcmp(chbuff, "DataFormat grouped") != NULL)	
						  shriek(CA1A, "FESTIVAL inventory - DataFormat is invalid", point);
				break;
			//NumEntries
			case 1034: if ( (strncmp(chbuff, "NumEntries", 10) != 0) && (cfg->paranoid))
						   shriek(CA1A, "FESTIVAL inventory - item 'NumEntries' expected", atoi(point));
				nmbr_unit = atoi(&chbuff[11]);
				//def_units = (label_hdr*)calloc(nmbr_unit, sizeof(label_hdr));
				break;
			case 1290: if (strcmp(chbuff, "EST_Header_End") != 0)
						   shriek(CA1A, "FESTIVAL inventory - header end expected (is %S)", point);
				lpc_units = point + 1;
				break; 
			//sig_file_format
			case 1578: if (strcmp(chbuff, "sig_file_format snd") != 0)	
						   shriek(CA1A, "FESTIVAL inventory - signal file format must be 'snd' (is %S)", point);
				break;
			//track_file_format
			case 1788: if (strcmp(chbuff, "track_file_format est_binary") != 0)	
						   shriek(CA1A, "FESTIVAL inventory - track file format must be binary (is %S)", point);
				break;
			default: shriek(CA1A, "FESTIVAL inventory - unexpected header error (is %S)", point);
				break;
			}
		}
		point++;
	}
	while (!lpc_units);
}

lpcfest::~lpcfest()
{
	unclaim(models);
	free(index_name);
	//free(def_units);
}

void hann(double *x, int n)
{
	for (int i=0; i < n; i++) {
		x[i] = 0.5*(1.0 - cos(2.0*pii*i/(n-1)));
	}
}

void hannAs(double *x, int n1, int n2)
{
    int i;
    
    if ( n2 < n1/2 ) //FIXME - what to do? - it cannot be done - solution: Pitch must change logarhytmically over longer period!
    {
        shriek(CA1A, "lpcfest::synth Asymmetric HANN failed.");
        for ( i=0; i < n1/2; i++) 
        {
            x[i] = 0.5*(1.0 - cos(2.0*pii*i/(n1/2-1)));
        }
        //shriek(CA1A, "lpcfest::synth Asymmetric HANN failed.");
    }

    //double xx=0, y=0;
	for ( i=0; i < n1/2+1; i++) 
    {
		x[i] = 0.5*(1.0 - cos(2.0*pii*i/(n1-1)));
        //xx += x[i];
    }

    for (int j = n2/2+1; i < n2; i++) 
    {
        if ( i < (n2/2-1) )
            x[i] = 1;
        else
        {
            x[i] = 0.5*(1.0 - cos(2.0*pii*j/(n2-1)));
            j++;
        }
        //xx += x[i];
    }
    //set the amplitude to have normal gain
    //double *y = (double*)malloc(sizeof(double)*n2);
    /*
    for ( i=0; i < n2; i++)
    {
        y += 0.5*(1.0 - cos(2.0*pii*i/(n2-1)));
    }
    y = y/xx;

    for ( i=0; i < n2; i++)
    {
        x[i] *= y;
    } */   
}

/*
void CompRepFact(int time, label_hdr *pv_unit, label_hdr *df_unit, label_hdr *nx_unit, int *rep)
{
#define DF .7
#define NX .3

    lpc_coeff_hdr *lpc = (lpc_coeff_hdr *)df_unit->coeffs;
    int iSFr = 
    int iNumSamp = (pv_unit->frames-pv_unit->length)
}
*/

/*
void lpcfest::synssif(voice *v, char *b, wavefm *w){
	
	int		i, j, k, l, m, p, n, order, segm, data_size, samp_rate, encoding, 
			num_phnm, buffer = 0, un_lngt, frames;
	char	*chbuf, delim[] = {10,0}, phonem[7] = {0}, *diphon, *unit, 
			path[60] = {0};
	float	*y, point, repeat, nasob, mbr_lngt, intrpl = 0, rpt_intr, time, fbuff;
	double	*win;
	int16_t	buff;
	short	*outp;
	bool	rpt = true;
	snd_hdr	*snd_head;
	uchar	*snd_data, *ubuff;
	//FILE	*stream = fopen("d:\\usr\\temp\\lpcfest\\segmentyAdd", "wb");
	label_hdr		nx_unit, df_unit, pv_unit = {0};
	mbr_format		*text;
	lpc_coeff_hdr	*lpc;

	diphon = (char*)calloc(30, sizeof(char));
	unit = diphon;
	strncpy(unit, strstr(coeffs, "NumChannels") + 12, 5);
	order = atoi(strtok(unit, delim));

	y = (float*)calloc(2048, sizeof(*y));
	outp = (short*)calloc(2048, sizeof(*outp));
	//win = (double*)calloc(2048, sizeof(*win));

	unit = b;
	for (num_phnm = 0; *unit != 0; num_phnm += *unit++ == 0x0A ? 1 : 0);

    // OLD MBROLA
	text = (mbr_format*)calloc(num_phnm+1, sizeof(mbr_format));
	change_mbrola(b, text, num_phnm, v);

    // NEW MBROLA
    mbrtdp *mb;
    mb = new mbrtdp(b, v, false);//constructor b - MBROLA, v - voice info, true - perform fromSAMPA translation;
    unit = mb->st;
	//text = mb->mbr;
	//cdeText = mb->triph;
	num_phnm = mb->num_phnm;

	get_unit(&df_unit, text[0].label, text[1].label);
	//HLAVNI CYKLUS
	for (k = 0; k < num_phnm-1; k++) {
		
		get_unit(&nx_unit, text[k+1].label, text[k+2].label);
		
		lpc = (lpc_coeff_hdr*)(df_unit.coeffs);
		snd_head = (snd_hdr*)(df_unit.residual);
		data_size = reverse32(snd_head->data_size);
			
		if ( 1 != (encoding = reverse32(snd_head->encoding)) )
			shriek(CA1A, "Inventory invalid sample coding %d - only mu-law (.snd) is supported", encoding);
		if ( v->inv_sampling_rate != (samp_rate = reverse32(snd_head->sample_rate)) )
			shriek(CA1A, "Inventory invalid sampling rate %d - only 44100Hz is supported", samp_rate);
		if ( v->channel != (reverse32(snd_head->channels)-1) )
			shriek(CA1A, "Inventory invalid number of signal channels - only MONO is supported");
		snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));

		// length of segm depends on prosody (f0)
		segm = samp_rate*(2.0/text[k].f0);
		time = segm/(2.0*samp_rate);

		if (segm + order > 2048)
			shriek(CA1A, "Allocated memory was exceeded"); // y, outp, win are allocated to size 2048B

		n = 0;
		do {
			// odhad poctu segmentu
			frames = 0;
			mbr_lngt = (text[k+n].length*samp_rate/1000.0)/(segm/2.0);
			if ( (pv_unit.coeffs) && (!n) ){
				un_lngt = df_unit.length + pv_unit.frames - pv_unit.length;
				nasob = repeat = un_lngt/mbr_lngt;
				mbr_lngt *= df_unit.length/(double)un_lngt;
			}
			else if (!n){
				un_lngt = df_unit.length;
				nasob = repeat = un_lngt/mbr_lngt;
			}
			else {
				un_lngt = df_unit.frames - df_unit.length + nx_unit.length;
				frames = df_unit.length;
				nasob = repeat = un_lngt/mbr_lngt;
				mbr_lngt *= (df_unit.frames - df_unit.length)/(double)un_lngt;
			}// odhad poctu segmentu
			
			// synteza jednotky s pozadovanou delkou
			for (i = 0, p = 0; (p < mbr_lngt) && (i+frames < df_unit.frames); i++, p++){

				fbuff = lpc[(i+frames)*(order+2)].p_mark - lpc[(i+frames-1)*(order+2)].p_mark;
				if (i+frames > df_unit.frames)
					shriek(CA1A, "Number of frames was exceeded, segment %d", k);
				if ( (point = lpc[(i+frames)*(order+2)].p_mark - time) > 0) {
					if ( ((i != 0) && (time > 0.8*fbuff)) ||
						 (data_size < (lpc[(i+frames)*(order+2)].p_mark + time)*samp_rate) )
						rpt_intr = intrpl = 2.0*time/(0.8*fbuff);
					else 
						intrpl = 0;
					point *= samp_rate;
				}
				else {
					point = 0;
					intrpl = 0;
				}

				if (intrpl){
					ubuff = &snd_data[(int)(samp_rate*(lpc[(i+frames)*(order+2)].p_mark - 0.8*fbuff))];
					memset(y, 0, sizeof(*y)*(segm + order));
					if ( data_size < (lpc[(i+frames)*(order+2)].p_mark + time)*samp_rate ){
						buff = (int)(samp_rate*(2*lpc[(i+frames)*(order+2)].p_mark - data_size/(float)samp_rate));
						ubuff = &snd_data[buff];
						for( j = 0; j < data_size - buff; y[order+ (segm-(data_size-buff))/2 + j++] = (float)ulaw2linear(*ubuff++));
					}
					else
						for( buff = (int)(samp_rate*(2*0.8*fbuff)), j = 0; j < buff; y[order + (segm-buff)/2 + j++] = (float)ulaw2linear(*ubuff++));
				}
				else
					ubuff = &snd_data[(int)point];

				
				for (buff = 0, l = order; l < segm+order; l++) {

					if (!intrpl)
						y[l] = (float)ulaw2linear(*ubuff++);
					for (m = 2; m < order+1; y[l] += lpc[(i+frames)*(order+2)].lpccoff[m]*y[l-m+1], m++); 

					if (y[l] > 32766)
						outp[l-order] = (short)32766;
					else if (y[l] < -32766)
						outp[l-order] = (short)-32766;
					else
						outp[l-order] = (short)y[l];
				}
				w->sampleAdd(outp, segm, samp_rate, segm/(2.0*samp_rate));
				if ( i == (int)repeat ){
					repeat += nasob;
					if ( nasob < 1 )
						i--;
					else
						i++;
				}
			}// synteza jednotky s pozadovanou delkou
		}
		while (!(n++));
		pv_unit = df_unit;
		df_unit = nx_unit;
	}
	for (i = 0; i < num_phnm+1; clear_text(&text[i++], false));

	free(y);
	free(outp);
	free(text);
}
*/

/* BACKUP 2009_06_18
segm = samp_rate*(2.0/text[k].f0);
        if ( segm/2 - (((segm/2)>>1)<<1) )
        {
            segm = ((segm>>1)+1)<<1;
            //shriek(CA1A, "FUCK");
        }
		//time = segm/(2.0*samp_rate);

		if (segm + order > SEGM)
			shriek(CA1A, "Allocated memory was exceeded"); // y, outp, win are allocated to size 2048B        

		n = 0;        

        int iPtch, iRep;
        bool bAlloc = true, bRes = true, bCross = false;
        float *fSnd = (float*)calloc(segm, sizeof(*fSnd));
        memset(win, 0, sizeof(*win)*segm);
        hann(win, segm);

        //CompRepFact(text[k]&pv_unit, &df_unit, &nx_unit, &repeat);
        //DO compute repetition coeff if needed
        int iNumSampDF = (int)(0.5 + 0.001*text[k].length*samp_rate) - iNumSampPV,
            iNumSampNX = (int)(0.5 + 0.001*text[k+1].length*samp_rate)*(1.0*(df_unit.frames-df_unit.length)/(df_unit.frames-df_unit.length+nx_unit.length));
        iNumSampPV = 0;
        float fRepeatDF = df_unit.length*segm/(2.0*iNumSampDF);
        float fRepeatNX = 1.0*(df_unit.frames - df_unit.length)*samp_rate/(text[k+1].f0*iNumSampNX);

        nasob = fabs(fRepeatDF - (int)(fRepeatDF));
        repeat = fRepeatDF;
        //DO synthetize all frames from unit
        for (i = 0, p = 0; i < df_unit.frames; p++)
        {
            if ( i >= df_unit.length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                nasob = fabs(fRepeatNX - (int)(fRepeatNX));
                repeat = fRepeatNX;
                iNumSampPV = segm;
                segm = samp_rate*(2.0/text[k+1].f0);
                if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                {
                    segm = ((segm>>1)+1)<<1;
                }
                free(fSnd);
                fSnd = (float*)calloc(segm, sizeof(*fSnd));
                if ( iNumSampPV != segm )//FIXME - posunout na asymetricky stred take pitch pulse?
                {
                    memset(win, 0, sizeof(*win)*segm);
                    hannAs(win, iNumSampPV, segm);
                    bCross = true;
                }
                iNumSampPV = 0;
                bRes = false;
            }
            if ( i >= df_unit.length && iNumSampPV + segm/2 > iNumSampNX )
                break;
            if ( bAlloc ) //not to allocate the segment twice
            {
                //DO allocate segm/2 from left side of the pitch .....|.
                iPtch = (int)(.5 + samp_rate*lpc[i*(order+2)].p_mark);
                for ( j=0; j < segm/2; j++ )
                {
                    //out of begining of the sound data or crossing the scope to previous pitch pulse
                    if ( iPtch-j < 0 || (i>0 && iPtch-j < samp_rate*lpc[(i-1)*(order+2)].p_mark) )
                        fSnd[segm/2-j] = 0;
                    else 
                        fSnd[segm/2-j] = (float)ulaw2linear(snd_data[ iPtch-j ]);
                }
                //DO allocate segm/2 from right side of the pitch .|.....
                for ( j=1; j < segm/2; j++ )
                {
                    //crossing the scope to next pitch pulse or out of the sound data
                    if ( (i-1 < df_unit.frames && iPtch+j > samp_rate*lpc[(i+1)*(order+2)].p_mark) || (iPtch+j >= data_size) )
                        fSnd[segm/2+j] = 0;
                    else
                        fSnd[segm/2+j] = (float)ulaw2linear(snd_data[ iPtch+j ]);
                }
                bAlloc = false;
            }

            //DO energy correction if crossing F0 - STILL in progress
            /*
            if ( bCross )
            {
                dEn1 = dEn2 = 0;
                for ( l=0; l < segm; l++ )
                {
                    dEn1 += win[l]*fSnd[l];
                    dEn2 += fSnd[l]*0.5*(1.0 - cos(2.0*pii*l/(segm-1)));
                }
            }

            //DO synthetize the segment
            for (buff = 0, l = order; l < segm+order; l++) 
            {
                y[l] = win[l-order]*fSnd[l-order];
                for (m = 2; m < order+1; y[l] += 1.0*lpc[i*(order+2)].lpccoff[m]*y[l-m+1], m++);

                if (y[l] > 32767 || y[l] < -32767)
                    shriek(CA1A, "lpcfest::synth> Out of short int scale.");
                else
                    outp[l-order] += (short)y[l];
            }

#ifndef _DEBUG
            if ( p == 0 )
                outp[segm-1] = 32767;
#endif

            w->sample(outp, segm/2);
            memmove(outp, outp+segm/2, sizeof(*outp)*segm/2);
            memset(outp+segm/2, 0, sizeof(*outp)*(SEGM-segm/2-1));
            iNumSampPV += segm/2;
            
            if ( bCross )//asymmetric window
            {
                memset(win, 0, sizeof(*win)*segm);
                hann(win, segm);
                bCross = false;
            }

            if ( repeat < 1 )
            {                
                if ( nasob >= 1 )
                {
                    nasob -= (int)nasob;
                    i++;
                    bAlloc = true;
                }
                nasob += repeat;
            }
            else
            {
                if ( nasob >= 1 )
                {
                    nasob = fabs(fRepeatDF - (int)(fRepeatDF));
                    i++;
                }
                else
                    nasob += repeat-(int)repeat;
                i += (int)repeat;
                bAlloc = true;
            }            
            //w->sampleAdd(outp, segm, samp_rate, segm/(2.0*samp_rate));
        }
        free(fSnd);
		pv_unit = df_unit;
		df_unit = nx_unit;
	}
*/