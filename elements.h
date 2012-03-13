/*
 *	ss/src/elements.h
 *	(c) geo@ff.cuni.cz
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
 *	class can be a diphone, phone, syllable, ... , whole text (see UNIT
 *	defined in common.h), depending on its "depth". Its contents is a
 *	bi-directional linked list between "firstborn" and "lastborn"; all of
 *	its elements have their "depth" lower by one. These and maybe other
 *	assumptions about the structure can be found in unit::sanity().
 *
 *	Note that the neural network stuff is implemented in nnet.cc.
 */


#define MAX_GATHER       256            // Maximum word size (for buffer allocation)
#define SSEG_QUESTION_LEN 32	        // see unit::sseg()
#define SMOOTH_CQ_SIZE    16		// max smooth request length, see unit::smooth()
#define OMEGA          10000            // Random infinite number, see unit::diph()

class unit
{
	friend void ss_catharsis();	  // necessary only #ifdef WANT_DMALLOC
	friend class r_inside;

	unit *next, *prev;                //same layer
	unit *firstborn, *lastborn;       //layer lower by one
	unit *father;                     //layer greater by one
	UNIT depth;        // 0=diphone 1=phone 2=syllable 3=word...
	int  cont;         // content (or terminating character)
	void insert_begin(unit *member, unit*to);         //insert a train of units
	void insert_end(unit *from, unit *member);        //insert as the last member
				      //if NULL, shrieks unintuitively
	void set_father(unit *new_fath);  //brothers will do that as well
	void fdump(FILE *outf);           //only for debugging
	void nnet_dump(FILE *outf);
	float nnet_type();
    
	inline bool subst(hash *table, unsigned int safe_grow,
		char*s1b,char*s1e,char*s2b,char*s3b,char*s3e); //inner, see implem.
	void syll_break(char *sonority, unit *before);    //Implements the side-syllable hack
	void syllablify(char *sonority);  //May split "father" just before "this", if sonority minimum
	void sseg(hash *templates, char symbol, int *quantity);
	void diph(hash *diph_inventory);  //Will create up to one diphone. Go see it if curious.
    
	void sanity();                    //check that this unit is sanely linked to the others
	void insane(const char *token);   //called exclusively by sanity() in case of a problem
    
	int f,i,t;
    public:
	bool scope;                       //true=don't pass on Next/Prev requests
		unit(UNIT layer, parser *);
		unit(UNIT layer, int content); 
		unit();               //(empty unit) constructor
		~unit();
	int  write_diphs(diphone *whither, int starting_at, int max);
                                      //Writes the diphones out to an array of
                                      // struct diphone. Returns: how many written
                                      // starting_at==0 for the first diphone
	void nnet_out(const char *filename, const char *dirname);
	void fout(char *filename);        //stdout if NULL
	void fprintln(FILE *outf);        //does not recurse, prints cont,f,i,t
	char *gather(char *buffer_start, char *buffer_end, bool suprasegm);
             // gather() returns the END of the string (which is unterminated!)
        void insert(UNIT target, bool backwards, char what, bool *left, bool *right);
//	void subst(UNIT target, hash *table, SUBST_METHOD method);
	inline void subst(char *subst_buff);  //replace this one by _subst_buff
	bool subst(hash *table, SUBST_METHOD method);
	bool relabel(hash *table, SUBST_METHOD method, UNIT target);				      
#ifdef WANT_REGEX
	void regex(regex_t *regex, int subexps, regmatch_t *subexp, const char *repl);
#endif
	void assim(UNIT target, bool backwards, char *fn, bool *left, bool *right);
                                      //Will convert cont using fn[256] if left[Next]
                                      // and right[Prev] are both true. Backwards=regressive. 
	void split(unit *before);         //Split this unit just before "before"
                                      //not too robust
	void syllablify(UNIT target, char *sonority);
                                      // Will split units (syllables),
                                      // according to sonority[cont] of "target"
                                      // units (phones) contained there
        void sseg(UNIT target, hash *templates);
        			      // Take freq, time or intensity from the hash*
	void contour(UNIT target, int *recipe, int rec_len, FIT_IDX what, bool additive);
        void smooth(UNIT target, int *ratio, int base, int len, FIT_IDX what);
        void project(UNIT target, int f, int i, int t);
        void raise(bool *what, bool *when, UNIT whither, UNIT whence);
        			      // Move characters between levels
	void diphs(UNIT target, hash *diph_inventory);
                                      //Will create the diphones
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
};

extern char * _subst_buff;
extern unit * _unit_just_unlinked;

