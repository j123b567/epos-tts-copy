/*
 *	ss/src/slab.h
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

#ifndef SLAB_FRAGMENT_SIZE
#define SLAB_FRAGMENT_SIZE 8192
#endif

class slab_free_list
{
   public: slab_free_list *n;
};

template <int size> class slab
{
	slab_free_list *tail;		// free cell list
	slab_free_list *slices;		// list of malloced blocks
	
   public:
	inline slab();
	inline ~slab();
	inline void *alloc();
	inline void release(void *ptr);
};

template <int size>
inline slab<size>::slab()
{
	tail = NULL;
	slices = NULL;
}

/*
 *	The destructor releases any memory previously allocated,
 *	and checks whether the space really was free.
 */

template <int size>
inline slab<size>::~slab()
{
	slab_free_list *tmp, *tmp2;
	int cells = 0;

	for (tmp = tail; tmp; tmp = tmp->n) cells++;
	tmp2 = NULL;
	for (tmp = slices; tmp; tmp = tmp->n) {
		if (tmp2) free(tmp2);
		tmp2 = tmp;
		cells -= SLAB_FRAGMENT_SIZE - 1;
	}
	free(tmp2);
	if (cells) shriek("%sHash slab lost %d cells", "", cells);
	tail = NULL;
	slices = NULL;
}

template <int size>
inline void *slab<size>::alloc()
{
	void *slot;

	if (!tail) {
		void *more = malloc(size * SLAB_FRAGMENT_SIZE);
		for (int i=1; i < SLAB_FRAGMENT_SIZE; i++) this->release(i*size +(char *)more);
		((slab_free_list *) more)->n = slices;
		slices = (slab_free_list *) more;
	}
	slot = tail;
	tail = tail->n;
	return slot;
}

template <int size>
inline void slab<size>::release(void *ptr)
{
	slab_free_list *slot = (slab_free_list *)ptr;
	slot->n = tail;
	tail = slot;
}

