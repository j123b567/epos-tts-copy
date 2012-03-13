/*
 *	epos/src/unit.h
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
 *	This file defines the "main" structure we use as the internal structure
 *	for representing the text we're going to process. One instance of this 
 *	class can be a segment, phone, syllable, ... , whole text (see UNIT
 *	defined in common.h), depending on its "depth". Its contents is a
 *	bi-directional linked list between "firstborn" and "lastborn"; all of
 *	its elements have their "depth" lower by one. These and maybe other
 *	assumptions about the structure can be found in unit::sanity().
 *
 *	Note that the neural network stuff is implemented in nnet.cc.
 */

#define JUNCTURE           '0'    // scope boundary in assim environment
#define DELETE_ME     JUNCTURE    // changing cont to this one is fatal

#define RATIO_TOTAL	   100	  // 100 % (unit::smooth percent sum)

// #define MAX_GATHER       16384          // Maximum word size (for buffer allocation)
// #define MAX_GATHER       16          // Maximum word size (for buffer allocation)

#define SMOOTH_CQ_SIZE    16		// max smooth request length, see unit::smooth()


class unit
{
	friend void epos_catharsis();	  // necessary only #ifdef WANT_DMALLOC
	friend class r_inside;

	unit *next, *prev;                //same layer
	unit *firstborn, *lastborn;       //layer lower by one
	unit *father;                     //layer greater by one
	UNIT depth;        // 0=segment 1=phone 2=syllable 3=word...
	int  cont;         // content (or terminating character)
	void insert_begin(unit *member, unit*to);         //insert a train of units
	void insert_end(unit *from, unit *member);        //insert as the last member
				      //if NULL, shrieks unintuitively
	void set_father(unit *new_fath);  //brothers will do that as well
	void fdump(FILE *outf);           //only for debugging
	void nnet_dump(FILE *outf);
	float nnet_type();
    
	inline bool subst(hash *table, int total_length,
		char*s1b,char*s1e,char*s2b,char*s3b,char*s3e); //inner, see implem.
	void syll_break(char *sonority, unit *before);
	void syllabify(char *sonority);  //May split "father" just before "this", if sonority minimum
	void sseg(hash *templates, char symbol, int *quantity);
	void seg(hash *segm_inventory);  //Will create up to one segment. Go see it if curious.
    
	void sanity();                    //check that this unit is sanely linked to the others
	void insane(const char *token);   //called exclusively by sanity() in case of a problem
    
	int f,i,t;
    public:
	bool scope;                       //true=don't pass on Next/Prev requests
		unit(UNIT layer, parser *);
		unit(UNIT layer, int content); 
		unit();               //(empty unit) constructor
		~unit();
	int  write_segs(segment *whither, int starting_at, int max);
                                      //Writes the segments out to an array of
                                      // struct segment. Returns: how many written
                                      // starting_at==0 for the first segment
	void show_phones();	      // printf() the phones
	void nnet_out(const char *filename, const char *dirname);
	void fout(char *filename);        //stdout if NULL
	void fprintln(FILE *outf);        //does not recurse, prints cont,f,i,t
	char *gather(char *buffer_start, char *buffer_end, bool suprasegm);
	char *gather(int *l, bool delimited, bool suprasegm);	// returns length in *l
             // gather() returns the END of the string (which is unterminated!)
        void insert(UNIT target, bool backwards, char what, charclass *left, charclass *right);
//	void subst(UNIT target, hash *table, SUBST_METHOD method);
	inline void subst();          //replace this unit by sb
	bool subst(hash *table, SUBST_METHOD method);
	bool relabel(hash *table, SUBST_METHOD method, UNIT target);				      
#ifdef WANT_REGEX
	void regex(regex_t *regex, int subexps, regmatch_t *subexp, const char *repl);
#endif
	void assim(UNIT target, bool backwards, charxlat *fn, charclass *left, charclass *right);
	void split(unit *before);         //Split this unit just before "before"
                                      //not too robust
	void syllabify(UNIT target, char *sonority);
                                      // Will split units (syllables),
                                      // according to sonority[cont] of "target"
                                      // units (phones) contained there
	bool contains(UNIT target, charclass *set);
        void sseg(UNIT target, hash *templates);
        			      // Take freq, time or intensity from the hash*
	void contour(UNIT target, int *recipe, int rec_len,
			int padd_start, FIT_IDX what, bool additive);
        void smooth(UNIT target, int *ratio, int base, int len, FIT_IDX what);
        void project(UNIT target, int f, int i, int t);
        void raise(charclass *what, charclass *when, UNIT whither, UNIT whence);
        			      // Move characters between levels
	void segs(UNIT target, hash *segm_inventory);
                                      //Will create the segments
	void unlink(REPARENT rmethod);//Delete this unit, possibly reparenting children 
	int  forall(UNIT target, bool userfn(unit *patiens));
				      //^how many times applied
				      // userfn ^ whether applied
	int effective(FIT_IDX which_quantity);  //evaluate total F, I or T
	inline unsigned char inside();
	unit *ancestor(UNIT level);   // the unit (depth level) wherein this lies
	int  index(UNIT what, UNIT where);
	int  count(UNIT what);
                                      //The following four ones use indirect recursion
	unit *RightMost(UNIT target);     //If no targets inside, will try Next->RightMost 
	unit *LeftMost(UNIT target);      //  (or Prev->LeftMost); if no targets within 
                                      //  the scope, returns EMPTY
	unit *Next(UNIT target);          //If no targets follow (or precede) within 
	unit *Prev(UNIT target);          //  the current scope, returns EMPTY

	void *operator new(size_t size);
	void operator delete(void *ptr);

	static char *gb;		// gather buffer
	static int gbsize;
	static char *sb;		// subst buffer
	static int sbsize;
	static void done();		// free buffers
	static void assert_sbsize(int);	// at least that big sbsize
};

// extern char * _subst_buff;
// extern char * _gather_buff;
extern unit * _unit_just_unlinked;

void shutdown_units();
