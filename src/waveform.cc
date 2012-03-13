/*
 *	epos/src/waveform.cc
 *	(c) 1998-99 geo@ff.cuni.cz
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

#ifndef O_BINARY	/* open */
	#define O_BINARY  0
#endif

//#pragma hdrstop

#ifdef KDGETLED		// Feel free to disable or delete the following stuff
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
inline void mark_voice(int) {};
#endif


#define HEADER_HEADER_SIZE  8
#define WAVE_HEADER_SIZE  ((long)(sizeof(wave_header) - HEADER_HEADER_SIZE))


struct cue_point
{
	long name;
	long pos;
	char chunk[4];
	long chunkstart;
	long blkstart;
	long sample_offset;
};

cue_point cue_point_template = { 0, 0, "data", 0, 0, 0 };

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

ltxt ltxt_template = { "ltxt", 0, 0, 0, "dphl", USA, English, American, Boring_CodePage };

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
	hdr.written_bytes = 0;
	hdr.total_length = hdr.written_bytes - HEADER_HEADER_SIZE;
//	write(fd, wavh, sizeof(wave_header));         //zapsani prazdne wav hlavicky na zacatek souboru

	fd = -1;
	written = 0;
//	buff_size = cfg->buffer_size;
//	buffer = (char *)xmalloc(buff_size);
	buff_size = 0;
	buffer = NULL;
	buffer_idx = 0;

	current_cp = adtl_offs = 0;
	cp_buff = NULL;
	adtl_buff = NULL;
	last_offset = 0;
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
#ifndef SOUND_PCM_SYNC
#error The sound card ioctls seem to be broken or absent, remove this #error
#endif
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
	while (buffer_idx) flush();		// FIXME: think slow network write
	DEBUG(2,9,fprintf(STDDBG,"Detaching waveform %s\n", ""););
	sync_soundcard(fd);
	if (cfg->wav_hdr && !ioctlable(fd)) write_header();
	mark_voice(-1);
	fd = -1;
//	free(buffer);
//	buffer = NULL;
}

void
wavefm::brk()
{
	buffer_idx = 0;		// forget wavefm hanging in userland
	reset_soundcard(fd);	// forget wavefm hanging in kernel
}


void
wavefm::attach(int d)
{
	DEBUG(2,9,fprintf(STDDBG,"Attaching waveform %s\n", ""););
	mark_voice(1);
	fd = d;
	hdr.written_bytes = 0;
	if (ioctlable(fd)) ioctl_attach();			// FIXME
	if (buff_size) buffer = buffer ? (char *)xrealloc(buffer, buff_size) : (char *)xmalloc(buff_size);
	DEBUG(0,9,fprintf(STDDBG,"(attached, now flushing predata)\n");)
	if (buffer_idx) flush();
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
	d = open(output, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, MODE_MASK);
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

bool
wavefm::flush()
{
	written = 0;

	if (fd != -1 && hdr.total_length < WAVE_HEADER_SIZE) {
		if (cfg->wav_hdr && !ioctlable(fd)) skip_header();
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
	if (fd == -1 || 1 > (written = ywrite(fd, buffer, buffer_idx))) {
		DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (deferred)\n");)
		written = 0;
		if (buffer_idx + 4 <= buff_size)
			return true;
		buff_size <<= 1;
		buffer = (char *)xrealloc(buffer, buff_size);
		return true;
	}
	DEBUG(2,9,fprintf(STDDBG, "Flushing the signal to device\n");)
	hdr.written_bytes += written;
	buffer_idx -= written;
	if (buffer_idx) {
		memmove(buffer, buffer + written, buffer_idx);
		return true;
	}
	return false;
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

void
wavefm::write_header()
{
	hdr.total_length = hdr.written_bytes + WAVE_HEADER_SIZE;
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
	int offs = cp_buff[current_cp].pos = cp_buff[current_cp].sample_offset = offset();
	current_cp++;

	if (!lbl) return;

	if (adtl_buff) {
		if (adtl_offs + strlen(lbl) + 2 + sizeof(ltxt) >= (unsigned int)adtl_max) {
			adtl_max <<= 1;
			adtl_buff = (char *)xrealloc(adtl_buff, adtl_max);
		}
	} else {
		adtl_max = ADTL_INITIAL_BUFF;
		adtl_buff = (char *)xmalloc(adtl_max);
	}

	ltxt *l = (ltxt *)(adtl_buff + adtl_offs);
	*l = ltxt_template;
	l->cp_name = current_cp - 1;
	l->sample_count = offs - last_offset;
	l->len = sizeof(ltxt) - HEADER_HEADER_SIZE + strlen(lbl) + 1;
	strcpy((char *)(l+1), lbl);
	adtl_offs += sizeof(ltxt) + strlen(lbl) + 1;

	last_offset = offs;
}

void
wavefm::become(void *b, int size)
{
	hdr = *(wave_header *)b;
	size -= sizeof(wave_header);
	if (size < 0) shriek(471, "tcpsyn got garbage");
	buffer = (char *)xmalloc(size);
	memcpy(buffer, (char *)b + sizeof(wave_header), size);
	buff_size = size;
	buffer_idx = size;
	hdr.written_bytes = 0;
	hdr.total_length = hdr.written_bytes - HEADER_HEADER_SIZE;
	channel = hdr.sf2 ? CT_BOTH : CT_MONO;
	samp_rate = hdr.sf1;
	samp_size_bytes = hdr.wlenB;
}
