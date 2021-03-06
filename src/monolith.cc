/*
 *	epos/src/monolith.cc
 *	(c) 1996-01 geo@cuni.cz
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

#include "common.h"

const int is_monolith = 1;

#ifdef HAVE_UNISTD_H		// fork() only
	#include <unistd.h>
#else
	int fork();
#endif

// int session_uid = 0;

int submain()
{
	unit *root;

	root = str2units(cfg->input_text);
	this_lang->ruleset->apply(root);
//	fprintf(stdout,"*********************************************\n");
	root->fout(NULL);

	if (cfg->show_segs | cfg->play_segs | cfg->imm_segs | cfg->show_phones) {
		if (cfg->play_segs) {
			if (cfg->forking) {
				switch (fork()) {
					case 0:	//ds_used_cnt++;
						play_segments(root, this_voice);
						return 0;
					case -1:play_segments(root, this_voice);
					default:;
				}
			} else play_segments(root, this_voice);
		}
		if (cfg->show_phones) root->show_phones();
		if (cfg->show_segs) show_segments(root);
	}

	if (cfg->neuronet) root->nnet_out(cfg->nnet_file, cfg->matlab_dir);
	delete(root);
//	fprintf(stdout,"***** The End. ******************************\n");
	return 0;
}

int main(int argc, char **argv)
{
	try {
		argc_copy = argc, argv_copy = argv;
		epos_init();
		submain();
		epos_done();
		return 0;
	} catch (any_exception *e) {
		unuse(e->code);
		return 4;
	}
}

