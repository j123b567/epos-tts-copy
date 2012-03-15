#ifndef __SET_H__
#define __SET_H__

#include "vector.h"

template<class T> class TSet: public TVector<T> {
public:
		TSet() : TVector<T>()		{};

		typedef viterator<T> iterator;			 
		virtual iterator insert (const T &x);
		virtual iterator find(const T& key) const;

private:
		// do not use!
		virtual iterator push_back (const T & x) { return TVector<T>::push_back (x); }	
};

#define SET(x,y) typedef TSet<x> y;

template<class T> TSet<T>::iterator TSet<T>::insert (const T &x)
{
	if (capacity == 0) Realloc (1);
	else Realloc (capacity + 1);
	int i;
	for (i=0; i < capacity-1 && d[i] < x; ++i); 
	int i2;
	for (i2=capacity-1; i2 > i; --i2)
		d[i2] = d[i2-1];
	d[i] = x;

	return d+i;
}

template<class T> TSet<T>::iterator TSet<T>::find(const T& x) const
{
	viterator<T> iter;
	for (iter = begin(); iter != end() && x > *iter; ++iter);
	if (iter == end()) return end();
	if (*iter > x) return end();
	else return iter;		
}

#endif