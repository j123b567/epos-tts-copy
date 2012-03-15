/*
 *	epos/src/function.h
 *	(c) geo@cuni.cz
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

template<class T> class booltab
{
	bool neg;
	int mult, size;
	T *t;
   public:
	booltab(T *s);
	~booltab();
	bool ismember(const T x);
};

template<class T> struct couple
{
	T x;
	T y;
};

template<class T> class function
{
	int mult, size;
	couple<T> *t;
   public:
	function(const T *s, const T *r);
	~function();
	T xlat(const T x);
};

typedef booltab<char> charclass;
typedef function<char> charxlat;
