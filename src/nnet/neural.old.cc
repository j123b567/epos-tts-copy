/** @file neural.cc
	 @author Jakub Adamek
	 @date 11/5/2001
	 @version 1.0

	 @brief This file implements the structures defined in neural.h
*/

//This file contains markups which allow the Doxygen documentation generator to work on it

/*
 *	epos/src/neural.cc
 *	(c) 2000 jakubadamek@bigfoot.com
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
 */
//Optimized for tab width: 3

#ifndef EPOS_NEURAL_CC
#define EPOS_NEURAL_CC

#include "neural.h"
#include "text.h"
#include "common.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <iostream.h>

REGVECTOR (t_chartofloat, t_chartofloats)
REGVECTOR (t_nnetfunc_tree, t_nnetfunc_trees)

extern unit EMPTY; 	//unit.cc

const int MAX_LENGTH_STATEMENT = 250;
float const t_chartofloat::CHARTOFLOAT_NULL = -100;

// name of chartofloat defining sonority in config file
static const char CHARTOFLOAT_SONORITY[] = "sonority";

int neuralparse (void *neuralnet);  //BISON

inline UNIT get_level (const char *level_name)
{
	return str2enum (level_name, cfg->unit_levels, U_DEFAULT);
}

void ERR (char *err, const char *x) 
{ 
	strncat (err,x,MSG_LENGTH-strlen(err)-1);
	if (strlen(err) == (size_t) MSG_LENGTH) err[MSG_LENGTH] = 0;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	t_neuralnet::init
/**
   Does all the job - prepares network to process data, 
	processes the config file. */
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	

CString t_neuralnet::read (CRox &xml) 
{
	CString err;
	CRox *ch;
	int i;
	if (xml ["chartofloats"].Exist()) {
		ch = &xml["chartofloats"];	
		for (i=0; i < ch.NChildren(); ++i)
			if (ch.Child(i).Tag() == "chars") {

		err += xml_read (ch, 

}
<chartofloats>
	<chartofloat name="mytype1">
		<chars src="$diphtong" dst="1.5"/>
		<chars src="$short$long" dst="2.5"/>
		<chars src="$sonant$SONANT" dst="2.0"/>
		<chars src="$voiced" dst="2.0"/>
		<chars src="$voiceless" dst="2.0"/>
		<chars src="." dst="2.0"/>
		<chars src="," dst="2.0"/>
		<chars src="?" dst="2.0"/>
		<chars src="!" dst="2.0"/>
		<chars src="'" dst="2.0"/>
		<default dst="0"/><comment src="vsechny ostatní znaky"/>
		<empty dst="3.0"/><comment src="neexistujici jednotky - napr. predchozí od první; default: 0"/>
	</chartofloat>
	<chartofloat name="sentence">
		<chars src="." dst="2.5"/>
		<chars src="?" dst="1.5"/>
		<chars src="!" dst="1.0"/>
		<default dst="8"/>
	</chartofloat>
	<chartofloat name="sonority">
		<chars src="$diphtong" dst="1"/>
		<chars src="$long" dst="0.9"/>
		<chars src="$short" dst="0.8"/>
		<chars src="$sonant$SONANT" dst="0.7"/>
		<chars src="$voiced" dst="0.6"/>
		<chars src="$voiceless" dst="0.5"/>
		<default dst="0"/>
		<empty dst="0"/>
	</chartofloat>
</chartofloats>



void
t_neuralnet::init ()
{
	if (initialized) return;
	
	//Preparing the two output files: one for numbers coming to the net and one for the outcoming ones:
/*
	if (outfilename) {
		FILE *outfile = fopen (outfilename, "w");
		if (!outfile) shriek (812,fmt ("t_neuralnet::init:Could not open output file %s. Error %u.", outfilename, errno));
		fprintf (outfile, "Neural network output:\n");
		DBG(1,10,fprintf(STDDBG,"%s file open for output.\n",outfilename));
		fclose (outfile);
	};
	
	if (infilename) {
		FILE *infile = fopen (infilename, "w");
		if (!infile) shriek (812,fmt ("t_neuralnet::init:Could not open file %s for neuralnet inputs. Error %u.", infilename, errno));
		fprintf (infile, "Neural network input values:\n");
		DBG(1,10,fprintf(STDDBG,"%s file open for output.\n",infilename));
		fclose (infile);
	};
*/	
/* * * * * * * * * * * * *  */
/* Running the BISON parser */
	nnet->filename;
	Bang::XMLFile xmlFile;
	xmlFile.setfile (nnet->filename);
	read (*xmlFile.parse());

	neuralparse (this);
/* * * * * * * * * * * * *  */ 

	for (int i_input=0; i_input < n_input; ++i_input) {
		if (!inputtree[i_input]) 
			shriek (812, fmt ("t_neuralnet::init:Input no. %i wasn't described. You should describe ALL inputs in a range, e.g. 0,1,2,3,4.", i_input));
		DBG (1,10,inputtree[i_input]->write_list (STDDBG));
	}

	for (int i=0; i < n_chartofloat; ++i)
	if (!strcmp (chartofloat[i]->name, CHARTOFLOAT_SONORITY))
		i_chartofloat_sonority = i;
				
	DBG (1,10,
		for (int i=0; i < n_chartofloat; ++i) {
			fprintf(STDDBG,"%s: ", chartofloat[i]->name);
			for (int c=0; c < 255; ++c)
				if (chartofloat[i]->val[c] != t_chartofloat::CHARTOFLOAT_NULL) 
					fprintf(STDDBG,"%c > %.2f; ", c, chartofloat[i]->val[c]);
			fprintf(STDDBG,"\n");
		}
	);
	
	if (!n_layer) {
		n_layer = 0;
		layer_size = new int [1];
		layer_size[0] = n_input;
	}

	int max_size = 0;
	for (i=0;i<=n_layer;++i) 
		if (max_size < layer_size[i]) max_size = layer_size[i];
	layer1 = new double [max_size];
	layer2 = new double [max_size];

	if (n_layer && !output) 
		shriek (812,"t_neuralnet::init:Output types must be set in configuration file.");

	DBG(2,10,fprintf(STDDBG,"Neuralnet initialized.\n"));
	initialized = true;
} //t_neuralnet::init

// * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	t_neuralnet::run
/**
	Contains the implementation of the network computing algorithm.

	Operates on feedforward multi-layer perceptron network only. */
// * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void t_neuralnet::run (unit *myunit)
{
/*int f0_o1d1a[] =
	{ 88, 145, 144, 88, 142, 124, 84, 84, 104, 97, 95, 85, 75, 75, 71 };

	int f0_o1d1a_phones[] =
	{ 80,76,88,94,94,94,145,147,140,144,117,88,86,74,142,93,124,105,118,
	115,76,81,84,78,78,104,94,94,97,95,95,85,88,85,79,75,75,75,108,71 };

	int f0_o1d2a[] =
	{ 90,141,137,120,121,129,104,100,171,143,107,72,92,89,76,74,76 };

	int f0_o1d2a_phones[] = {
	77,90,110,141,141,137,121,126,120,112,116,121,100,129,132,104,104,100,100,171,
		171,143,98,81,107,110,86,72,70,100,92,85,85,89,80,76,76,74,71,65,65};*/

	int max_size=0;
	int i;

	for (i=0;i<=n_layer;++i) 
		if (max_size < layer_size[i]) max_size = layer_size[i];
	
	fill_input (myunit, layer1);
	/*//if (layer1[8] <= 41)
		myunit->f = f0_o1d2a [int(layer1 [8])-1] - layer1[11];
	myunit->t=layer1[9];
	myunit->i=layer1[10];*/

	// If no layers are set, we only want the input data to form training data
	if (!n_layer) return;

	FILE *outfile = NULL;
	if (outfilename) {
		outfile = fopen (outfilename, "a");
		if (!outfile) shriek (812,"t_neuralnet::run:Cannot open file for output.");
	}

	//The feedforward perceptron algorithm:

	double *tmp;
	int i1,i2;
	for (int i_layer = 1; i_layer <= n_layer; ++i_layer) {
		DBG (1,10,if (outfilename) fprintf(outfile,"\nLayer %i:",i_layer));
		for (i2=0; i2<layer_size[i_layer]; ++i2) {
			layer2[i2] = 0;
			for (i1=0; i1<layer_size[i_layer-1]; ++i1) 
				layer2[i2] += layer1[i1] * weight[i_layer][i2][i1];
			layer2[i2] += bias[i_layer][i2];
			switch (transfer_function [i_layer]) {
				case TF_PURELIN: break;
				case TF_LOGSIG: layer2[i2] = logsig(layer2[i2]); break;
			}
			DBG (1,10,if (outfilename) fprintf (outfile,"%.2f ",layer2[i2]));
		}
		tmp = layer1;
		layer1 = layer2;
		layer2 = tmp;
	}

	// Placing the output:

	for (i=0;i<layer_size[n_layer];++i) {
		layer1[i] *= output[i].multiply;
		switch (output[i].type) {
			case t_nnet_output::OT_FREQUENCE: 
				myunit->f = int(layer1[i]); break;	//overwrite the value of frequence
			case t_nnet_output::OT_NONE: break;		//do nothing
			default: shriek (812,"t_neuralnet::run:Program uncomplete.");
		}
		if (outfile) fprintf (outfile,"Unit: %c Output: %f ",myunit->cont,layer1[i]);
	}
	if (outfile) {
		fprintf(outfile,"\n");
		fclose (outfile);
	}
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	t_neuralnet::fill_input
/** 
   Prepares the inputs for the input layer.
	Calls calculate on all the input trees. */ 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void t_neuralnet::fill_input (unit *myunit, double *input_val)
{
	FILE *infile = NULL;
	if (infilename) {
		infile = fopen (infilename, "a");
		if (!infile) shriek (812,"t_neuralnet::fill_input:Cannot open file for output.");
	}

	if (!initialized || !input_val) shriek (861, "t_neuralnet::fill_input:Neuralnet not initialized");
	if (log_level_input > -1 && infilename) {
		unit *subunit = myunit->LeftMost (log_level_input);
		for (int i_unit = 0; i_unit < myunit->count (log_level_input); ++i_unit, subunit = subunit->Next(log_level_input))
			fprintf (infile, "%c", subunit->cont);
		fprintf (infile, "\t");
	}
	for (int i_input = 0; i_input < n_input; ++i_input) {
		input_val [i_input] = inputtree [i_input]->calculate (myunit);
		if (infile) fprintf (infile,fmt("%s%df ","%.",format_decdigits[i_input]),input_val [i_input]);
	}
	if (infile) {
		fprintf (infile, "\n");
		fclose (infile);
	}
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	t_nnetfunc_tree::calculate 
/**
	Calculates one input tree - all nodes will have values.
	Contains implementations of all the functions defined in enum_func. */	
// * * * * * * * * * * * * * * * * * * * * * * * * * * * */

float
t_nnetfunc_tree::calculate (unit *myunit)
{
	float f;
	unit *pomunit, *iunit;
	int i;
	t_chartofloat *mychtof;

#define par0 (*tree->son->v())
#define par1 (*tree->son->brother->v())
#define par2 (*tree->son->brother->brother->v())
#define float0 float (par0)
#define float1 float (par1)
#define bool0 (float (par0) != 0)
#define bool1 (float (par1) != 0)
#define int0 int (par0)
#define int1 int (par1)
#define int2 int (par2)
#define result *tree->v()

	if (head == NULL) make_list ();

	for (t_nnetfunc_tree *tree = head; tree; tree = tree->next) {
		switch (tree->function) {
			case fu_value: break;
			case fu_not: 		result = ! bool0; break;
			case fu_multiply:	result = float0 * float1; break;
			case fu_divide:		if (!bool1) shriek (812,"t_neuralnet::calculate_input:divide by zero.");
						result = float0 / float1; break;
			case fu_add: 		result = float0 + float1; break;
			case fu_subtract:	result = float0 - float1; break;
			case fu_less:		result = float0 < float1; break;
			case fu_lessorequals:	result = float0 <= float1; break;
			case fu_greater:	result = float0 > float1; break;
			case fu_greaterorequals:result = float0 >= float1; break;
			case fu_equals:		result = float0 == float1; break;
			case fu_notequals:	result = float0 != float1; break;
			case fu_and:		result = bool0 && bool1; break;
			case fu_or:		result = bool0 || bool1; break;
			case fu_count:
				result = myunit->ancestor( par1 )->count( par0 ); break;
			case fu_index:
				result = 1 + myunit->index( par0, par1 ); break;
			case fu_this:
				result = myunit; break;
			case fu_f0:
				result = myunit->getF(); break;
			case fu_cont:
				result = ((unit *)par0)->getCont(); break;
			case fu_next:
			case fu_prev:
			{
				pomunit = par0;
				int level;
				if (!par1.get_value_type()) 
					level = static_cast<const unit *>(par0)->getDepth();
				else level = par1;
				if (tree->function == fu_next)
					for (i=0; i < int2; ++i)
						pomunit = pomunit->Next(level);
				else 
					for (i=0; i < int2; ++i)
						pomunit = pomunit->Prev(level);
				result = pomunit; 
				break;
			}
			case fu_ancestor:
				result = myunit->ancestor (par0); break;
			case fu_chartofloat: 
				if (par0 == &EMPTY) 
					f = owner->chartofloat[tree->i_chartofloat()]->empty;
				else {
					f = owner->chartofloat[tree->i_chartofloat()]->val[static_cast<const unit *>(par0)->getCont()]; 	
					if (f == t_chartofloat::CHARTOFLOAT_NULL) 
						shriek (812, fmt ("t_neuralnet::calculate_input:on char '%c' is not %s defined",static_cast<const unit *>(par0)->getCont(), 
									owner->chartofloat[tree->i_chartofloat()]->name));
				}
				result = f;
			 	break;
			case fu_sonor: 
			{
				pomunit = myunit->ancestor (par1);
				float maxsonor = t_chartofloat::CHARTOFLOAT_NULL;
				
				if (owner->i_chartofloat_sonority < 0)
					shriek (861, fmt ("You must define the %s chartofloat when you use sonority function.",CHARTOFLOAT_SONORITY));
				mychtof = owner->chartofloat[owner->i_chartofloat_sonority];
				int count = pomunit->count (par0);
				pomunit = pomunit->LeftMost (par0);
				int i;
				for (iunit = pomunit, i=0; i < count; ++i, iunit = iunit->Next(par0))
					if (mychtof->val[iunit->getCont()] > maxsonor) {
						maxsonor = mychtof->val[iunit->getCont()];
						pomunit = iunit;
					}
				result = pomunit;
				break;
			}
			default:
				shriek (861, fmt ("t_neuralnet::calculate_input:non-handled function %i - add to the source code", tree->function));
		}

	}

	return (*v());
}//calculate_input		

t_neuralnet::t_neuralnet (const char *my_filename, hash *my_vars)
{
  filename = strdup (my_filename); 
  outfilename = NULL;
  infilename = NULL;
  vars = my_vars;
  initialized = false;
  n_chartofloat = 0;
  i_chartofloat_sonority = -1;
  n_input = 0;
  n_layer = 0;
  weight = NULL;
  bias = NULL;
  layer_size = NULL;
  for (int i=0; i<MAX_COUNT_CHARTOFLOAT; ++i)
	  chartofloat [i] = NULL;
  for (i=0; i<MAX_COUNT_INPUT; ++i) {
	  inputtree [i] = NULL;
	  format_decdigits [i] = 1;
  }
  weight = NULL;
  transfer_function = NULL;
  output = NULL;
  layer1 = NULL;
  layer2 = NULL;
  log_level_input = -1;

  init ();
}

t_neuralnet::~t_neuralnet ()
{
	free (filename);
	int i;
	for (i=0; i<n_chartofloat; ++i) 
		if (chartofloat[i]) delete chartofloat[i];
	for (i=0; i<n_input; ++i)
		if (inputtree [i]) inputtree[i]->erase_tree ();

	for (i=1; i<=n_layer; ++i) {
		if (bias[i]) delete[] bias[i];
		for (int j=0; j<=layer_size [i]; ++j)
			if (weight[i][j]) delete[] weight[i][j];
		if (weight[i]) delete[] weight[i];
	}
	if (bias) delete[] bias;
	if (weight) delete[] weight;
	if (layer_size) delete[] layer_size;
	if (transfer_function) delete[] transfer_function;

	if (outfilename) free (outfilename);
	if (infilename) free (infilename);
	if (output) delete[] output;
	if (layer1) delete[] layer1;
	if (layer2) delete[] layer2;
}

t_chartofloat::t_chartofloat ()
{
	for (int c = 0; c < 256; ++c) val [c] = CHARTOFLOAT_NULL;
	name = NULL;
	empty = 0;
}

t_chartofloat::~t_chartofloat ()
{
	delete[] name;
}

void
t_nnetfunc_tree::write_list (FILE *file)
{
	if (head == NULL) make_list ();
	head->write_list_node (file);
	fprintf (file,"\n");
}

void
t_nnetfunc_tree::write_list_node (FILE *file)
{
	switch (function) {
		case fu_nothing: fprintf(file,"~ "); break;
		case fu_value: v()->print(file); fprintf(file," "); break;
		case fu_chartofloat: fprintf(file,"%s ",owner->chartofloat[i_chartofloat()]->name); break;
		default:	fprintf(file," func.%u ",(int) function);
	}

	if (next) next->write_list_node (file);
}	

t_nnetfunc_tree *
t_nnetfunc_tree::make_list (t_nnetfunc_tree **new_head)
{
	if (father) next = *new_head;
	*new_head = this;
	if (son) son->make_list (new_head);
	if (brother) brother->make_list (new_head);
	head = *new_head;
	return (head);
}

void 
t_nnetfunc_tree::make_list ()
{
	t_nnetfunc_tree *new_head = this;
	head = make_list (&new_head);
}

int 
t_nnetfunc_tree::new_son ()
{
	if (son) return (0);
	son = new t_nnetfunc_tree (owner);
	son->father = this;
	son->birth_order = 0;
	return (1);
}

void
t_nnetfunc_tree::insert_node_on_my_place (t_nnetfunc_tree *node)
{
	node->birth_order=birth_order; 
	birth_order = 0;
	node->brother = brother; 
	brother = NULL;
	node->father = father;		
	father = node;
	if (!node->birth_order) node->father->son = node;
	node->son = this;
}

int 
t_nnetfunc_tree::new_brother ()
{
	if (brother) return (0);
	brother = new t_nnetfunc_tree (owner);
	brother->father = this->father;
	brother->birth_order = this->birth_order + 1;
	return (1);
}

t_nnetfunc_tree::t_nnetfunc_tree (t_neuralnet *my_owner) 
{
	head = NULL;
	next = NULL;
	owner = my_owner;
	brother = NULL;
	son = NULL;
	father = NULL;
	birth_order = 0;
	function = fu_nothing;
	which_value = new typed_value ();
}

t_nnetfunc_tree::~t_nnetfunc_tree ()
{
	delete which_value;
}

typed_value::typed_value(const typed_value&x)
{
	init (x);
}

typed_value & typed_value::operator= (const typed_value&x)
{
	init (x);
	return *this;
}

void typed_value::init (const typed_value &x)
{
	value_type = x.value_type;
	switch (value_type) {
		case 0: break;
		case 'i': int_val = x.int_val; break;
		case 'f': float_val = x.float_val; break;
		case 'c': char_val = x.char_val; break;
		case 'b': bool_val = x.bool_val; break;
		case 's': string_val = x.string_val; break;
		case 'u': unit_val = x.unit_val; break;
		default: shriek (861, "typed_value:Type not handled in init.");
	}
}

typed_value::~typed_value ()
{ 
	clear ();
}

void typed_value::clear ()
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
		default: shriek (861, fmt ("typed_value:Type %c (%i) not handled in destructor.",value_type, value_type));
	}
	value_type = 0;
}

void
t_nnetfunc_tree::erase_tree () 
{
	if (son) son->erase_tree();
	if (brother) brother->erase_tree();
	delete this;
}

void
t_nnetfunc_tree::write (FILE *file) 
{
	fprintf (file, "Tree content:\n");
	write (0);
}

void
t_nnetfunc_tree::write (int level, FILE *file) 
{	
	for (int i=0; i<level; ++i) fprintf (file,"...");
	switch (function) {
		case fu_nothing: fprintf (file,"~"); break;
		case fu_value: break;
		case fu_chartofloat: fprintf (file,"%s\t\t", owner->chartofloat[i_chartofloat()]->name); break;
		default: fprintf (STDDBG,"Function %i ?! ", function);
	}
	switch (function) {
		case fu_nothing: break;
		default:
			v()->print(file); break;
	}
	fprintf (file, "\n");
	
	if (son) son->write (level+1, file);
	if (brother) brother->write (level, file);
}


void
typed_value::print (FILE *file)
{
	switch (value_type) {
		case 's': fprintf (file, "%s", string_val); break;
		case 'f': fprintf (file, "%.2f", float_val); break;
		case 'i': fprintf (file, "%i", int_val); break;
		case 'c': fprintf (file, "%c", char_val); break;
		case 'b': fprintf (file, "%s", bool_val ? "true" : "false"); break;
		case 'u': fprintf (file, "unit var %u", unsigned (unit_val)); break;
		default: fprintf (file, "Cannot print value type %c", value_type);
	}
}

#endif
