/*
 *	ss/src/options.cc
 *	(c) 1997-98 geo@ff.cuni.cz
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
 *	This file is the only place where the individual options are defined.
 *	It gets included thrice in interf.h & interf.cc, to produce first 
 *	the declaration, then the definition, and finally, the array of options
 *	to be parsed by load_config().
 *	
 *	Note that you should #define exactly one of the following:
 * 
 *	CONFIG_DECLARE, CONFIG_INITIALIZE, CONFIG_DESCRIBE,
 *	CONFIG_INV_DECLARE, CONFIG_INV_INITIALIZE, CONFIG_INV_DESCRIBE
 *	CONFIG_LANG_DECLARE, CONFIG_LANG_INITIALIZE, CONFIG_LANG_DESCRIBE
 *
 *	when this file is included and any of these flags will be undefined
 *	again when this file is over. They will cause OPTION to be expanded
 *	to a declaration and initialization of the respective member of
 *	struct configuration, and to its corresponding item in the optlist
 *	(which drives the type dependent parsing of the member). INV_OPTION
 *	expands to OPTION and, moreover, it will setup a struct voice member
 *	in an analogous way. The same for LNG_OPTION and struct lang.
 *
 *	This scheme (the first three flags) was introduced in version 1.1.6.
 *
 *	The format is obvious. The last field is the default value. 
 */

#ifdef CONFIG_DECLARE

#define BOOL   bool
#define INT    int
#define STR    const char*
#define CHAR   Char
#define ELEM   UNIT
#define FILE   const char*
#define MARKUP OUT_ML
#define TYPE   SYNTH_TYPE
#define CHANNEL    CHANNEL_TYPE
#define DEBUG_AREA _DEBUG_AREA_

#define OPTION(member,name,type,default) type member;

#define OPTIONAGGR(x) x;
#define OPTIONITEM(w,x,y,z) 
#define OPTIONAGGRENDS 

#define LNG_OPTIONAGGR(x) OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS OPTIONAGGRENDS

#define INV_OPTION(member,member_inv,name,type,default) OPTION(member,name,type,default)
#define LNG_OPTION(member,member_lang,name,type,default) OPTION(member,name,type,default)

#undef CONFIG_DECLARE

#else                 //#ifdef CONFIG_INITIALIZE or CONFIG_DESCRIBE or CONFIG_INV*

#ifdef CONFIG_INITIALIZE
#define OPTION(member,name,type,default) cfg->member=default,

#define OPTIONAGGR(x) {
#define OPTIONITEM(w,x,y,z) OPTION (w,x,y,z)
#define OPTIONAGGRENDS },

#define LNG_OPTIONAGGR(x) OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS OPTIONAGGRENDS

#define INV_OPTION(member,member_inv,name,type,default) OPTION(member,name,type,default)
#define LNG_OPTION(member,member_lang,name,type,default) OPTION(member,name,type,default)

#undef CONFIG_INITIALIZE

#else                 //#ifdef CONFIG_DESCRIBE or CONFIG_INV*
#ifdef CONFIG_DESCRIBE

#define BOOL   O_BOOL
#define INT    O_INT
#define STR    O_STRING
#define CHAR   O_CHAR
#define ELEM   O_UNIT
#define FILE   O_FILE
#define MARKUP O_MARKUP
#define TYPE   O_SYNTH
#define CHANNEL    O_CHANNEL
#define DEBUG_AREA O_DBG_AREA

#define OPTION(member,name,type,default) {name, type, OS_CFG, A_PUBLIC, A_PUBLIC, (int)&((configuration *)NULL)->member},

#define OPTIONAGGR(x) 
#define OPTIONITEM(w,x,y,z) OPTION (w,x,y,z)
#define OPTIONAGGRENDS 

#define LNG_OPTIONAGGR(x) OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS OPTIONAGGRENDS

#define INV_OPTION(member,member_inv,name,type,default) OPTION(member,name,type,default)
#define LNG_OPTION(member,member_lang,name,type,default) OPTION(member,name,type,default)

#undef CONFIG_DESCRIBE


#else
#ifdef CONFIG_LANG_INITIALIZE

#define OPTIONAGGR(x)
#define OPTIONITEM(w,x,y,z)
#define OPTIONAGGRENDS

#define LNG_OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) LNG_OPTION(w,w,x,y,z)
#define LNG_OPTIONAGGRENDS

#define OPTION(w,x,y,z)
#define INV_OPTION(member,member_inv,name,type,default)   member_inv = cfg->member;
#define LNG_OPTION(member,member_lang,name,type,default)  member_lang = cfg->member;

#undef CONFIG_LANG_INITIALIZE

#else
#ifdef CONFIG_LANG_DECLARE

#define BOOL   bool
#define INT    int
#define STR    const char*
#define CHAR   Char
#define ELEM   UNIT
#define FILE   const char*
#define MARKUP OUT_ML
#define TYPE   SYNTH_TYPE
#define CHANNEL    CHANNEL_TYPE
#define DEBUG_AREA _DEBUG_AREA_

#define OPTIONAGGR(x)
#define OPTIONITEM(w,x,y,z)
#define OPTIONAGGRENDS

#define LNG_OPTIONAGGR(x) x;
#define LNG_OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS

#define OPTION(w,x,y,z)
#define INV_OPTION(member,member_inv,name,type,default)    type  member_inv;
#define LNG_OPTION(member,member_lang,name,type,default)   type  member_lang;

#undef CONFIG_LANG_DECLARE

#else
#ifdef CONFIG_LANG_DESCRIBE

#define BOOL   O_BOOL
#define INT    O_INT
#define STR    O_STRING
#define CHAR   O_CHAR
#define ELEM   O_UNIT
#define FILE   O_FILE
#define MARKUP O_MARKUP
#define TYPE   O_SYNTH
#define CHANNEL    O_CHANNEL
#define DEBUG_AREA O_DBG_AREA

#define OPTIONAGGR(x)
#define OPTIONITEM(w,x,y,z)
#define OPTIONAGGRENDS

#define LNG_OPTIONAGGR(x) 
#define LNG_OPTIONITEM(member,name,type,default) {name, type, OS_LANG, A_PUBLIC, A_PUBLIC, (int)&((lang *)NULL)->member}, 
#define LNG_OPTIONAGGRENDS 

#define OPTION(w,x,y,z)
#define INV_OPTION(member,member_inv,name,type,default)   {name, type, OS_LANG, A_PUBLIC, A_PUBLIC, (int)&((lang *)NULL)->member_inv},
#define LNG_OPTION(member,member_lang,name,type,default)  {name, type, OS_LANG, A_PUBLIC, A_PUBLIC, (int)&((lang *)NULL)->member_lang},

#undef CONFIG_LANG_DESCRIBE


#else
#ifdef CONFIG_INV_INITIALIZE

#define OPTIONAGGR(x)
#define OPTIONITEM(w,x,y,z)
#define OPTIONAGGRENDS

#define LNG_OPTIONAGGR(x) OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS OPTIONAGGRENDS

#define OPTION(w,x,y,z)
#define INV_OPTION(member,member_inv,name,type,default)  member_inv = parent_lang->member_inv;
#define LNG_OPTION(member,member_lang,name,type,default)

#undef CONFIG_INV_INITIALIZE

#else
#ifdef CONFIG_INV_DECLARE

#define BOOL   bool
#define INT    int
#define STR    const char*
#define CHAR   Char
#define ELEM   UNIT
#define FILE   const char*
#define MARKUP OUT_ML
#define TYPE   SYNTH_TYPE
#define CHANNEL    CHANNEL_TYPE
#define DEBUG_AREA _DEBUG_AREA_

#define OPTIONAGGR(x)
#define OPTIONITEM(w,x,y,z)
#define OPTIONAGGRENDS

#define LNG_OPTIONAGGR(x) OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS OPTIONAGGRENDS

#define OPTION(w,x,y,z)
#define INV_OPTION(member,member_inv,name,type,default)   type  member_inv;
#define LNG_OPTION(member,member_inv,name,type,default)

#undef CONFIG_INV_DECLARE

#else
#ifndef CONFIG_INV_DESCRIBE
#error Impossible inclusion of config.cc, wheee...
#endif

#define BOOL   O_BOOL
#define INT    O_INT
#define STR    O_STRING
#define CHAR   O_CHAR
#define ELEM   O_UNIT
#define FILE   O_FILE
#define MARKUP O_MARKUP
#define TYPE   O_SYNTH
#define CHANNEL    O_CHANNEL
#define DEBUG_AREA O_DBG_AREA

#define OPTIONAGGR(x)
#define OPTIONITEM(w,x,y,z)
#define OPTIONAGGRENDS

#define LNG_OPTIONAGGR(x) OPTIONAGGR(x)
#define LNG_OPTIONITEM(w,x,y,z) OPTIONITEM(w,x,y,z)
#define LNG_OPTIONAGGRENDS OPTIONAGGRENDS

#define OPTION(w,x,y,z)
#define INV_OPTION(member,member_inv,name,type,default)  {name, type, OS_VOICE, A_PUBLIC, A_PUBLIC, (int)&((voice *)NULL)->member_inv},
#define LNG_OPTION(member,member_lang,name,type,default)

#undef CONFIG_INV_DESCRIBE

#endif                //CONFIG_INV_DECLARE
#endif                //CONFIG_INV_INITIALIZE
#endif                //CONFIG_LANG_DESCRIBE
#endif                //CONFIG_LANG_DECLARE
#endif                //CONFIG_LANG_INITIALIZE
#endif                //CONFIG_DESCRIBE
#endif                //CONFIG_INITIALIZE
#endif                //CONFIG_DECLARE





/*** it is important that INV_OPTION copy on write goes first:   ***/
INV_OPTION (cow, cow, "",	  BOOL, false)	// copy on write (this cfg struct: is shared)

OPTION (use_dbg,    "use_debug",  BOOL, false)  // Whether to print any debug info at all

OPTION (interf_dbg, "interf_debug",INT, 2)      // Debugging levels of various
OPTION (rules_dbg,  "rules_debug", INT, 2)      // code categories. See debug.doc
OPTION (elem_dbg,   "elem_debug",  INT, 2)      // 
OPTION (subst_dbg,  "subst_debug", INT, 2)      // 
OPTION (assim_dbg,  "assim_debug", INT, 2)      // 
OPTION (split_dbg,  "split_debug", INT, 2)      // 
OPTION (parser_dbg, "parser_debug",INT, 2)      // 
OPTION (synth_dbg,  "synth_debug", INT, 2)	//
OPTION (cfg_dbg,    "cfg_debug",   INT, 4)	//
OPTION (daemon_dbg, "daemon_debug",INT, 2)	//

OPTION (focus_dbg,  "focus_debug", DEBUG_AREA, _NONE_) 
						// _DEBUG_AREA_ not affected by limit_debug 
OPTION (always_dbg, "always_debug",INT, 3)      // Always debug this level and upwards
OPTION (limit_dbg,  "limit_debug", INT, 0)      // Never debug under this level, unless focussed

OPTION (loaded, "",               BOOL, false)  // Do we have already compiled the .ini files?
OPTION (ssfixed, "ssfixed_file",   STR, "ssfixed.ini")
OPTION (inifile, "cfg_file",       STR, "ss.ini")	// What is our favourite .ini file?
OPTION (token_esc, "",             STR, "nt[E\\ #;")	// Escape sequences usable in the .ini files
OPTION (value_esc, "",             STR, "\n\t\033\033\\\377#;")
OPTION (slash_esc, "",            CHAR, '/')	// Path separator ('/' or '\') .ini escape seq

OPTION (hash_full, "hashes_full",  INT, 100)    // How full should a hash table become?
OPTION (multi_subst,"multi_subst", INT, 100)    // How many substs should apply to a word?
OPTION (lowmemory, "memory_low",  BOOL, false)  // Should we free hash tables after first use?
OPTION (forking, "forking",	  BOOL, false)  // (UNIX only) Speak using a child process?
OPTION (colored, "colored",       BOOL, false)  // Use the color escape sequences?
OPTION (languages, "languages",     STR, "")	// Which language to simulate?
// OPTION (inventory, "inventory",    STR, "")	// What voice (diphone inventory) to use?
OPTION (ml, "markup_language",  MARKUP, ML_NONE)// Are these ANSI escape seqs or the RTF ones?
OPTION (version, "version",	  BOOL, false)	// Print version info to stdwarn on startup?
OPTION (help, "help",		  BOOL, false)	// Print help on stdwarn and exit on startup?
OPTION (long_help, "long_help",   BOOL, false)	// Print also a simple list of long options?
OPTION (neuronet, "neuronet",	  BOOL, false)  // Allow nnet_out()?
OPTION (trusted, "trusted",       BOOL, false)  // Are sanity checks in unit::sanity() unnecessary?
OPTION (paranoid, "paranoid",     BOOL, true)   // Are config files out to get us? (strict syntax)
OPTION (showrule, "show_rule",    BOOL, false)  // Print each rule before its application? DEBUGGING only!
OPTION (pausing, "pausing",       BOOL, false)  // Pause after each rule application?
OPTION (r_dbg_sh_all, "verbose",  BOOL, false)  // When dumping rules, print them all?
OPTION (warnpause, "warn_pause",  BOOL, false)  // Pause after every warning?
OPTION (warnings, "warnings",     BOOL, false)	// Show warnings at all?
OPTION (allpointers,"ptr_trusted",BOOL, true)   // When sanity checking, allow any pointers?

LNG_OPTION (colloquial, colloquial, "colloquial", BOOL, false)  // Colloquial pronunciation?
LNG_OPTION (irony, irony, "irony",	          BOOL, false) // Ironical intonation?

OPTION (vars, "variables",         INT, 43)     // About how many variables in the rules?
OPTION (rules, "rules_in_block",   INT, 32)	// About how many rules in a block?
OPTION (mrw, "max_rule_weight",    INT, 900000)	// Maximum rule weight in a choice?
OPTION (max_line, "max_line_len",  INT, 512)	// How long may an input line be?
OPTION (scratch, "scratch_size",   INT, 512)	// How long may some temporary strings be?
OPTION (dev_txtlen,"dev_text_len", INT, 50000)  // How much data can we expect from a device?

OPTION (eof_char, "end_of_file",  CHAR, '\033') // Which key should terminate user input?

OPTION (base_dir, "base_dir",      STR, "/usr/lib/ss")    // base path to everything
LNG_OPTION (rules_dir, rules_dir, "rules_dir", STR, ".")  // path to the rules and banner files
LNG_OPTION (hash_dir, hash_dir, "hash_dir",     STR, ".") //  to the dictionaries
LNG_OPTION (input_dir, input_dir, "input_dir", STR, ".")  //  to the input file
INV_OPTION (invent_dir, inv_dir, "invent_dir", STR, ".")  //  to the diphone inventories
LNG_OPTION (lang_dir, lang_dir, "lang_dir",    STR, ".")  //  to the language descriptions
LNG_OPTION (pros_dir, pros_dir, "prosody_dir", STR, ".")  //  to the prosody files
OPTION (ini_dir, "ini_dir",	   STR, "cfg")  //      to ssfixed.ini
OPTION (matlab_dir, "matlab_dir",  STR, ".")	//      where to store nnet output
OPTION (wav_dir, "wav_dir",        STR, ".")	//      where to store .wav files
LNG_OPTION (input_file, input_file, "input_file",  STR, NULL)   // relative filename of the input text
OPTION (input_text, "",            STR, NULL)			// the input text itself
LNG_OPTION (rules_file, rules_file, "rules_file",  STR, NULL)   // relative filename of the rules text
OPTION (nnet_file, "nnet_file",    STR, NULL)	// relative filename of nnet output
OPTION (stddbg, "stddbg_file",     STR, NULL)   // file to write debug info to (NULL...stdout)
OPTION (stdwarn, "stdwarn_file",   STR, NULL)   // file to write warnings to (NULL...stderr)
OPTION (stdshriek,"stdshriek_file",STR, NULL)   // file to write fatal errors to (NULL...stderr)

OPTION (trans, "show_transcript", BOOL, true)	// Should we display transcription on exit?
OPTION (use_diph,"",		  BOOL, true)	// (enabled if one of the following is on)
OPTION (show_diph,"show_diphones",BOOL, false)  // Should we display the diphones on exit?
OPTION (play_diph,"play_diphones",BOOL, false)  // Should we write the sound to a file?
OPTION (diph_raw,"show_raw_diphs",BOOL, false)  // show_diphones including diphone numbers?
// OPTION (show_crc, "show_crc",     BOOL, false)	// Near-unique signature of the sound
OPTION (wav_file, "wave_file",     STR, NULL)	// File to write .wav into if play_diphones
OPTION (imm_diph,"immed_diphones",BOOL, false)  // Should we output them after a DIPHONES rule?

INV_OPTION (inv_name, name, "name",	   STR, "(unnamed)")
INV_OPTION (inv_type, type, "type",       TYPE, S_NONE)	// Int, float, vector quantified...
//INV_OPTION (inv_num, number, "number",     INT, -1)	// Back end voice number (?)
INV_OPTION (inv_ct,channel, "channel", CHANNEL, CT_MONO)// (unused; not stereo)
INV_OPTION (inv_counts, counts, "counts",  STR, NULL)	// File with the model counts for every diphone?
INV_OPTION (inv_models, models, "models",  STR, NULL)	// File with the models themselves?
INV_OPTION (inv_book, book, "codebook",    STR, NULL)	// No idea what this file is
INV_OPTION (inv_dph, dphfile, "dph_file",  STR, NULL)	// File describing the diphones?
INV_OPTION (inv_dpt, dptfile, "dpt_file",  STR, NULL)	// File naming the diphones?
INV_OPTION (inv_f0, init_f, "init_f",      INT, 100)	// Neutral frequency for the inventory
INV_OPTION (inv_i0, init_i, "init_i",      INT, 100)	// Neutral intensity
INV_OPTION (inv_t0, init_t, "init_t",      INT, 100)	// Neutral time factor

INV_OPTION (inv_hz, samp_rate, "inv_sampling_rate", INT, 8000)	// The real sampling rate of the inventory
INV_OPTION (inv_samp_size, samp_size, "sample_size",INT, 16)	// (not supported)

INV_OPTION (wav_hdr, wav_hdr, "wave_header",	BOOL, false)  // Should .wav output contain .wav file header?
INV_OPTION (ioctlable, ioctlable, "ioctlable",	BOOL, false)  // Is the voice a real device not file?

OPTION (daemon_log, "daemon_log",  STR, NULL)	// Log file of daemon.cc activities (TTSCP server)
OPTION (allow_file, "allow_options_file", STR, "allowed.ini") // options anonymous may change
INV_OPTION (buffer_size, buff_size, "buffer_size",	INT, 4096)
OPTION (max_children, "",	   INT, 32)	// maximum number of simult. talking children
OPTION (max_net_cmd, "max_net_cmd",INT, 16384)  // Max TCP command length
OPTION (listen_port, "",	   INT, 8778)	// TCP port where the daemon should listen
OPTION (sd, "",			   INT, 0)	// network socket of the current session
						// (can also be used to detect the daemon mode)
OPTION (persistence,"persistence", INT, 60)	// Seconds to try to bind the socket (0 forever)

LNG_OPTION (std_voices, voice_names, "voices", STR, "")	// voices supported for this languages

OPTION (f_neutral, "f_neutral",    INT, 100)    // Neutral (unmarked) frequency
OPTION (i_neutral, "i_neutral",    INT, 100)    // Neutral intensity
OPTION (t_neutral, "t_neutral",    INT, 100)    // Neutral time factor
INV_OPTION (ti_adj, t_i_adjustments, "t_i_adj", BOOL, false)	// Adjust neutral time/intensity for some diphones

// OPTION (ktd_pitch, "ktd_pitch",    INT, 100)
// OPTION (ktd_speed, "ktd_speed",    INT, 4000)	//          FIXME

// OPTION (lowercase,"lower_case",    STR, NULL)   // Which characters are accepted as 
// OPTION (uppercase,"upper_case",    STR, NULL)   // denoting phones

OPTION (relax_input,"relax_input",BOOL, false)  // Survive unknown characters on input
OPTION (dflt_char, "default_char",CHAR, ' ')	// Replace them with this char

OPTION (header_xscr, "header",    FILE, "")     // Header and footer printed
OPTION (footer_xscr, "footer",    FILE, "")     //   in unit::fout()

LNG_OPTION (syll_hack, syll_hack, "suppress_side_syll",  BOOL,  false)   
						// Try to avoid initial pseudo-syllables,
						// e.g. "j-sem", "rm-en"
LNG_OPTION (limit_syll_hack, syll_thr, "limit_side_syll", CHAR, 'a') 
						// What sonority the initial phone must have
						// in order to become syllable peak? (Example)

OPTION (normal_col, "normal_color",   STR, "")
OPTION (curul_col, "curr_rule_color", STR, "")
OPTION (shriek_col, "fatal_color",    STR, "")
OPTION (warn_col, "warning_color",    STR, "")

OPTION (shriek_art, "shriek_art",     INT, 0)	// Number of picture printed on fatal errors

OPTION (comma, "comma", 	      STR, "\n")// Delimiter character for generated lists

LNG_OPTIONAGGR (const char *perm[U_TEXT+1])
LNG_OPTIONITEM (perm[U_PHONE], "perm_phone", STR, "")
LNG_OPTIONITEM (perm[U_SYLL], "perm_syll", STR, "")
LNG_OPTIONITEM (perm[U_WORD], "perm_word", STR, "")
LNG_OPTIONITEM (perm[U_COLON], "perm_colon", STR, "")
LNG_OPTIONITEM (perm[U_SENT], "perm_sent", STR, "")
LNG_OPTIONITEM (perm[U_TEXT], "perm_text", STR, "")
LNG_OPTIONAGGRENDS

OPTION (out_verbose,"structured", BOOL, true)   // Will unit::fout display other units than phones?
OPTION (out_postfix, "postfix",   BOOL, false)  // Content char follows the offspring?
OPTION (out_prefix, "prefix",   BOOL, false)    // Content char precedes the offspring?
OPTION (out_swallow__, "swallow_underbars", BOOL, true) // Skip underbars to be printed?

OPTIONAGGR (const char *out_opening[U_TEXT+1])
OPTIONITEM (out_opening[U_DIPH],"",STR,"")
OPTIONITEM (out_opening[U_PHONE],"begin_phone",STR,"")
OPTIONITEM (out_opening[U_SYLL],"begin_syll",STR,"")
OPTIONITEM (out_opening[U_WORD],"begin_word",STR,"")
OPTIONITEM (out_opening[U_COLON],"begin_colon",STR,"")
OPTIONITEM (out_opening[U_SENT],"begin_sent",STR,"")
OPTIONITEM (out_opening[U_TEXT],"begin_text",STR,"")
OPTIONAGGRENDS

OPTIONAGGR (const char *out_separ[U_TEXT+1])
OPTIONITEM (out_separ[U_DIPH],"",STR,"")
OPTIONITEM (out_separ[U_PHONE],"separ_phone",STR,"")
OPTIONITEM (out_separ[U_SYLL],"separ_syll",STR,"")
OPTIONITEM (out_separ[U_WORD],"separ_word",STR,"")
OPTIONITEM (out_separ[U_COLON],"separ_colon",STR,"")
OPTIONITEM (out_separ[U_SENT],"separ_sent",STR,"")
OPTIONITEM (out_separ[U_TEXT],"",STR,"")
OPTIONAGGRENDS

OPTIONAGGR (const char *out_closing[U_TEXT+1])
OPTIONITEM (out_closing[U_DIPH],"",STR,"")
OPTIONITEM (out_closing[U_PHONE],"close_phone",STR,"")
OPTIONITEM (out_closing[U_SYLL],"close_syll",STR,"")
OPTIONITEM (out_closing[U_WORD],"close_word",STR,"")
OPTIONITEM (out_closing[U_COLON],"close_colon",STR,"")
OPTIONITEM (out_closing[U_SENT],"close_sent",STR,"")
OPTIONITEM (out_closing[U_TEXT],"close_text",STR,"")
OPTIONAGGRENDS

OPTIONAGGR (const char *out_color[U_TEXT+1])
OPTIONITEM (out_color[U_DIPH],"color_diphone",STR,"")
OPTIONITEM (out_color[U_PHONE],"color_phone",STR,"")
OPTIONITEM (out_color[U_SYLL],"color_syll",STR,"")
OPTIONITEM (out_color[U_WORD],"color_word",STR,"")
OPTIONITEM (out_color[U_COLON],"color_colon",STR,"")
OPTIONITEM (out_color[U_SENT],"color_sent",STR,"")
OPTIONITEM (out_color[U_TEXT],"color_text",STR,"")
OPTIONAGGRENDS


#undef BOOL
#undef STR
#undef INT
#undef CHAR
#undef ELEM
#undef FILE
#undef MARKUP
#undef TYPE
#undef CHANNEL
#undef DEBUG_AREA

#undef OPTIONAGGR
#undef OPTIONITEM
#undef OPTIONAGGRENDS
#undef LNG_OPTIONAGGR
#undef LNG_OPTIONITEM
#undef LNG_OPTIONAGGRENDS
#undef OPTION
#undef INV_OPTION
#undef LNG_OPTION
