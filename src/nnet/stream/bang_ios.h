#ifndef IOS_INCLUDED
#define IOS_INCLUDED

#include <stdio.h>


//-----------------------------------------------------------------------------

class bang_ios {
public:
  bang_ios(): state(0) { }
	virtual ~bang_ios() { }

	enum _Iostate {
		goodbit = 0x0,
		eofbit = 0x1,
		failbit = 0x2,
		badbit = 0x4,
	};

	enum _Openmode {
		in = 0x01,
		out = 0x02,
		ate = 0x04,
		app = 0x08,
		trunc = 0x10,
		binary = 0x20
	};

  virtual bool eof() const { return (state & eofbit) != 0; }
	virtual bool fail() const { return (state & failbit) != 0; }
	virtual bool bad() const { return (state & badbit) != 0; }
  virtual void clear() { state = state & !failbit; }

protected:
	int state;
};


//-----------------------------------------------------------------------------
#endif // IOS_INCLUDED
