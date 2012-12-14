/*
 * 	epos/src/tdpmbr.cc
 * 	(c) 2006 Zdenìk Chaloupka, chaloupka@ure.cas.cz
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
	int energy;
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
*/

void tdpmbr::syn(mbr_format *ph, float len, int16_t code, wavefm *w){
	int32_t		pitch, npitch, origlen, newlen, n, i, l, buff;
	double		repeat, nasob, pricti;
	mbr_format	*last = ph;
	bool		rep = false;

	FILE		*f;
	f = fopen("C:\\Users\\stanek\\Desktop\\epos-2.5.37.Lib.tdpmbr\\src\\synt01.txt", "w");
	if(f==NULL)
		shriek(418,"no file opened\n");

	printf("Synthesis begins, actual unit is %d\n",code);

	pricti = pitch = 0;
	i = start;
	do 
    {
		npitch = 1.0*srate/ph->f0 +.5;
		if ( npitch && ola/2 > npitch)
			shriek(418, "Overlapping area is bigger than segment");

		if ( length && ph->perc+1 < 100*(len+last_len) ){	//FIXME: v jake fazi hlasky jsme? (obecne 2 pruchody)
			newlen = (ph->perc+1)*(length-pitch)/100.0;
			origlen = (ph->perc+1)*npitch*(slen-i)/100.0 +.5;
			rep = true;
		}
		else if ( length ){
			newlen = length-pitch;	//zatim vse pouze pro tento pripad
			origlen = npitch*(slen-i);
		}
		else {
			newlen = origlen = npitch*(slen-i);
		}
		repeat = nasob = origlen>newlen ? 1.0*origlen/newlen : 1.0*newlen/origlen;
		//for(n = 0; (i < slen) && ( newlen-npitch/2 > npitch*n); n++){
		for(n = 0; (i < slen) && ( newlen-npitch/3 > npitch*n ); n++){	//nejpresnejsi delky jsou pri tomto nastaveni
			
			for (buff = l = 0; l < npitch+ola/2; l++){				
				buffer[l - ola/2] += (l < ola/2 || l > npitch ? win[buff++] : 1.0)*tdp_buff[ ppulses[diph_offs[code]+i] +l-(npitch+ola/2)/2 ];
				pitch++;
			}
			pitch -= 1+ola/2;

			printf("writing out %d samples\n",npitch);
			w->sample(buffer-ola/2, npitch);
			memmove(buffer-ola/2, buffer-ola/2+npitch, ola);	// copy overlapping SAMPLES					memset(buffer, 0, 2*buf_len-ola);
			memset(buffer, 0, 2*alloc-ola);
			fwrite(buffer-ola/2, sizeof(*buffer), npitch, f);			

			//w->sampleAdd(buffer, npitch+ola, srate, 0.01);	// FIXME not correct overlapping function
			
			// REPEAT to gain correct lengths
			if ( !length ) { i++; continue; }
			if ( origlen > newlen ){
				i += nasob + ( repeat-(int)repeat+nasob-(int)nasob >= 1 ? 1 : 0);
			}
			else{
				pricti += repeat-(int)repeat+nasob-(int)nasob >= 1 ? 0 : 1.0/(int)nasob;
				if ( pricti == 1 ){	i++; pricti = 0; }
			}
			repeat += nasob;
			//i++;
		}
		//start = i;// = ph->next ? (ph->perc+1)*(slen-start)/100.0 : i;
	}
	while (rep && (ph = ph->next) );
	//D_PRINT(1, "phone <%s>, segments <%d>, desired/synthesized length %d\\/%d\n, length", last->label, n, last->length, int(1000*pitch/srate));
	D_PRINT(1, "phone <%s>, segments <%d>, desired/synthesized length %d/%d\n", last->label, n, int(1000.0*length/srate), int(1000.0*pitch/srate));
	last_len = (len == (float)1.0 || len == (float)0.7) ? 0 : len;
}

void tdpmbr::synssif(voice *v, char *b, wavefm *w){
	int32_t		num_phnm, exc, i, j, l, m, n, len, segs, phns, end, buf_len;
	char		*unit, tmp[16] = {0}, tmp1[16] = {0}, last[2] = {0}, *cdeText;
	mbr_format	*mbr;
	bool		prenos = false, soubor;
	float		whole;
	//file		*segment_table;
	FILE		*f;

	soubor = true;
	if (!(f = fopen("C:\\Users\\stanek\\Desktop\\epos-2.5.37.Lib.tdpmbr\\src\\synt02.txt", "w")) )
		soubor = false;

	//hamkoe(ola, win);

	mb = new mbrtdp(b, v, true);//constructor b - MBROLA, v - voice info, true - perform SAMPA translation;
	unit = mb->st;
	mbr = mb->mbr;
	cdeText = mb->triph;
	num_phnm = mb->num_phnm;

	if ( soubor )
		fprintf(f, "%s\n",strchr(b, 0x0A)+1);

	alloc = 2.0*srate/mb->maxlen+ola/2;
	buffer = (int16_t*)calloc( alloc, sizeof(*buffer));
	D_PRINT(1, "Allocating memmory for %d samples\n", alloc );
	if (!buffer || *buffer)
		shriek(418, "TDP-MBR - something goes wrong while allocating memory!");

	//if ( !strcspn(mb->triph, "0?") ) mbr--;
	buffer += ola/2;	// place for the first overlap
	whole = 0;
	for (exc=-1, m=phns=end=segs=0; mb->cds[segs] != -1; segs++){ 


		for (i=mb->cds[segs], j=0; i--; j += num_phones[mb->cds[segs]-i-1]);


		D_PRINT(2, "Synthesizing code %d desired length <%d>\n", mb->cds[segs], mbr[m].length);

		printf("Synthesizing code %d desired length <%d>\n", mb->cds[segs], mbr[m].length);
		
		slen = start = len = 0;
		for (l = 0; l < 3; l++){
			//*length = 0.001*mbr[mbr_text-tline-1+l].length*mb->lengths[phns++]*srate + len;
			length = 0.001*mbr[m].length*mb->lengths[phns++]*srate + len +.5;
			//end = 
			switch(l) {
			case 0: 
				start = 0;
				if ( !length ) {
					if ( !num_phones[mb->cds[segs]] || !strcspn(mbr[m].label, "ptk'") ) { slen = 0; continue; }
					else if ( (phones[j] != '_' && num_phones[mb->cds[segs]] < 2) || 
						(!strcspn(mbr[m].label, phones+j) && mb->triph[3*segs] == '0') ) { slen = 0; continue; }	//osetrit toto pri synteze			
				}
				for ( n = 1, i = 0; 
					ppulses[ diph_offs[mb->cds[segs]]+ n++ ] - ppulses[ diph_offs[mb->cds[segs]] ] < loc_phones[j];);
				slen = n-2;
				break;

			case 1: 
				start = slen;
				if ( num_phones[mb->cds[segs]] ) {
					for ( n = start, i = 0; 
						ppulses[ diph_offs[mb->cds[segs]]+ n++ ] - ppulses[ diph_offs[mb->cds[segs]] ] < loc_phones[j];);
					slen = n-2;
				}
				if ( slen == start ){ 
					if (num_phones[mb->cds[segs]] < 2) { 
						len = length; /*whole = 1??*/ 
						//FIXME> pro koncovky ?a# atd. nejsou znacky konce hlasky - odhadnout ticho?
						whole += mb->lengths[phns-1];
						continue; 
					}
					else { j++; l--; phns--; continue; }
				}
				if ( phones[j-1] == '_' || (!strcspn(phones+j, "ptk") && !strcspn(mb->triph+3*segs+l-1, "?'0#%^")) ){	// pro hlasky obsahujici ticho
					if ( !strcspn(mbr[m].label, "ptk") ) {
						if ( num_phones[mb->cds[segs]] > 2 || !strcspn(mb->triph+3*segs+l-1, "?'0#%^") ) {
							len = length;
							j++;

							for ( n = slen, i = 0; 
								ppulses[ diph_offs[mb->cds[segs]]+ n++ ] - ppulses[ diph_offs[mb->cds[segs]] ] < loc_phones[j];);
							buf_len = n-2;
							length = 1.0*length*(slen - start)/(buf_len-start) +.5;
							// syn(mbr+m, mb->lengths+phns-1, mb->cds[segs], w);
							syn(mbr+m, 1.0*(slen - start)/(buf_len-start), mb->cds[segs], w);

							start = slen;
							slen = buf_len;
							length = len - length;
							syn(mbr+m, 1.0*length/len, mb->cds[segs], w);
							m++;
							whole = 0;
							len = 0;
							continue;
						}
						else { 
							len = 5*srate/mbr[m].f0;
							if ( length - len <= 0 ) 
								shriek(418, "Shit shit tdpmbr::synssif, exploziva (1)");
							length -= len;
							len = 1.0*(diph_len[mb->cds[segs]]-slen)*srate/mbr[m].f0;//FIXME: ?a# a podobne
						}
					}
					else { 
						length = 0;
						syn(mbr+m, mb->lengths[phns-1], mb->cds[segs], w);
						j++; phns--; l--; continue;
					}
					//whole
				}
				break;
			case 2: 
				start = slen;
				slen = diph_len[mb->cds[segs]]+1;
				if ( len ) { 
					//if ( num_phones[mb->cds[segs]] > 1 && phones[j-1] == '_' )  m--; 
					if ( len != length ) {
						whole = 1-mb->lengths[phns-1];
					}
					else if ( whole != 1.0 && *mbr[m].label == '#' && mb->triph[3*segs+1] == '#'){	//kvuli ?a# - koncovym jednotkam
						whole = 1.0;
					}
					len = 0; 
				} //jsem si jisty ze whole bylo 0...
				break;
			}
			syn(mbr+m, mb->lengths[phns-1], mb->cds[segs], w);
			if ( whole == 1 && mb->lengths[phns-1] ) 
				shriek(412, "Shit v tdp_mbr - whole == 1");
			whole +=  mb->lengths[phns-1];
			if ( whole == 1 ) { m++; whole = 0; }
			//m++= (l && *length) ? 1 : 0;
		}

		/*
		if ( soubor ) {
			if ( *mbr[exc].label && *mbr[exc].label == tmp[3-m]) mbr[exc].length += pitch;
			else{
				if (strchr("0?^%@", tmp[3-m]) ) continue;
				*mbr[++exc].label = tmp[3-m];
				mbr[exc].length += pitch;
			}
		}*/
		//FIXME? ZAPAMATOVAT SI POCET USEKU SYNTETIZOVANYCH NA POSLEDNIM FONEMU TRIFONU
		//FIXME? PRO DALSI JEDNOTKU v pripade vice prozodickych bodu

		
		//LABEL phone
		if (0 && length && (cfg->label_seg || cfg->label_phones) && mbr[m].label[0] != *last ){ //&& tmp[3-m] != *b
			*last = mbr[m].label[0];
			w->label(0, last, enum2str(scfg->_phone_level, scfg->unit_levels));
		}
	}

	if ( soubor ){
		for (i = 0; i < exc; i++)
			fprintf(f, "%s %d\n", mbr[i].label, int(1000.0*mbr[i].length/srate));
	}

	/* w->sample(outbuf-ola/2, end);	14.06.07*/

	//free(rmbr);
	delete mb;
	free(buffer-ola/2);
	//free(code);
	//free(mbr_text);
	//free(tline);
	if ( soubor ) fclose(f);
}

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
	
	//*chaloupka 
	srate = v->inv_sampling_rate;
	ola = 0.005*srate;	// overlapping factor is 5ms
	last_len = 0;
	win = (double*)calloc(ola, sizeof(*win));			//freed
	hannkoe(ola, win);
	// chaloupka*/
}

tdpmbr::~tdpmbr(void)
{
	unclaim(tdi);
	free(win);//chaloupka
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