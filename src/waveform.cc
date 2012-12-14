/*
 *	epos/src/waveform.cc
 *	(c) 1998-02 geo@cuni.cz
 *	(c) 2000-02 horak@ure.cas.cz
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

#include "common.h"
#include "client.h"

#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
	#include <signal.h>
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

#ifdef HAVE_MMSYSTEM_H
	#include <mmsystem.h>
	#include <windows.h>
	#include <windowsx.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>		/* open, (ioctl,) ... */
#endif

int localsound = -1;

#ifndef SNDCTL_DSP_SYNC
	#ifndef SOUND_PCM_SYNC
		#ifndef FORGET_SOUND_IOCTLS
			#define FORGET_SOUND_IOCTLS
		#endif
	#endif
#endif


#ifndef	WAVE_FORMAT_PCM
	#define WAVE_FORMAT_PCM		0x0001
#endif

#ifndef IBM_FORMAT_MULAW
	#define IBM_FORMAT_MULAW	0x0101
#endif


static int downsample_factor(int working, int out)
{
	if (!out) return 1;
	if (working == out) return 1;
	if (working / 2 == out) return 2;
	if (working / 3 == out) return 3;
	if (working / 4 == out) return 4;
	shriek(462, fmt("Currently, you can only get %d Hz, %d Hz, %d Hz or %d Hz output signal with this voice",
		working, working / 2, working / 3, working / 4));
	return 1;
}


#define FOURCC_INIT(x) {(x[0]), (x[1]), (x[2]), (x[3])}

//#pragma hdrstop

// #define RIFF_HEADER_SIZE  8
#define WAVE_HEADER_SIZE  ((long)(sizeof(wave_header) - RIFF_HEADER_SIZE))


struct cue_point
{
	long name;
	long pos;
	char chunk[4];
	long chunkstart;
	long blkstart;
	long sample_offset;
};

cue_point cue_point_template = { 0, 0, FOURCC_INIT("data"), 0, 0, 0 };

#define ADTL_MAX_ITEM		64
#define ADTL_INITIAL_BUFF	256	/* must be at least ADTL_MAX_ITEM */

struct ltxt
{
	char txt[4];
	long len;
	long cp_name;
	long sample_count;
	char purpose[4];
	short country;
	short language;
	short dialect;
	short codepage;
};

struct labl
{
	char txt[4];
	long len;
	long cp_name;
};

#define USA	 1		// FIXME (etc.)
#define English  9
#define American 1
#define Boring_CodePage	437

ltxt ltxt_template = { FOURCC_INIT("ltxt"), 0, 0, 0, FOURCC_INIT("dphl"), USA, English, American, Boring_CodePage };
labl labl_template = { FOURCC_INIT("labl"), 0, 0 };
labl note_template = { FOURCC_INIT("note"), 0, 0 };

wavefm::wavefm(voice *v)
{
	samp_rate = v->samp_rate;
//	samp_size_bytes = sizeof(SAMPLE);
	channel = v->channel;
	int stereo = channel==CT_MONO ? 0 : 1;
	if (this_voice->samp_size <= 0 || (this_voice->samp_size >> 3) > (signed)sizeof(int))
		shriek(447, fmt("Invalid sample size %d", this_voice->samp_size));
	if (cfg->ulaw && !cfg->wav_hdr)
		this_voice->samp_size = 8, this_voice->out_rate = 8000;
//	cfg->samp_size = cfg->samp_size + 7 & ~7;	// Petr had this
	downsamp = downsample_factor(samp_rate, v->out_rate);
	translated = false;

	memcpy(hdr.string1, "RIFF", 4);
	memcpy(hdr.string2, "WAVEfmt ", 8);
	memcpy(hdr.string3,"data", 4);
	hdr.datform = WAVE_FORMAT_PCM;
	hdr.numchan = stereo ? 2 : 1;
	hdr.sf1 = samp_rate; //OLD VERSION hdr.sf2 = stereo ? hdr.sf1 : 0;
	hdr.avr1 = 2 * samp_rate; //OLD VERSION hdr.avr2 = stereo ? hdr.avr1 : 0;
	hdr.alignment = sizeof(SAMPLE); hdr.samplesize = sizeof(SAMPLE) << 3;
	hdr.fmt_length = 0x010;
//	written_bytes = 0;
	hdr.total_length = - RIFF_HEADER_SIZE;
//	hdr.total_length = hdr.written_bytes - RIFF_HEADER_SIZE;

//	write(fd, wavh, sizeof(wave_header));         //zapsani prazdne wav hlavicky na zacatek souboru

	fd = -1;
	written = 0;
//	buff_size = cfg->buffer_size;
//	buffer = (char *)xmalloc(buff_size);
	buff_size = 0;
	buffer = NULL;
	hdr.buffer_idx = 0;

	cuehdr.len = 4;
	cuehdr.n = adtlhdr.len = 0;
	memcpy(cuehdr.string1, "cue ", 4);
	memcpy(adtlhdr.string1, "LIST", 4);
	memcpy(adtlhdr.string2, "adtl", 4);

	current_cp = 0;
	cp_buff = NULL;
	adtl_buff = NULL;

	ophase = 0;
	ooffset = 0;

	//chaloupka
	s = NULL;
}

wavefm::~wavefm()
{
	if (fd != -1) detach();
	if (buffer) free(buffer);
	if (cp_buff) free(cp_buff);
	if (adtl_buff) free(adtl_buff);
}


#define DEFAULT_BUFF_SIZE	4096


#ifndef FORGET_SOUND_IOCTLS

#ifndef SNDCTL_DSP_SETFMT
#define SNDCTL_DSP_SETFMT	SOUND_PCM_WRITE_BITS
#endif

#ifndef SNDCTL_DSP_GETFMTS
#ifdef SOUND_PCM_GETFMTS
#define SNDCTL_DSP_GETFMTS	SOUND_PCM_GETFMTS
#endif
#endif

#ifndef SNDCTL_DSP_SPEED
#define SNDCTL_DSP_SPEED	SOUND_PCM_WRITE_RATE
#endif

#ifndef SNDCTL_DSP_CHANNELS
#define SNDCTL_DSP_CHANNELS	SOUND_PCM_WRITE_CHANNELS
#endif

#ifndef SNDCTL_DSP_GETBLKSIZE
#define SNDCTL_DSP_GETBLKSIZE	SOUND_PCM_GETBLKSIZE
#endif

#ifndef SNDCTL_DSP_SYNC
#define SNDCTL_DSP_SYNC		SOUND_PCM_SYNC
#endif

#ifndef SNDCTL_DSP_RESET
#define SNDCTL_DSP_RESET	SOUND_PCM_RESET
#endif



const static inline bool ioctlable(int fd)
{
	int tmp;
	return !ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &tmp);
}

static inline void set_samp_size(int fd, int samp_size_bits)
{
	if (!ioctl (fd, SNDCTL_DSP_SETFMT, &samp_size_bits))
		return;
   #ifdef SNDCTL_DSP_GETFMTS
	int mask = (unsigned int)-1;
	ioctl (fd, SNDCTL_DSP_GETFMTS, &mask);
	DEBUG(3,9,fprintf(STDDBG,"Hardware format mask is 0x%04x\n", mask);)
	if (!(samp_size_bits & mask)) shriek(439, "Sampling rate not supported");
   #endif
}

static inline void set_samp_rate(int fd, int samp_rate)
{
	ioctl(fd, SNDCTL_DSP_SPEED, &samp_rate);
}


static inline void set_channels(int fd, int channels)
{
	ioctl(fd, SNDCTL_DSP_CHANNELS, &channels);
}


#ifndef SNDCTL_DSP_NONBLOCK
	#ifdef	SOUND_PCM_NONBLOCK
		#define SNDCTL_DSP_NONBLOCK	SOUND_PCM_NONBLOCK
	#endif
#endif

static inline void set_nonblocking(int fd)
{
   #ifdef SNDCTL_DSP_NONBLOCK
	ioctl(fd, SNDCTL_DSP_NONBLOCK);
   #endif
}

static inline int get_blksize(int fd)
{
	int buff_size = 0;
	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &buff_size))
		buff_size = DEFAULT_BUFF_SIZE;
	return buff_size ? buff_size : DEFAULT_BUFF_SIZE;
}

static inline void sync_soundcard(int fd)
{
	if (ioctlable(fd)) ioctl (fd, SNDCTL_DSP_SYNC);
}

static inline void reset_soundcard(int fd)
{
	if (ioctlable(fd)) ioctl (fd, SNDCTL_DSP_RESET);
}


#else		// not FORGET_SOUND_IOCTLS


#ifdef HAVE_MMSYSTEM_H

static const inline bool ioctlable(int fd)
{
	return fd == localsound && fd != -1;
}

#else

static const inline bool ioctlable(int fd)
{
	DEBUG(2,9,fprintf(STDDBG, "Sound ioctl's absent\n");)
	return false;
}

#endif		// HAVE_MMSYSTEM_H


static inline void set_samp_size(int fd, int samp_size_bits)
{
}

static inline void set_samp_rate(int fd, int samp_rate)
{
}

static inline void set_channels(int fd, int channels)
{
}

static inline void set_nonblocking(int fd)
{
}

static inline int get_blksize(int fd)
{
	return DEFAULT_BUFF_SIZE;
}

static inline void sync_soundcard(int fd)
{
}

static inline void reset_soundcard(int fd)
{
}

#endif		// FORGET_SOUND_IOCTLS


void
wavefm::ioctl_attach()
{
	set_channels(fd, channel==CT_MONO ? 1 : 2);
	set_samp_rate(fd, samp_rate);
	set_samp_size(fd, hdr.samplesize);
	set_nonblocking(fd);
	if (!buff_size) buff_size = get_blksize(fd);
#ifdef HAVE_MMSYSTEM_H
	static WAVEHDR wavehdr;
	static HWAVEOUT hWaveOut;
	static char *activebuffie = NULL;
	if (ioctlable(fd)) {
		DWORD          dwResult;
		WAVEFORMATEX   pFormat;
		if (activebuffie)
			while (!(wavehdr.dwFlags | WHDR_DONE)) ;  // FIXME: busy waiting
		pFormat.wFormatTag = hdr.datform;
		pFormat.wBitsPerSample = hdr.samplesize;
		pFormat.nSamplesPerSec = hdr.sf1;
		pFormat.nChannels = hdr.numchan;
		pFormat.nBlockAlign = hdr.alignment;
		pFormat.nAvgBytesPerSec = hdr.avr1;
		pFormat.cbSize = 0;
		if(hWaveOut) {
			waveOutReset(hWaveOut);
			waveOutClose(hWaveOut);
			hWaveOut = NULL;
		}
		if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, 0, 0L,WAVE_FORMAT_QUERY))
			shriek(445, "Wave fmt not supported ...");
		if (waveOutOpen(&hWaveOut, WAVE_MAPPER,&pFormat, 0, 0L, CALLBACK_NULL))
			shriek(445, "Cannot open wave device ...");
		if (activebuffie) free(activebuffie);
		wavehdr.lpData = activebuffie = (char *)buffer; buffer = NULL;
		wavehdr.dwBufferLength = hdr.buffer_idx; hdr.buffer_idx = 0;
		wavehdr.dwFlags = 0L;
		wavehdr.dwLoops = 0L;
		if(waveOutPrepareHeader(hWaveOut, &wavehdr, sizeof(WAVEHDR)))
			shriek(445, "Cannot prepare wave header ...");
		dwResult = waveOutWrite(hWaveOut, &wavehdr, sizeof(WAVEHDR));
		if (dwResult != 0)
			shriek(445, "Cannot write to wave device ...");
	}
#endif
}


void
wavefm::detach(int)
{
	while (flush()) ;		// FIXME: think slow network write
	DEBUG(2,9,fprintf(STDDBG,"Detaching waveform %s\n", ""););
	sync_soundcard(fd);		// FIXME: necessary, but unwanted
	if (cfg->wav_hdr && !ioctlable(fd)) write_header();
	fd = -1;
}

void
wavefm::brk()
{
	hdr.buffer_idx = 0;		// forget wavefm hanging in userland
	reset_soundcard(fd);	// forget wavefm hanging in kernel
}


void
wavefm::attach(int d)
{
	DEBUG(2,9,fprintf(STDDBG,"Attaching waveform %s\n", ""););
	fd = d;
	translate();
	hdr.total_length = get_total_len() - RIFF_HEADER_SIZE;
	if (ioctlable(fd)) {
		ioctl_attach();
	}
	DEBUG(0,9,fprintf(STDDBG,"(attached, now flushing predata)\n");)
	if (hdr.buffer_idx) flush();
	DEBUG(0,9,fprintf(STDDBG,"(predata flushed)\n");)
	if (buff_size) buffer = buffer ? (SAMPLE *)xrealloc(buffer, buff_size * sizeof(SAMPLE)) : (SAMPLE *)xmalloc(buff_size);
}

void
wavefm::attach()
{
	char *output;
	int d;

	if (fd != -1) shriek(862, "Nested voice::attach()");
	if (!cfg->play_segs) cfg->local_sound_device = NULL_FILE;
	output = compose_pathname(cfg->local_sound_device, cfg->wav_dir);

#ifdef S_IRGRP
	d = open(output, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK | O_BINARY, MODE_MASK);
#else
	d = open(output, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
#endif
#ifdef	HAVE_MMSYSTEM_H
	localsound = d;
#endif
	if (d == -1) shriek(445, fmt("Failed to %s %s", strncmp(output, "/dev/", 5)
			? "create output file" : "open audio device", output));
	free(output);
	attach(d);
}

void
wavefm::detach()
{
	int from_fd = fd;
	detach(fd);
	async_close(from_fd);
	fd = -1;
}

static const int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                           5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                           6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                           6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                           7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                           7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                           7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                           7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};


/* The following is a fixed (FIXME: generalize) low-pass band filter, 0 to 4 kHz */

void
wavefm::band_filter(int ds)
{
	const double a[3][9] = {{1,-2.95588833875285,6.05358859038796,
				 -8.21891083745587,8.44331226032319,-6.3984876438294,
				 3.5670260078076,-1.33913357618129,0.282464627918022},
				{1,-5.64288854258764,15.23495961198290,
				 -25.26198227870461,27.94306848298673,-21.03831667925559,
				 10.52011230037758,-3.19916382894058,0.45524725438187},
				{1,-6.42466721653009,18.92807679520927,
				 -33.22220198267817,37.88644682915341,-28.70015825434698,
				 14.09511202954144,-4.10429644762400,0.54321917883107}};
	const double b[3][9] = {{0.0085475284871725,0.0230290518801368,0.0495486106178739,
				0.0712620928385417,0.0820025736317015,0.0712620928385417,
				0.049548610617874,0.0230290518801368,0.00854752848717252},
				{0.00182487646825,-0.00196817471641,0.00491609616544,
				-0.00250076958374,0.00529207410096,-0.00250076958374,
				0.00491609616544,-0.00196817471641,0.00182487646825},
				{0.00101813708883,-0.00274510939089,0.00509342597553,
				 -0.00601560729994,0.00666275143843,-0.00601560729994,
				 0.00509342597553,-0.00274510939089,0.00101813708883}};
	
	static double filt[9];
	int i,j;
	
	DEBUG(1,9,fprintf(STDDBG,"Low-pass filter is applied\n");)
	for (i = 0; i < 9; i++) filt[i] = 0;

	for (i = 0; i < hdr.buffer_idx; i++) {
		filt[0] = (double)(SIGNED_SAMPLE)buffer[i];
		for (j=1;j<9;filt[0]=filt[0]-a[ds-2][j]*filt[j++]);
		double outsamp = 0;
		for (j=0;j<9;outsamp=outsamp+b[ds-2][j]*filt[j++]);
//		printf("%d %f        ", buffer[i], (float)outsamp);
		buffer[i] = (SAMPLE)(SIGNED_SAMPLE)outsamp;
		for (j=8;j>0;j--) filt[j]=filt[j-1];		//FIXME: speed up
	}
}



// #define put_sample_8(sample)  *newbuff++ = (char)sample
// #define put_sample_16(sample) *(unsigned short *)newbuff = sample, newbuff += 2
// #define put_sample(sample) if (eight_bit) put_sample_8(sample); else put_sample_16(sample)

#define put_sample(sample) *(int *)newbuff = sample, newbuff += ssbytes;

inline void
wavefm::translate_data(char *newbuff)
{
	DEBUG(1,9,fprintf(STDDBG,"(translating the data, too)");)
//	if (cfg->ulaw && this_voice->samp_size != 8) shriek(462, "Mu law implies 8 bit");
	int ssbytes  = (this_voice->samp_size + 7) >> 3;  // sample size in bytes
	int shift1 = (sizeof(int) - sizeof(SAMPLE)) << 3;
	int shift2 = cfg->big_endian ? 0 : (sizeof(int) - ssbytes) << 3;
	int unsign = ssbytes == 1 ? 0x80 : 0;
	for (int i = 0; i < hdr.buffer_idx; i += downsamp) {
		int sample = buffer[i];
		if (cfg->ulaw) {		// Convert from 16 bit linear to ulaw. 
			int sign = (sample >> 8) & 0x80;          
	                if (sign) sample = -sample;              
	                int exponent = exp_lut[(sample >> 7) & 0xFF];
	                int mantissa = (sample >> (exponent + 3)) & 0x0F;
	                sample = ~(sign | (exponent << 4) | mantissa);
	        } else {
			sample <<= shift1;
			sample >>= shift2;
			sample += unsign;
		}
		switch(channel)
		{
			case CT_MONO:	put_sample(sample); break;
			case CT_LEFT:	put_sample(sample); put_sample(0); break;
			case CT_RIGHT:	put_sample(0); put_sample(sample); break;
			case CT_BOTH:	put_sample(sample); put_sample(sample); break;
		}
	}
}

void
wavefm::translate()
{
	DEBUG(1,9,fprintf(STDDBG,"Translating waveform, buffer_idx=%d\n", hdr.buffer_idx);)
	if (translated || !hdr.buffer_idx) return;
	
	int working_size = sizeof (SAMPLE);
	working_size *= downsamp;

	int target_size = this_voice->samp_size >> 3;
	target_size *= (1 + (channel != CT_MONO));
	
//	if (downsamp != 1 && cfg->autofilter) band_filter();	//output is downsampled
	if (cfg->autofilter) {
		if (downsamp == 2) band_filter(downsamp);	//output is downsampled
		if (downsamp == 3) band_filter(downsamp);	//output is downsampled
		if (downsamp == 4) band_filter(downsamp);	//output is downsampled
	}
	
	if (downsamp == 1 && working_size == target_size && channel == CT_MONO
						&& !cfg->ulaw) goto finis;
	if (working_size < target_size) {
		char *newbuff = (char *)xmalloc(hdr.buffer_idx * target_size / downsamp);
		translate_data(newbuff);
		free(buffer);
		buffer = (SAMPLE *)newbuff;
	} else {
		translate_data((char *)buffer);		// strange semantics
	}
	
	if (this_voice->out_rate) samp_rate = this_voice->out_rate;
	hdr.sf1 = samp_rate;		//if (hdr.sf2) hdr.sf2 = hdr.sf1;
	hdr.avr1 = samp_rate * target_size;	//if (hdr.avr2) hdr.avr2 = hdr.avr1;
	hdr.alignment = target_size;	hdr.samplesize = this_voice->samp_size;
	if (cfg->ulaw) hdr.datform = IBM_FORMAT_MULAW;
   finis:
	translated = true;
	hdr.buffer_idx = (hdr.buffer_idx + 1) / downsamp * target_size;
}

/*
 *	flush() is called whenever it is desirable to write out some data.
 *	This method can be called even for a detached waveform. The semantics
 *	is to write() out as much data as possible, and to have at least four
 *	bytes of buffer space available upon return (two would suffice btw).
 *
 *	This implementation writes out as much data as possible; if that is
 *	zero (detached waveform or out of kernel buffers), the buffer
 *	size is doubled.
 *
 *	Returns: true  ...more data remains to be written
 *		 false ...flushed completely
 *
 *	Also, "written" is set to the number of bytes actually written by the last
 *	invocation of flush()
 */

struct w_ophase
{
	bool inlined;
	int  adjustment;
	char **buff;
	int  *len;
};

#define INLINED_WOPH(begin, type)  { true, 0, (char **)&((wavefm *)NULL)->begin, (int *)sizeof(type) },
#define VAR_WOPH(ptr, len, adj)        { false, adj, (char **)&((wavefm *)NULL)->ptr, &((wavefm *)NULL)->len },

#define WOPHASE_NO_MORE_BUFFS  ((char **)-1)

const w_ophase wavefm::ophases[] = {
	INLINED_WOPH(hdr, wave_header)
	VAR_WOPH(buffer, hdr.buffer_idx, 0)
	INLINED_WOPH(cuehdr, cue_header)
	VAR_WOPH(cp_buff, cuehdr.len, -4)
	INLINED_WOPH(adtlhdr, adtl_header)
	VAR_WOPH(adtl_buff, adtlhdr.len, 0)
	{true, 0, WOPHASE_NO_MORE_BUFFS, (int *)0}
};

#define WAVEFM_ALL_FLUSHED  (ophases[ophase].buff == WOPHASE_NO_MORE_BUFFS)

int
wavefm::get_total_len()
{
	int total = 0;
	for (int x = 0; ophases[x].buff != WOPHASE_NO_MORE_BUFFS; x++) {
		if (ophases[x].inlined && !get_ophase_len(&ophases[x + 1]))
			continue;	/* inlined buffers followed by empty buffers are */
					/* assumed to be superfluous headers and skipped */
		total += get_ophase_len(&ophases[x]);
	}
	return total;
}

inline char *
wavefm::get_ophase_buff(const w_ophase *p)
{
	char *tmp = (char *)this + (int)p->buff;
	return p->inlined ? tmp : *(char **)tmp;
}

inline int
wavefm::get_ophase_len(const w_ophase *p)
{
	return (p->inlined ? (int)p->len : *(int *)((char *)this + (int)p->len)) + p->adjustment;
}

inline bool
wavefm::update_ophase()
{
	if (!cfg->wav_hdr || (fd != -1 && ioctlable(fd))) {	// sound card treated specially
		if (ophase == 0) {
			ophase++, ooffset = 0;
		}
		if (ophase > 1) return false;
		return get_ophase_len(&ophases[ophase]) > ooffset;
	}
	while (1) {
		if (get_ophase_len(&ophases[ophase]) > ooffset)
			return true;
		if (WAVEFM_ALL_FLUSHED)
			return false;
		ooffset = 0, ophase++;
		if (ophases[ophase].inlined && !WAVEFM_ALL_FLUSHED && !get_ophase_len(&ophases[ophase + 1]))
			ophase++;	/* inlined buffers followed by empty buffers are */
					/* assumed to be superfluous headers and skipped */
	}
}


bool
wavefm::flush()
{
	written = 0;

	if (buff_size == 0) {
		buff_size = cfg->buffer_size;
		buffer = (SAMPLE *)xmalloc(buff_size * sizeof(SAMPLE));
		return false;
	}
	if (!update_ophase())
		return false;
	if (!s && fd == -1)
		return flush_deferred();
	translate();
	written = ywrite(fd, get_ophase_buff(&ophases[ophase]) + ooffset,
			get_ophase_len(&ophases[ophase]) - ooffset);
	if (1 > written) return flush_deferred();
	
	ooffset += written;

	DEBUG(2,9,fprintf(STDDBG, "Flushing the signal\n");)
//	hdr.total_length += written;
	return true;
}

bool
wavefm::flush_deferred()
{
	DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (deferred)\n");)
	DEBUG(1,9,fprintf(STDDBG, fmt("adtlhdr.len is %d\n", adtlhdr.len));)
	written = 0;
	if (hdr.buffer_idx + 4 <= buff_size)
		return true;
	buff_size <<= 1;
	buffer = (SAMPLE *)xrealloc(buffer, buff_size * sizeof(SAMPLE));
	return true;
}

void
wavefm::write_header()
{
//	hdr.total_length = hdr.written_bytes + WAVE_HEADER_SIZE;
	if (lseek(fd, 0, SEEK_SET) == -1)
		return;		/* devices incapable of lseek() don't need
				 *	the length field filled in correctly
				 */
	ywrite(fd, &hdr, sizeof(wave_header));         //zapsani prazdne wav hlavicky na zacatek souboru
}

void 
wavefm::write_pa() {

#ifdef HAVE_PULSE_ERROR_H && HAVE_PULSE_SIMPLE_H
	/* The Sample format to use */
	ss.format = hdr.samplesize == 8 ? PA_SAMPLE_U8 : PA_SAMPLE_S16LE;
	ss.rate = samp_rate;
	ss.channels = channel == CT_MONO ? 1 : 2;

    	/* Create new simple playback stream 
	if (!(s = pa_simple_new(NULL, "Epos", PA_STREAM_PLAYBACK, NULL, "TTS-Epos", &ss, NULL, NULL, &error))) {
		const char *err = pa_strerror(error);
        	shriek(445, err);
    	}

	translate();
	hdr.total_length = get_total_len() - RIFF_HEADER_SIZE;
	if (ophase == 0) {
		ophase++, ooffset = 0;
	}
	
	written = get_ophase_len(&ophases[ophase]) - ooffset;
	if (pa_simple_write(s, get_ophase_buff(&ophases[ophase]) + ooffset, (size_t)written, &error) < 0) {
		const char *err = pa_strerror(error);
	        shriek(445, err);
	}
	if (pa_simple_drain(s, &error) < 0) {
       		const char *err = pa_strerror(error);
        	shriek(445, err);
	}	
	if (s) pa_simple_free(s);	//chaloupka
	*/

	/*Create new asynchro playback stream */	
	int ret = 1, r;
	pa_asyn_user user_params;

	user_params.ss = &ss;
	user_params.data = get_ophase_buff(&ophases[ophase]) + ooffset;
	user_params.length = written;

	assert(pa_sample_spec_valid(&ss));

	//Set up a new main loop
	if (!(m = pa_mainloop_new())) {
		DEBUG(2,9,fprintf(stderr, ("pa_mainloop_new() failed.\n")););
        	quit_pa();
	}

	DEBUG(2,9,fprintf(stderr, ("pa_mainloop_new() added.\n")););
	mainloop_api = pa_mainloop_get_api(m);

	r = pa_signal_init(mainloop_api);
	assert(r == 0);
	pa_signal_new(SIGINT, exit_signal_callback, NULL);
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif

	// Create a new connection context
	if (!(cntxt = pa_context_new(mainloop_api, client_name))) {
        	DEBUG(2,9,fprintf(stderr, ("pa_context_new() failed.\n")););
        	quit_pa();
	}
	
	//pa_context_set_state_callback(cntxt, context_state_callback, NULL);
	pa_context_set_state_callback(cntxt, context_state_callback, (void*)(&user_params));

	// Connect the context
	//    pa_context_connect(cntxt, server, (pa_context_flags_t)0, NULL);
	pa_context_connect(cntxt, NULL, (pa_context_flags_t)0, NULL);

	//* Run the main loop
	if (pa_mainloop_run(m, &ret) < 0) {
		DEBUG(2,9,fprintf(stderr, ("pa_mainloop_run() failed.\n")););
		quit_pa();
	}
#endif
}

void
wavefm::quit_pa()
{
#ifdef HAVE_PULSE_ERROR_H && HAVE_PULSE_SIMPLE_H
	if (pa_strm)
        	pa_stream_unref(pa_strm);

	if (cntxt)
        	pa_context_unref(cntxt);

	if (m) {
        	pa_signal_done();
        	pa_mainloop_free(m);
		m = NULL;
	}
#endif
}

void
wavefm::put_chunk(labl *chunk_template, const char *string)
{
	int varlen = strlen(string) + 2 & ~1;

	if (!adtl_buff) {
		adtl_max = ADTL_INITIAL_BUFF;
		adtl_buff = (char *)xmalloc(adtl_max);
	}
	while (adtlhdr.len + sizeof(labl) + varlen >= (unsigned int)adtl_max) {
		adtl_max <<= 1;
		adtl_buff = (char *)xrealloc(adtl_buff, adtl_max);
	}

	labl *la = (labl *)(adtl_buff + adtlhdr.len);
	*la = *chunk_template;
	la->len = sizeof(labl) + varlen - RIFF_HEADER_SIZE;
	la->cp_name = current_cp;
	strcpy((char *)(la + 1), string);
	decode_string((char *)(la + 1), this_lang->charset);
	((char *)(la+1))[varlen - 1] = 0;	// padding if odd
	adtlhdr.len += sizeof(labl) + varlen;
}

void
wavefm::label(int pos, char *label, const char *note)
{
	if (current_cp) {
		if (!(current_cp & (current_cp - 1)))
			cp_buff = (cue_point *)xrealloc(cp_buff, sizeof(cue_point) * current_cp * 2);
	} else cp_buff = (cue_point *)xmalloc(sizeof(cue_point));

	cp_buff[current_cp] = cue_point_template;
	cp_buff[current_cp].name = current_cp + 1;	/* numbered starting from 1 */
	cp_buff[current_cp].pos = cp_buff[current_cp].sample_offset = hdr.buffer_idx - pos;
	current_cp++;
	cuehdr.len += sizeof(cue_point);
	cuehdr.n++;

	if (!label) return;

	put_chunk(&labl_template, label);
	put_chunk(&note_template, note);
}

#ifdef SIMPLE_WFM

void
wavefm::become(void *b, int size)
{
	hdr = *(wave_header *)b;
	size -= sizeof(wave_header);
	if (size < 0) shriek(471, "tcpsyn got garbage");
	buffer = (char *)xmalloc(size);
	memcpy(buffer, (char *)b + sizeof(wave_header), size);
	buff_size = size;
	hdr.buffer_idx = size;
//	written_bytes = 0;
	hdr.total_length = 0 - HEADER_HEADER_SIZE;
	channel = hdr.sf2 ? CT_BOTH : CT_MONO;
	samp_rate = hdr.sf1;
}

#else

#define FOURCC_ID(x)  ((((x)[0])<<24) + (((x)[1])<<16) + (((x)[2])<<8) + (((x)[3])))

#define CHUNK_HEADER_SIZE 8

void pchu(char *p, int size)
{
	int l = *((int *)p+1);
//	printf("pchu %p+%d, %.4s len %d\n", p, size, p, *((int *)p+1));
	if (FOURCC_ID(p) == FOURCC_ID("RIFF")) {
//		printf("RIFF here\n");
		if (FOURCC_ID(p + RIFF_HEADER_SIZE) != FOURCC_ID("WAVE")) shriek(471, "Other RIFF than WAVE received");
		for (char *q = p+12; l>0; ) {
			int chunksize = ((int *)q)[1];
			pchu(q, chunksize);
			l -= chunksize + CHUNK_HEADER_SIZE;
			q += chunksize + CHUNK_HEADER_SIZE;
		}
	}
}

void wavefm::become(void *b, int size)
{
	((wave_header *)b)->total_length = size - RIFF_HEADER_SIZE;
	((wave_header *)b)->buffer_idx = size - sizeof(wave_header);
	pchu((char *)b, size);
}

#endif

