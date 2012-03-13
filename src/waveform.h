/*
 *	epos/src/waveform.h
 *	(c) 1998-99 Jirka Hanika, geo@cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 */

void async_close(int fd);
int ywrite(int, const void *, int size);
int yread(int, void *, int size);

#define	RIFF_HEADER_SIZE	8

struct wave_header
{
	char string1[4];
	int  total_length;
	char string2[8];
	int  xnone;
	short int  datform, numchan, sf1, sf2, avr1, avr2, wlenB, wlenb;
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

struct w_ophase;

class wavefm
{
   public:	/* FIXME - a global initializer in waveform.cc needs this */
	wave_header hdr;
	cue_header cuehdr;
	adtl_header adtlhdr;

	char *buffer;
//	int written_bytes;
	int buff_size;
	int samp_size_bytes;
	int samp_rate;
	CHANNEL_TYPE channel;
	int fd;

	int current_cp;
	int last_offset;
	cue_point *cp_buff;
	char *adtl_buff;
//	int adtl_offs;
	int adtl_max;

//	char *bbf;		/* buffer being flushed */
//	int  bbf_len;		/* buffer being flushed len */
//	void update_bbf();

	int ophase;
	int ooffset;

	bool update_ophase();	/* returns whether more work to do */
	char *get_ophase_buff(w_ophase *);
	int get_ophase_len(w_ophase *);

	bool flush_deferred();

   public:
	wavefm(voice *);
	~wavefm();

	int written;		// bytes written by the last flush() only
//	inline int offset()
//	{
//		return written + buffer_idx;
//	}

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

	inline void put_sample(unsigned int sample)
	{
		if (buff_size <= hdr.buffer_idx + samp_size_bytes)
			flush();
		switch (samp_size_bytes)
		{
			case 1:	*(unsigned char *) (buffer + hdr.buffer_idx) = sample; break;
			case 2:	*(unsigned short *)(buffer + hdr.buffer_idx) = sample; break;
		}
		hdr.buffer_idx += samp_size_bytes;
	}

	inline void sample(unsigned int sample)
	{
		switch(channel)
		{
			case CT_MONO:	put_sample(sample); break;
			case CT_LEFT:	put_sample(sample); put_sample(0); break;
			case CT_RIGHT:	put_sample(0); put_sample(sample); break;
			case CT_BOTH:	put_sample(sample); put_sample(sample); break;
		}
	}

	void label(int position, char *label, char *note);

	void become(void *buffer, int size);

	inline int written_bytes() { return hdr.total_length + RIFF_HEADER_SIZE; }
};
