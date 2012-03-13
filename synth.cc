/*
 *	ss/src/synth.cc
 *	(c) 1994-98 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
 *	(c) 1997-98 Jirka Hanika, geo@ff.cuni.cz
 *	
 *	This file has not been fully integrated yet.
 */

#include "common.h"

//#ifdef HAVE_FCNTL_H
#include <fcntl.h>
//#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_AUDIO_H
#include <sys/audio.h>
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>		/* open, write, (ioctl,) ... */
#endif

#ifndef O_BINARY	/* open */
#define O_BINARY  0
#endif


#pragma hdrstop
#pragma warn -pia


#define BUFF_SIZE 1024	//every item is a quadruple of ints

void play_diphones(unit *root, dsynth *ds)
{
	static diphone d[BUFF_SIZE]; 
	int i=BUFF_SIZE;

	if (!cfg.play_diph && !cfg.show_diph) return;
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_diphs(d,k,BUFF_SIZE);
		for (int j=0;j<i;j++) {
			d[j].f=cfg.samp_hz*100 / (d[j].f*cfg.inv_f0);
			ds->syndif((diphone)(d[j]));
		}
	}
}

void show_diphones(unit *root, dsynth *ds)
{
	static diphone d[BUFF_SIZE]; 
	int i=BUFF_SIZE;
	
	if (!cfg.show_diph) return;
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_diphs(d,k,BUFF_SIZE);
		for (int j=0;j<i;j++) {
			if (cfg.diph_raw) fprintf(stddbg,  "%5d", d[j].code);
			fprintf(stddbg," %3s f=%d t=%d i=%d\n", d[j].code<441 ? ds->tdiph[d[j].code] : "?!", d[j].f, d[j].t, d[j].e);
		}
	}
}

#undef BUFF_SIZE

synteza::synteza(int l)
{
	char *filename;

	ipitch = 0;
	lastl = l;
	pocet_vzorku=0;
	iyold= 0 ;
	buff_idx=0;
	crc = 0;
	
	for(i=0;i<rad;i++)ifilt[i]=0;
	if (!cfg.play_diph) cfg.wav_file = NULL_FILE;

	filename = compose_pathname(cfg.wav_file, cfg.wav_dir);
	
#ifdef S_IRGRP
	wavout = open(filename, O_WRONLY | O_CREAT | O_TRUNC, MODE_MASK);
#else
	wavout = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
#endif
	if (wavout == -1) shriek("Failed to %s %s", strncmp(filename, "/dev/", 5)
			? "create output file" : "open audio device", filename);
	free(filename);
	buffer_size = 4096;
#ifdef SOUND_PCM_GETBLKSIZE
	int stereo = cfg.stereo + 1;
	ioctl (wavout, SOUND_PCM_WRITE_CHANNELS, &stereo);  // keep it mono (disabled)
	ioctl (wavout, SOUND_PCM_WRITE_BITS, &cfg.samp_bits);
	ioctl (wavout, SOUND_PCM_WRITE_RATE, &cfg.samp_hz);
	ioctl (wavout, SOUND_PCM_GETBLKSIZE, &buffer_size);
#else
   #ifdef SNDCTL_DSP_SPEED
	ioctl (wavout, SNDCTL_DSP_STEREO, &cfg.stereo);  // keep it mono (disabled)
	ioctl (wavout, SNDCTL_DSP_SPEED, &cfg.samp_hz);
	ioctl (wavout, SNDCTL_DSP_SAMPLESIZE, &cfg.samp_bits);
	ioctl (wavout, SNDCTL_DSP_GETBLKSIZE, &buffer_size);
   #else
	DEBUG(3,9,fprintf(stddbg, "Sound ioctl's absent\n");)
   #endif
#endif
	wave_buffer = (sample_t *)malloc(buffer_size*(cfg.stereo+1)*sizeof(sample_t));
	
	if (cfg.wav_header) {					//naplneni hlavicky wav
		strcpy(wavh.string1,"RIFF");
		strcpy(wavh.string2,"WAVEfmt ");
		strcpy(wavh.string3,"data");
		wavh.datform=1;
		wavh.numchan=1;
		wavh.sf1=cfg.samp_hz; wavh.sf2=0;
		wavh.avr1=2*cfg.samp_hz; wavh.avr2=0;
		wavh.wlenB=2; wavh.wlenb=16;
		wavh.xnone=0x010;
		wavh.dlen=0;
		wavh.flen=0+0x24;
		write(wavout, &wavh, 44);         //zapsani prazdne wav hlavicky na zacatek souboru
	}
}

synteza::~synteza(void)
{
	if (buff_idx) write(wavout, wave_buffer, sizeof(sample_t) * buff_idx);
	if (cfg.wav_header) {
		lseek(wavout, 0, SEEK_SET);            //navrat na zacatek
		wavh.dlen=2*pocet_vzorku;              //a doplneni udaju o delce
		wavh.flen=wavh.dlen+0x24;              //do wav hlavicky
		write(wavout, &wavh, 44);
	}
	if (cfg.show_crc && crc) fprintf(stddbg, "Sound output crc is %u\n", crc);
#ifdef SOUND_SYNCH
	ioctl (wavout, SOUND_SYNCH, 0);
#endif
	close(wavout);
}

/*
 *	FIX the following. If ipitch>0, mod.lsyn>0, and
 *	ipitch+mod.lsyn-lastl!=0, then ihilb is used before inited.
 *	
 *	If this can't ever happen, remove ihilb = 0 as well as the shriek.
 *	Else remove ihilb = 0 and fix the warning which results.
 *
 */

void synteza::synmod(model mod)
{
	int i,k,ihilb = 0;
	long gain,finp,iy,jrc[8],kz;

	if (cfg.paranoid && ipitch>0 && mod.lsyn>0 && ipitch+mod.lsyn-lastl)
		shriek("ihilb is undefined in synteza.synmod! Please contact the authors.\n");
	for(i=0;i<rad;i++) jrc[rad-i-1]=(long)mod.rc[i];
	gain=(long)mod.lsq;
	if(mod.lsyn!=0)
	{
		if(ipitch!=0 && ipitch+mod.lsyn-lastl>0) ipitch += mod.lsyn-lastl;
	} else 	ipitch = 0;
	if(lastl==0 && mod.lsyn!=0) for(i=0;i<rad;i++) ifilt[i]=0;

	for(k=0;k<mod.nsyn;k++)
	{	//buzeni
		if(mod.lsyn==0) finp = rand()%(2*UPRAV)-UPRAV;
		else {
			finp=0;
			if (ipitch==0) {
				ipitch=mod.lsyn; ihilb=0; finp=hilbk[ihilb++];
			} else if (ihilb<nhilb) finp=hilbk[ihilb++];
		}
		//kriz_clanek
		iy=finp*gain-jrc[0]*ifilt[0];
		for(i=1;i<rad;i++) {
			iy=iy-jrc[i]*ifilt[i];
			ifilt[i-1]=(((iy>>15)*jrc[i])>>15)+ifilt[i];
		}
		ifilt[rad-1]=(int)(iy>>15);
		//vyst_uprav
		iy=(iy>>15)*(long)mod.esyn+(DEEM*iyold>>4);
		iyold=iy>>11;
		kz=kyu+iy;
		kyu=-iy+(kz>>15)*HPF;

		if (mod.lsyn!=0) ipitch--;
		wave_buffer[buff_idx++] = kz>>11;
		if (cfg.show_crc) crc = crc * 733 + (kz >> 11);
				// (could check faster a line below and in the destructor)
		if (buff_idx==buffer_size) {
			write(wavout, wave_buffer, sizeof(sample_t) * buffer_size);
			buff_idx = 0;
		}
		pocet_vzorku++;
	}
	lastl=mod.lsyn;
}//synmod


dsynth::dsynth(SYNTH_TYPE type, const char *counts, const char *models,
			const char *codebook, const char *dpt,
			int f0, int i0, int t0, int Hz)
{
	unuse(i0);
	unuse(t0);
	unuse(Hz);

	FILE *soubor;
	int i,delmod;
	char *sdel, *smod, *skni, *sdpt;
	
	sdel = compose_pathname(counts, cfg.invent_dir);
	smod = compose_pathname(models, cfg.invent_dir);
	skni = compose_pathname(codebook, cfg.invent_dir);
	sdpt = compose_pathname(dpt, cfg.invent_dir);
	use_syn = type;

	nvyrov = 0;
	lold = f0;


	syn = new synteza(lold);

	soubor=fopen(sdpt, "rt", "diphone names");
	for (i=0;i<441;i++) fscanf(soubor,"%3s\n",tdiph[i]);
	fclose(soubor);
	soubor=fopen(sdel, "rb", "model counts");
	fread(ldelky,1,441,soubor);
	fclose(soubor);
	pzac[0]=0;
	for(i=1;i<441;i++)pzac[i]=pzac[i-1]+(int)ldelky[i-1];
	delmod=pzac[440]+ldelky[440];

	switch (use_syn) {
	case S_FLOAT:
		soubor=fopen(smod, "rb", "float synthesis");
		fread(fcmodely,40,delmod,soubor);
		fclose(soubor);
		break;
	case S_INT:
		soubor=fopen(smod, "rb", "integer synthesis");
		fread(cmodely,20,delmod,soubor);
		fclose(soubor);
		break;
	case S_VQ:
		vqmodel pmod;
		soubor=fopen(skni, "rb", "vector quant synthesis");
		fread(kodk,16,256,soubor);
		fread(ener,2,64,soubor);
		fclose(soubor);

		soubor=fopen(smod ,"rb", "vector quant synthesis");
		for(i=0; i<3392; i++) {
			fread(&pmod,6,1,soubor);
			if (pmod.incrl == 999) vqmodels[i].inf0 = 0;
			else vqmodels[i].inf0 = -pmod.incrl;
			vqmodels[i].adre = pmod.adren-1;
			vqmodels[i].adrrc = pmod.adrrc-1;
			vqmodels[i].znel = (pmod.incrl!=999);
		}
		fclose(soubor);

		/* Korekce casu a intensity se zapinaji timto cfg parametrem */

		if (cfg.ti_adj) {
				/* FIX this code (make tunable) */
			soubor=fopen("korekce.set","rb", "adjustments");   //nacteni souboru s korekcemi
			fread(kor_t,445,2,soubor);            //nejak moc nefungovalo
			fread(kor_i,445,2,soubor);            // tak jsem to zatim zrusil
			for(i=0;i<441;i++) {
			  	fkor_t[i]=kor_t[i]/100;       //korekce se pouzivaji pouze u Ptackova
				kor_i[i]=100-(15-kor_i[i]);   // nejnovejsiho inventare
			}
			fclose(soubor);
		}
	}
	free(sdpt); free(smod); free(sdel); free(skni);
}

void dsynth::syndif(diphone d)
{
	int i,imodel,lincr,incrl,numodel,zaklad,znely;
	model m;
	if (cfg.ti_adj) {
		int pomf;
		pomf=d.t;                         //korekce zatim nepouzivam
		pomf=kor_t[d.code]*pomf;
		d.t=pomf / 100;
		d.e=d.e*(10000-625*(15-kor_i[d.code]));  //Checkme
	}
	DEBUG(1,9,fprintf(stddbg, "dsynth processing another diphone\n"));
//	d.eproz=(d.eproz-100) / 9 + (cfg.ti_adj ? kor_i[d.hlaska]-15 : 0);
	lincr=0;
	numodel=(int)ldelky[d.code];
	if(!numodel) {
		DEBUG(4,9,fprintf(stddbg, "Unknown diphone %d, %3s\n", d.code, d.code<445 ? "in range" : "out of range"));
		return;
	}
	/* nacti_mem_popis(code,numodel) */
	
/*	
	if (use_syn==S_VQ) for(i=0;i<numodel;i++) {		//fixme, see below
		...
	}  // se menil vqdifon, divne
*/	
	
	for (imodel=0; imodel<numodel; imodel++) {
		// nacti_k_model(imodel)
		switch (use_syn) {
		case S_VQ:
/*			popisy[imodel].hlas=d.hlaska;
			popisy[imodel].znel=celypop[pzac[d.hlaska]+imodel].znel;
			popisy[imodel].inf0=celypop[pzac[d.hlaska]+imodel].inf0;
			popisy[imodel].adre=celypop[pzac[d.hlaska]+imodel].adre;
			popisy[imodel].adrrc=celypop[pzac[d.hlaska]+imodel].adrrc;
*/			
			incrl = vqmodels[pzac[d.code]+imodel].inf0;
			znely = vqmodels[pzac[d.code]+imodel].znel;
			d.e = (d.e-100) / 9; // + (cfg.ti_adj ? kor_i[d.code]-15 : 0);
			i = vqmodels[pzac[d.code]+imodel].adre+d.e;
			if(i>63) i=63;                    //uprava indexu
			if(i<0) i=0;                      //(tabulka energii ma jen 64 poli)
			DEBUG(1,9,fprintf(stddbg, "energeia %d\n", i);)
			m.esyn=ener[i];                   //vyber energii z tabulky
			for(i=0;i<rad;i++) m.rc[i]=kodk[vqmodels[pzac[d.code]+imodel].adrrc][i];
			break;
		case S_FLOAT:
			incrl=(int)(32768.0*fcmodely[pzac[d.code]].incrl);
			if(fcmodely[pzac[d.code]].incrl<-1e-30) incrl=999;
			m.esyn=(int)(32768.0*fcmodely[pzac[d.code]+imodel].ener);
			m.esyn=m.esyn*d.e/100;
			for(i=0;i<rad;i++)
				m.rc[i]=(int)(32768.0*fcmodely[pzac[d.code]+imodel].rc[i]);
			znely=(incrl!=999);
			if(incrl==999)incrl=0;
			break;
		case S_INT:
			incrl=(int)cmodely[pzac[d.code]].incrl;
			m.esyn=(int)cmodely[pzac[d.code]+imodel].ener;
			m.esyn=m.esyn*d.e/140;
			for(i=0;i<rad;i++)
				m.rc[i]=cmodely[pzac[d.code]+imodel].rc[i];
			znely=(incrl!=999);
			if(incrl==999)incrl=0;
			break;
		default:
			shriek("Unknown synth type");
			znely = incrl = 0;		// to fool the compiler
		}
		// synchr(imodel)
		if(!znely) lincr += incrl;
		if(nvyrov < d.t / 2) {
			if(!znely) m.lsyn=0, m.lsq=12288, m.nsyn=d.t-nvyrov;
			else {
				if(imodel==numodel-1) m.lsyn=d.f;
				else {
					zaklad=(d.f-lold) * 256 / numodel;
					       // Deliberately ^^^ changed associativity - geo
					m.lsyn=lold + zaklad*(imodel+1) / 256;
				}
				m.lsyn=m.lsyn+lincr;
				m.lsq=lroot;
				i = d.t / m.lsyn + 1;
				if(i==1)m.nsyn=m.lsyn; else
					if(abs(nvyrov-d.t+i*m.lsyn)<abs(nvyrov-d.t+(i-1)*m.lsyn))m.nsyn=i*m.lsyn;
					else m.nsyn=(i-1)*m.lsyn;
			}
			nvyrov=nvyrov+m.nsyn-d.t;
		} else {
			if(imodel!=numodel-1) m.nsyn=0, m.lsyn=0, m.lsq=12288;
			else if(!znely) m.lsyn=0, m.lsq=12288, m.nsyn=minsynt;
				else m.lsyn=d.f, m.lsq=lroot, m.nsyn=m.lsyn;
			nvyrov=nvyrov-d.t;
		}
		DEBUG(1,9,fprintf(stddbg, "Model %d\n", imodel+1);)
		if(m.nsyn>=minsynt) syn->synmod(m);  //proved syntezu modelu
	}
	lold=d.f;
}//hlask_synt

dsynth::~dsynth(void)
{
	delete syn;
}
