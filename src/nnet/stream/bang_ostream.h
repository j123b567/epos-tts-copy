#ifndef OSTREAM_H_INCLUDED
#define OSTREAM_H_INCLUDED

#include "bang_ios.h"

//-----------------------------------------------------------------------------

extern const char *bang_endl;


//-----------------------------------------------------------------------------

class bang_ostream: virtual public bang_ios{
public:
	virtual bang_ostream &operator<<(int i) { return *this; }
	virtual bang_ostream &operator<<(unsigned u) { return *this; }
	virtual bang_ostream &operator<<(long l) { return *this; }
	virtual bang_ostream &operator<<(unsigned long ul) { return *this; }
	virtual bang_ostream &operator<<(double d) { return *this; }
	virtual bang_ostream &operator<<(char c) { return *this; }
	virtual bang_ostream &operator<<(const char *s) { return *this; }
};


//-----------------------------------------------------------------------------
#endif // OSTREAM_H_INCLUDED
