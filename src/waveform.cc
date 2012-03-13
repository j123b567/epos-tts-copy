/*
 *	epos/src/waveform.cc
 *	(c) 1998-99 geo@cuni.cz
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

#include <fcntl.h>

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

#ifdef HAVE_LINUX_KD_H
	#include <linux/kd.h>	// too unimportant
#endif

#ifdef HAVE_IO_H
	#include <io.h>		/* open, (ioctl,) ... */
#endif

#ifndef SNDCTL_DSP_SYNC

#ifndef SOUND_PCM_SYNC
#define FORGET_SOUND_IOCTLS
#endif
#endif


#define FOURCC_INIT(x) {(x[0]), (x[1]), (x[2]), (x[3])}

//#pragma hdrstop

#ifdef KDGETLED_NEVER_DEFINED	// Feel free to disable or delete the following stuff
inline void mark_voice(int a)
{
	static int voices_attached = 0;
	voices_attached += a;
	int kbd_flags = 0;
	ioctl(1, KDGETLED, &kbd_flags);
	kbd_flags = kbd_flags & ~LED_SCR;
	if (voices_attached) kbd_flags |= LED_SCR;
	ioctl(1, KDSETLED, kbd_flags);
}
#else
// inline void mark_voice(int) {};
#endif


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

#define USA	 1		// FIXME (etc.)
#define English  9
#define American 1
#define Boring_CodePage	437

ltxt ltxt_template = { FOURCC_INIT("ltxt"), 0, 0, 0, FOURCC_INIT("dphl"), USA, English, American, Boring_CodePage };

wavefm::wavefm(voice *v)
{
	samp_rate = v->samp_rate;
	samp_size_bytes = v->samp_size >> 3;
	channel = v->channel;
	int stereo = channel==CT_MONO ? 0 : 1;

	memcpy(hdr.string1, "RIFF", 4);
	memcpy(hdr.string2, "WAVEfmt ", 8);
	memcpy(hdr.string3,"data", 4);
	hdr.datform = 1;
	hdr.numchan = 1;
	hdr.sf1 = samp_rate; hdr.sf2 = stereo ? hdr.sf1 : 0;
	hdr.avr1 = 2 * samp_rate; hdr.avr2 = stereo ? hdr.avr1 : 0;
	hdr.wlenB = samp_size_bytes; hdr.wlenb = v->samp_size;
	hdr.xnone = 0x010;
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
	last_offset = 0;

	ophase = 0;
	ooffset = 0;
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


#else		// FORGET_SOUND_IOCTLS

static const inline bool ioctlable(int fd)
{
	DEBUG(2,9,fprintf(STDDBG, "Sound ioctl's absent\n");)
	return false;
}

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
	set_samp_size(fd, samp_size_bytes << 3);
	set_nonblocking(fd);
	if (!buff_size) buff_size = get_blksize(fd);
}

void
wavefm::detach(int)
{
	while (flush()) ;		// FIXME: think slow network write
	DEBUG(2,9,fprintf(STDDBG,"Detaching waveform %s\n", ""););
	sync_soundcard(fd);		// FIXME: necessary, but unwanted
	if (cfg->wav_hdr && !ioctlable(fd)) write_header();
//	mark_voice(-1);
	fd = -1;
//	free(buffer);
//	buffer = NULL;
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
//	mark_voice(1);
	fd = d;
//	written_bytes = 0;
	if (ioctlable(fd)) {
		ioctl_attach();
	}
	if (buff_size) buffer = buffer ? (char *)xrealloc(buffer, buff_size) : (char *)xmalloc(buff_size);
	DEBUG(0,9,fprintf(STDDBG,"(attached, now flushing predata)\n");)
	if (hdr.buffer_idx) flush();
	DEBUG(0,9,fprintf(STDDBG,"(predata flushed)\n");)
}

void
wavefm::attach()
{
	char *output;
	int d;

	if (fd != -1) shriek(862, "Nested voice::attach()");
	if (!cfg->play_diph) cfg->wav_file = NULL_FILE;
	output = compose_pathname(cfg->wav_file, cfg->wav_dir);

#ifdef S_IRGRP
	d = open(output, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK | O_BINARY, MODE_MASK);
#else
	d = open(output, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
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
	char **buff;
	int  *len;
};

#define INLINED_WOPH(begin, type)  { true, (char **)&((wavefm *)NULL)->begin, (int *)sizeof(type) },
#define VAR_WOPH(ptr, len)        { false, (char **)&((wavefm *)NULL)->ptr, &((wavefm *)NULL)->len },

#define WOPHASE_NO_MORE_BUFFS  ((char **)-1)

w_ophase w_ophases[] = {
	INLINED_WOPH(hdr, wave_header)
	VAR_WOPH(buffer, hdr.buffer_idx)
	INLINED_WOPH(cuehdr, cue_header)
	VAR_WOPH(cp_buff, cuehdr.len)
	INLINED_WOPH(adtlhdr, adtl_header)
	VAR_WOPH(adtl_buff, adtlhdr.len)
	{true, WOPHASE_NO_MORE_BUFFS, (int *)0}
};

#define WAVEFM_ALL_FLUSHED  (w_ophases[ophase].buff == WOPHASE_NO_MORE_BUFFS)

inline char *
wavefm::get_ophase_buff(w_ophase *p)
{
	char *tmp = (char *)this + (int)p->buff;
	return p->inlined ? tmp : *(char **)tmp;
}

inline int
wavefm::get_ophase_len(w_ophase *p)
{
	return p->inlined ? (int)p->len : *(int *)((char *)this + (int)p->len);
}

inline bool
wavefm::update_ophase()
{
	if (fd != -1 && ioctlable(fd)) {		// sound card treated specially
		if (ophase == 0) {
			ophase++, ooffset = 0;
		}
		if (ophase > 1) return false;
		return get_ophase_len(&w_ophases[ophase]) > ooffset;
	}
	while (1) {
		if (get_ophase_len(&w_ophases[ophase]) > ooffset)
			return true;
		if (WAVEFM_ALL_FLUSHED)
			return false;
		ooffset = 0, ophase++;
		if (w_ophases[ophase].inlined && !WAVEFM_ALL_FLUSHED && !get_ophase_len(&w_ophases[ophase + 1]))
			ophase++;	/* inlined buffers followed by empty buffers are */
					/* assumed to be superfluous headers and skipped */
	}
}


bool
wavefm::flush()
{
//	printf("A ophase %d  ooffset %d\n", ophase, ooffset);
	written = 0;

	if (buff_size == 0) {
		buff_size = cfg->buffer_size;
		buffer = (char *)xmalloc(buff_size);
		return false;
	}
//	if (hdr.buffer_idx == 0) {
//		DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (nothing to do)\n");)
//		return false;
//	}
	if (!update_ophase())
		return false;
//	printf("B ophase %d  ooffset %d\n", ophase, ooffset);
	if (fd == -1)
		return flush_deferred();
	written = ywrite(fd, get_ophase_buff(&w_ophases[ophase]) + ooffset,
			get_ophase_len(&w_ophases[ophase]) - ooffset);
	if (1 > written) return flush_deferred();
	
	ooffset += written;

	DEBUG(2,9,fprintf(STDDBG, "Flushing the signal to device\n");)
	hdr.total_length += written;
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
	buffer = (char *)xrealloc(buffer, buff_size);
	return true;
}

#ifdef ___

bool
wavefm::flush()
{
	written = 0;

	if (fd != -1 && hdr.total_length < WAVE_HEADER_SIZE) {
		if (cfg->wav_hdr && !ioctlable(fd)) skip_header();
		if hdr.total_length < WAVE_HEADER_SIZE) return true;
	}

	if (buff_size == 0) {
		buff_size = cfg->buffer_size;
		buffer = (char *)xmalloc(buff_size);
		return false;
	}
	if (buffer_idx == 0) {
		DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (nothing!)\n");)
		return false;
	}
	if (fd == -1) return flush_deferred();

	update_bbf();
	written = ywrite(fd, bbf, bbf_len);
	if (1 > written) return flush_deferred();

	DEBUG(2,9,fprintf(STDDBG, "Flushing the signal to device\n");)
	hdr.written_bytes += written;
	buffer_idx -= written;
	if (buffer_idx) {
		memmove(buffer, buffer + written, buffer_idx);
		return true;
	}
	return false;
}

int wavefm::flush_deferred()
{
	DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (deferred)\n");)
	written = 0;
	if (buffer_idx + 4 <= buff_size)
		return true;
	buff_size <<= 1;
	buffer = (char *)xrealloc(buffer, buff_size);
	return true;
}

struct wbdes
{
	char **buffer;
	int  *len;
};

void update_bbf()
{
	int tmp = WAVE_HEADER_SIZE + buff_size;
	if (hdr.total_length < tmp) {
		bbf = buffer;
		bbf_len = buffer_idx;
		return;
	}
	tmp += current_cp * sizeof(cue_point);
	if (hdr.total_length < tmp) {
		bbf = (char *)cp_buff + current_cp * sizeof(cue_point)
	}
	tmp += ...
	if () {
	}
}

/*
 *	skip_header() is called before the samples are written, if a waveform header
 *	is required. It will try to skip enough space (the header cannot be written
 *	at this moment because the number of samples may still be unknown).
 *
 *	write_header() is called after the samples are written, if a waveform header
 *	is required. It will return to the beginning of the file and write the header.
 *
 *	However, if the descriptor is actually a network socket or a non-seekable
 *	device, the header has to be written before the data are computed. In this
 *	case, the written_bytes and flen items in the header may be invalid.
 *
 *	become() is currently only called by tcpsyn; it makes *this a copy of a waveform
 *	received via network. It therefore has to fill in any invalid fields.
 *	become() should be able to cope with both attached and detached waveforms.
 */

void
wavefm::skip_header()
{
	if (lseek(fd, sizeof(wave_header), SEEK_SET) == -1) {
		int tmp = ywrite(fd, ((char *)&hdr) + hdr.total_length,
					sizeof(wave_header) - hdr.total_length);
		if (tmp == -1) shriek(462, "skip_header got error, contact the authors");
		hdr.total_length += tmp;
	}
}

#endif // ___

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
wavefm::label(char *lbl)
{
	if (current_cp) {
		if (!(current_cp & (current_cp - 1)))
			cp_buff = (cue_point *)xrealloc(cp_buff, sizeof(cue_point) * current_cp * 2);
	} else cp_buff = (cue_point *)xmalloc(sizeof(cue_point));

	cp_buff[current_cp] = cue_point_template;
	cp_buff[current_cp].name = current_cp + 1;	/* numbered starting from 1 */
	int offs = cp_buff[current_cp].pos = cp_buff[current_cp].sample_offset = hdr.buffer_idx;
	current_cp++;
	cuehdr.len += sizeof(cue_point);
	cuehdr.n++;

	if (!lbl) return;

	if (adtl_buff) {
		if (adtlhdr.len + strlen(lbl) + 2 + sizeof(ltxt) >= (unsigned int)adtl_max) {
			adtl_max <<= 1;
			adtl_buff = (char *)xrealloc(adtl_buff, adtl_max);
		}
	} else {
		adtl_max = ADTL_INITIAL_BUFF;
		adtl_buff = (char *)xmalloc(adtl_max);
	}

	ltxt *l = (ltxt *)(adtl_buff + adtlhdr.len);
	*l = ltxt_template;
	l->cp_name = current_cp - 1;
	l->sample_count = offs - last_offset;
	l->len = sizeof(ltxt) - RIFF_HEADER_SIZE + strlen(lbl) + 1;
	strcpy((char *)(l+1), lbl);
	adtlhdr.len += sizeof(ltxt) + strlen(lbl) + 1;

	last_offset = offs;
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
	samp_size_bytes = hdr.wlenB;
}

#else

#define FOURCC_ID(x)  ((((x)[0])<<24) + (((x)[1])<<16) + (((x)[2])<<8) + (((x)[3])))

#define CHUNK_HEADER_SIZE 8

void pchu(char *p, int size)
{
	int l = *((int *)p+1);
	printf("pchu %p+%d, %.4s len %d\n", p, size, p, *((int *)p+1));
	if (FOURCC_ID(p) == FOURCC_ID("RIFF")) {
		printf("RIFF here\n");
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
