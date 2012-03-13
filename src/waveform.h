/*
 *	epos/src/waveform.h
 *	(c) 1998 Jirka Hanika, geo@ff.cuni.cz
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

struct wave_header
{
	char string1[4];
	long flen;
	char string2[8];
	long xnone;
	short int  datform, numchan, sf1, sf2, avr1, avr2, wlenB, wlenb;
	char string3[4];
	long written_bytes;
};			// .wav file header

class wavefm
{
	wave_header hdr;

	char *buffer;
	int buffer_idx;
	int buff_size;
	int samp_size_bytes;
	int samp_rate;
	CHANNEL_TYPE channel;
	int fd;
   public:
	wavefm(voice *);
	~wavefm();

	bool flush();		// write out at least something
				// see waveform.cc for more documentation
	void ioctl_attach();
	void attach(int fd);
	void attach();
	void detach(int fd);	// does not close fd
	void detach();		// also closes fd
	void brk();		// forgets pending data; does not detach()
	void skip_header();	// see waveform.cc for comments
	void write_header();

	inline void put_sample(unsigned int sample)
	{
		if (buff_size <= buffer_idx + samp_size_bytes)
			flush();
		buffer_idx += samp_size_bytes;
		switch (samp_size_bytes)
		{
			case 1:	*(unsigned char *) (buffer + buffer_idx) = sample; break;
			case 2:	*(unsigned short *)(buffer + buffer_idx) = sample; break;
		}
	}

	inline void sample(unsigned int sample)
	{
		switch(channel)
		{
			case CT_MONO:	put_sample(sample); break;
			case CT_FIRST:	put_sample(sample); put_sample(0); break;
			case CT_SECOND:	put_sample(0); put_sample(sample); break;
			case CT_BOTH:	put_sample(sample); put_sample(sample); break;
		}
	}

	void become(void *buffer, int size);

	inline int written_bytes() { return hdr.written_bytes + buffer_idx; }
};
