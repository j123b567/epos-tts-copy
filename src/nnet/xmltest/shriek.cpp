#include <stdio.h>
#include <stdlib.h>
#include "../../neural.h"

void shriek(const char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
	while (true);
	exit(1);
}

void shriek(int, const char *txt)
{
	shriek(txt);
}

char error_fmt_scratch[1000];

char *fmt(const char *s, int i) 
{
	sprintf(error_fmt_scratch, s, i);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, const char *u) 
{
	sprintf(error_fmt_scratch, s, t, u);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, int i)
{
	sprintf(error_fmt_scratch, s, t, i);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t)
{
	sprintf(error_fmt_scratch,s,t);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, const char *u, int i)
{
	sprintf(error_fmt_scratch, s, t, u, i);
	return error_fmt_scratch;
}

char *fmt(const char *s, int i, int j)
{
	sprintf(error_fmt_scratch, s, i, j);
	return error_fmt_scratch;
}

char *fmt(const char *s, const char *t, int i, const char *u)
{
	sprintf(error_fmt_scratch, s, t, i, u);
	return error_fmt_scratch;
}


char *fmt(const char *s, const char *t, const char *u, const char *v)
{
	sprintf(error_fmt_scratch, s, t, u, v);
	return error_fmt_scratch;
}

char *fmt(const char *s, int t, const char *u)
{
	sprintf(error_fmt_scratch, s, t, u);
	return error_fmt_scratch;
}

char *fmt(const char *s, int t, const char *u, const char *v)
{
	sprintf(error_fmt_scratch, s, t, u, v);
	return error_fmt_scratch;
}


TTypedValue::TTypedValue(const TTypedValue&x)
{
	init (x);
}

TTypedValue & TTypedValue::operator= (const TTypedValue&x)
{
	init (x);
	return *this;
}

void TTypedValue::init (const TTypedValue &x)
{
	value_type = x.value_type;
	switch (value_type) {
		case 0: break;
		case 'i': int_val = x.int_val; break;
		case 'f': float_val = x.float_val; break;
		case 'c': char_val = x.char_val; break;
		case 'b': bool_val = x.bool_val; break;
		case 's': string_val = strdup (x.string_val); break;
		case 'u': unit_val = x.unit_val; break;
		default: shriek (861, "TTypedValue:Type not handled in init.");
	}
}

TTypedValue::~TTypedValue ()
{ 
	clear ();
}

void TTypedValue::clear ()
{
	switch (value_type) {
		case 0:
		case 'i':
		case 'f':
		case 'c':
		case 'b':
		case 'u':
			break;
		case 's': delete[] string_val; break;
		default: shriek (861, fmt ("TTypedValue:Type %c (%i) not handled in destructor.",value_type, value_type));
	}
	value_type = 0;
}
