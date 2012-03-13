/*
 *	ss/src/config.cc
 *	(c) 1997 geo@ff.cuni.cz
 *
 *	This file is the only place where the individual options are defined.
 *	It gets included thrice in interf.h & interf.cc, to produce first 
 *	the declaration, then the definition, and finally, the array of options
 *	to be parsed by load_config().
 *	
 *	Note that you should #define CONFIG_DECLARE, CONFIG_INITIALIZE or 
 *	CONFIG_DESCRIBE, respectively, when this file is included and any
 *	of these flags will be undefined again when this file is over.
 *	This scheme was introduced in version 1.1.6.
 *
 *	The format is obvious. The last field is the default value. 
 */

#ifdef CONFIG_DECLARE

#define OPTION(member,name,type,default) type member;

#define BOOL   bool
#define INT    int
#define STR    const char*
#define CHAR   Char
#define ELEM   UNIT
#define FILE   const char*
#define MARKUP OUT_ML
#define TYPE   SYNTH_TYPE
#define DEBUG_AREA _DEBUG_AREA_

#define OPTIONAGGR(x) x;
#define OPTIONITEM(w,x,y,z) 
#define OPTIONAGGRENDS 

#undef CONFIG_DECLARE

#else                 //#ifdef CONFIG_INITIALIZE or CONFIG_DESCRIBE

#define BOOL   O_BOOL
#define INT    O_INT
#define STR    O_STRING
#define CHAR   O_CHAR
#define ELEM   O_UNIT
#define FILE   O_FILE
#define MARKUP O_MARKUP
#define TYPE   O_SYNTH
#define DEBUG_AREA O_DBG_AREA

#ifdef CONFIG_INITIALIZE
#define OPTION(member,name,type,default) cfg.member=default,

#define OPTIONAGGR(x) {
#define OPTIONITEM(w,x,y,z) OPTION (w,x,y,z)
#define OPTIONAGGRENDS },

#undef CONFIG_INITIALIZE

#else                 //#ifdef CONFIG_DESCRIBE
#ifndef CONFIG_DESCRIBE
#error Impossible inclusion of config.cc, wheee...
#endif

#define OPTION(member,name,type,default) {name, type, &cfg.member},

#define OPTIONAGGR(x) 
#define OPTIONITEM(w,x,y,z) OPTION (w,x,y,z)
#define OPTIONAGGRENDS 

#undef CONFIG_DESCRIBE

#endif                //CONFIG_INITIALIZE
#endif                //CONFIG_DECLARE






OPTION (use_dbg,    "use_debug",  BOOL, false)  // Whether to print any debug info at all

OPTION (interf_dbg, "interf_debug",INT, 4)      // Debugging levels of various
OPTION (rules_dbg,  "rules_debug", INT, 2)      // code categories. See debug.doc
OPTION (elem_dbg,   "elem_debug",  INT, 2)      // 
OPTION (subst_dbg,  "subst_debug", INT, 2)      // 
OPTION (assim_dbg,  "assim_debug", INT, 2)      // 
OPTION (split_dbg,  "split_debug", INT, 2)      // 
OPTION (parser_dbg, "parser_debug",INT, 2)      // 
OPTION (synth_dbg,  "synth_debug", INT, 2)	//

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
OPTION (lng, "language",           STR, "")	// Which language to simulate?
OPTION (inventory, "inventory",    STR, "")	// What speaker (diphone inventory) to use?
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

OPTION (colloquial, "colloquial", BOOL, false)  // Colloquial pronunciation? Language specific.
OPTION (irony, "irony", 	  BOOL, false)	// Ironical intonation?

OPTION (vars, "variables",         INT, 43)     // About how many variables in the rules?
OPTION (rules, "rules_in_block",   INT, 32)	// About how many rules in a block?
OPTION (mrw, "max_rule_weight",    INT, 900000)	// Maximum rule weight in a choice?
OPTION (max_line, "max_line_len",  INT, 512)	// How long may an input line be?
OPTION (scratch, "scratch_size",   INT, 512)	// How long may some temporary strings be?
OPTION (dev_txtlen,"dev_text_len", INT, 50000)  // How much data can we expect from a device?

OPTION (eof_char, "end_of_file",  CHAR, '\033') // Which key should terminate user input?

OPTION (base_dir, "base_dir",      STR, "/usr/lib/ss")    // base path to everything
OPTION (rules_dir, "rules_dir",    STR, ".")    // path to the rules and banner files
OPTION (hash_dir,  "hash_dir",     STR, ".")    //      to the dictionaries
OPTION (input_dir, "input_dir",    STR, ".")    //      to the input file
OPTION (invent_dir, "invent_dir",  STR, ".")	//      to the diphone inventories
OPTION (lang_dir, "lang_dir",      STR, ".")	//	to the language descriptions
OPTION (pros_dir, "prosody_dir",   STR, ".")	//	to the prosody files
OPTION (ini_dir, "ini_dir",	   STR, "cfg")  //      to ssfixed.ini
OPTION (matlab_dir, "matlab_dir",  STR, ".")	//      where to store nnet output
OPTION (wav_dir, "wav_dir",        STR, ".")	//      where to store .wav files
OPTION (input_file, "input_file",  STR, NULL)   // relative filename of the input text
OPTION (input_text, "",            STR, NULL)	// the input text itself
OPTION (rules_file, "rules_file",  STR, NULL)   // relative filename of the rules text
OPTION (nnet_file, "nnet_file",    STR, NULL)	// relative filename of nnet output
OPTION (stddbg, "stddbg_file",     STR, NULL)   // file to write debug info to (NULL...stdout)
OPTION (stdwarn, "stdwarn_file",   STR, NULL)   // file to write warnings to (NULL...stderr)
OPTION (stdshriek,"stdshriek_file",STR, NULL)   // file to write fatal errors to (NULL...stderr)

OPTION (use_diph,"",		  BOOL, true)	// (enabled if one of the following is on)
OPTION (show_diph,"show_diphones",BOOL, false)  // Should we display the diphones on exit?
OPTION (play_diph,"play_diphones",BOOL, false)  // Should we write the sound to a file?
OPTION (diph_raw,"show_raw_diphs",BOOL, false)  // show_diphones including diphone numbers?
OPTION (show_crc, "show_crc",     BOOL, false)	// Near-unique signature of the sound
OPTION (wav_file, "wave_file",     STR, NULL)	// File to write .wav into if play_diphones
OPTION (imm_diph,"immed_diphones",BOOL, false)  // Should we output them after a DIPHONES rule?

//OPTION (syn_f, "synth_f0",	   INT, 100)	// ???, obsolete anyway
//OPTION (sw_syn, "synth_modus",   INT,   3)	// (index into inventar[] in syn32.cc)

OPTION (inv_type, "inv_type",     TYPE, S_VQ)	// Int, float, vector quantified...
OPTION (inv_counts, "inv_counts",  STR, NULL)	// File with the model counts for every diphone?
OPTION (inv_models, "inv_models",  STR, NULL)	// File with the models themselves?
OPTION (inv_book, "inv_book",      STR, NULL)	// No idea what this file is
OPTION (inv_dph, "inv_dph",	   STR, NULL)	// File describing the diphones?
OPTION (inv_dpt, "inv_dpt",        STR, NULL)	// File naming the diphones?
OPTION (inv_f0, "inv_f0",          INT, 100)	// Neutral frequency for the inventory
OPTION (inv_i0, "inv_i0",          INT, 100)	// Neutral intensity
OPTION (inv_t0, "inv_t0",          INT, 100)	// Neutral time factor
OPTION (inv_hz,"inv_sampling_rate",INT, 8000)	// The real sampling rate of the inventory

OPTION (wav_header,"wave_header", BOOL, false)  // Should .wav output contain .wav file header?

OPTION (samp_hz, "sampling_rate",  INT, 8000)	// Sampling rate (in Hz)
OPTION (samp_bits, "",             INT, 16)	// (not supported)
OPTION (stereo, "",               BOOL, false)  // (not supported)

OPTION (f_neutral, "f_neutral",    INT, 100)    // Neutral (unmarked) frequency
OPTION (i_neutral, "i_neutral",    INT, 100)    // Neutral intensity
OPTION (t_neutral, "t_neutral",    INT, 100)    // Neutral time factor
OPTION (ti_adj, "t_i_adjustments",BOOL, false)	// Adjust neutral time/intensity for some diphones

OPTION (lowercase,"lower_case",    STR, NULL)   // Which characters are accepted as 
OPTION (uppercase,"upper_case",    STR, NULL)   // denoting phones

OPTION (header_xscr, "header",    FILE, "")     // Header and footer printed
OPTION (footer_xscr, "footer",    FILE, "")     //   in unit::fout()

OPTION (syll_hack, "suppress_side_syll",  BOOL,  false)   
						// Try to avoid initial pseudo-syllables,
						// e.g. "j-sem", "rm-en"
OPTION (limit_syll_hack, "limit_side_syll", CHAR, 'a') 
						// What sonority the initial phone must have
						// in order to become syllable peak? (Example)

OPTION (normal_col, "normal_color",   STR, "")
OPTION (curul_col, "curr_rule_color", STR, "")
OPTION (shriek_col, "fatal_color",    STR, "")
OPTION (warn_col, "warning_color",    STR, "")

OPTION (shriek_art, "shriek_art",     INT, 0)	// Number of picture printed on fatal errors

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
#undef DEBUG_AREA

#undef OPTIONAGGR
#undef OPTIONITEM
#undef OPTIONAGGRENDS
#undef OPTION
