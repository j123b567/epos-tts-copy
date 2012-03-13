/*
 *	epos/src/waveform.cc
 *	(c) 1998 geo@ff.cuni.cz
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
#include <fcntl.h>

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

#ifdef HAVE_LINUX_KD_H
#include <linux/kd.h>	// too unimportant
#endif

#ifdef HAVE_IO_H
#include <io.h>		/* open, write, (ioctl,) ... */
#endif

#ifndef O_BINARY	/* open */
#define O_BINARY  0
#endif

#pragma hdrstop

#ifdef KDGETLED		// Feel free to disable or delete the following stuff
inline void mark_voice(int a)
{
	static voices_attached = 0;
	voices_attached += a;
	int kbd_flags = 0;
	ioctl(1, KDGETLED, kbd_flags);
	kbd_flags = kbd_flags & ~LED_SCR;
	if (voices_attached) kbd_flags |= LED_SCR;
	ioctl(1, KDSETLED, kbd_flags);
}
#else
inline void mark_voice(int) {};
#endif




const inline bool ioctlable(int fd)
{
#ifdef SOUND_PCM_GETFMTS
	int tmp;
	return !ioctl (fd, SOUND_PCM_GETFMTS, &tmp);
#else
   #ifdef SNDCTL_DSP_GETFMTS
	int tmp;
	return !ioctl (fd, SNDCTL_DSP_GETFMTS, &tmp);
   #else
//	DEBUG(3,9,fprintf(STDDBG, "Sound ioctl's absent\n");)
	return false;
	#error The sound card ioctls seem to be broken or absent, remove this #error
   #endif
#endif
	
}


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
	hdr.wlenB = samp_size_bytes; hdr.wlenb = v->samp_size;	// FIXME ?
	hdr.xnone = 0x010;
	hdr.written_bytes = 0;
	hdr.flen = hdr.written_bytes + 0x24;
//	write(fd, wavh, sizeof(wave_header));         //zapsani prazdne wav hlavicky na zacatek souboru

	fd = -1;
//	buff_size = cfg->buffer_size;
//	buffer = (char *)malloc(buff_size);
	buff_size = 0;
	buffer = NULL;
	buffer_idx = 0;
}

wavefm::~wavefm()
{
	if (fd != -1) detach();
	free(buffer);
}

#define DEFAULT_BUFF_SIZE	4096

void
wavefm::ioctl_attach()
{
	int stereo = channel==CT_MONO ? 0 : 1;
	int samp_size = samp_size_bytes << 3;
#ifdef SOUND_PCM_GETBLKSIZE
	stereo++;	/* 1 or 2 channels */
	ioctl (fd, SOUND_PCM_WRITE_CHANNELS, &stereo);  // keep it mono (disabled)
	ioctl (fd, SOUND_PCM_WRITE_RATE, &samp_rate);
	ioctl (fd, SOUND_PCM_WRITE_BITS, &samp_size);
	ioctl (fd, SOUND_PCM_NONBLOCK);
//	ioctl (fd, SOUND_PCM_GETBLKSIZE, &buff_size) && (buff_size = DEFAULT_BUFF_SIZE);
#else
   #ifdef SNDCTL_DSP_SPEED
	ioctl (fd, SNDCTL_DSP_STEREO, &stereo);		// keep it mono (disabled)
	ioctl (fd, SNDCTL_DSP_SPEED, &samp_rate);

      #ifdef DEBUGGING		/* Badly placed */
	int mask = (unsigned int)-1;
	ioctl (fd, SNDCTL_DSP_GETFMTS, &mask);
	DEBUG(2,9,fprintf(STDDBG,"Hardware format mask is 0x%04x\n", mask);)
	if (!(samp_size & mask)) shriek(813, "Sampling rate not supported");
      #endif

	ioctl (fd, SNDCTL_DSP_SETFMT, &samp_size);
	ioctl (fd, SNDCTL_DSP_NONBLOCK);
//	ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &buff_size) && (buff_size = DEFAULT_BUFF_SIZE);
   #else
	DEBUG(3,9,fprintf(STDDBG, "Sound ioctl's absent\n");)
	unuse(stereo);
//	buff_size = DEFAULT_BUFF_SIZE;
   #endif
#endif
}

void
wavefm::attach(int d)
{
	DEBUG(2,9,fprintf(STDDBG,"Attaching waveform %s\n", ""););
	mark_voice(1);
	fd = d;
	hdr.written_bytes = 0;
	if (ioctlable(fd)) ioctl_attach();			// FIXME
	if (buff_size) buffer = buffer ? (char *)realloc(buffer, buff_size) : (char *)malloc(buff_size);
	if (cfg->wav_hdr && !ioctlable(fd)) skip_header();
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
wavefm::detach(int)
{
	while (buffer_idx) flush();		// FIXME: think slow network write
	DEBUG(2,9,fprintf(STDDBG,"Detaching waveform %s\n", ""););
#ifdef SNDCTL_DSP_SYNC
	if (ioctlable(fd)) ioctl (fd, SNDCTL_DSP_SYNC);
#else
   #ifdef SOUND_PCM_SYNC
	if (ioctlable(fd)) ioctl (fd, SOUND_PCM_SYNC);
   #endif
#endif
	if (cfg->wav_hdr && !ioctlable(fd)) write_header();
	mark_voice(-1);
	fd = -1;
//	free(buffer);
//	buffer = NULL;
}

void
wavefm::detach()
{
	detach(fd);
	async_close(fd);
	fd = -1;
}

void
wavefm::brk()
{
	buffer_idx = 0;		// forget wavefm hanging in userland
#ifdef SNDCTL_DSP_RESET		// forget wavefm hanging in kernel
	if (ioctlable(fd)) ioctl (fd, SNDCTL_DSP_RESET);
#else
   #ifdef SOUND_PCM_RESET
	if (ioctlable(fd)) ioctl (fd, SOUND_PCM_RESET);
   #endif
#endif
}

/*
 *	flush() is called whenever it is desirable to write out some data.
 *	This method can be called even for a detached waveform. The semantics
 *	is to write() out as much data as possible, and to have at least one
 *	byte of buffer space available upon return.
 *
 *	This implementation writes out as much data as possible; if that is
 *	zero (detached waveform or kernel tx buffer overflow), the buffer
 *	size is doubled.
 *
 *	Returns: true  ...more data remains to be written
 *		 false ...flushed completely
 */

bool
wavefm::flush()
{
	int written;

	if (buff_size == 0) {
		buff_size = cfg->buffer_size;
		buffer = (char *)malloc(buff_size);
		return false;
	}
	if (buffer_idx == 0) {
		DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (nothing!)\n");)
		return false;
	}
	if (fd == -1 || ! (written = write(fd, buffer, buffer_idx))) {
		DEBUG(2,9,fprintf(STDDBG, "Flushing the signal (deferred)\n");)
		buff_size <<= 1;
		buffer = (char *)realloc(buffer, buff_size);
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
	if (lseek(fd, sizeof(wave_header), SEEK_SET) == -1)
		write(fd, &hdr, sizeof(wave_header));
}

void
wavefm::write_header()
{
	hdr.flen = hdr.written_bytes + 0x24;
	if (lseek(fd, 0, SEEK_SET) == -1)
		return;		/* devices incapable of lseek() don't need
				 *	the length field filled in correctly
				 */
	write(fd, &hdr, sizeof(wave_header));         //zapsani prazdne wav hlavicky na zacatek souboru
}

void
wavefm::become(void *b, int size)
{
	hdr = *(wave_header *)b;
	size -= sizeof(wave_header);
	if (size < 0) shriek(471, "tcpsyn got garbage");
	buffer = (char *)malloc(size);
	memcpy(buffer, (char *)b + sizeof(wave_header), size);
	buff_size = size;
	buffer_idx = size;
	hdr.written_bytes = 0;
	hdr.flen = 0;
}
