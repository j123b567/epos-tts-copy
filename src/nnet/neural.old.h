/** 	@file neural.h
	@author Jakub Adamek jakubadamek@bigfoot.com
	@date 5/11/2001
	@version 1.0

	@brief This file implements a neuralnet description language. 

	The syntax of the language is desribed in the neural.y bison source file.

	In the current version only feedforward multi-layer perceptron networks are supported.
	But I hope the code is written transparently enough you can easily add some another network architecture.
*/

//This file contains markups which allow the Doxygen documentation generator to work on it

/*
 *	epos/src/neural.h
 *	(c) 2000-2001 jakubadamek@bigfoot.com
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

//Intendation optimized for tab width: 8 (vim command: set tabstop=8)

#ifndef EPOS_NEURAL_H
#define EPOS_NEURAL_H

#include "common.h"
#include "../../../bang/bang3/base/bang.h"
#include <stdlib.h>
#include <math.h>

const signed int MSG_LENGTH = 250;

///Functions available for describing neuralnet inputs 
enum enum_func { 
	fu_nothing, 	///< empty node
	fu_value,	///< leaf node containing a typed value (as defined farther)
	fu_chartofloat,	///< float myfunction (unit *) .. user-defined char to float function

	/** @name func1
	    Basic mathematical functions
	*/
	//@{
	
	fu_add,		///< +
	fu_subtract, 	///< -
	fu_multiply, 	///< *
	fu_divide,	///< / 

	//@}
	
	//@{
	/** Functions operating with units:
	    These functions always operate on the calling unit or on ancestors of it.
		 Epos has it's Text Structure Representation which consists of units organized in levels. E.g. each word-unit has it's ancestor sentence-unit. See more in the Epos documentation.
	 */
	
	/** int count (string level, string up_level), e.g. count("syll","sent")
		 Count of syllables in the sentence. */
	fu_count, 			
	
	/** int index (string level, string up_level), e.g. index("phone","word")
		 Order of the phone in the word starting with 1.			*/
	fu_index, 
	fu_this,	///< unit *this .. the calling unit itself. Useful only to apply a user-defined chartofloat function. 
	
	/** unit *next (string level, int how_many), e.g. next ("syll",2)
		 Syllable two to the right.
		 If there are no more syllables there, it returns the EMPTY unit. */
	fu_next, 			
	fu_prev,	///< like next, but in the opposite direction (to the left)
	
	/** unit *ancestor (string level), e.g. ancestor("sent")
		 Ancestor on the sentence level. */
	fu_ancestor,	
	
	/** unit *sonor (string level) finds the most sonorous 
	(and if there are more possibilities, the rightmost) 
	phone on given level */
	fu_sonor,

	//@}

	//@{
	/** <b>Basic logical functions:</b> */

	fu_equals,	///< logical == .. 2 == 2
	fu_notequals,	///< logical != .. 3 != 2
	fu_greater, 	///< logical > .. 3 > 2
	fu_less, 	///< logical < .. 2 < 3
	fu_and, 	///< logical && .. 2 == 2 && 3 == 3
	fu_or, 		///< logical || .. 2 < 3 || 3 < 2
	fu_not,		///< logical ! .. !(2 > 3)
	fu_lessorequals,///< logical <= .. 2 <= 2
	fu_greaterorequals, ///< logical >= .. 5 >= 1
	
	//@}

	/** Special function: int basal_f (unit *) - f0 of given unit */

	fu_f0,

	/** Content of unit */
	fu_cont
};

///User-defined function from char to float. The @b cont of some unit is used as the char.

class t_chartofloat 
{
public:
	static const float CHARTOFLOAT_NULL;
	
	float val[256]; 	///<each char has its value
	float empty;	 	///<what to return when called on an empty unit
	char *name;	 	///<user-defined name
	t_chartofloat ();
	~t_chartofloat ();
private:
  //these two functions are here just to avoid copying of objects of this class (see Stroustroup: Programovaci jazyk C++, p. 245)
  t_chartofloat& operator= (const t_chartofloat&); 
  t_chartofloat(const t_chartofloat&);
};

VECTOR (t_chartofloat, t_chartofloats)

class t_neuralnet;
class typed_value;

/// Expression tree: describes the expression by which a neuralnet input will be calculated
/** The main work - creating the tree - is done in the bison input neural.y
  		
  	 A node may contain:
	 <UL>
	 	 <LI>a constant only - than it is a leaf - or
		 <LI>a function (or user-defined char-to-float) AND (during the evaluation time) a constant - result of the function - than it is "this" function or not a leaf	
	 </UL>	 
*/ 

class t_nnetfunc_tree
{
public:
	friend t_nnetfunc_tree *add_prefix_func (enum_func, const char *, t_nnetfunc_tree *);
	friend t_nnetfunc_tree *make_tree (typed_value val);

	t_neuralnet *owner;			///<Used to get the chartofloat user-function definitions
  	t_nnetfunc_tree *brother,  ///<Next parameter of the father function
		  				*son,			///<First parameter of this function
						*father;		
	int birth_order; 				///<First parameter (or root) .. 0, second parameter ..  1 etc.
  	enum_func function;			///<Which function does this node represent 
	
	typed_value *v ()			{ return which_value; }			
	
	///Type-secure access to the chartofloat index
	int i_chartofloat ()	
	{	if (function == fu_chartofloat) return which_chartofloat; 
		else {
			shriek (812,"t_nnetfunc_tree::i_chartofloat called on non-chartofloat node"); 
			return -1; }}

  	t_nnetfunc_tree (t_neuralnet *owner);
	~t_nnetfunc_tree ();
	void erase_tree ();			///<erases all the sons and brothers - prepares the node to be destroyed

	int new_son ();				///<creates the son (firstborn) and fills appropriate pointers of both the father and the son
  	int new_brother ();			///<creates brother (next parameter) and fills appropriate pointers of both brothers
	void insert_node_on_my_place (t_nnetfunc_tree *node); ///<makes the "node" father of me, edits pointers (lots of)
	
	void write (FILE *file = stdout);	///<writes contents of the tree
	float calculate (unit *);				///<Calculates the expression on given unit.

	/// @ingroup tree_to_list
	///  Prints the list form of the tree
	void write_list (FILE *file = stdout);

private:
	void write (int level, FILE *file = stdout);		//writes with indent appropriate to level .. used by public @b write
	
	int which_chartofloat;			///<Used when function=fu_chartofloat
	typed_value *which_value;		///<Used when function=fu_value

	/** @defgroup tree_to_list Converting the tree to list
 		 In the list there each node is calculated only after calculating its subnodes (therefore the list begins which some leaf). 
		 This way it allows to go on without any recursion, which spares all the function calling time.	
		 @{
	*/
	t_nnetfunc_tree *head;			///<the head of the whole list (some leaf)
	t_nnetfunc_tree *next;			///<next node
	void make_list ();				///<creates the whole list 
	t_nnetfunc_tree *make_list (t_nnetfunc_tree **head); ///<recursively called on sons and brothers. Returns head of the list.
	void write_list_node (FILE *); ///<recursively writes the rest of the list

	/// @}

	//these two functions are here just to avoid copying of objects of this class (see Stroustroup: Programovaci jazyk C++, p. 245)
	t_nnetfunc_tree& operator= (const t_nnetfunc_tree&); 
	t_nnetfunc_tree(const t_nnetfunc_tree&);
};

VECTOR (t_nnetfunc_tree, t_nnetfunc_trees)

///What to do with one neuralnet output: where to place it (@b type) and a constant multiplier.

class t_nnet_output
{
public:
	///Tells the network where to place output after calculating it:
	enum enum_type { 
		OT_NONE, 		///<discards the output - does nothing with it
		OT_FREQUENCE 	///<overwrites the value of the basal frequence by the output
	};
	enum_type type; 
	int multiply; ///<you can multiply the network output 
};

static const int MAX_COUNT_CHARTOFLOAT = 100;
static const int MAX_COUNT_INPUT = 100; 

///Main and top class. Implementation of the whole neural network.
/** Main part of the neuralnet rule. */

class t_neuralnet
{
public:
	friend int yyparse (void *neuralnet); 			///<BISON parser
	/** @defgroup neuralnet_friends Bison friend functions:
	    Some functions used by the Bison parser have to be declared friends.
		 @{
	*/
	friend t_nnetfunc_tree *add_prefix_func (enum_func, const char *, t_nnetfunc_tree *); 
	friend void read_weight_or_bias_file (char *filename, bool bias);
	friend class t_nnetfunc_tree;
	friend void add_chartofloat (const char *name);
  	/// @}

	/// The constructor only fills initial values - the configuration file is parsed in t_neuralnet::init
  
	t_neuralnet (const char *my_filename, hash *my_vars); 
  	void init (); 											///<Fills the structures with the config file contents. Called from the constructor.
	void run (unit *);									///<Runs the network on some unit and places appropriately the output.
	
	~t_neuralnet ();

	enum enum_transfer_function { TF_LOGSIG, TF_PURELIN };

private:
	/** The output depends on the debugging level set for cfg: if it is less than 2, the output file will contain outputs from all layers.
		 These two variables are used to place the input coming to the net and the outgoing output. */

	char *outfilename, *infilename;	 			
	int format_decdigits [MAX_COUNT_INPUT]; ///< count of decimal digits printed for each input in input log
  	bool initialized;			///< helps to avoid multiple initialization
	char log_level_input;			///< level the units of which will be written into input log

	///possible neuron transfer functions:
	double logsig (double x) { return (1 / (1 + exp (-x))); }
	double *layer1,*layer2; 		///<used when computing the net output in t_neuralnet::run

	t_nnet_output *output;			///<output count is in layer_size[n_layer]

	void fill_input (unit *, double *input_val);			///<fills input_val with numbers calculated from given unit

	hash *vars; 													///<e.g. $voiced = bdïgvz¾Z®hø; got from next_rule in block.cc
	
	t_chartofloats chartofloats;					///<user-defined chartofloats
	/** If sonority is defined, which chartofloat to use? */
	int i_chartofloat_sonority;

	char *filename;												///<config file
  	t_nnetfunc_tree (*inputtree [MAX_COUNT_INPUT]); 	///<tree describing expressions for each input neuron
  	int n_input;													///<count of inputs

private:
  //these two functions are here just to avoid copying of objects of this class (see Stroustroup: Programovaci jazyk C++, p. 245)
  t_neuralnet& operator= (const t_neuralnet&); 
  t_neuralnet(const t_neuralnet&);
};

///Helps to manage different result types and parameter types.
/** 	A nice type-secure typed union, see Stroustrup: Programovaci jazyk C++, p. 175.
  		Contains overtyped operators = and type casting operators. You can use the value in place of a string, int, char, unit, bool, float. 
		
		You have to use the type casting when it is ambiguous whether to convert to more than one type: e.g. when writing <code>val1 / val2</code> ... you have to write e.g. <code>(float) val1 / (float) val2<code>.

		Has built-in type conversion, which allows only sencefull conversions (like int to float) and doesn't allow another ones (like float to int).
*/

class typed_value
{
private:
	union {
		/// a physical copy of the string - to be deleted
		char *string_val;
		int int_val;
		float float_val;
		/// only a reference to a TSR unit
		unit *unit_val;
		char char_val;
		bool bool_val;
	};

	//As it is an inline function, the conversion is as quickly as possible:
	void try_convert(char type) { if (!convert (type)) 
		shriek (861,"typed_value: demanding non-convertable value type"); }
	char value_type;

public:
	typed_value () : value_type (0) {}; 
	typed_value (const float x)	{ *this = x; }
	typed_value (const int x)	{ *this = x; }
	typed_value (unit *x)		{ *this = x; }
	typed_value (const char *x)	{ *this = x; }
	typed_value (const bool x)	{ *this = x; }
	typed_value (const char x)	{ *this = x; }

	typed_value(const typed_value&x);
	typed_value& operator= (const typed_value&x); 

	~typed_value ();

	// throws away contents
	void clear ();
	// fills with contents from another typed value
	void init (const typed_value &x);

	char get_value_type () const { return value_type; }

	void print (FILE * = stdout);

	/** @defgroup typed_val Overtyping 
		 @{ 
	*/

	operator const char * (){ try_convert ('s'); return string_val; }
	operator int ()		{ try_convert ('i'); return int_val; }
	operator float ()	{ try_convert ('f'); return float_val; }
	operator unit * ()	{ try_convert ('u'); return unit_val; }
	operator char ()	{ try_convert ('c'); return char_val; }
	operator bool ()	{ try_convert ('b'); return bool_val; }

	/// @}

	/** @defgroup typed_val Assigning values
		 @{ 
	*/

	typed_value& operator= (const float x)		{ value_type = 'f'; float_val = x; return (*this); }
	typed_value& operator= (const int x)		{ value_type = 'i'; int_val = x; return (*this); }
	typed_value& operator= (const char x)		{ value_type = 'c'; char_val = x; return (*this); }
	typed_value& operator= (const bool x)		{ value_type = 'b'; bool_val = x; return (*this); }
	typed_value& operator= (unit *x)		{ value_type = 'u'; unit_val = x; return (*this); }
	typed_value& operator= (const char *x)		{ value_type = 's'; string_val = strdup(x); return (*this); }
	
	/// @}
	
	/** Tries to convert to another type. Returns whether succeeded.
	  * Used in calculate_input, thus should be very fast, thus inline.
	*/
	
	bool convert (char new_type) //result: true = OK, false = not convertable
	{
		if (new_type == value_type) return (true);
		switch (new_type) {
			case 'b':
				switch (value_type) {
					case 'c': bool_val = char_val != 0; break;
					case 'i': bool_val = int_val != 0; break;
					case 'f': bool_val = float_val != 0; break;
					default: return (false);
				}
			case 'c':
				switch (value_type) {
					case 'b': char_val = (char) bool_val; break;
					default: return (false);
				}
				break;
			case 'i':
				switch (value_type) {
					case 'b': int_val = (int) bool_val; break;
					case 'c': int_val = (int) char_val; break;
					default: return (false);
				}
				break;
			case 'f':
				switch (value_type) {
					case 'b': float_val = (float) bool_val; break;
					case 'c': float_val = (float) char_val; break;
					case 'i': float_val = (float) int_val; break;
					default: return (false);
				}
				break;
			default:
				return (false);
		}
		value_type = new_type;
		return (true);
	}

};

#endif
