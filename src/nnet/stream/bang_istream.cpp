#include "bang_istream.h"


//-----------------------------------------------------------------------------

bang_istream &bang_istream::get(char &c)
{
  return (*this >> c);
}


//-----------------------------------------------------------------------------

bang_istream &bang_istream::get(char *buff, int count, char delim)
{
  char c;

  if (state) {
    state |= failbit;
    return *this;
  }

  for (int i = 1; i < count; buff++, i++) {
    *this >> c;
	if (eof()) {
		break;
	}
	if (c == delim) {
		putback(c);
		break;
	}
	*buff = c;
  }
  *buff = 0;
  return *this;
}

 

//-----------------------------------------------------------------------------
