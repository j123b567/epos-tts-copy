// class MBRTDP for mbrtdp processing

#include "clambr.h"

mbrtdp::mbrtdp( char *b, voice *v, bool tr) : t(b),vce(v), num_phnm(1), num_segs(0), 
alloc(strlen(b)), trans(tr)
{
	if ( *t != ';' || strncmp(t+1, "V0.1", 4) ) 
    {
        st = b;
        num_phnm = 0;
        while( (char*)1 != (st = strstr(st, "\n")+1) ) ++num_phnm;
        st = b;
        cds = NULL;
        triph = NULL;
        lengths = NULL;

        classic = true; //shriek(461, "mbrtdp::mbrtdp: SHIT - using old version of the SSIF?"); 
    }
    else
        classic = false;
    if ( !classic )
    {
        if ( *(b = strstr(b, "\n")+1) != ';' ) shriek(461, "mbrtdp::mbrtdp: SHIT - no phoneme number information!"); 
        num_phnm = atoi(++b);
        st = strstr(b, "\n")+1;

#define MBR_SAFETY 5                
        cds = (int16_t *)calloc(num_phnm+MBR_SAFETY, sizeof(*cds));
        triph = (char *)calloc(3*(num_phnm+MBR_SAFETY), sizeof(*triph));
        lengths = (float *)calloc(3*(num_phnm+MBR_SAFETY), sizeof(*lengths));
    }

    mbr = (mbr_format*)calloc(num_phnm+MBR_SAFETY, sizeof(*mbr));

	write_mbr();
    if ( !classic ) timing();
}

mbrtdp::~mbrtdp()
{
	//delete *mbrs
	for (int i=num_phnm+MBR_SAFETY; i--; clear_mem(mbr+i, false)); // free the memory //FIXME - really working?
	free(mbr);
    if (cds) free(cds);
	if (triph) free(triph);
	if (lengths) free(lengths);
}


void mbrtdp::write_mbr(){
	int		i=0, j=0, k=0, m=1, n, len, akt = 0;
	//int16_t *o=offs;
	char	*unit, num_buff[6] = {0}, *b;
	mbr_format	*ths;
	maxlen = 0;

	for ( b = st; *b != 0; k++, m++){
		for ( ; *b == ';' || *b == 0x0A; b = strstr(b, "\n")+1 );
        if ( classic )
        {
            unit = strstr(b, "\n");
            for (i = 0; *unit != 0x09; i += *unit-- == ' ' ? 1 : 0);
            i /= 2;
        }
        else
            for (unit = b, i = 0; *unit != 0x0A; i += *unit++ == ')' ? 1 : 0);
		do {
			//b += sscanf(b, "%s", num_buff)+1;
            //decode_to_sampa(
			for (n=0; *b != ' ' && *b != 0x09; num_buff[n++] = *b++);
			num_buff[n] = 0;
			if (trans || strcmp(vce->parent_lang->name, "czech") == 0 ) {
                if ( classic )
                {
                    strcat(mbr[k].label, (const char*)decode_to_sampa((unsigned char)*num_buff, this_voice->sampa_alternate));
                }
				else 
                {
                    encode_from_sampa(num_buff, (unsigned char*)mbr[k].label, this_voice->sampa_alternate);
                    //if (mbr[k].label[0] == '_') mbr[k].label[0] = '#';
                }
			}
			else
				strcpy(mbr[k].label, num_buff);

			//mbr[k].length = (vce->init_t/100.0)*atoi(++b); //b++;
            len = atoi(++b);
            ths = mbr+k;
			if (i) {	// zde bude muset byt cyklus zavisly na poctu zavorek
				do {
                    ths = ths->next ? ths->next : ths;

                    ths->length = len;
                    b += strcspn(b, "( ")+1;
					sscanf(b, "%d", &ths->perc);
                    b += strcspn(b, "( ")+1;
					sscanf(b, "%d", &ths->f0);
                    akt = ths->f0;

					if (--i){
						ths->next = (mbr_format*)calloc(1, sizeof(mbr_format));
						strcpy(ths->next->label, mbr[k].label);
					}
					else
						b = strstr(b, "\n");
				}
				while ( ths->next );
			}
			else {
                mbr[k].length = len;
				mbr[k].perc = 0xFFFF;   //NAZNACIME, ze je NEZNELA
                mbr[k].f0 = akt ? akt : this_voice->init_f; //Pouzijeme hodnotu predchozi hlasky
				b = strstr(b, "\n");
			}
			D_PRINT(2, "Mbrola unit <%s> - F0: %d, in %d, next prosody point -> %s%\n", mbr[k].label, mbr[k].f0, mbr[k].perc, mbr[k].next ? "yes" : "no");
			if ( maxlen > ths->f0 || !maxlen )	maxlen = ths->f0;//FIXME maxlen needed?
		}
		while ( *b++ != 0x0A );

		while ( 0 == strncmp(b, ";CDS ", 5) ) {
			b += 5;
			memcpy(triph+(j*3), b, 3);
			cds[j++] = atoi(b += 3);
			//offs[j++] = m;	pocet hlasek mezi segmenty neni potreba
			//m = 0;
			b = strstr(b, "\n")+1;
		}
	}
    if ( j > 0 ) cds[j] = -1;
}

void mbrtdp::write_b(){
}

void mbrtdp::write_b(float *f){
	int i=0, j=0, k=0, bf1, bf2, beg;
	*st = 0;
	beg = st-t;
	for( ; i < num_phnm; i++){
		bf1 = f[j++]+.5;
		bf2 = f[j++]+.5;
		if ( bf1 == bf2 )
			k += sprintf(st+k, "%s %d (%d,%d)\n", mbr[i].label, mbr[i].length, 99, bf1);
		else
			k += sprintf(st+k, "%s %d (%d,%d) (%d,%d)\n", mbr[i].label, mbr[i].length, 60, bf1, 40, bf2);

		if ( beg+k > alloc - 64){
			t = (char*)realloc(t, alloc += 1024);
			st = t+beg;
		}
	}
}

void mbrtdp::clear_mem(mbr_format *m, bool flag){
	if (m->next)
		clear_mem(m->next, true);
	if (flag)
		free(m);
}

/*
void mbrtdp::cds_extract(){
	int16_t i;
	char	*b=t;

	cds = (int16_t *)calloc(num_segs, sizeof(*cds));
	triph = (char *)calloc(3*num_segs+1, sizeof(*triph));
	for(i=0; i<num_segs; ){
		b = strstr(b, ",")+1;
		cds[i++] = atoi(b);
		strncat(triph, strstr(b, " ")+1, 3);
	}

}*/

void mbrtdp::timing(){
	int i=0, j;
	char *last = triph;
	float *buff = lengths;

	for ( ; *last; ) 
		*buff++ = !strcspn(last++, "@^%?0") ? 0 : 1;

	for ( last = triph+2, buff = lengths+2; *last; last +=3, buff += 3 ){
		if ( *last == last[1] ) {*buff *= .3; buff[1] *= .7; }
		if ( *last == last[2] ) {*buff *= .3; buff[2] *= .7; }
	}
	for (last = triph, buff = lengths; *last; last +=3, buff += 3 ){
		if (!strcspn(last+1, "áéíóúù")) {
			buff[-1] = .25;
			buff[1] = .5;
			if ( buff[3] ) buff[3] = .25;
			else buff[4] = .25;
			buff += 3;
			last += 3;
		}
	}
	/*
	for (;*last;){
		// zahrnout offs + pocet a umisteni aktivnich znaku
		*buff = buff[1] = buff[2] = 1;
		if ( *last == '?' ){ //TELO hlasky
			//if (!strcspn(last+1, "AEOáéíóúù")) {	//AEO maji vlastni delky hlasek, neni potreba dopocitavat
			if (!strcspn(last+1, "áéíóúù")) {
				buff[-1] = .25;
				if ( buff[-2] != 0.5 ) buff[1] = .5;
			}
			else if ( last[-1] == last[1] && !strcspn(last+2,"0?#'")){
				//if ( last[2] == '#' ) buff[2] = 0;
				buff[-1] = .3;
				buff[1] = .7;
				if ( last[2] == '\'' ) buff[2] = .3;
			}
			else if (last[-1] == '\'') buff[1] = 0.7;
		}
		else if ( buff[-2] == (float)0.5 )
			*buff = .25;
		else 
			*buff = (buff[-1] == (float)(0.3)) ? .7 : 1;

		for (j=0; j++ < 3; ){
			*buff++ *= strcspn(last++, "0%^?@") != 0 ? 1 : 0;
		}
		
		if (buff[-1] && last[-1] == *last)	buff[-1] = .3;
	}*/
}