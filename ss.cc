/*
 *	ss/src/ss.cc
 *	(c) 1996-98 geo@ff.cuni.cz
 *
 *	Take this as an example how to call the rest of code.
 *	Delete, rewrite, edit.
 */

#include "common.h"

#ifdef HAVE_UNISTD_H		// fork() only
	#include <unistd.h>	// in DOS, fork(){return -1;} is supplied in ss.cpp
#endif

int main(int argc, char **argv)
{
	unit *root;
	PARSER *parsie;
	rules *ruleset;

	check_lib_version(VERSION);
	ss_init(argc, argv);
	parsie = cfg.input_text ? new PARSER(cfg.input_text, 1)
		: new PARSER(cfg.input_file, 0);
	root=new unit(U_TEXT, parsie);
	delete parsie;
	ruleset=new rules(cfg.rules_file, 0);  //int gets ignored
	ruleset->apply(root);
	delete ruleset;
	fprintf(stddbg,"*********************************************\n");
	root->fout(NULL);

	if (cfg.use_diph) {
		dsynth *ds = new dsynth(cfg.inv_type, cfg.inv_counts,
			cfg.inv_models, cfg.inv_book, cfg.inv_dpt,
			cfg.inv_f0, cfg.inv_i0, cfg.inv_t0, cfg.inv_hz);
//		int ds_used_cnt = 0;
		if (cfg.play_diph) {
			if (cfg.forking) {
				switch (fork()) {
					case 0:	//ds_used_cnt++;
						play_diphones(root, ds);
						delete ds;
						return 0;
					case -1:play_diphones(root, ds);
					default:;
				}
			} else play_diphones(root, ds);
		}
		if (cfg.show_diph) show_diphones(root, ds);
		delete ds;
	}

	if (cfg.neuronet) root->nnet_out(cfg.nnet_file, cfg.matlab_dir);
	delete(root);
	fprintf(stddbg,"***** The End. ******************************\n");
	ss_done();
	return 0;
}
