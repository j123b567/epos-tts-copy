/*
 *	epos/src/hash.cc
 *	(c) 1994-98 geo@cuni.cz (Jirka Hanika)
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
 *	This version of hash::hash can handle tricky comments acceptably,
 *	i.e. a comment must begin with a COMMENT_LINES character, which
 *	must either be preceded by a WHITESPACE one or come as the first
 *	char in the line. I don't know of a better way how to interpret 
 *	or rewrite e.g.
 *
some_key  this;is;the;replacement   ;this is a comment; why not.
 *
 *	where ';' is a comment-out character. If you need an initial ";" or
 *	a " ;" substring in the replacement, you might consider changing
 *	the exported variable COMMENT_LINES to "\n" only and refrain from
 *	using comments at all.  Or, use COMMENT_LINES to choose another
 *	comment-out character.
 *
 */

#ifndef HASH_IS_SLABBING
	#define HASH_IS_SLABBING
	#define HASH_SLAB_FRAGMENT_SIZE  8192
#endif

#ifndef HASH_CANNOT_READ_FILES
	#define HASH_CAN_READ_FILES
	#include "defaults.h"         // Comment out if doesn't exist
#endif

#include "hash.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef WANT_DMALLOC
	#include <dmalloc.h>
#endif

#define key_t  hash_key_t
#define data_t hash_data_t

// #define DEBUG_HASH          // Quite detailed debugging printouts
// #define PROF_HASH           // Useless profiling (maximum tree depth printed on delete)

#define DOWNSIZE_HESITATE 8 // Downsizing should be less frequent for very small tables

#define unconst(x) ((key_t *)(x))

#define key_t_is_string (sizeof(key_t)==1)
#define data_t_is_string (sizeof(data_t)==1)
#define keyhash(x)    (key_t_is_string  ? 0 /* unreachable */ : (unsigned int) *unconst(key))
#define keycmp(x,y)   (key_t_is_string  ? strcmp((char *)x, (char *)y)  : *unconst(x)-*unconst(y))
#define keydodup(x)   (key_t_is_string  ? (key_t*)strdup((char *)(x))   : new key_t(*x))
#define datadodup(x)  (data_t_is_string ? (data_t *)strdup((char *)(x)) : new data_t(*x))

#define keydup(x)	(dupkey  ? keydodup(x)  : unconst(x))
#define datadup(x)	(dupdata ? datadodup(x) : (data_t *)(x))
#define keydel(x)	(dupkey  ? delete(x)	: (void)(x))
#define datadel(x)	(dupdata ? delete(x)	: (void)(x))

// #pragma warn -rch	Enable for Borland to avoid some warnings

template <class key_t, class data_t>
struct hsearchtree {
	key_t *key;
	data_t *data;
	hsearchtree<hash_key_t, hash_data_t> *l,*r;
	/* short */ int height;
};


#ifdef HASH_IS_SLABBING

#include "slab.h"

#define hsearchtree_size sizeof(hsearchtree<char, char>)

slab <hsearchtree_size> *hash_tree_slab = 0;

inline void *tree_alloc()
{
	if (!hash_tree_slab) {
		hash_tree_slab = new slab<hsearchtree_size>(HASH_SLAB_FRAGMENT_SIZE);
	}
	return hash_tree_slab->alloc();
}

inline void tree_delete(void *tree) 
{
	hash_tree_slab->release(tree);
	return;
}

void shutdown_hashing()
{
	if (hash_tree_slab) delete hash_tree_slab;
}

#else

inline void * tree_alloc()
{
	return xmalloc(sizeof (hsearchtree<char, char>));
}

inline void tree_delete(void *tree) 
{
	free(tree);
}

void shutdown_hashing()
{
}

#endif

#if (0) //try #ifdef DEBUG_HASH if you wanna hash_shriek()
#define push(x) (_hash_sp >= _HASH_DEPTH ? hash_shriek("push %s%d","",_hash_sp) : NULL, _hash_stk[_hash_sp++] = (x))
#define pop (_hash_sp < 1 ? hash_shriek("pop %s%d","",_hash_sp) : NULL, *_hash_stk[--_hash_sp])
#else

#define UNTYPED(x) ((hsearchtree<void, void> **)(x))
#define UNTYPED_LVALUE(x) (*(hsearchtree<void, void> **)(&x))
#define push(x) (_hash_stk[_hash_sp++]=(UNTYPED(x)))
#define pop (*_hash_stk[--_hash_sp])

#endif // unconditional conditional

#define heightof(x) ((x)?((x)->height):(-1))

/****************************************************************************
  balance - locally rebalance the required tree nodes. The request
 	stack _hash_stk will get cleared in the process. The time to
 	process one item is constant. There is one global stack
 	for all template instances, thus adding/removing elements
 	is not reentrant.
 	
 	The case when the left subtree is at least 2 levels deeper
 	and its mirror image are, unfortunately, handled separately,
 	though they're done identically in fact. This makes this
 	routine twice as big as it really is. I don't believe this
 	algorithm (AVL) can be coded much more cleanly in C.
 ****************************************************************************/

inline void
balance()		/* Looking for params? To be found on the _hash_stk */
{
	hsearchtree <void, void> *tree;
	hsearchtree <void, void> **ptree;

	while (_hash_sp) {
		ptree=&pop;
		tree=*ptree;
		if (tree->l && tree->l->height>=tree->height) {   //adj height
			tree->height++;
#ifdef DEBUG_HASH
printf("hash::balance Gonna balance at %s:\n", tree->key);
//listtree(tree, tree->height);
#endif
			if (heightof(tree->r)+1<tree->l->height) {     //rebalance
				if (!tree->l->r || tree->l->l  //.....&&.....
					 && tree->l->l->height>=tree->l->r->height) {
					*ptree=tree->l;
					tree->l=tree->l->r;
					(*ptree)->r=tree;
					tree->height=heightof(tree->l)+1;
					(*ptree)->height=tree->height+1;
				} else {
					*ptree=tree->l->r;
					tree->l->r=tree->l->r->l;
					(*ptree)->l=tree->l;
					tree->l=(*ptree)->r;
					(*ptree)->r=tree;
					tree->height=heightof(tree->r)+1;
					tree=(*ptree)->l;
					tree->height=heightof(tree->l)+1;
					(*ptree)->height=tree->height+1;
				}
			}
		}
		else if (tree->r && tree->r->height>=tree->height) {
			tree->height++;
#ifdef DEBUG_HASH
printf("hash:: balance Gonna balance at %s:\n", tree->key);
//listtree(tree, tree->height);
#endif
			if (heightof(tree->l)+1<tree->r->height) {     //rebalance
				if (!tree->r->l || tree->r->r  //.....&&.....
					 && tree->r->r->height>=tree->r->l->height) {
					*ptree=tree->r;
					tree->r=tree->r->l;
					(*ptree)->l=tree;
					tree->height=heightof(tree->r)+1;
					(*ptree)->height=tree->height+1;
				} else {
					*ptree=tree->r->l;
					tree->r->l=tree->r->l->r;
					(*ptree)->r=tree->r;
					tree->r=(*ptree)->l;
					(*ptree)->l=tree;
					tree->height=heightof(tree->l)+1;
					tree=(*ptree)->r;
					tree->height=heightof(tree->r)+1;
					(*ptree)->height=tree->height+1;
				
				}
			}
			
		}
		else _hash_sp=0;
	}
}

int _hash_sp = 0;
hsearchtree <void, void> **_hash_stk[_HASH_DEPTH+1];



/****************************************************************************
 This constructor will construct an empty "size"-sized hash table
 ****************************************************************************/

template <class key_t, class data_t>
hash_table<key_t, data_t>::hash_table(int size)
{
#ifdef DEBUG_HASH
printf("hash::hash: sized %d\n", size);
#endif
	dupkey = dupdata = true;
	if (!size) size++;
	capacity=size;
	maxdep=longest=items=0;
	perc_optimal=60;        // i.e. not memory-hungry speed
	cfg_rehash(0,400,5);	// keep the table full ratio 0% - 400%
				// i.e. never downsize, upsize rarely
	ht=(hsearchtree<key_t, data_t> **)xcalloc(capacity,
				sizeof(hsearchtree<key_t, data_t>*));
#ifdef DEBUG_HASH
printf("eoc\n");
#endif	
}

/****************************************************************************
 Copy constructor, keeps its parent's size
 ****************************************************************************/

template <class key_t, class data_t>
hash_table<key_t, data_t>::hash_table(hash_table *parent)
{
	int i;

	dupkey = dupdata = true;
	capacity = parent->capacity;
	ht = (hsearchtree<key_t, data_t> **)xcalloc(capacity,
				sizeof(hsearchtree<key_t, data_t> *));
	maxdep = longest = items = 0;
	perc_optimal = parent->perc_optimal;
	cfg_rehash(parent->perc_too_sparse, parent->perc_too_dense, parent->tree_too_deep);
	for (i=0; i<parent->capacity; i++) rehash_tree(parent->ht[i]);
	
}

/****************************************************************************
 Destructor
 ****************************************************************************/

template <class key_t, class data_t>
hash_table<key_t, data_t>::~hash_table()
{
	int i;
	for(i=0;i<capacity;i++) dissolvetree(ht[i]);
	free(ht);
#ifdef PROF_HASH    
	printf("Releasing a hash table, maximum depth %i\n",maxdep);
#endif    
}

#ifdef HASH_CAN_READ_FILES    //to be defined in default.h

const char* COMMENT_LINES = "#;\n";    //these characters can start a comment
const char* WHITESPACE    = " \t\r";     //i.e. space and tab

int hash_max_line = 1024;

/****************************************************************************
 This constructor will construct a hash table out of a text file "filename". 
 Its potential capacity will be adjusted to the percentage_full specified.
 The parameters are explained in hash.h
 ****************************************************************************/

hash::hash(const char *filename,
		int perc_full, int perc_downsize, int perc_upsize,
		int max_tree_depth, const char * no_data, bool multi_data,
		const char *not_found, char esc(char)): hash_table<char, char>(0)
{
	FILE *hashfile;
	char *buff;
	char *key;
	const char *value;
	int l = 0;
	char *tmp;
	char *dst;
	free(ht);

	dupkey = dupdata = true;
	buff=(char *)xmalloc(hash_max_line);

#ifdef DEBUG_HASH
printf("hash::hash: using file %s\n", filename);
#endif

	maxdep = longest = items = 0;
	if (max_tree_depth == -1) max_tree_depth=_HASH_DEPTH;
	perc_optimal = perc_full;
	
	if((hashfile=fopen(filename, "rt"))==NULL) {
		ht = (hsearchtree<char, char> **)xcalloc(sizeof(int *), 1);
		capacity = 0;
		if (not_found == ANYWAY) {
			items = -1;
			goto fail;
		}
		hash_shriek(not_found ? not_found : "Can't hash in file %s", filename, 0);
	}
	while (fgets(buff, hash_max_line, hashfile)) if (!strspn(buff+strspn(buff,WHITESPACE),COMMENT_LINES)) l++;
	capacity = l*100/perc_full | 1;
	cfg_rehash(perc_downsize, perc_upsize, max_tree_depth);
	ht = (hsearchtree<char, char> **)xcalloc(capacity,sizeof(hsearchtree<char, char>*));
	fseek(hashfile, 0, SEEK_SET);
	l = 0;
	while (l++, fgets(buff, hash_max_line, hashfile)) {
		if (strchr(COMMENT_LINES, *buff)) continue;
		for (tmp = dst = buff; !strchr(COMMENT_LINES, tmp[1]) ||
				!strchr(WHITESPACE, tmp[0]) && tmp[1] && tmp[1] != '\n';
				tmp++, dst++) {
			if (esc && tmp[0] == ESCAPE) *dst = esc(*++tmp);
			else *dst = *tmp;
		}
		*dst = *tmp;
		*++dst = 0;
					
		while (strchr(WHITESPACE, *--dst) && dst>=buff);
		dst[1]=0;			//Strip out comments and/or trailing whitespace
		tmp = key = buff + strspn(buff, WHITESPACE);
	        if (!*key) continue;   		//Nothing but a comment
	        tmp += strcspn(key, WHITESPACE);
		if (*tmp) *tmp++ = 0;		//terminate the key and go on
		value = tmp += strspn(tmp, WHITESPACE);
		if (!*value) switch ((int)no_data) {
			case (int)DATA_EQUALS_KEY: value = key; break;
			case (int)DATA_OBLIGATORY: hash_shriek("No value specified in %s, line %d",filename,l);
			default: value = no_data;
		}
		else if (!multi_data && tmp[strcspn(tmp,WHITESPACE)]) 
			hash_shriek("Multiple values specified in %s, line %d",filename,l);
		add(key, value);
	}
	fclose(hashfile);
   fail:
	free(buff);

#ifdef DEBUG_HASH
printf("hash::hash: successfully returning, file %s\n", filename);
if (_hash_sp) hash_shriek("Hash stack dirty! %s%d", "", _hash_sp);
#endif

}

void
_writetree(char *key, char *data, void *outfile)
{
	fprintf((FILE *)outfile, strcmp(key, data) ? "%s \t%s\n" : "%s\n", key, data);
}

/****************************************************************************
 hash::write - save to file
 	The file may or may not exist. If it exists, only its leading
 	comment lines will be kept. If the second argument is true,
 	or if the process fails, a backup of the original file is kept
 	with a leading tilde in the filename.
 ****************************************************************************/


int
hash::write(char *filename, bool keep_backup)
{
	FILE *old = fopen(filename, "rt");
	if (!old && errno != ENOENT) return 1;
	FILE *outfile;
	char *buff = (char *)xmalloc(hash_max_line);
	char *backup;
	int result;
	
	if (old) {
		FILE *outfile;

		strcpy(buff, "~");
		strcat(buff, filename);
		backup = strdup(buff);
		rename (filename, backup);
		outfile = fopen(filename, "w");
		if (!outfile && errno == ENOMEM) {
			rename(backup, filename);
			return 1;
		}
		fgets(buff, hash_max_line, old);
		while (!feof(old) && strchr(COMMENT_LINES, buff[strspn(buff, WHITESPACE)])) {
			fputs(buff, outfile);
			fgets(buff, hash_max_line, old);
		}
		fclose(old);
		forall(_writetree, outfile);
		result = !fprintf(outfile,"\n");
		fclose (outfile);
	
		if (!keep_backup && !result) remove(backup);
		free(backup);
	} else {
		outfile = fopen(filename, "w");
		if (!outfile && errno == ENOMEM) return 1;
		forall(_writetree, outfile);
		result = !fprintf(outfile,"\n");
		fclose (outfile);
	}
	free(buff);
	return result;
}

/****************************************************************************
 hash::update - update and delete
 	The file should already exist. Update those options that were
 	found in the file and remove them from memory. Append the rest.
 ****************************************************************************/

int
hash::update(char *filename, bool keep_backup, bool remove_removed)
{
	FILE *old = fopen(filename, "rt");
	FILE *outfile;
	char *buff = (char *)xmalloc(hash_max_line);
	char *backup;

	char *tmp;
	char *zero;
	char *key;
	char *val;
	char *appendix;
	char holdchar;
	int result = 0;
	
	if (!old) hash_shriek("Cannot update %s (errno=%d)", filename, errno);

	strcpy(buff, "~");
	strcat(buff, filename);
	backup = strdup(buff);
	rename (filename, backup);

	outfile = fopen(filename, "w");
	if (!outfile && errno == ENOMEM) hash_shriek("Out of memory for update of %s", filename, 0);
	while (fgets(buff, hash_max_line, old)) {
		tmp = buff + strspn(buff, WHITESPACE); key = tmp;
		tmp += strcspn(tmp, WHITESPACE); zero = tmp;
		tmp += strspn(tmp, WHITESPACE);
		
		holdchar = *zero;
		*zero = 0;
		if ((val=translate(key))) {
			appendix = strdup(tmp + strcspn(tmp, WHITESPACE));
			strncpy(tmp, val, hash_max_line + buff - tmp);
			tmp += strlen(tmp);
			strncpy(tmp, *appendix ? appendix : "\n", hash_max_line + buff - tmp);
			free(appendix);
			keydel(remove(key));
		} else if (remove_removed) continue;
		*zero = holdchar;
		if (fputs(buff, outfile)==EOF) result = 1;
	}
	fclose(old);

	forall(_writetree, outfile);

	fclose (outfile);
	
	if (!keep_backup && !result) remove(backup);
	free(backup);
	free(buff);
	return result;
}


#endif              //ifdef HASH_CAN_READ_FILES


/****************************************************************************
 hash::fn - the hash function
 	Exactly one of the conditional lines will actually be used, 
 	the other case is expected to be optimized out.
 ****************************************************************************/

template <class key_t, class data_t>
inline int 
hash_table<key_t, data_t>::fn(const key_t *key)
{
	unsigned int j=0;
	const char *p;
	
	if (key_t_is_string) for (p = (char *)key; *p; p++) j = 23*j+*p;
	else j = keyhash(key);

//	else if (sizeof (key_t)%sizeof(int)) for (p=(char *)key;p<(char *)(key+1);p++) j=73*j+*p;
//	else for (p=(char *)key;p<(char *)(key+1);p+=sizeof(int)) j=233+*(unsigned int *)p;

#ifdef POWER_OF_TWO
	return(j & hash_fn_mask);
#else
	return(j % capacity);
#endif
}


/****************************************************************************
 hash::add
 ****************************************************************************/


template <class key_t, class data_t>
void
hash_table<key_t, data_t>::add(const key_t *key, const data_t *value)     //if present, replace addit. data
{
#ifdef DEBUG_HASH
printf("hash::add \"%s\" to \"%s\"\n",key,value);
if (_hash_sp) hash_shriek("Hash stack dirty! %s%d","",_hash_sp);
#endif
	register int result;
	hsearchtree<key_t, data_t> *tree;
	*_hash_stk=UNTYPED(ht+fn(key));_hash_sp=1;
	for(UNTYPED_LVALUE(tree)=(**_hash_stk);tree;) {
		if(!(result=keycmp(key, tree->key))) {          //key already there
#ifdef DEBUG_HASH
printf("hash::add %s already there, old value \"%s\"\n",key, tree->data);
#endif		
			datadel(tree->data);
			tree->data=datadup(value);
			_hash_sp=0;
			return;
		}
		tree=(hsearchtree<key_t, data_t> *)*push(result>0?&tree->l:&tree->r);
	}
	if(key_t_is_string && longest < (result=strlen((char *)key)) && strncmp((char *)key, "!META_", 6)) longest=result;

	/* "tree=new hsearchtree;" would confuse DMALLOC */
	tree=(hsearchtree <key_t, data_t> *)tree_alloc();
	
	pop=(hsearchtree<void, void> *)tree;
	tree->l=tree->r=NULL; tree->height=0;
	tree->key=keydup(key);
	tree->data=value==NULL?(data_t *)NULL:datadup(value);
	if (_hash_sp>maxdep) maxdep=_hash_sp;
	balance();    //rebalances the nodes found on the _hash_stk stack
#ifdef DEBUG_HASH
printf("hash::add is OK\n");
#endif
	if (items++>max_items || maxdep>=tree_too_deep) rehash();
}

void hash::add_int(const char *key, int value)
{
	char buff[MAX_DIGITS+1];
	char *i;
	buff[MAX_DIGITS]=0;
	i=buff+MAX_DIGITS-1;
	char negative=value<0;
	if (negative) value *=-1;
	do {
		*i=(char)((unsigned int)value%10 + '0');
		value = (unsigned int)value / 10;
		i--;
	} while (value);
	if (negative) *i='-'; else i++;
#ifdef DEBUG_HASH
printf("hash::add_int %s to '%s'\n",key,i);
#endif
	add(key, (char *)i); 
}

/****************************************************************************
 hash::remove
 ****************************************************************************/


template <class key_t, class data_t>
data_t *
hash_table<key_t, data_t>::remove(const key_t *key)     //returns the data (NULL if absent)
{
#ifdef DEBUG_HASH
printf("hash::remove \"%s\"\n",key);
//if (_hash_sp) hash_shriek("Hash stack dirty! %s%d","",_hash_sp);
#endif
	register int result;
	register data_t *val;
	hsearchtree <key_t, data_t> **tree;
	hsearchtree <key_t, data_t> *here;	
	hsearchtree <key_t, data_t> **xchg;
	hsearchtree <key_t, data_t> *there;	
	for (tree=(ht+fn(key));*tree && (result=keycmp(key,(*tree)->key));) 
		push(tree),tree=&(result>0?(*tree)->l:(*tree)->r);
	here=*tree;
	if (!here) {
		_hash_sp=0;
		return KEY_NOT_FOUND;
	}
#ifdef DEBUG_HASH
printf("hash::remove %s is there, value \"%s\"\n",key, (*tree)->data);
#endif		
	keydel(here->key);
	val=here->data;
	while (here->l && here->r) {	// "if" should be enough
		for (xchg=&here->l; (*xchg)->r; xchg=&(*xchg)->r) push(xchg);
		there=*xchg;
		here->key=there->key;
		here->data=there->data;
		here=there;
		tree=xchg;
	}
	if (here->l) *tree=here->l; else *tree=here->r;
	while (_hash_sp) {
		UNTYPED_LVALUE(there)=pop;
		if      (there->l && there->l->height>=there->height) there->height++;
		else if (there->r && there->r->height>=there->height) there->height++;
		else _hash_sp=0;	// if no changes there, break out.
	}
	tree_delete (here);
#ifdef DEBUG_HASH
printf("Rehash? (%d of %d)\n",items, min_items);
#endif
	if (items--<min_items) rehash();
	return val;
}	
	
/****************************************************************************
 hash::fortree - auxiliary function for forall()
 ****************************************************************************/

			
template <class key_t, class data_t>
void 
hash_table<key_t, data_t>::fortree(hsearchtree<key_t, data_t> *tree,
				void usefn(key_t *key, data_t *value, void *parm),
				void *parm)
{
	if (tree!=NULL) {
		fortree(tree->r,usefn, parm);
		usefn(tree->key,tree->data, parm);
		fortree(tree->l,usefn, parm);
	}
}

/****************************************************************************
 hash::forall - do something for every item
 ****************************************************************************/


template <class key_t, class data_t>
void 
hash_table<key_t, data_t>::forall(void usefn(key_t *key, data_t *value, void *parm), void *parm)
{
	int i;
	for(i = 0; i < capacity; i++) fortree(ht[i],usefn, parm);
}

/****************************************************************************
 hash::dissolvetree - aux function for the destructor
 ****************************************************************************/


template <class key_t, class data_t>
void 
hash_table<key_t, data_t>::dissolvetree(hsearchtree<key_t, data_t> *tree)
{
	if (tree!=NULL) {
		dissolvetree(tree->l);
		dissolvetree(tree->r);
		keydel(tree->key);
		datadel(tree->data);
		tree_delete(tree);
	}
}


/****************************************************************************
 hash::get_random - find a random key in the table; some colliding keys may
	never come up
 ****************************************************************************/

template <class key_t, class data_t>
key_t *
hash_table<key_t, data_t>::get_random()
{
	static int seed;	/* move seed to class hash_table, if used wide and wild */
	if (seed >= capacity) {
		if (!items) return NULL;
		seed %= capacity;
	}
	while (!ht[seed]) if (++seed == capacity) seed = 0;
	return ht[seed]->key;
}


/****************************************************************************
 hash::translate - find a key in the table, return its associated data
 ****************************************************************************/


template <class key_t, class data_t>
data_t *
hash_table<key_t, data_t>::translate(const key_t *key)
{
	register int result;
	hsearchtree<key_t, data_t> **tree;
#ifdef DEBUG_HASH
printf("hash::translate %s\n",key);
#endif
	for(tree=(ht+fn(key));(*tree);) {
#ifdef DEBUG_HASH
printf("hash::translate compares tree holding %s to %s\n",(*tree)->key, (*tree)->data);
#endif
		if(!(result=keycmp(key,(*tree)->key))) return((*tree)->data);
		if(result>0) tree=&((*tree)->l); else tree=&((*tree)->r);
	}
	return(KEY_NOT_FOUND);
}

int
hash::translate_int(const char *key)
{
	char *result;
	char *i;
	signed char sign=1;
	unsigned int to_return=0;
	if(!(result=(char *)translate(key))) return INT_NOT_FOUND;
	if (*result=='-') result++,sign=-1;
	if (*result=='+') result++;
	for(i=result;*i;i++) to_return=to_return*10+*i-'0';
	return (int)to_return*sign;
}

/****************************************************************************
 hash::rehash - rebuild the hash table, because its size has become
 	inadequate during hash::add (too small) or hash::remove (too 
 	loose). The new size will be deduced from perc_optimal.
 	
 	This will about never get called if you don't like it, as it
 	can make a single add/remove request very time-consuming.
 	It can sometimes be quite useful if properly configured with
 	cfg_rehash();
 ****************************************************************************/


template <class key_t, class data_t>
void
hash_table<key_t, data_t>::rehash(int new_capacity)
{
	int capold=capacity;
	hsearchtree<key_t, data_t> **htold=ht;
	int ttdold=tree_too_deep;
	int i;
	
#ifdef DEBUG_HASH
printf("hash::rehash Gonna rehash from %d to %d for %d items\n", capold, new_capacity, items);
//debug();
#endif

#ifdef POWER_OF_TWO
	hash_fn_mask = new_capacity - 1;
	if (new_capacity & hash_fn_mask) hash_shriek("Not a power of two %d","",new_capacity);   // :-(
#endif
	capacity=new_capacity;
	ht=(hsearchtree<key_t, data_t> **)xcalloc(capacity, sizeof(hsearchtree<key_t, data_t> *));
	cfg_rehash(0, ((unsigned)-1)/2-1,_HASH_DEPTH); //infinity (no rehash()ing in rehash(), please)
	longest=items=maxdep=0;
	
	for (i=0; i<capold; i++) {
		rehash_tree(htold[i]);
		dissolvetree(htold[i]);
	}
	free(htold);
	cfg_rehash(perc_too_sparse, perc_too_dense, ttdold);
}

template <class key_t, class data_t>
void
hash_table<key_t, data_t>::rehash()
{
#ifdef POWER_OF_TWO
	int i, j=1;
	for (i=items*100/perc_optimal; i; i>>=1) j<<=1;
	rehash(j);
#else
	rehash(items*100/perc_optimal|1);
#endif
}

template <class key_t, class data_t>
void
hash_table<key_t, data_t>::rehash_tree(hsearchtree<key_t, data_t> *t)
{
	if (!t) return;
	rehash_tree(t->l);
	rehash_tree(t->r);
	add(t->key, t->data);
}

template <class key_t, class data_t>
void
hash_table<key_t, data_t>::cfg_rehash(int new_minimum, int new_maximum,int new_max_tree_depth)
{
#ifdef POWER_OF_TWO
	int j = 1;
	for (; capacity; capacity>>=1) j<<=1;
	capacity = j>>1;
	hash_fn_mask = capacity - 1;
#endif
	if (new_minimum>=perc_optimal) new_minimum=perc_optimal-1;
	if (new_maximum<=perc_optimal) new_maximum=perc_optimal+1;
	perc_too_sparse=new_minimum;	
	min_items=capacity*perc_too_sparse/100 - DOWNSIZE_HESITATE;
	perc_too_dense=new_maximum;
	max_items=capacity*perc_too_dense/100;
	tree_too_deep = new_max_tree_depth;
}

void
hash::debug()
{
	int i;
	printf("hash::debug dump follows\n");
	for(i=0;i<capacity;i++) listtree(ht[i], 0);
	printf("hash::debug done.\n");
}


//	forall(_hdump);


/****************************************************************************
 Various little nothings
 ****************************************************************************/

void
hash::listtree(hsearchtree <char, char> *tree, int indent)
{
	if (tree!=NULL) {
		for(int i=0; i<indent; i++) printf("   ");
		printf("%s %s %d\n",(char *)tree->key,(char *)tree->data, tree->height);
		if (tree->l && tree->r && tree->height!=tree->l->height+1 &&
		        tree->height!=tree->r->height+1 ||
			!tree->r && tree->l && tree->l->height+1!=tree->height ||
			!tree->l && tree->r && tree->r->height+1!=tree->height )
//			hash_shriek("Bad height %s%d","",tree->height);
		listtree(tree->r, indent+1);
		listtree(tree->l, indent+1);
//		if (tree->l && tree->r && abs(tree->l->height-tree->r->height)>1)
//			hash_shriek("Both children, but bad %s%d","",tree->height);
	}
}

#undef unconst

#undef key_t_is_string 
#undef data_t_is_string
#undef keyhash
#undef keycmp
#undef keydup
#undef datadup

#undef push
#undef pop
#undef heightof
