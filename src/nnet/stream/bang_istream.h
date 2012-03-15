#ifndef ISTREAM_H_INCLUDED
#define ISTREAM_H_INCLUDED

#include "stdio.h"
#include "bang_ostream.h"

//-----------------------------------------------------------------------------

class bang_istream: virtual public bang_ios{
public:
	virtual bang_istream &operator>>(int &i) = 0;
	virtual bang_istream &operator>>(char &c) = 0;
	virtual bang_istream &operator>>(char *s) = 0;
	virtual bang_istream &operator>>(double &d) = 0;

  virtual bang_istream &get(char &c);
  virtual bang_istream &get(char *buff, int count, char delim = '\n');
  virtual bang_istream &putback(char &c) = 0;
	virtual bang_istream &getline(char *s, size_t n, int delim = EOF) = 0;
	virtual bang_istream &ignore(size_t n = 1, int delim = EOF) = 0;
};


//-----------------------------------------------------------------------------

class bang_iostream: public bang_istream, public bang_ostream {
public:
  bang_iostream() { };
};


//-----------------------------------------------------------------------------
#endif // ISTREAM_H_INCLUDED
