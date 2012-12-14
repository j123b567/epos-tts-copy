/*
 * 	epos/src/tdpmbr.cc
 * 	(c) 2006 ZdenÏk Chaloupka, chaloupka@ure.cas.cz
 *
 *	tdpmbr version 0.1 (20.9.2002)
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

#include "epos.h"
#include "tdpmbr.h"
#include <math.h>
#include "endian_utils.h"
#include <malloc.h>

#define MAX_STRETCH	30	/* stretching beyond 30 samples per OLA frame must be done through repeating frames */
#define MAX_OLA_FRAME	4096	/* sanity check only */
#define HAMMING_PRECISION 15	/* hamming coefficient precision in bits */
#define LP_F0_STEP 8			/* step of F0 analysis for linear prediction */
#define LP_DECIM 10				/* F0 analysis decimation coeff. for linear prediction */
#define LP_F0_ORD 4				/* order of F0 contour LP analysis */
#define F0_FILT_ORD 9			/* F0 contour filter order */
#define LP_EXC_MUL 1.0			/* LP excitation multipicator */

#ifdef WIN32
#define snprintf _snprintf
#endif

void tdpoven(char *p, int l);

/* F0 contour filter coefficients */
const double a[9] = {1,-6.46921563821389,18.43727805607084,-30.21344177474595,31.11962012720199,
					-20.62061607537661,8.58111044795433,-2.04983423923570,0.21516477151414};

const double b[9] = {0.01477848982115,-0.08930749676388,0.25181616063735,-0.43840842338467,0.52230821454923,
					-0.43840842338467,0.25181616063735,-0.08930749676388,0.01477848982115};

/* lp f0 contour filter coefficients (mean of 144 sentences from speaker Machac) */
// const double lp[LP_F0_ORD] = {-1.23761, 0.60009, -0.32046, 0.10699};

// these coeffs are from the version where the f0 is constant inside a syllable
const double lp[LP_F0_ORD] = {-0.900693, 0.043125, -0.003700, 0.069916};
// FIXME! lp coefficients must be configurable

int hamkoe(int winlen, double *data)
{
	D_PRINT(0, "%d\n", winlen);
	int i;
	double fn;
	fn = 2 * pii / (winlen - 1);
	for (i=0; i < winlen; i++)
		data[i] = 0.54 - 0.46 * cos(fn * i);
	return 0;
}

int hannkoe(int winlen, double *data)
{
	D_PRINT(0, "%d\n", winlen);
	int i;
	double fn;
	fn = 2 * pii / (winlen - 1);
	for (i=0; i < winlen; i++)
		data[i] = 0.5*(1.0 - cos(fn * i));
	return 0;
}

int tdpmbr::explosives_detection(int begin, int code){
	for (int i=0; begin+i < diph_len[code]; i++){
		for(int j=0, energy = 0; j < ppulses[ diph_offs[code]+begin+i+1 ] - ppulses[ diph_offs[code]+begin+i ]; )
			energy += abs(tdp_buff[ ppulses[ diph_offs[code]+begin+i ] + j++]);
		if (energy > 30000)
			return i;
	}
	return 0;
}

/*
void tdpmbr::clear_text(mbr_format *mbr, bool flag){
	if (mbr->next)
		clear_text(mbr->next, true);
	else if (flag)
		free(mbr);
}

void tdpmbr::change_mbrola(char *b, mbr_format *mbr, voice *v){
	int		i = 0, j = 0;
	char	*unit, num_buff[6] = {0};
	mbr_format	*ths;

	for (; *b != 0; mbr++){
		while (*b == ';'){
			//for (; *b != 0x0A; *b++);
			b = strstr(b, "\n")+1;
			//b++;
		}
		unit = b;
		for (i = 0; *unit != 0x0A; i += *unit++ == ')' ? 1 : 0);
		do {
			sscanf(b, "%s", num_buff);
			b = strstr(b, " ");
			if ( strcmp(v->parent_lang->name, "czech") == 0 ) {
				encode_from_sampa(num_buff, (unsigned char*)mbr->label, v->sampa_alternate);
				if (mbr->label[0] == '_') mbr->label[0] = '#';
			}
			else
				strcpy(mbr->label, num_buff);

			mbr->length = (v->init_t/100.0)*atoi(++b); //b++;

			if (i) {	// zde bude muset byt cyklus zavisly na poctu zavorek
				do {
					ths = mbr->next ? mbr->next : mbr;
					b = strstr(b, "(")+1;
					b += sscanf(b, "%d", &ths->perc)+2;
					b += sscanf(b, "%d", &ths->f0);
					//b = strstr(b, --i ? " " : "\n");

					// kvuli zmene pri opicarne s mbrola

					if (--i)
						ths->next = (mbr_format*)calloc(1, sizeof(mbr_format));
					else
						b = strstr(b, "\n");
				}
				while (ths->next);
			}
			else {
				ths = mbr;
				mbr->perc = 99;
				mbr->f0 = v->init_f;
				b = strstr(b, "\n");
			}
			D_PRINT(2, "Mbrola unit <%s> - F0: %d, in %d\%\n", ths->label, ths->f0, ths->perc);
		}
		while ( *b++ != 0x0A );
	}
}*/

char tdpmbr::convert(char *txt, char *sampaT, char *iso){
	char tmp[10], *tline, *stopstr;

	tmp[0] = 0x09;
	memset(&tmp[1], 0, 10);
	strcpy(&tmp[1], txt);
	if (tmp[2] == '\\') 
		tmp[3] = '\\';
	memset(tmp + strlen(tmp), 0x0A, 1);
	if ( (tline = strstr(sampaT, tmp)) == NULL)
		return tmp[1];
	tline -= 4;
	strncpy(tmp, tline, 5);
	tline = strstr(iso, tmp) -5;
	strncpy(tmp, tline, 3);

	stopstr = &tmp[2];
	return (char)strtol( tmp, &stopstr, 16);
}

void tdpmbr::synssif(voice *v, char *b, wavefm *w){
	int			num_phnm, exc, pitch, npitch, ola, length, i, j, k, l, m, n, 
				slen, start, len[3]={0}, segs, str_len, end, buff, buf_len, origlen, newlen;
	double		*win, repeat, nasob, pricti;
	int16_t		*code, *outbuf;
	char		*unit, tmp[16] = {0}, tmp1[16] = {0}, last[2] = {0}, *tline, *cdeText;
	mbr_format	*cde_format, *rmbr, *ths, *mbr;
	bool		prenos = false, soubor;
	//file		*segment_table;
	FILE		*f;
	mbrtdp		mb(b, v, true, true);

	soubor = false;
	if ( soubor && !(f = fopen("d:\\out.txt", "w")) )
		soubor = false;

	ola = 0.005*v->inv_sampling_rate;	// overlapping factor is 5ms
	win = (double*)calloc(ola, sizeof(*win));			//freed
	hannkoe(ola, win);
	//hamkoe(ola, win);

	unit = b;

	rmbr = cde_format = mb.mbr;
	mbr = mb.mbr;
	//mb.cds_extract();
	
	cdeText = mb.triph;
	//code = extract(b, cdeText); mb.cds
	//change_mbrola(b, cde_format, v); mb.triph// convert standart MBROLA into mbr_format

	if ( soubor )
		fprintf(f, "%s\n",strchr(b, 0x0A)+1);

	//k = atoi(b+1);
	//

	char *mbr_text = tline = (char*)calloc(num_phnm+2, sizeof(*mbr_text));	//freed
	for ( segs = num_phnm, buf_len = cde_format[1].f0; --segs; ){
		mbr_text[num_phnm-segs-1] = *cde_format[num_phnm-segs].label;
		if ( buf_len > cde_format[num_phnm-segs].f0 )
			buf_len = cde_format[num_phnm-segs].f0;
	}

	buf_len = 2.0*v->inv_sampling_rate/buf_len+ola/2;
	outbuf = (int16_t*)calloc( buf_len, sizeof(*outbuf));
	D_PRINT(1, "Allocating memmory for %d samples\n", buf_len );
	if (!outbuf || *outbuf)
		shriek(418, "TDP-MBR - something goes wrong while allocating memory!");

	if ( !strcspn(b, "?0") ){
		memmove(mbr_text+1, mbr_text, strlen(mbr_text));
		*mbr_text = *b;
	}

	outbuf += ola/2;	// place for the first overlap
	for (exc = -1, end = 0, cde_format++, segs = 0; --k; segs++){ 

		for (i=code[segs], j=0; i--; j += num_phones[code[segs]-i-1]);
		//j += num_phones[code[segs]] > 2 ? 1 : 0;
		
		/*for ( i=0, j=0; i < 1856; i++){	//test zda-li je na v kazde jednotce nejaky fonem
			if (num_phones[i] == 0)
				i = i;
			j += num_phones[i];
		}*/
		strncpy(tmp, b, 3);
		b+=3;
		do{
			strncpy(tmp1, mbr_text++, 3);
			/*if ( (tmp[2] == '0' || tmp[2] == '#' || tmp1[1] == 0x27) && *tmp == '?' && tmp1[1] != tmp[1] ) {
				memmove(tmp1+1, tmp1, 2);
				mbr_text--;
			}*/
			if (*tmp == '?' && tmp1[0] == tmp[1] && strcmp(tmp, "?#?")){
				memmove(tmp1+1, tmp1, 2);
				mbr_text--;
			}
			for (i = 3; i--; ) {
				if ( !strcspn(tmp+2-i, "@^%#0?") )
					tmp1[2-i] =  tmp[2-i];
			}
		}
		while (strncmp(tmp, tmp1, 3) );

		D_PRINT(2, "Synthesizing unit <%s> code %d desired length <%d>\n", tmp1, code[segs], cde_format[mbr_text-tline-1].length);
		len[0] = len[2] && tmp[0] != '?' ? 3*len[2] : (strcspn(tmp, "%^@0?#·ÈÌÛ˙") ? 0.001*cde_format[mbr_text-tline-2].length*v->inv_sampling_rate : 0) ;
		len[1] = 0.001*cde_format[mbr_text-tline-1].length*v->inv_sampling_rate;
		len[2] = 0.001*cde_format[mbr_text-tline].length*v->inv_sampling_rate * ( strcspn(tmp+2, "@^%#0?") && tmp[2] == *b ? 1.0/4.0 : 0 );

		if ( (str_len = strcspn(tmp, "·ÈÌÛ˙")) != 3)
			switch(str_len){
			case 0:
				//len[0] = 0.00025*cde_format[mbr_text-tline-2].length*v->inv_sampling_rate;
				len[0] = 0.0001*cde_format[mbr_text-tline-2].length*v->inv_sampling_rate;
				break;
			case 1:
				len[0] = 0;
				len[1] *= strcspn(tmp+2, "0#") ? 0.8 : 0.25;
				break;
			case 2:
				//len[2] = 0.00025*cde_format[mbr_text-tline].length*v->inv_sampling_rate;
				len[2] = 0.0001*cde_format[mbr_text-tline].length*v->inv_sampling_rate;
				break;
		}
		if ( !strcspn(b+1, "AEO") ){
			len[2] = 0.001*cde_format[mbr_text-tline].length*v->inv_sampling_rate;
		}
		else if( !strcspn(b-5, "AEO") && *tmp != '?')
			len[0] = 0.001*cde_format[mbr_text-tline-2].length*v->inv_sampling_rate;
		if ( strcspn(tmp, "AEO") < 2)
			len[0] = 0;

		if (b[1] == tmp[2] && *b == '?' && str_len == 3) {
			len[2] = 0.001*cde_format[mbr_text-tline].length*v->inv_sampling_rate * 1.0/4.0;
			cde_format[mbr_text-tline].length *= 3.0/4.0;
		}
		slen = diph_len[code[segs]];		
		m = 3;
		tmp1[1] = 0;
		str_len = 1;

		do {
			switch (m) {	// the three cases for the three phones of the triphone
			case 3:
				if (!len[0])
					i = slen;
				else { 
					for ( l = 1, i = 0; 
					ppulses[ diph_offs[code[segs]]+ l++ ] - ppulses[ diph_offs[code[segs]] ] < loc_phones[j];);
					slen = l-2;
				}
				break;	
			case 2:
				j += num_phones[code[segs]] >= 2 ? 1 : 0;
				if ( !str_len ){
					slen += length;
					len[1] = 1.0*(slen-i)*v->inv_sampling_rate/(1.0*cde_format[mbr_text-tline +1-m].f0);
					break;
				}
				str_len = 1;
				if (phones[j-1] == '_' && !strcspn(phones+j, "ptùk") && num_phones[code[segs]]>1){
					for ( i = l = len[0] ? slen : 0; 
					ppulses[ diph_offs[code[segs]]+ l++] - ppulses[ diph_offs[code[segs]] ] < loc_phones[j];);
					slen = l-2;
					if ( strcspn(tmp+2, "%@^") ){
						for ( l -= 2; 
						ppulses[ diph_offs[code[segs]]+ l++] - ppulses[ diph_offs[code[segs]] ] < loc_phones[j+1];);
						l -= slen+2;
					}
					else
						l = diph_len[code[segs]]-slen+1;
					
					//len[1] -= 1.0*l*v->inv_sampling_rate/(1.0*cde_format[mbr_text-tline +1-m].f0); //A co kdyz je mensi???
					str_len = 0;
					length = l;
				}
				else if ( num_phones[code[segs]] ) {
					for ( i = l = len[0] ? slen : 0; 
					ppulses[ diph_offs[code[segs]]+ l++] - ppulses[ diph_offs[code[segs]] ] < loc_phones[j];); 					
					slen = l-2;
				}
				else 
					i = 0;
				if (!code[segs])
					slen = diph_len[code[segs]];
				if ( str_len && (!(slen-i) || (!len[0] && !len[2] && slen-i < 2)) ){
					for ( i = l = len[0] ? slen : 0; 
					ppulses[ diph_offs[code[segs]]+ l++] < ppulses[ diph_offs[code[segs]] + diph_len[code[segs]]];);
					slen = l-2;
				}
				break;
/*					j += num_phones[code[segs]] == 2 ? 1 : 0;
				//avpitch = 
				for ( i = l = len[0] ? slen : 0; 
					ppulses[ diph_offs[code[segs]]+ l++] - ppulses[ diph_offs[code[segs]] ] < loc_phones[j];); 
				slen = l-2;
				if (!code[segs])
					slen = diph_len[code[segs]];
				if (!(slen-i)){
					for ( i = l = len[0] ? slen : 0; //FIXME pro ·Ë%
					ppulses[ diph_offs[code[segs]]+ l++] < ppulses[ diph_offs[code[segs]] + diph_len[code[segs]]];);
					slen = l-2;
				}
				break;*/
			case 1:
				if ( strcspn(tmp+2, "?0^") ) 
					i = slen;
				else 
					i = diph_len[code[segs]];
				slen = diph_len[code[segs]];
				break;
			}
			if ( !(ths = &cde_format[mbr_text-tline +1-m]) )	
				shriek(878, "TDP-MBR: Phone has not been found!");
			start = i;
			
			// LABEL every phone
			if ( (strcspn(tmp+3-m, "@%^0?")) && (cfg->label_seg || cfg->label_phones) && tmp[3-m] != *last ){ //&& tmp[3-m] != *b
				*last = tmp[3-m];
				w->label(0, last, enum2str(scfg->_phone_level, scfg->unit_levels));
			}
			
			// Handle the explosives
			if ( phones[j-1] != '_' && (!(buff = strcspn(tmp+3-m, "ptùk")) || (tmp[0] == '0' && m == 2)) ) {
				str_len = explosives_detection(start, code[segs]);
				if ( !str_len && !buff ){	// AM I SURE that this WORKs?
					j += phones[j] == tmp[3-m] ? 1 : 0;
					if (phones[j-1] != tmp[3-m])
						shriek(418, "FIXME TDP-MBR pocitam s tim ze najde explozivu");
					
					for ( l = start; 
					ppulses[ diph_offs[code[segs]]+ l++] - ppulses[ diph_offs[code[segs]] ] < loc_phones[j-1];);	
					str_len = explosives_detection(start+l, code[segs])+l-1;
				}
				ths->perc = buff ? -1+100.0*(slen-start-str_len)/(slen-start) : 
				-1+100.0*(len[3-m] - 1.0*(slen-start-str_len)*v->inv_sampling_rate/ths->f0)/len[3-m];
				if (!ths->next && str_len){
					ths->next = (mbr_format*)calloc(1, sizeof(*ths));
					ths->next->f0 = ths->f0;
					ths->next->perc = 99-ths->perc;
				}
			}

			pricti = pitch = 0;
			do {
				npitch = 1.0*v->inv_sampling_rate/ths->f0 +.5;
				if ( npitch && ola/2 > npitch)
					shriek(418, "Overlapping area is bigger than segment");

				newlen = phones[j-1] == '_' && strcspn(phones+j, "ptùk") && str_len == 1 ? (ths->perc+1)*npitch*(slen-start)/100.0 +.5 : (ths->perc+1)*len[3-m]/100.0 +.5;
				origlen = (ths->perc+1)*npitch*(slen-start)/100.0 +.5;
				repeat = nasob = origlen>newlen ? 1.0*origlen/newlen : 1.0*newlen/origlen;
				//for(n = 0; (i < slen) && ( newlen-npitch/2 > npitch*n); n++){
				for(n = 0; (i < slen) && ( newlen-npitch/3 > npitch*n ); n++){	//nejpresnejsi delky jsou pri tomto nastaveni

					for (buff = l = 0; l < npitch+ola/2; l++){
						outbuf[l - ola/2] += (l < ola/2 || l > npitch ? win[buff++] : 1.0)*
						tdp_buff[ ppulses[diph_offs[code[segs]]+i] +l-(npitch+ola/2)/2 ];
						pitch++;
					}
					pitch -= 1+ola/2;

					w->sample(outbuf-ola/2, npitch);
					memmove(outbuf-ola/2, outbuf-ola/2+npitch, ola);	// copy overlapping SAMPLES
					memset(outbuf, 0, 2*buf_len-ola);

					//w->sampleAdd(outbuf, npitch+ola, v->inv_sampling_rate, 0.01);	// FIXME not correct overlapping function
				
					// REPEAT to gain correct lengths
					if ( origlen > newlen ){
						i += nasob + ( repeat-(int)repeat+nasob-(int)nasob >= 1 ? 1 : 0);
					}
					else{
						pricti += repeat-(int)repeat+nasob-(int)nasob >= 1 ? 0 : 1.0/(int)nasob;
						if ( pricti == 1 ){	i++; pricti = 0; }
					}
					repeat += nasob;
				}
				//start = i;// = ths->next ? (ths->perc+1)*(slen-start)/100.0 : i;
			}
			while ( (ths = ths->next) && ( m > 1 || !len[3-m]) );

			D_PRINT(1, "\tphone <%c>, num. segments <%d>, length <%d>\n", (int)(tmp[3-m]), n, int(1000*pitch/v->inv_sampling_rate));
			if ( soubor ) {
				if ( *mbr[exc].label && *mbr[exc].label == tmp[3-m]) mbr[exc].length += pitch;
				else{
					if (strchr("0?^%@", tmp[3-m]) ) continue;
					*mbr[++exc].label = tmp[3-m];
					mbr[exc].length += pitch;
				}
			}
			if (phones[j-1] == '_' && !strcspn(phones+j, "ptùk") && num_phones[code[segs]]>1)
				m++;
			//FIXME? ZAPAMATOVAT SI POCET USEKU SYNTETIZOVANYCH NA POSLEDNIM FONEMU TRIFONU
			//FIXME? PRO DALSI JEDNOTKU v pripade vice prozodickych bodu
		}
		while (--m);
	}

	if ( soubor ){
		for (i = 0; i < exc; i++)
			fprintf(f, "%s %d\n", mbr[i].label, int(1000.0*mbr[i].length/v->inv_sampling_rate));
	}

	/* w->sample(outbuf-ola/2, end);	14.06.07*/

	//free(rmbr);
	free(win);
	free(outbuf-ola/2);
	free(code);
	free(mbr);
	free(tline);
	if ( soubor ) fclose(f);
}

struct tdi_hdr {
	int32_t  magic;
	int32_t  samp_rate;
	int32_t  samp_size;
	int32_t  bufpos;
	int32_t  n_segs;
	int32_t  diph_offs;
	int32_t  diph_len;
	int32_t  phone_ons;
	int32_t  res1;
	int32_t  ppulses;
	int32_t  res2;
	int32_t  res3;
};

tdpmbr::tdpmbr(voice *v)
{
	tdi_hdr *hdr;

	difpos = 0;

	tdi = claim(v->models, v->location, scfg->inv_base_dir, "rb", "inventory", tdpoven);
	hdr = (tdi_hdr *)tdi->data;
	D_PRINT(0, "Got %d and config says %d\n", hdr->n_segs, v->n_segs);
	if (v->n_segs != hdr->n_segs) shriek(463, "inconsistent n_segs");
	if (sizeof(SAMPLE) != hdr->samp_size) shriek(463, "inconsistent samp_size");
	tdp_buff = (SAMPLE *)(hdr + 1);
	diph_offs = (int32_t *)((char *)tdp_buff + sizeof(SAMPLE) * hdr->bufpos);
	diph_len = diph_offs + hdr->diph_offs;
	ppulses = diph_len + hdr->diph_len;
	num_phones = ppulses + hdr->ppulses;
	loc_phones = num_phones + hdr->n_segs;
	phones = (char *)loc_phones + sizeof(*loc_phones)*hdr->phone_ons;

	// this is debugging only!
	D_PRINT(0, "Samples are %d bytes long.\n", sizeof(SAMPLE) * hdr->bufpos);
	max_frame = MAX_OLA_FRAME;
}

tdpmbr::~tdpmbr(void)
{
	unclaim(tdi);
}

inline int tdpmbr::average_pitch(int offs, int len)
{
	// const int npitch = 145;
	const int npitch = 580;
	int tmp;

	int total = 0;
	int i = 0;
	for (int j = 0; j <= len + 1; j++) {
		tmp = ppulses[offs + j] - ppulses[offs + j - 1];
		if (tmp < 2 * npitch) {
			total += tmp;
			i++;
		}
	}
	if (i <= 0) {
		if (cfg->paranoid) shriek(463,"pitch marks not found");
		return 160;
	}
	return total / i;
}

void tdpoven(char *p, int l){
	if (!scfg->_big_endian)
		return;
	/* Convert the header */
	tdi_hdr* hdr = (tdi_hdr*)p;
	hdr->samp_rate = from_le32s(hdr->samp_rate);
	hdr->samp_size = from_le32s(hdr->samp_size);
	hdr->bufpos = from_le32s(hdr->bufpos);
	hdr->n_segs = from_le32s(hdr->n_segs);
	hdr->diph_offs = from_le32s(hdr->diph_offs);
	hdr->diph_len = from_le32s(hdr->diph_len);
	hdr->phone_ons = from_le32s(hdr->phone_ons);
	hdr->res1 = from_le32s(hdr->res1);
	hdr->ppulses = from_le32s(hdr->ppulses);
	hdr->res2 = from_le32s(hdr->res2);
	hdr->res3 = from_le32s(hdr->res3);
	
	/* Convert the buffer */
	SAMPLE *bufS = (SAMPLE *)(hdr + 1);
	uint32_t *bufL = (uint32_t *)(bufS + hdr->bufpos);
	char *stop = p + l;
	
	for ( ; bufS < (SAMPLE *)bufL; bufS++)
		*bufS = from_le16s(*bufS);
	
	for ( ; bufL < (uint32_t*)stop; bufL++)
		*bufL = from_le32u(*bufL);
}

void tdpmbr::synseg(voice *v, segment d, wavefm *w){
	shriek(418, "TDPMBR needs flag MBROLA text input (-s)");
}

/*
int16_t *tdpmbr::extract(char *b, char *text){

	int16_t *code, i, n;
	n = atoi(b+1);
	code = (int16_t *)calloc(n, sizeof(code));
	for(i=0; i<n; ){
		b = strstr(b, ",")+1;
		code[i++] = atoi(b);
		strncat(text, strstr(b, " ")+1, 3);
	}


	return code;
}
*/