/*
 *	epos/src/waveform.h
 *	(c) 1998-01 Jirka Hanika, geo@cuni.cz
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
 *	In Epos, waveforms are externally (as required by TTSCP) and
 *	also internally represented by the RIFF waveform pseudostandard
 *	format.  As generating the waveform is likely to become
 *	a performance bottleneck, and quite a few aspects of this process
 *	have been gradually made configurable, the sample() method
 *	has been made inline in this header file.
 *
 *	The design is that the synthesizer calls a wavefm object's sample()
 *	method every time it outputs a sample.  In theory, the sample
 *	should be processed using configuration parameters.  This is
 *	however rather slow for a lightning-fast synthesizer: loop
 *	optimization won't probably work here, because the optimizer is afraid
 *	of configuration parameters and/or any internal wavefm structure
 *	members being pointed to by the buffer being filled, and thus it
 *	doesn't see they're really invariant.  So we just store the samples
 *	in the buffer and we translate the header and/or buffer later
 *	if necessary.  No translation should be needed if you request
 *	a 16-bit monophonic waveform at its "natural" (recorded) sampling
 *	rate.
 */

void async_close(int fd);
int ywrite(int, const void *, int size);
int yread(int, void *, int size);

#define	RIFF_HEADER_SIZE	8

#ifdef SAMPLE
	#error Macro conflict: SAMPLE
#endif

#define SAMPLE		unsigned short	/* working sample type */
#define SIGNED_SAMPLE	signed short	/* FIXME: get rid of this */

//chaloupka
#ifdef HAVE_PULSE_ERROR_H && HAVE_PULSE_SIMPLE_H
	#include "pa_asyn.h"
	#include <pulse/error.h>
	#include <pulse/simple.h>
#else
	#define	pa_sample_spec void*
	#define pa_simple void
#endif
//#include <pulse/gccmacro.h>


struct wave_header
{
	char string1[4];
	int  total_length;
	char string2[8];
	//int  fmt_length;
	//short int  datform, numchan, sf1, sf2, avr1, avr2, alignment, samplesize;
	//chaloupka
	int  fmt_length,sf1, avr1;
	short int datform, numchan,alignment, samplesize;
	char string3[4];
	int  buffer_idx;
};			// .wav file header

struct cue_point;

struct cue_header
{
	char string1[4];
	int len;
	int n;
};

struct adtl_header
{
	char string1[4];
	int len;
	char string2[4];
};

struct labl;

struct w_ophase;

class wavefm
{
   protected:
	wave_header hdr;
	cue_header cuehdr;
	adtl_header adtlhdr;

	SAMPLE *buffer;
	int buff_size;

//	int samp_size_bytes;
	int samp_rate;
	CHANNEL_TYPE channel;
	int fd;

	int current_cp;
	cue_point *cp_buff;
	char *adtl_buff;
	int adtl_max;

	//chaloupka
	pa_sample_spec ss;
	pa_simple *s;
	int error;


	static const w_ophase ophases[];
	int ophase;
	int ooffset;

	bool update_ophase();	/* returns whether more work to do */
	char *get_ophase_buff(const w_ophase *);
	int get_ophase_len(const w_ophase *);
	int get_total_len();

	bool flush_deferred();
	
	void translate_data(char *new_buff);	/* recode data from buffer to new_buff */
	void translate();	/* downsample, stereophonize, eightbitize or ulawize */
	void band_filter(int ds);	/* low band filter applied if downsampling */
	bool translated;
	int downsamp;
	
	void put_chunk(labl *chunk_template, const char *label);

   public:
	wavefm(voice *);
	~wavefm();

	int get_buffer_index() { return hdr.buffer_idx; };
	char *get_buffer_data() {  return (char *)buffer; };
	int written;		// bytes written by the last flush() only

	bool flush();		// write out at least something
				// see waveform.cc for more documentation
	void ioctl_attach();
	void attach(int fd);
	void attach();
	void detach(int fd);	// does not close fd
	void detach();		// also closes fd
	void brk();		// forgets pending data; does not detach()
//	void skip_header();	// see waveform.cc for comments
	void write_header();
	void write_pa();	//chaloupka pulseaudio output
	void quit_pa();
	
	inline void sample(unsigned int sample)
	{
		if (buff_size <= hdr.buffer_idx + 1)
			flush();
		buffer[hdr.buffer_idx] = sample;
		hdr.buffer_idx ++;
	}
	
	inline void sample(SAMPLE *b, int count)
	{
		while (buff_size < hdr.buffer_idx + count) {
			int avail = buff_size - hdr.buffer_idx;
			sample(b, avail);
			b += avail;
			count -= avail;
			flush();
		}
		memcpy(buffer + hdr.buffer_idx, b, count * sizeof (SAMPLE));
		hdr.buffer_idx += count;
	}

	void label(int position, char *label, const char *note);

	void become(void *buffer, int size);

	inline int written_bytes() { return hdr.total_length + RIFF_HEADER_SIZE; }
};
