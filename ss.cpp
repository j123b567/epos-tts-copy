/* Please ignore this file. If in DOS, compile it instead.*/

#ifndef __MSDOS__
#define __MSDOS__
#endif

#pragma warn -pia
#pragma warn -ccc
#pragma warn -rch

#define bool int

#define inline /* Borland can't handle inline */

#include "ss.cc"
#include "hash.cc"
#include "interf.cc"
#include "parser.cc"
#include "elements.cc"
#include "rule.cc"
#include "text.cc"
#include "synth.cc"
