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
//#include "remez.h"
//#include "ccosyn.h" //define hamkoe
#include "endian_utils.h"

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
	shriek(418, "Use option -s for correct FESTIVAL synthesis");
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

int lpcfest::convert_tab(char *chr){
	int i;
	switch(*chr){
	case '?': return (int)('_');
	case 'x': 
		return (int)('c') + (int)('h');
		//case pro znele ch* zatim neni z duvodu EPOS
	case 't': 
		if (strncmp(chr, "t_S", 3) == 0) 
			return (int)('c') + (int)('~');
		if (strncmp(chr, "t_s", 3) == 0) 
			return (int)('c');
		else
			return (int)('t');
	case 'd': 
		if (strncmp(chr, "d_z", 3) == 0)
			return (int)('d')+(int)('z');
		if (strncmp(chr, "d_Z", 3) == 0)
			return (int)('d')+(int)('z')+(int)('~');
		else
			return (int)('d');
	case 'N':
		return (int)('n')+(int)('*');
	case 'J': 
		if (strncmp(chr, "J\\", 2) == 0)
			return (int)('d')+(int)('~');
		else
			return (int)('n')+(int)('~');
	case 'P':
		if (strncmp(chr, "P\\", 2) == 0)
			return (int)('r')+(int)('~');
	case 'Q':
		if (strncmp(chr, "Q\\", 2) == 0)
			return (int)('r')+(int)('~')+(int)('*');
	case 'h':
		if (strncmp(chr, "h\\", 2) == 0)
			return (int)('h');
	case 'S':
		return (int)('s')+(int)('~');
	case 'c':
		return (int)('t')+(int)('~'); //c je 
	case 'Z':
		return (int)('z')+(int)('~');
	case '_': 
		return (int)('#');
	default: for (i = 0; *chr != 0; i += (int)*chr++) {}
		return i;
		//shriek(418, "SAMPA's and FESTIVAL's notation do not match");
	}
}
/*
void lpcfest::change_mbrola(char *b, mbr_format *mbr, int n){
	int		i = 0, j = 0;
	char	*unit, num_buff[6] = {0};
	
	unit = b;
	for (i = 0, j = 0; *b != 0; j += *b == ')' ? 1 : 0, i += *b++ == 0x0A ? 1 : 0) {}
	if ( j != i )
		shriek(418, "Internal EPOS error - sampa notation failed");
	b = unit;

	for (i = 1; i < n; ){
		for (j = 0; *b != 0x20; num_buff[j++] = *b++) {}
		num_buff[j] = 0;
		mbr[i].label = convert_tab(num_buff);

		for (j = 0; *(++b) != '('; num_buff[j++] = *b) {}
		num_buff[j] = 0;
		mbr[i].length = atoi(num_buff);

		for (j = 0; *(++b) != ','; num_buff[j++] = *b) {}
		num_buff[j] = 0;
		mbr[i].perc = atoi(num_buff);

		for (j = 0; *(++b) != ')'; num_buff[j++] = *b) {}
		num_buff[j] = 0; b +=3;
		mbr[i++].f0 = atoi(num_buff);
	}
}
*/

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
	//	shriek(418, "Internal EPOS error - sampa notation failed");

	for (; *b != 0; mbr++){
		unit = b;
		for (i = 0; *unit != 0x0A; i += *unit++ == ')' ? 1 : 0) {}
		do {
			for (j = 0; *b != 0x20; num_buff[j++] = *b++) {}
			num_buff[j] = 0;
			mbr->label = convert_tab(num_buff);

			for (j = 0; (*(++b) != '(') && (*b != ' '); num_buff[j++] = *b) {}
			num_buff[j] = 0;
			mbr->length = (v->init_t/100.0)*atoi(num_buff); b++;

			if (i) {	// zde bude muset byt cyklus zavisly na poctu zavorek
				do {
					if (mbr->next)
						ths = mbr->next;
					else
						ths = mbr;
					for (j = 0; *(++b) != ','; num_buff[j++] = *b) {}
					num_buff[j] = 0;
					ths->perc = atoi(num_buff);

					for (j = 0; *(++b) != ')'; num_buff[j++] = *b) {}
					num_buff[j] = 0;
					ths->f0 = atoi(num_buff);
					b++;
					if (--i)
						ths->next = (mbr_format*)calloc(1, sizeof(mbr_format));
					else {
						b++; break;
					}
				}
				while (1);
			}
			else {
				mbr->perc = 99;
				mbr->f0 = v->init_f;
			}
		}
		while ( *b++ != 0x0A );
	}
}

void lpcfest::synssif(voice *v, char *b, wavefm *w){
	
	int		i, j, k, l, m, p, n, order, segm, data_size, samp_rate, encoding, 
			num_phnm, buffer = 0, un_lngt, frames;
	char	*delka = 0, delim[] = {10,0}, phonem[7] = {0}, *diphon, *unit, 
			path[60] = {0};
	float	*y, point, repeat, nasob, mbr_lngt, intrpl = 0, rpt_intr, time, fbuff, last, next;
	double	*win, *pWin;
	int16_t	buff;
	short	*outp, *pout;
	bool	rpt = true;
	snd_hdr	*snd_head;
	uchar	*snd_data, *ubuff;
	label_hdr		*df_units, *nx_units, *pv_units = 0;
	mbr_format		*text;
	lpc_coeff_hdr	*lpc;

	//FILE	*stream = fopen("d:\\usr\\temp\\lpcfest\\segmentyAdd", "wb");
	//fclose(stream); //priprava pro testovani w->segmAdd
	//stream = fopen("d:\\usr\\temp\\lpcfest\\segmenty", "wb");
	
	diphon = (char*)calloc(30, sizeof(char));
	unit = diphon;
	strncpy(unit, strstr(coeffs, "NumChannels") + 12, 5);

	if ( 47 != (order = atoi(strtok(unit, delim))) )
		shriek(418, "Inventory number of LPC coefficients is invalid ");
	y = (float*)calloc(2048, sizeof(*y));
	outp = (short*)calloc(2048, sizeof(*outp));
	win = (double*)calloc(2048, sizeof(*win));

	unit = b;
	for (num_phnm = 0; *unit != 0; num_phnm += *unit++ == 0x0A ? 1 : 0) {}

	text = (mbr_format*)calloc(num_phnm+1, sizeof(mbr_format));
	change_mbrola(b, &text[1], num_phnm, v);
	text[0] = text[1];
	text[0].label = (int)'#';
	text[0].length /= 3.0;
	text[1].length *= 2.0/3.0;
	for (df_units = &def_units[-1]; ((++df_units)->label[0] != text[0].label) || (df_units->label[1] != text[1].label);){}

	//HLAVNI CYKLUS
	for (k = 0; k < num_phnm-1; k++) {
		
		for (nx_units = &def_units[-1]; ((++nx_units)->label[0] != text[k+1].label) || (nx_units->label[1] != text[k+2].label);){}
		lpc = (lpc_coeff_hdr*)(df_units->coeffs);
		snd_head = (snd_hdr*)(df_units->residual);
		data_size = reverse32(snd_head->data_size);
			
		if ( 1 != (encoding = reverse32(snd_head->encoding)) )
			shriek(418, "Inventory invalid sample coding %d - only mu-law (.snd) is supported", encoding);
		if ( v->inv_sampling_rate != (samp_rate = reverse32(snd_head->sample_rate)) )
			shriek(418, "Inventory invalid sampling rate %d - only 44100Hz is supported", samp_rate);
		if ( v->channel != (reverse32(snd_head->channels)-1) )
			shriek(418, "Inventory invalid number of signal channels - only MONO is supported");
		snd_data = (uchar*)((char*)snd_head + sizeof(snd_hdr));

		// length of segm depends on prosody (f0)
		segm = samp_rate*(2.0/text[k].f0);
		time = segm/(2.0*samp_rate);

		if (segm + order > 2048)
			shriek(418, "Allocated memory was exceeded"); // y, outp, win are allocated to size 2048B

		n = 0;
		do {
			frames = 0;
			mbr_lngt = (text[k+n].length*samp_rate/1000.0)/(segm/2.0);
			if ( (pv_units) && (!n) ){
				un_lngt = df_units->length + pv_units->frames - pv_units->length;
				nasob = repeat = un_lngt/mbr_lngt;
				mbr_lngt *= df_units->length/(double)un_lngt;
				//mbr_lngt *= (df_units->length - mbr_lngt)/fabs(df_units->length - mbr_lngt);
			}
			else if (!n){
				un_lngt = df_units->length;
				//nasob = repeat = un_lngt/(df_units->length - mbr_lngt);
				nasob = repeat = un_lngt/mbr_lngt;
			}
			else {
				un_lngt = df_units->frames - df_units->length + nx_units->length;
				frames = df_units->length;
				nasob = repeat = un_lngt/mbr_lngt;
				mbr_lngt *= (df_units->frames - df_units->length)/(double)un_lngt;
			}
			
			for (i = 0, p = 0; (p < mbr_lngt) && (i+frames < df_units->frames); i++, p++){

				fbuff = lpc[i+frames].p_mark - lpc[i+frames-1].p_mark;
				if (i+frames > df_units->frames)
					shriek(418, "Number of frames was exceeded, segment %d", k);
				if ( (point = lpc[i+frames].p_mark - time) > 0) {
					if ( ((i != 0) && (time > 0.8*fbuff)) ||
						 (data_size < (lpc[i+frames].p_mark + time)*samp_rate) )
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
					ubuff = &snd_data[(int)(samp_rate*(lpc[i+frames].p_mark - 0.8*fbuff))];
					memset(y, 0, sizeof(*y)*(segm + order));
					if ( data_size < (lpc[i+frames].p_mark + time)*samp_rate ){
						buff = (int)(samp_rate*(2*lpc[i+frames].p_mark - data_size/(float)samp_rate));
						ubuff = &snd_data[buff];
						for( j = 0; j < data_size - buff; y[order+ (segm-(data_size-buff))/2 + j++] = (float)ulaw2linear(*ubuff++)) {}
					}
					else
						for( buff = (int)(samp_rate*(2*0.8*fbuff)), j = 0; j < buff; y[order + (segm-buff)/2 + j++] = (float)ulaw2linear(*ubuff++)) {}
				}
				else
					ubuff = &snd_data[(int)point];
				
				for (buff = 0, l = order; l < segm+order; l++) {

					if (!intrpl)
						y[l] = (float)ulaw2linear(*ubuff++);
					//fwrite(&buff, sizeof(buff), 1, stream);
					for (m = 2; m < order+1; y[l] += lpc[i+frames].lpccoff[m]*y[l-m+1], m++) {}

					if (y[l] > 32766)
						outp[l-order] = (short)32766;
					else if (y[l] < -32766)
						outp[l-order] = (short)-32766;
					else
						outp[l-order] = (short)y[l];
				}
				//fwrite(outp, sizeof(*outp), segm, stream);
				w->sampleAdd(outp, segm, samp_rate, segm/(2.0*samp_rate));

				if ( i == (int)repeat ){
					repeat += nasob;
					if ( nasob < 1 )
						i--;
					else
						i++;
				}
			}
		}
		while (!(n++));
		pv_units = df_units;
		df_units = nx_units;
	}
	for (i = 0; i < num_phnm+1; clear_text(&text[i++], false)) {}

	free(y);
	free(outp);
	free(win);
	free(text);
	//free(impls);
	//fclose(stream);
}

void festoven(char *p, int l){
	if (!scfg->_big_endian)
		return;
	else 
		shriek(418, "nevim co delat, prisel na radu OVEN");
}

lpcfest::lpcfest(voice *v)
{
	int			i = 0, j = 0;
	lpc_hdr		*hdr;
	char		*point, set_delim[] = {10, 0}, *buff, num_buff[10] = {0},
				chbuff[50] = {0};
	//label_hdr	*units;

	lpc_units = 0;
	models	= claim(v->models, v->location, scfg->inv_base_dir, "rb", "inventory", festoven);
	hdr	= (lpc_hdr *)models->data;
	
	hdr->hdr_name[8] = 0;
	hdr->EST_type[5] = 0;

	
	if ( (strcmp(hdr->hdr_name, "EST_File") != 0) && (strcmp(hdr->EST_type, "index") != 0) ) 
		shriek(418, "FESTIVAL grouped file header expected %S", models->filename);

	point	= models->data + sizeof(lpc_hdr)+1;
	coeffs	= strstr(point, "EST_File");

	do {
		//strtok(point, set_delim);
		for (i = 0; (*point != 0x0A) && (*point != 0); chbuff[i++] = *point++) {}
		chbuff[i] = 0;

		if (!lpc_units) {
			switch (*chbuff) {
			case 'D': 
				i = strcmp(chbuff, "DataType ascii");
				j = strcmp(chbuff, "DataFormat grouped");

				if ( (i) && (j))
					shriek(418, "FESTIVAL DataType or DataFormat is invalid", point);
				break;
			case 'N': 
				if ( (strcmp(chbuff, "NumEntries 2375") != 0) && (cfg->paranoid))
					shriek(418, "FESTIVAL invalid number of diphone units (is %d) expected 2375", atoi(point));
				nmbr_unit = atoi(&chbuff[11]);
				def_units = (label_hdr*)calloc(nmbr_unit, sizeof(label_hdr));
				break; ///
			case 'I': 
				if (strcmp(chbuff, "IndexName czech_ph") != 0)
					shriek(418, "EPOS can handle only czech diphone units (is %S)", point);
				break; ///
			case 'V': 
				if ( (strcmp(chbuff, "Version 2") != 0) && cfg->paranoid )
					shriek(418, "uncompatible version FESTIVAL file type %S", models->filename);;
				break;
			case 't': 
				if (strcmp(chbuff, "track_file_format est_binary") != 0)
					shriek(418, "FESTIVAL track file format must be binary (is %S)", point);
				break;
			case 's': 
				if (strcmp(chbuff, "sig_file_format snd") != 0)
					shriek(418, "FESTIVAL signal file format must be 'snd' (is %S)", point);
				break;
			case 'E': 
				if (strcmp(chbuff, "EST_Header_End") != 0)
					shriek(418, "Header end expected (is %S)", point);
				lpc_units = point + 1;
				break;
			default: shriek(418, "Unexpected header error (is %S)", point);
				break;
			}
		}
		point++;
	}
	while (!lpc_units);

	for (i = 0; i < nmbr_unit; i++) {
		//strtok(point, set_delim);
		for (j = 0; *point != ' '; ) {
			if (*point != '-')
				def_units[i].label[j] += (int16_t)(*point++);
			else {
				j++; point++;
			}
		}

		for (j = 0; *(++point) != 0x20; num_buff[j++] = *point) {}
		num_buff[j] = 0;

		if ( !(buff = strstr(coeffs + atoi(num_buff), "NumFrames ") + 10) )
			shriek(418, "Invalid inventory file - NumFrames not found.");
		
		for (j = 0; *buff != 0x0A; num_buff[j++] = *buff++) {}
		num_buff[j] = 0;

		if ( !(def_units[i].coeffs = (float*)(strstr(buff, "EST_Header_End") + 15)) )
			shriek(418, "Invalid inventory file - EST_Header_End not found.");

		def_units[i].frames = atoi(num_buff);

		for (j = 0; *(++point) != 0x20; num_buff[j++] = *point) {}
		num_buff[j] = 0;
		def_units[i].residual = coeffs + atoi(num_buff);

		for (j = 0; *(++point) != 0x0A; num_buff[j++] = *point) {}
		num_buff[j] = 0; point++;
		def_units[i].length = atoi(num_buff);
	}
}

lpcfest::~lpcfest()
{
	unclaim(models);
	free(def_units);
}

void hann(double *x, int n){
	for (int i=0; i < n; i++) {
		x[i] = 0.5*(1.0 - cos(2.0*pii*i/(n-1)));
	}
}
