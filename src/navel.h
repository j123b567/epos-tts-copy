/*
 *	epos/src/navel.h
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
 *
 *
 *	This template takes care of constructed objects that should be
 *	deleted if the object which references them fails to be constructed
 *	itself in a later stage.  The navelcord object should be directly
 *	placed on the stack.  It should be declared just after the
 *	operator new returns the object to be watched.  The release()
 *	method should be called when the navelcord is no more needed.
 *
 *	If the destructor is called between the constructor and the
 *	release() method, the referenced object is deleted.  That
 *	is usually due to stack unwinding caused by an exception.
 */


template <class t> class navelcord
{
	t *cont;
   public:
	inline navelcord(t *x)
	{
		cont = x;
	}

	inline ~navelcord()
	{
		if (cont) delete cont;
	}

	inline void release()
	{
		cont = NULL;
	}
};

