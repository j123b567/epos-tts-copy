#ifndef FSTREAM_INCLUDED
#define FSTREAM_INCLUDED

#include "bang_istream.h"
#include <stdio.h>


//-----------------------------------------------------------------------------

class bang_ifstream: public bang_istream {
public:
	bang_ifstream() : bang_istream()	{ file = NULL; }
	bang_ifstream(const char *name, int mode = in);
	bang_ifstream(FILE *file, int mode = in);
	virtual ~bang_ifstream();
  void close();
  void open (const char *name, int mode = in);

  operator bool() { return (file!=NULL); }

	virtual bang_istream &operator>>(int &i);
	virtual bang_istream &operator>>(char &c);
	virtual bang_istream &operator>>(char *s);
	virtual bang_istream &operator>>(double &d);

  virtual bang_istream &putback(char &c);
	virtual bang_istream &getline(char *s, size_t n, int delim = EOF);
	virtual bang_istream &ignore(size_t n = 1, int delim = EOF);

private:
	FILE *file;
};


//-----------------------------------------------------------------------------

class bang_ofstream: public bang_ostream {
public:
	bang_ofstream() : bang_ostream()	{ file = NULL; }
	bang_ofstream(const char *name, int mode = out);
	bang_ofstream(FILE *file, int mode = out);
	virtual ~bang_ofstream();
  void close();
  void open (const char *name, int mode = out);

  operator bool() { return (file!=NULL); }

	virtual bang_ostream &operator<<(int i);
	virtual bang_ostream &operator<<(unsigned u);
	virtual bang_ostream &operator<<(long li);
	virtual bang_ostream &operator<<(unsigned long lu);
	virtual bang_ostream &operator<<(double g);
	virtual bang_ostream &operator<<(char c);
	virtual bang_ostream &operator<<(const char *s);

private:
	FILE *file;
};


//-----------------------------------------------------------------------------
#endif // FSTREAM_INCLUDED
