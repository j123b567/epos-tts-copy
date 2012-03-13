/*
 *	ss/src/hash.cc
 *	(c) 1994-98 geo@ff.cuni.cz (Jirka Hanika)
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
 *	This is a good place for hash table instantiation.
 *
 */

#include "hashtmpl.cc"

#include "common.h"

template class hash_table<char, freadin_file>;
template class hash_table<char, option>;		// used by daemon only
template class hash_table<char, char>;
