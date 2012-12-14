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
//#include "tdpsyn.h"
#include <math.h>
#include "endian_utils.h"

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

void tdpmbr::clear_text(mbr_format *mbr, bool flag){
	if (mbr->next)
		clear_text(mbr->next, true);
	else if (flag)
		free(mbr);
}

void tdpmbr::change_mbrola(char *b, mbr_format *mbr, int n, voice *v){
	int		i = 0, j = 0;
	char	*unit, num_buff[6] = {0};
	mbr_format	*ths;

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
			strcpy(mbr->label, num_buff);

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
					if (mbr->next){
						ths->f0 -= 10;
						ths->perc = 40;
						mbr->perc = 60;
					}
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


int16_t *tdpmbr::tri_trans(mbr_format *txt, int numPhn, char *b){
	char tmp[5] = {0}, *tline, strnum[7] = {0}, *new_line, *rem, *rmbr,
		vowel[] ={'a','e','i','o','u','u','y','ì','ä','ö','ü','á','é','í','ó','ú','ù','ý','ì','O','A','E',0},
		samohl[] = {'b','d','ï','g','j','l','r',0},
		frikat[] = {'p','t','','k',0},
		sonant_v[] = {'j','l','r','m','n','ò','N','v',0};
	int16_t	k = 1, *code, i, rule = 1;
	file *segment_table, *sampaST_table, *iso88592;
	/*
	$voiced    = bdïgvzžZŽhø		$voiceless = ptkfsšcèxØ	$nasal     = mnòN	
	$sonant    = jlr$nasal			$SONANT    = JLR$NASAL		$short     = aeiouuyìäöü
	$long      = áéíóúùýìäöü		$diphthong  = OAE			$vowel     = $short$long$diphthong
	$VOWEL     = AEIOUUYÁÉÍÓÚÙÝÌ								$consonant = $voiced$voiceless$sonant

	(01) progress  0>^(ptk_!$vowel)	  colon			; neznìlá n.s. po p,t,,k
	(02) regress   ^>@(ptk_v$sonant)     colon			; znìlá n.s. pøed v a jedineènými souhl.
	(03) progress  0>@(bdïgjlr_!#$vowel)   colon		; znìlá n.s. po b,d,ï,g,j,l,r
	(04) progress  0>%(cèfsšxZŽvzžhNøØ_!$vowel)  colon  ; nepøítomná n.s. po cèfsšxZŽvzžhNøØ
	(05) progress  0>@(mnò_!$vowel)       colon			; znìlá n.s. po m,n,ò
	(06) regress   @>%(m_pbfv)            colon			;	nepøítomná n.s. po m a pøed p,b,f,v
	(07) regress   @>%(n_tdcZsz)          colon			; nepøítomná n.s. po n a pøed tdcZsz
	(08) regress   @>%(ò_ïèŽšž)          colon			; nepøítomná n.s. po ò a pøed ïèŽšž
	(09) regress   0>a(!_A)							; ošetøení diftongù
	(10) regress   0>o(!_O)							; ošetøení diftongù
	(11) regress   0>e(!_E)							; ošetøení diftongù
	(12) regress   0>u(AOE_$consonant)    colon		; ošetøení diftongù
	*/

	iso88592 = claim("8859-2.txt", "mappings", "", "rt", "segment_table", NULL);
	sampaST_table = claim("sampa-std.txt", "mappings", "", "rt", "segment_table", NULL);
	segment_table = claim("betty.dph", "czech", scfg->lang_base_dir, "rt", "segment_table", NULL);

	rmbr = tline = (char*)calloc(2*numPhn, sizeof(*tline));	//freed
	// convert all phones (SAMPA <=> ISO88592)
	for ( k = 0; k < numPhn; *txt[k].label = tline[k] = convert(txt[k].label, sampaST_table->data, iso88592->data), 
		 txt[k].label[1]=0, k++);
	rem = tline;
	do {
		switch(rule){
		// (01) progress  0>^(ptk_!$vowel)	  colon			; neznìlá n.s. po p,t,,k
		case 1:	
			if ( (rem = strpbrk(rem, frikat)) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( strchr(vowel, (int)rem[1]) == NULL ) {
					memmove(rem+2, rem+1, strlen(rem)-1);
					rem[1] = 0x5e;
				}
			break;
		// (02) regress   ^>@(ptk_v$sonant)     colon		; znìlá n.s. pøed v a jedineènými souhl.
		case 2:
			if ( (rem = strpbrk(rem, "^")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( (strcspn(&rem[-1], frikat) == 0) && (strcspn(&rem[1], sonant_v) == 0) )
					*rem = 0x40;
			break;
		// (03) progress  0>@(bdïgjlr_!#$vowel)   colon		; znìlá n.s. po b,d,ï,g,j,l,r
		case 3:
			if ( (rem = strpbrk(rem, samohl)) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( (strchr(vowel, (int)rem[1]) == NULL) && (rem[1] != '#') ){
					memmove(rem+2, rem+1, strlen(rem)-1);
					rem[1] = 0x40;
				}
			break;
		// (04) progress  0>%(cèfsšxZŽvzžhNøØ_!$vowel)  colon  ; nepøítomná n.s. po cèfsšxZŽvzžhNøØ
		case 4:
			if ( (rem = strpbrk(rem, "cèfsšxZŽvzžhNøØ")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( strchr(vowel, (int)rem[1]) == NULL) {
					memmove(rem+2, rem+1, strlen(rem)-1);
					rem[1] = 0x25;
				}
			break;
		// (05) progress  0>@(mnò_!$vowel)       colon			; znìlá n.s. po m,n,ò
		case 5:
			if ( (rem = strpbrk(rem, "mnò")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( strchr(vowel, (int)rem[1]) == NULL) {
					memmove(rem+2, rem+1, strlen(rem)-1);
					rem[1] = 0x40;
				}
			break;
		// (06) regress   @>%(m_pbfv)            colon			;	nepøítomná n.s. po m a pøed p,b,f,v
		case 6:
			if ( (rem = strpbrk(rem, "@")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( (rem[1] == 'm') && (strchr("pbfv", (int)rem[-1]) != NULL) )
					*rem = 0x25;
			break;
		// (07) regress   @>%(n_tdcZsz)          colon			; nepøítomná n.s. po n a pøed tdcZsz
		case 7:
			if ( (rem = strpbrk(rem, "@")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( (rem[1] == 'n') && (strchr("tdcZsz", (int)rem[-1]) != NULL) )
					*rem = 0x25;
			break;
		// (08) regress   @>%(ò_ïèŽšž)          colon			; nepøítomná n.s. po ò a pøed ïèŽšž
		case 8:
			if ( (rem = strpbrk(rem, "@")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( (rem[1] == 'ò') && (strcspn(&rem[-1], "ïèŽšž") == 0) )
					*rem = 0x25;
			break;
		// (09) regress   0>a(!_A)							; ošetøení diftongù
		case 9:
			if ( (rem = strpbrk(rem, "A")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else {
				memmove(rem+2, rem+1, strlen(rem)-1);
				rem[1] = 'a';
			}
			break;
		// (10) regress   0>o(!_O)							; ošetøení diftongù
		case 10:
			if ( (rem = strpbrk(rem, "O")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else {
				memmove(rem+1, rem, strlen(rem));
				*rem = 'o';
				rem++;
			}
			break;
		// (11) regress   0>e(!_E)							; ošetøení diftongù
		case 11:
			if ( (rem = strpbrk(rem, "E")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else {
				memmove(rem+2, rem+1, strlen(rem)-1);
				rem[1] = 'e';
			}
			break;
		// (12) regress   0>u(AOE_$consonant)    colon		; ošetøení diftongù, $voiced+voiceless    = bdïgvzžZŽhø+ptkfsšcèxØ
		case 12:
			if ( (rem = strpbrk(rem, "AOE")) == NULL ) {
				rule++;
				rem = tline-1;
			}
			else 
				if ( strcspn(&rem[1], "bdïgvzžZŽhøptkfsšcèxØjlrmnòN") == 0 ) {
					memmove(rem+2, rem+1, strlen(rem)-1);
					rem[1] = 'u';
				}
			break;
		// (13) regress   #>0(!_!#)    colon		; po posledni hlasce je "0", po # pred "'" je "?"
		case 13:
			if ( (rem = strpbrk(rem, "_")) == 0 ) {
				rule++;
				rem =tline-1;
			}
			else {
				if ( (strncmp(rem, "__",2) == 0) && (strcspn(rem-1, "@%^") == 0) )
					rem[-1] = '0';
				*rem = *rem == '_' ? '#' : *rem;
				if ( (strcspn(rem+1, "'") == 0) && rem[1]) {
					memmove(rem+1, rem, strlen(rem));
					rem[1] = '?';
				}
			}	
			break;
		}
		rem++;
	}
	while (rule < 14);
	
	memmove(rem+1, rem, strlen(rem));
	*rem = *rem == 0x27 ? '?' : '0';
	numPhn = strlen(tline);
	code = (int16_t*)calloc(numPhn, sizeof(int16_t));
	rem = tline-1;
	new_line = tline + numPhn;
	for (k = 0, i = 0; new_line - rem++ -2; k++) {
		strncpy(tmp, rem, 3);

		if ( strcspn(&tmp[1], "áéíóúù") == 0 ){ //resi dlouhe hlasky + jejich navazani
			if ( (tmp[2] == 0x27) && (*tmp != tmp[1]) ) {
				*rem = tmp[1];
				*tmp = tmp[2] = '?';
				rem--;
			}
			else
				if (*tmp == tmp[1])
					*tmp = '?';
				else
					*tmp = tmp[2] = '?';
		}
		if (tmp[1] == '#')
			*tmp = tmp[2] = '?';
		if ( (tmp[2] == 0x27) || (tmp[1] == 0x27) )	// osetreni pauzy mezy slovy
			*tmp = '?';
		if( (tline = strstr(segment_table->data, tmp)) != NULL ){
			strncpy(strnum, tline + 4, strchr(tline + 4, 0x0A) - tline-4);
			code[i++] = atoi(strnum);
			strcpy(b+strlen(b), tmp);
		}
		memset(strnum, 0, 7);
	}
	code[i] = 2222; // to mark the end

	if (segment_table) unclaim(segment_table);
	if (sampaST_table) unclaim(sampaST_table);
	if (iso88592) unclaim(iso88592);
	free(rmbr);
	return code;
}

void tdpmbr::synssif(voice *v, char *b, wavefm *w){
	int			num_phnm, exc, pitch, npitch, ola, length, i, j, k, l, m, n, 
				slen, avpitch, start, *len;
	char		*unit, one, two, three;
	int16_t		*code, *outbuf;
	double		*win, repeat, nasob;
	mbr_format	*text, *rmbr, *ths;
    char tmp[16] = {0}, *tline;
	file		*segment_table;

	outbuf = (int16_t*)calloc(max_frame, sizeof(*outbuf));	//freed
	win = (double*)calloc(max_frame, sizeof(*win));			//freed
	len = (int*)calloc(3, sizeof(*len));					//freed
	unit = b;

	for (num_phnm = 0; *unit != 0; num_phnm += *unit++ == 0x0A ? 1 : 0);
	rmbr = text = (mbr_format*)calloc(num_phnm, sizeof(*text));	//freed
	change_mbrola(b, &text[1], num_phnm, v); // convert standart MBROLA into mbr_format

	// TADY JE ZMENA - KODY TRIFONU JIZ MAM
	//memset(b, 0, strlen(b));
	code = tri_trans(&text[1], num_phnm, b);	// converts text into SAMPA, finds code numbers  //freed
	ola = 10*0.001*v->inv_sampling_rate;	// overlapping factor
	for (k = 0; code[k] != 2222; k++, b += 3){ //code 2222 is stoping number

		for (i=code[k], j=0; i--; j += num_phones[code[k]-i-1]); //index of the phones
		if (b[1] == '#') // EPOS to MBROLA notation
			b[1] = '_';
		for (; b[1] != *text->label; text++);
		
		strncpy(tmp, b, 3);
		// the lengths of the phones according MBROLA
		if ( ((i = strcspn(tmp, "áéíóúù")) != 3) && (strlen(tmp) == 3) ){ // long vowels exceptions
			if ( (*tmp == '?') && (tmp[2] == 0x27) )
				i--;
			switch (i){	
			case 0: 
				len[0] = 0.001*text->length*v->inv_sampling_rate/4.0;
				len[2] = b[2] == text[1].label[0] ? 0.0004*text[1].length*v->inv_sampling_rate : 0;
				len[1] = 0.001*text->length*v->inv_sampling_rate;
				break;
			case 1: 
				len[0] = len[2] = 0;
				len[1] = 0.001*text->length*v->inv_sampling_rate/2.0;

				break;
			case 2: 
				len[0] = len[2] ? 3.0*len[2]/2.0 : 0;
				len[2] = b[2] == text[1].label[0] ? 0.001*text[1].length*v->inv_sampling_rate/4.0 : 0;
				len[1] = 0.001*text->length*v->inv_sampling_rate;
				break;
			}
		}
		else{
			i = strcspn(tmp, "@^%");
			len[0] = len[2] ? len[2] : 0;
			if ((b[2] == text[1].label[0]) && (b[2] == b[3]))
				len[2] = 0.0005*text[1].length*v->inv_sampling_rate;
			else
				len[2] = (b[2] == text[1].label[0]) ? 0.001*text[1].length*v->inv_sampling_rate : 0;
			//len[2] = (b[2] == text[1].label[0] && b[3] == b[2]) ? 0.0005*text[1].length*v->inv_sampling_rate : 0.0001*text[1].length*v->inv_sampling_rate) : 0;
			len[1] = 0.001*text->length*v->inv_sampling_rate;
		}
		slen = diph_len[code[k]];
		npitch = 1.0*v->inv_sampling_rate/text->f0 + 0.5;
		//if ( npitch != (int)(1.0*v->inv_sampling_rate/text[-1].f0 + 0.5) )
		//	hamkoe(npitch+ola, win);
		m = 3;
		do {
			switch (m) {	// the three cases for three phones of the triphone
			case 3: 
				if (!len[0])
					i = slen;
				else { 
					for ( l = 1, i = 0; 
					ppulses[ diph_offs[code[k]]+ l++ ] - ppulses[ diph_offs[code[k]] ] < loc_phones[j];);
					slen = l-2;
				}
				break;	
			case 2: 
				j = num_phones[code[k]] == 2 ? j+1 : j;
				for ( i = l = len[0] ? slen : 0; 
					ppulses[ diph_offs[code[k]]+ l++] - ppulses[ diph_offs[code[k]] ] < loc_phones[j];);
				slen = l-2;
				if (!code[k])
					slen = diph_len[code[k]];
				break;
			case 1:
				if ( b[2] != '?' ) 
					i = slen;
				else 
					i = diph_len[code[k]];
				slen = diph_len[code[k]];
				break;
			}
			ths = &text[2-m];
			start = i;
			do {
				//repeat = nasob = 1.0*(npitch*(int)(ths->perc*(slen-start)/100.0 +.5))/length;
				//repeat = nasob = 1.0*(length - npitch*(int)(ths->perc*(slen-start)/100.0 +.5))/npitch;
				//repeat = nasob = (len[3-m]) ? npitch*ths->perc*(slen-i)/(100.0*length) : 0;	// repeating factor to hold speech rate still
				for(npitch = 1.0*v->inv_sampling_rate/ths->f0 +.5,
					length = ths->perc*len[3-m]/100 +.5,
					repeat = nasob = 1.0*length/(npitch*ths->perc*(slen-start)/100.0),
					n = 0, exc = i; (i < slen) && ( length > npitch*n); i++, n++) {
					for (l = 0; l < npitch+ola; l++)
						//outbuf[l] = win[l]*tdp_buff[ ppulses[ diph_offs[code[k]]+i ]+l-(npitch+ola)/2 ];
						outbuf[l] = tdp_buff[ ppulses[ diph_offs[code[k]]+i ]+l-(npitch+ola)/2 ];
					w->sampleAdd(outbuf, npitch+ola, v->inv_sampling_rate, 0.01);	// output with overlapping window
					
					if ( i == exc+(int)repeat-1 ){
						repeat += nasob;
						i += (repeat > 1) ? -1 : 1;
						//if ( nasob < 1 )
						//	i++;
						//else
						//	i--;
					}
				}
				i = ths->next ? ths->perc*(slen-start)/100.0 : 0;
				//len[3-m] = (m == 1) ? 
			}
			while ( ths = ths->next);
		}
		while (--m);
	}
	for (i = 0; i < num_phnm; clear_text(&rmbr[i++], false)); // free the memory 

	free(rmbr);
	free(win);
	free(outbuf);
	free(len);
	free(code);
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

