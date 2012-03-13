/*
 *	ss/src/ss.cc
 *	(c) 1996-98 geo@ff.cuni.cz
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
 *	Take this as an example how to call the rest of code.
 *	Delete, rewrite, edit.
 */

#include "common.h"

#ifdef HAVE_UNISTD_H		// fork() only
	#include <unistd.h>
#else
	int fork();
#endif

int session_uid = 0;

int submain()
{
	unit *root;
	parser *parsie;

	check_lib_version(VERSION);
	parsie = cfg->input_text && *cfg->input_text ? new parser(cfg->input_text, 1)
		: new parser(this_lang->input_file, 0);
	root=new unit(U_TEXT, parsie);
	delete parsie;
	this_lang->ruleset->apply(root);
	fprintf(stddbg,"*********************************************\n");
	root->fout(NULL);

	if (cfg->use_diph) {
		if (cfg->play_diph) {
			if (cfg->forking) {
				switch (fork()) {
					case 0:	//ds_used_cnt++;
						play_diphones(root, this_voice);
						return 0;
					case -1:play_diphones(root, this_voice);
					default:;
				}
			} else play_diphones(root, this_voice);
		}
		if (cfg->show_diph) show_diphones(root);
	}

	if (cfg->neuronet) root->nnet_out(cfg->nnet_file, cfg->matlab_dir);
	delete(root);
	fprintf(stddbg,"***** The End. ******************************\n");
	return 0;
}

int main(int argc, char **argv)
{
	try {
		ss_init(argc, argv);
		ss_reinit();
		/*return */ submain();
		ss_reinit();
		ss_done();
	} catch (exception *e) {
		unuse(e);
		printf("*****************\n");
		return 4;
	}
}
