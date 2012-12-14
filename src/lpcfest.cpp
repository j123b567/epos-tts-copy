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

//#define _DEBUG
#include <assert.h>

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

void lpcfest::get_unit(label_hdr* lpc_unit, char* this_phnm, char* next_phnm){
	char	phonem[16]={0}, *unit, *chbuf, path[256] = {0};
	long int buff = 0, j = 0, a, b, c;

#define NUM_FRM 10
#define EST_H_END 15

    if ( -1 == sprintf(phonem, "\n%s-%s ", this_phnm, next_phnm) )
        shriek(CA1A, "LPCFEST::get_unit - Can not make unit.");
	while ( !(unit = strstr(lpc_units-1, phonem)) )
    {        
        if ( buff > 3 ) 
        {
            sprintf(path, "LPCFEST::get_unit - FESTIVAL inventory is missing unit %s-%s", this_phnm, next_phnm);
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
        shriek(CA1A, "LPCFEST::get_unit - FESTIVAL inventory invalid track header, 'NumFrames' not found.");    
	if ( !(lpc_unit->coeffs = (float*)(strstr(chbuf, "EST_Header_End") + EST_H_END)) )
		shriek(CA1A, "LPCFEST::get_unit - FESTIVAL inventory invalid track header, 'EST_Header_End' not found.");;
	
    sscanf(chbuf, "%d", &a);
    lpc_unit->frames = a;
	lpc_unit->residual = coeffs + b;
	lpc_unit->length = c;
}

int lpcfest::prepare_units(label_hdr **lpc_unit, mbr_format *mb_text, int num)
{
    lpc_coeff_hdr *lpc;
    snd_hdr *snd_head;
    int data_size, samp_rate, segm, k;

    label_hdr *lpcu = (label_hdr*)xcalloc(num, sizeof(label_hdr));
    get_unit(lpcu, mb_text[0].label, mb_text[1].label);
    for ( k = 0; NULL != *mb_text[k+1].label; k++ ) 
    {
        if ( k+2 == num )
        {
            num += 20;
            lpcu = (label_hdr*)xrealloc(lpcu, num*sizeof(label_hdr));
        }

        if ( NULL != *mb_text[k+2].label ) 
            get_unit(lpcu+k+1, mb_text[k+1].label, mb_text[k+2].label);
		
		lpc = (lpc_coeff_hdr*)(lpcu[k].coeffs);
		snd_head = (snd_hdr*)(lpcu[k].residual);
		data_size = reverse32(snd_head->data_size);
			
		if ( 1 != reverse32(snd_head->encoding) )
			shriek(CA1A, "LPCFEST::prepare_units - Inventory invalid sample coding %d - only mu-law (.snd) is supported", reverse32(snd_head->encoding));
		if ( vc->inv_sampling_rate != (samp_rate = reverse32(snd_head->sample_rate)) )
			shriek(CA1A, "LPCFEST::prepare_units - Inventory invalid sampling rate %d - only 44100Hz is supported", samp_rate);
		if ( vc->channel != (reverse32(snd_head->channels)-1) )
			shriek(CA1A, "LPCFEST::prepare_units - Inventory invalid number of signal channels - only MONO is supported");

		// length of segm depends on prosody (f0)
		segm = samp_rate*(2.0/mb_text[k].f0);
		if (segm + order > SEGM)
			shriek(CA1A, "LPCFEST::prepare_units - Allocated memory was exceeded"); // y, outp, win are allocated to size 2048B        
	}
    *lpc_unit = lpcu;
    return k;
}

void lpcfest::synssif(voice *v, char *b, wavefm *w)
{	
	int		i, j, k, l, m, p, n, segm, data_size, samp_rate, 
			num_phnm, buffer = 0;
    int iNumSampPV = 0, iNumSampDF, iNSamp, iPtch;
    bool bRealoc, bNext;
	char	*unit;
	float	*y, repeat, nasob;
	double	*win;
	int16_t	buff, sbuff;
	short	*outp;
	bool	rpt = true;
	snd_hdr	*snd_head;
	uchar	*snd_data;
	//FILE	*stream = fopen("d:\\usr\\temp\\lpcfest\\segmentyAdd", "wb");
	mbr_format		*text;
	lpc_coeff_hdr	*lpc;

    // NEW STYLUS
    char *cdeText, *new_b; //*unit, int32_t num_phnm;
    mbr_format *mbr;

    mbrtdp *mb;
    if ( strncmp(b, "_", 1) )
    {
        new_b = (char*)xcalloc(strlen(b)+10, 1);
        sprintf(new_b, "_\t30\n%s", b);
    }
    else new_b = b; 

    mb = new mbrtdp(new_b, v, true);//constructor b - MBROLA, v - voice info, true - perform fromSAMPA translation;
    unit = mb->st;
	mbr = mb->mbr;
	cdeText = mb->triph;
	num_phnm = mb->num_phnm;
    text = mbr;

    unit = strstr(coeffs, "NumChannels") + 12;
    sscanf(unit, "%d", &order);

    y = (float*)xcalloc(SEGM, sizeof(*y));
	outp = (short*)xcalloc(SEGM, sizeof(*outp));
    win = (double*)malloc(SEGM*sizeof(*win));

    //if ( *text[0].label == '_' && text[0].length < 50 ) text[0].length = 80; //the initial pause must be at least 80ms
    
    label_hdr *lpcu;
    p = prepare_units(&lpcu, mbr, mb->get_count());
	
    //HLAVNI CYKLUS
    //segm = 16000*(2.0/text[0].f0);
    int f0 = vc->init_f;
	for (text = mbr, k = 0, iNSamp = 0; k < p; k++) {	
		
		lpc = (lpc_coeff_hdr*)(lpcu[k].coeffs);
		snd_head = (snd_hdr*)(lpcu[k].residual);
		data_size = reverse32(snd_head->data_size);
        samp_rate = reverse32(snd_head->sample_rate);
		snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));

        //CompRepFact(text[k]&pv_unit, &lpcu[k], &lpcu[k+1], &repeat);
        //DO compute repetition coeff if needed

        iNumSampDF = (int)(0.5 + 0.001*mbr[k].length*samp_rate) - iNumSampPV;
        int iUnitPP, pct[32], len, mb_f0, lst_f0, boundary;
        mbr_format *ths;
        iUnitPP = lpcu[k].frames-lpcu[k].length+lpcu[k+1].length;
        len = 0;
        for ( ths=mbr+k+1; ths; ths = ths->next )
        {
            pct[len++] = ths->perc;
        }

        switch (len)
        {
        case 1: 
        case 2: 
            ths = mbr+k+1;
            if ( ths->perc != -1 )
            {
                mb_f0 = ths->next->f0;
            }
            f0 = ths->f0;
            buffer = iUnitPP*samp_rate/f0;
            break;
        default:
            buffer = 0;
            ths = mbr[k+1].next;
            f0 = mbr[k+1].f0;
            j = 0;
            for (int i=1; i < len; i++)
            {
                buffer += ( i+1 == len ? iUnitPP-j : (int)(.5+(pct[i]-pct[i-1])*iUnitPP/100.0) )*samp_rate/ths->f0;
                j += (int)(.5+(pct[i]-pct[i-1])*iUnitPP/100.0);
                ths = ths->next;
            }
            ths = mbr[k+1].next;
        }
        lst_f0 = f0;

        segm = samp_rate*(2.0/f0);// length of segm depends on prosody (f0)
        if ( segm/2 - (((segm/2)>>1)<<1) )// we need even length
        {
            segm = ((segm>>1)+1)<<1;
        }

        len = mbr[k+1].length*0.001*samp_rate;
        float fRepeatDF = 1.0*buffer/len;

        bRealoc = true;        
        nasob = fRepeatDF - (int)(fRepeatDF);
        repeat = fRepeatDF;

        float *fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
        memset(win, 0, sizeof(*win)*segm);
        hann(win, segm);

        //DO synthetize all frames from unit
        //text = mbr[k].next;
        bNext = true;
        ths = mbr+k+1;
        iNSamp = iNumSampPV = n = 0;
        w->label(0, mbr[k].label, enum2str(scfg->_phone_level, scfg->unit_levels));
        //buff = -32766;
        //w->sample(&buff, 1);
        for (sbuff = segm/2, i = lpcu[k].length, boundary = lpcu[k].frames; i < lpcu[k].frames+lpcu[k+1].length; )
        {
            if ( i >= lpcu[k].frames && bNext)
            {
                n = lpcu[k].frames;
                boundary = lpcu[k+1].length;
                lpc = (lpc_coeff_hdr*)(lpcu[k+1].coeffs);
                snd_head = (snd_hdr*)(lpcu[k+1].residual);
                data_size = reverse32(snd_head->data_size);
                samp_rate = reverse32(snd_head->sample_rate);
                snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));
                bNext = false;
                bRealoc = true;
            }
            if ( ths->perc != -1 )
            {
                if ( iNSamp > ths->perc*len/100.0 && ths->perc < 100 )
                {
                    *pct = ths->perc;                    
                    lst_f0 = f0 = ths->f0;
                    ths = ths->next;
                    pct[1] = ths->perc;
                    mb_f0 = ths->f0;
                    iNumSampPV = iNSamp;
                }
                else if ( mb_f0 != f0 )
                {
                    buffer = lst_f0+(mb_f0-lst_f0)*(iNSamp-iNumSampPV)/(len*(pct[1] - *pct)/100.0);
                    if ( abs(1.0*f0-buffer) > 1.0 )
                    {
                        D_PRINT(2, "LPCFEST::synth - Changing F0 from %d to %d\n", f0, (int)(buffer+.5));
                        f0 =  (int)(0.5 + buffer);
                        assert(f0 > 0, __FILE__, __LINE__);
                        bRealoc = true;
                    }
                }
            }

            if ( bRealoc ) //not to allocate the same segment twice
            {
                //FIXME - posunout na asymetricky stred take pitch pulse?
                segm = samp_rate*(2.0/f0);// length of segm depends on prosody (f0)
                if ( segm/2 - (((segm/2)>>1)<<1) )// we need even length
                {
                    segm = ((segm>>1)+1)<<1;
                }
                sbuff = segm/2;
                free(fSnd);
                fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
                memset(win, 0, sizeof(*win)*segm);
                hann(win, segm);

                //DO allocate segm/2 from left side of the pitch .....|.
                iPtch = (int)(.5 + samp_rate*lpc[(i-n)*(order+2)].p_mark);
                for ( j=0; j < sbuff; j++ )
                {
                    //out of begining of the sound data or crossing the scope to previous pitch pulse
                    if ( iPtch-j < 1 || ((i-n)>0 && iPtch-j <= samp_rate*lpc[((i-n)-1)*(order+2)].p_mark) )
                        fSnd[sbuff-j-1] = 0.0;
                    else 
                        fSnd[sbuff-j-1] = (float)ulaw2linear(snd_data[ iPtch-j-1 ]);
                }
                //DO allocate segm/2 from right side of the pitch .|.....
                for ( j=0; j < segm-sbuff; j++ )
                {
                    //crossing the scope to next pitch pulse or out of the sound data
                    if ( ( i-n-1 < boundary && iPtch+j >= samp_rate*lpc[(i-n+1)*(order+2)].p_mark) || (iPtch+j >= data_size) )
                        fSnd[sbuff+j] = 0.0;
                    else
                        fSnd[sbuff+j] = (float)ulaw2linear(snd_data[ iPtch+j ]);
                }

                bRealoc = false;
            }

            //DO synthetize the segment
            for ( l = order; l < segm+order; l++) 
            {
                //y[l] *= win[l-order];
                y[l] = win[l-order]*fSnd[l-order];
                for (m = 2; m < order+1; y[l] += 1.0*lpc[(i-n)*(order+2)].lpccoff[m]*y[l-m+1], m++);

                if (y[l] > 32767 || y[l] < -32767)
                    shriek(CA1A, "LPCFEST::synth - Out of short int scale. Unit = %d, segment = %d, sample = %d ", k, i, l);
                else
                    outp[l-order] += (short)y[l];
            }
            
            //buff = 32767;
            //w->sample(&buff, 1);
            //w->sample(outp, segm);
            //memset(outp, 0, sizeof(*outp)*(SEGM-1));

            w->sample(outp, sbuff);
            memmove(outp, outp+sbuff, sizeof(*outp)*(segm-sbuff));
            memset(outp+sbuff, 0, sizeof(*outp)*(SEGM-sbuff-1));                        
            iNSamp += sbuff;

            if ( repeat < 1 )   // release or repeat pitch?
            {   // releasing
                if ( nasob >= 1 )   //we reach the cummulative edge
                {
                    nasob -= (int)nasob;    //restoring
                    ++i;
                    bRealoc = true;
                }
                nasob += repeat;    //increase by the repeating const
            }
            else
            {   //repeating
                if ( nasob >= 1 )   //we reach the cummulative edge (1)
                {
                    nasob -= (int)nasob;    //save the remainder
                    ++i;
                }
                nasob += repeat-(int)repeat;    // the reminder of the repeating const
                i += (int)repeat;   //increase by the repeating const
                bRealoc = true;
            }
        }
        D_PRINT(2, "LPCFEST::Synth - Phoneme %s synthesised. Desired length %d, real length %d samples\n", mbr[k+1].label, len, iNSamp);
        free(fSnd);
	}

    delete mb;
    free(lpcu);
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
	char		*point, set_delim[] = {10, 0}, num_buff[10] = {0},
				chbuff[50] = {0};
	bool		yesno;
	//label_hdr	*units;

    vc = v;
	lpc_units = 0;
	models	= claim(v->models, v->location, scfg->inv_base_dir, "rb", "inventory", festoven);
	
	if ( (strncmp(models->data, "EST_File", 8) != 0) && (strncmp(&models->data[9], "index", 5) != 0) ) 
		shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL grouped file header expected %S", models->filename);

	point	= models->data + strcspn(models->data, set_delim)+1;
	coeffs	= strstr(point, "EST_File Track");
    if ( 1 != sscanf(strstr(coeffs, "NumChannels") + 12, "%d", &order) )
        shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory has not revealed the LPC order");

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
						  shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory - uncompatible version file type %S", models->filename);
				break;
			//DataFormat
			case 796: if (strcmp(chbuff, "DataType ascii") != NULL)
						  shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory - DataType is invalid", point);
				break;
			//IndexName
			case 889: if (strncmp(chbuff, "IndexName", 9) != 0) 
						  shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory item 'IndexName' expected", point);
				index_name = (char*)xcalloc(strlen(chbuff)-10, sizeof(char));
				strcpy(index_name, chbuff+10);
				///pridat polozku pro index name
				break;
			//DataFormat
			case 995: if (strcmp(chbuff, "DataFormat grouped") != NULL)	
						  shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory data format is invalid", point);
				break;
			//NumEntries
			case 1034: if ( (strncmp(chbuff, "NumEntries", 10) != 0) && (cfg->paranoid))
						   shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory item 'NumEntries' expected", atoi(point));
				nmbr_unit = atoi(&chbuff[11]);
				//def_units = (label_hdr*)xcalloc(nmbr_unit, sizeof(label_hdr));
				break;
			case 1290: if (strcmp(chbuff, "EST_Header_End") != 0)
						   shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory - header end expected (is %S)", point);
				lpc_units = point + 1;
				break; 
			//sig_file_format
			case 1578: if (strcmp(chbuff, "sig_file_format snd") != 0)	
						   shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory signal file format must be 'snd' (is %S)", point);
				break;
			//track_file_format
			case 1788: if (strcmp(chbuff, "track_file_format est_binary") != 0)	
						   shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory track file format must be binary (is %S)", point);
				break;
			default: shriek(CA1A, "LPCFEST::lpcfest - FESTIVAL inventory crossed unexpected header error (is %S)", point);
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

	diphon = (char*)xcalloc(30, sizeof(char));
	unit = diphon;
	strncpy(unit, strstr(coeffs, "NumChannels") + 12, 5);
	order = atoi(strtok(unit, delim));

	y = (float*)xcalloc(2048, sizeof(*y));
	outp = (short*)xcalloc(2048, sizeof(*outp));
	//win = (double*)xcalloc(2048, sizeof(*win));

	unit = b;
	for (num_phnm = 0; *unit != 0; num_phnm += *unit++ == 0x0A ? 1 : 0);

    // OLD MBROLA
	text = (mbr_format*)xcalloc(num_phnm+1, sizeof(mbr_format));
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
        float *fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
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
                fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
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

/* Another BEFORE Matlab one 2009_08_14
for (text = mbr, k = 0, iNSamp = 0; k < n; k++) {	
		
		lpc = (lpc_coeff_hdr*)(lpcu[k].coeffs);
		snd_head = (snd_hdr*)(lpcu[k].residual);
		data_size = reverse32(snd_head->data_size);
        samp_rate = reverse32(snd_head->sample_rate);
		snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));        
		
        segm = samp_rate*(2.0/mbr[k].f0);// length of segm depends on prosody (f0)
        if ( segm/2 - (((segm/2)>>1)<<1) )// we need even length
        {
            segm = ((segm>>1)+1)<<1;
        }

        //CompRepFact(text[k]&pv_unit, &lpcu[k], &lpcu[k+1], &repeat);
        //DO compute repetition coeff if needed

        iNumSampDF = (int)(0.5 + 0.001*mbr[k].length*samp_rate) - iNumSampPV;
        iNumSampNX = (int)(0.5 + 0.001*mbr[k+1].length*samp_rate)*(1.0*(lpcu[k].frames-lpcu[k].length)/(lpcu[k].frames-lpcu[k].length+lpcu[k+1].length));
        iNumSampPV = 0;

        if ( iNumSampDF < 0 )
            continue;

        bRes = true, bCross = false, bRealoc = true;
        float fRepeatDF = lpcu[k].length*segm/(2.0*iNumSampDF);
        float fRepeatNX = 1.0*(lpcu[k].frames - lpcu[k].length)*samp_rate/(mbr[k+1].f0*iNumSampNX);
        //nasob = fabs(fRepeatDF - (int)(fRepeatDF));
        nasob = fRepeatDF - (int)(fRepeatDF);
        repeat = fRepeatDF;
        frames = lpcu[k].frames << 1;

        short int *siIdx = (short int*)xcalloc(frames, sizeof(*siIdx));        
        float *fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
        memset(win, 0, sizeof(*win)*segm);
        hann(win, segm);

        i = skip ? lpcu[k].length : 0;
        p = 0;
        for (int seg=segm; i < lpcu[k].frames; ++p)
        {            
            if ( p == frames )
            {
                frames += lpcu[k].frames;
                siIdx = (short int*)xrealloc(siIdx, frames*sizeof(*siIdx));
            }

            siIdx[p] = i;

            if ( i >= lpcu[k].length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                un_lngt = p;  //save length of the unit
                nasob = fabs(fRepeatNX - (int)(fRepeatNX));
                repeat = fRepeatNX;
                seg = samp_rate*(2.0/mbr[k+1].f0);
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

            if ( i >= lpcu[k].length && iNumSampPV + seg/2 > iNumSampNX ) break; //END when you are almost at the desired length

            iNumSampPV += seg/2;    //we are synthetise this F0

            //FIXME - nasledujici podminka funguje uplne spatne
            if ( repeat < 1 )   // release or repeat pitch?
            {   // releasing
                if ( nasob >= 1 )   //we reach the cummulative edge
                {
                    nasob -= (int)nasob;    //restoring
                    ++i;
                }
                nasob += repeat;    //increase by the repeating const
            }
            else
            {   //repeating
                if ( nasob >= 1 )   //we reach the cummulative edge (1)
                {
                    nasob -= (int)nasob;    //save the remainder
                    ++i;
                }
                nasob += repeat-(int)repeat;    // the reminder of the repeating const
                i += (int)repeat;   //increase by the repeating const
            }
        }
        //DO check the F0 difference between following units
            //FIXME - co bude znit lepe - menit F0 na konci prvni, nebo zacatku druhe casti difonu?
        buffer = abs(mbr[k].f0 - mbr[k+1].f0);
        int iF0idx = .5 + un_lngt*(buffer>5 ? 0.65 : 0.75);  //how much the F0 changes and where to begin?

        //DO synthetize all frames from unit
        //text = mbr[k].next;
        text = NULL;
        iAktF0 = mbr[k].f0;

        for (sbuff = segm/2, bRes=true, i = 0; i < p; i++)
        {
            if ( siIdx[i] >= lpcu[k].length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                iNumSampPV = segm;
                segm = samp_rate*(2.0/mbr[k+1].f0);
                if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                {
                    segm = ((segm>>1)+1)<<1;
                }
                sbuff = segm/2;
                //memset(y, 0, (segm+order)*sizeof(*y));
                free(fSnd);
                fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
                if ( iNumSampPV != segm )//abs(iNumSampPV-segm) > 1000 )
                {                    
                    memset(win, 0, sizeof(*win)*segm);
                    //hannAs(win, iNumSampPV, segm);
                    //sbuff = iNumSampPV/2;
                    hann(win, segm);
                    bCross = true;
                }
                iNumSampPV = 0;
                iNSamp = 0;
                iPrevPerc = 0;
                //text = mbr[k+1].next;
                text = NULL;
                iAktF0 = mbr[k+1].f0;
                bRes = false;
                bRealoc = true;
            }

            //if ( text && text->perc < 100.0*iNSamp/(0.001*text->length*samp_rate) )
            if ( text )
            {
                if ( text->next && text->perc < 100.0*iNSamp/(0.001*text->length*samp_rate) )
                {
                    text = text->next;
                    iPrevPerc = iNSamp;
                }
                if ( text->perc > 100.0*iNSamp/(0.001*text->length*samp_rate) && 1.0 < 1.0*abs(text->f0-iAktF0)*(iNSamp-iPrevPerc)/(text->perc*0.00001*text->length*samp_rate) )
                {
                    iAktF0 += 1.0*(text->f0-iAktF0)*(iNSamp-iPrevPerc)/(text->perc*0.00001*text->length*samp_rate);
                    segm = samp_rate*(2.0/iAktF0);
                    if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                    {
                        segm = ((segm>>1)+1)<<1;
                    }
                    sbuff = segm/2;
                    //memset(y, 0, (segm+order)*sizeof(*y));
                    free(fSnd);
                    fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
                    if ( iNumSampPV != segm )//abs(iNumSampPV-segm) > 1000 )
                    {                    
                        memset(win, 0, sizeof(*win)*segm);
                        hann(win, segm);                    
                        //bCross = true;
                    }
                    bRealoc = true; //FIXME - pro zkracovani neni potreba prealokovavat
                }            
            }
            if ( siIdx[i] != siIdx[i-1] || bRealoc) //not to allocate the same segment twice
            {
                //FIXME - posunout na asymetricky stred take pitch pulse?

                //DO allocate segm/2 from left side of the pitch .....|.
                iPtch = (int)(.5 + samp_rate*lpc[siIdx[i]*(order+2)].p_mark);
                for ( j=0; j < sbuff; j++ )
                {
                    //out of begining of the sound data or crossing the scope to previous pitch pulse
                    if ( iPtch-j < 0 || (siIdx[i]>0 && iPtch-j < samp_rate*lpc[(siIdx[i]-1)*(order+2)].p_mark) )
                        //y[order+sbuff-j-1] = 0.0;
                        fSnd[sbuff-j-1] = 0.0;
                    else 
                        //y[order+sbuff-j-1] = (double)ulaw2linear(snd_data[ iPtch-j-1 ]);
                        fSnd[sbuff-j-1] = (float)ulaw2linear(snd_data[ iPtch-j-1 ]);
                }
                D_PRINT(2, "LPCFEST::Synth - Pitch point is at (%d) of segment length = %d\n", sbuff, segm);
                //DO allocate segm/2 from right side of the pitch .|.....
                for ( j=0; j < segm-sbuff; j++ )
                {
                    //crossing the scope to next pitch pulse or out of the sound data
                    if ( (siIdx[i]-1 < lpcu[k].frames && iPtch+j > samp_rate*lpc[(siIdx[i]+1)*(order+2)].p_mark) || (iPtch+j >= data_size) )
                        //y[order+sbuff+j] = 0.0;
                        fSnd[sbuff+j] = 0.0;
                    else
                        fSnd[sbuff+j] = (float)ulaw2linear(snd_data[ iPtch+j ]);
                }
                D_PRINT(2, "LPCFEST::Synth - Allocating crossing (%d) pitch length = %d\n", bCross, segm/2);

                bRealoc = false;
            }

            //DO synthetize the segment
            for (buff = 0, l = order; l < segm+order; l++) 
            {
                //y[l] *= win[l-order];
                y[l] = win[l-order]*fSnd[l-order];
                for (m = 2; m < order+1; y[l] += 1.0*lpc[siIdx[i]*(order+2)].lpccoff[m]*y[l-m+1], m++);

                if (y[l] > 32767 || y[l] < -32767)
                    shriek(CA1A, "LPCFEST::synth - Out of short int scale. Unit = %d, segment = %d, sample = %d ", k, i, l);
                else
                    outp[l-order] += (short)y[l];
            }
            
            //buff = 32767;
            //w->sample(&buff, 1);
            w->sample(outp, sbuff);
            memmove(outp, outp+sbuff, sizeof(*outp)*(segm-sbuff));
            memset(outp+sbuff, 0, sizeof(*outp)*(SEGM-sbuff-1));
            iNumSampPV += sbuff;
            iNSamp += sbuff;
            
            if ( bCross )//asymmetric window
            {
                memset(win, 0, sizeof(*win)*segm);
                hann(win, segm);
                bCross = false;
            }
        }
        free(fSnd);
        free(siIdx);
	}

    delete mb;
    free(lpcu);
	free(y);
	free(outp);
    free(win);
}
*/

/* NOT NICE MBROLA
void lpcfest::synssif(voice *v, char *b, wavefm *w)
{	
	int		i, j, k, l, m, p, n, segm, data_size, samp_rate, 
			num_phnm, buffer = 0, un_lngt, frames, skip = 0;
    int iNumSampPV = 0, iNumSampDF, iNumSampNX, iNSamp, iPtch, iAktF0, iPrevPerc;
    bool bRes, bCross, bRealoc;
	char	delim[] = {10,0}, phonem[7] = {0}, *unit, 
			path[60] = {0};
	float	*y, repeat, nasob;
	double	*win, dEn1, dEn2;
	int16_t	buff, sbuff;
	short	*outp;
	bool	rpt = true;
	snd_hdr	*snd_head;
	uchar	*snd_data;
	//FILE	*stream = fopen("d:\\usr\\temp\\lpcfest\\segmentyAdd", "wb");
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

    y = (float*)xcalloc(SEGM, sizeof(*y));
	outp = (short*)xcalloc(SEGM, sizeof(*outp));
    win = (double*)malloc(SEGM*sizeof(*win));

    if ( *text[0].label == '_' && text[0].length < 50 ) text[0].length = 80; //the initial pause must be at least 80ms
    
    label_hdr *lpcu;
    n = prepare_units(&lpcu, mbr, mb->get_count());
	
    //HLAVNI CYKLUS
    //segm = 16000*(2.0/text[0].f0);
	for (text = mbr, k = 0, iNSamp = 0; k < n; k++) {	
		
		lpc = (lpc_coeff_hdr*)(lpcu[k].coeffs);
		snd_head = (snd_hdr*)(lpcu[k].residual);
		data_size = reverse32(snd_head->data_size);
        samp_rate = reverse32(snd_head->sample_rate);
		snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));        
		
        segm = samp_rate*(2.0/mbr[k].f0);// length of segm depends on prosody (f0)
        if ( segm/2 - (((segm/2)>>1)<<1) )// we need even length
        {
            segm = ((segm>>1)+1)<<1;
        }

        //CompRepFact(text[k]&pv_unit, &lpcu[k], &lpcu[k+1], &repeat);
        //DO compute repetition coeff if needed

        iNumSampDF = (int)(0.5 + 0.001*mbr[k].length*samp_rate) - iNumSampPV;
        iNumSampNX = (int)(0.5 + 0.001*mbr[k+1].length*samp_rate)*(1.0*(lpcu[k].frames-lpcu[k].length)/(lpcu[k].frames-lpcu[k].length+lpcu[k+1].length));
        iNumSampPV = 0;

        if ( iNumSampDF < 0 )
            continue;

        bRes = true, bCross = false, bRealoc = true;
        float fRepeatDF = lpcu[k].length*segm/(2.0*iNumSampDF);
        float fRepeatNX = 1.0*(lpcu[k].frames - lpcu[k].length)*samp_rate/(mbr[k+1].f0*iNumSampNX);
        nasob = fabs(fRepeatDF - (int)(fRepeatDF));
        repeat = fRepeatDF;
        frames = lpcu[k].frames << 1;

        short int *siIdx = (short int*)xcalloc(frames, sizeof(*siIdx));        
        //float *fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
        memset(win, 0, sizeof(*win)*segm);
        hann(win, segm);

        i = skip ? lpcu[k].length : 0;
        p = 0;
        for (int seg=segm; i < lpcu[k].frames; p++)
        {            
            if ( p == frames )
            {
                frames += lpcu[k].frames;
                siIdx = (short int*)xrealloc(siIdx, frames*sizeof(*siIdx));
            }

            siIdx[p] = i;

            if ( i >= lpcu[k].length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                un_lngt = p;  //save length of the unit
                nasob = fabs(fRepeatNX - (int)(fRepeatNX));
                repeat = fRepeatNX;
                seg = samp_rate*(2.0/mbr[k+1].f0);
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

            if ( i >= lpcu[k].length && iNumSampPV + seg/2 > iNumSampNX ) break; //END when you are almost at the desired length

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
        buffer = abs(mbr[k].f0 - mbr[k+1].f0);
        int iF0idx = .5 + un_lngt*(buffer>5 ? 0.65 : 0.75);  //how much the F0 changes and where to begin?

        //DO synthetize all frames from unit
        text = mbr[k].next;
        if ( text && k > 0 )
        {

            while ( text->next && text->perc < 100.0*iNSamp/(0.001*text->length*samp_rate) ) 
                text = text->next;
            iAktF0 = text->f0;
        }
        else iAktF0 = mbr[k].f0;

        for (sbuff = segm/2, bRes=true, i = 0; i < p; i++)
        {
            if ( siIdx[i] >= lpcu[k].length && bRes )//FIXME - nesedeji OKNA pro krajni
            {
                iNumSampPV = segm;
                segm = samp_rate*(2.0/mbr[k+1].f0);
                if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                {
                    segm = ((segm>>1)+1)<<1;
                }
                sbuff = segm/2;
                memset(y, 0, (segm+order)*sizeof(*y));
                //free(fSnd);
                //fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
                if ( iNumSampPV != segm )//abs(iNumSampPV-segm) > 1000 )
                {                    
                    memset(win, 0, sizeof(*win)*segm);
                    //hannAs(win, iNumSampPV, segm);
                    //sbuff = iNumSampPV/2;
                    hann(win, segm);                    
                    bCross = true;
                }
                iNumSampPV = 0;
                iNSamp = 0;
                iPrevPerc = 0;
                text = mbr[k+1].next;
                iAktF0 = mbr[k+1].f0;
                bRes = false;
            }

            //if ( text && text->perc < 100.0*iNSamp/(0.001*text->length*samp_rate) )
            if ( text )
            {
                if ( text->next && text->perc < 100.0*iNSamp/(0.001*text->length*samp_rate) )
                {
                    text = text->next;
                    iPrevPerc = iNSamp;
                }
                if ( text->perc > 100.0*iNSamp/(0.001*text->length*samp_rate) && 1.0 < 1.0*abs(text->f0-iAktF0)*(iNSamp-iPrevPerc)/(text->perc*0.00001*text->length*samp_rate) )
                {
                    iAktF0 += 1.0*(text->f0-iAktF0)*(iNSamp-iPrevPerc)/(text->perc*0.00001*text->length*samp_rate);
                    segm = samp_rate*(2.0/iAktF0);
                    if ( segm/2 - (((segm/2)>>1)<<1) ) //odd segments are badly moved
                    {
                        segm = ((segm>>1)+1)<<1;
                    }
                    sbuff = segm/2;
                    memset(y, 0, (segm+order)*sizeof(*y));
                    //free(fSnd);
                    //fSnd = (float*)xcalloc(segm, sizeof(*fSnd));
                    if ( iNumSampPV != segm )//abs(iNumSampPV-segm) > 1000 )
                    {                    
                        memset(win, 0, sizeof(*win)*segm);
                        hann(win, segm);                    
                        //bCross = true;
                    }
                    bRealoc = true; //FIXME - pro zkracovani neni potreba prealokovavat
                }            
            }
            if ( siIdx[i] != siIdx[i-1] || bRealoc) //not to allocate the same segment twice
            {
                //FIXME - posunout na asymetricky stred take pitch pulse?

                //DO allocate segm/2 from left side of the pitch .....|.
                iPtch = (int)(.5 + samp_rate*lpc[siIdx[i]*(order+2)].p_mark);
                for ( j=0; j < sbuff; j++ )
                {
                    //out of begining of the sound data or crossing the scope to previous pitch pulse
                    if ( iPtch-j < 0 || (siIdx[i]>0 && iPtch-j < samp_rate*lpc[(siIdx[i]-1)*(order+2)].p_mark) )
                        y[order+sbuff-j-1] = 0.0;
                    else 
                        y[order+sbuff-j-1] = (double)ulaw2linear(snd_data[ iPtch-j-1 ]);
                }
                D_PRINT(2, "LPCFEST::Synth - Pitch point is at (%d) of segment length = %d\n", sbuff, segm);
                //DO allocate segm/2 from right side of the pitch .|.....
                for ( j=0; j < segm-sbuff; j++ )
                {
                    //crossing the scope to next pitch pulse or out of the sound data
                    if ( (siIdx[i]-1 < lpcu[k].frames && iPtch+j > samp_rate*lpc[(siIdx[i]+1)*(order+2)].p_mark) || (iPtch+j >= data_size) )
                        y[order+sbuff+j] = 0.0;
                    else
                        y[order+sbuff+j] = (double)ulaw2linear(snd_data[ iPtch+j ]);
                }
                D_PRINT(2, "LPCFEST::Synth - Allocating crossing (%d) pitch length = %d\n", bCross, segm/2);

                bRealoc = false;
            }

            //DO energy correction if crossing F0 - STILL in progress
            if ( bCross )
            {
                dEn1 = dEn2 = 0;
                for ( l=0; l < segm; l++ )
                {
                    dEn1 += win[l]*y[order+l];
                    dEn2 += y[order+l]*0.5*(1.0 - cos(2.0*pii*l/(segm-1)));
                }
            }

            //DO synthetize the segment
            for (buff = 0, l = order; l < segm+order; l++) 
            {
                y[l] *= win[l-order];
                for (m = 2; m < order+1; y[l] += 1.0*lpc[siIdx[i]*(order+2)].lpccoff[m]*y[l-m+1], m++);

                if (y[l] > 32767 || y[l] < -32767)
                    shriek(CA1A, "LPCFEST::synth - Out of short int scale.");
                else
                    outp[l-order] += (short)y[l];
            }
            
            //buff = 32767;
            //w->sample(&buff, 1);
            w->sample(outp, sbuff);
            memmove(outp, outp+sbuff, sizeof(*outp)*(segm-sbuff));
            memset(outp+sbuff, 0, sizeof(*outp)*(SEGM-sbuff-1));
            iNumSampPV += sbuff;
            iNSamp += sbuff;
            
            if ( bCross )//asymmetric window
            {
                memset(win, 0, sizeof(*win)*segm);
                hann(win, segm);
                bCross = false;
            }
        }
        //free(fSnd);
        free(siIdx);
	}

    delete mb;
    free(lpcu);
	free(y);
	free(outp);
    free(win);
}
*/
