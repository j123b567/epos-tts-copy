/*
 *	(c) 1998 Jirka Hanika <geo@ff.cuni.cz>
 *
 *	This single source file src/ss.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precendence.
 *
 *	This file implements a simple TTSCP client. See doc/english/protocol.doc
 *	for a preliminary technical specification.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define SSD_TCP_PORT	8778

int sd;
char *data;

#define SCRATCH_SPACE 16384
char scratch[SCRATCH_SPACE + 2];

void warn(char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
}

void shriek(char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
	exit(1);
}

#define SS_COMMON_H	// this is a lie
#include "client.cc"

void get_result()
{
	while (sgets(scratch, SCRATCH_SPACE, sd)) {
		scratch[SCRATCH_SPACE] = 0;
		switch(*scratch) {
			case '1': continue;
			case '2': return;
			case '3': break;
			case '4': printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
					exit(0);
				  } /* else fall through */
			case '8': printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  exit(2);

			case '5':
			case '7':
			case '9':
			case '0': printf("%s\n", scratch); shriek("Unhandled response code");
			default : ;
		}
		printf("%s\n", scratch+strspn(scratch, "0123456789x "));
	}
}

void say_data()
{
	sputs("say ", sd);
	sputs(data, sd);
	sputs("\n", sd);
	get_result();
}

void trans_data()
{
	sputs("trans ", sd);
	sputs(data, sd);
	sputs("\n", sd);
	get_result();
}

void send_option(char *name, char *value)
{
	xmit_option(name, value, sd);
	get_result();
}

#define CMD_LINE_OPT "-"
#define CMD_LINE_VAL '='

void send_cmd_line(int argc, char **argv)
{
	char *ar;
	char *j;
	register char *par;

	for(int i=1; i<argc; i++) {
		ar=argv[i];
		switch(strspn(ar, CMD_LINE_OPT)) {
		case 3:
			ar+=3;
			if (strchr(ar, CMD_LINE_VAL)) 
				shriek("Thrice dashed options have an implicit value");
			send_option(ar, "0");
			break;
		case 2:
			ar+=2;
			par=strchr(ar, CMD_LINE_VAL);
			if (par) {					//opt=val
				*par=0;
				send_option(ar, par+1);
				*par=CMD_LINE_VAL;
			} else	if (i+1==argc || strchr(CMD_LINE_OPT, *argv[i+1])) 
					send_option(ar, "");	//opt
				else send_option(ar, argv[++i]);	//opt val
			break;
		case 1:
			for (j=ar+1; *j; j++) switch (*j) {
				case 'b': send_option("out_verbose", "false"); break;
				case 'c': send_option("colloquial", "true"); break;
				case 'd': send_option("show_diphones", "true"); break;
				case 'H': send_option("long_help", "true");	/* fall through */
				case 'h': send_option("help", "true"); break;
				case 'i': send_option("irony", "true"); break;
				case 'k': sputs("shutdown\n", sd);
					  get_result();
					  exit(0);
			//	case 'n': cfg->rules_file="nnet.rul";
			//		  if (this_lang)
			//			this_lang->rules_file = cfg->rules_file;
			//		  cfg->neuronet=true; break;
				case 'p': send_option("pausing", "true"); break;
				case 's': send_option("play_diph", "true"); break;
				case 'v': send_option("version", "true"); break;
				case 'D':
					send_option("use_debug", "true");
			//		if (!cfg->use_dbg) cfg->use_dbg=true;
			//		else if (cfg->warnings)
			//			cfg->always_dbg--;
					break;
				default : shriek("Unknown short option");
			}
			if (j==ar+1) send_option("input_file", "");	//dash only
			break;
		case 0:
			if (data) {
				sprintf(scratch, "%s %s", data, ar);
				free(data);
				data = strdup(scratch);
			} else data = strdup(ar);
			
			break;
		default:
			shriek("Too many dashes ");
		}
	}
	if (data) for(char *p=data; *p; p++) if (*p=='\n') *p=' ';
}


int main(int argc, char **argv)
{
	sd = connect_socket(SSD_TCP_PORT);
	send_cmd_line(argc, argv);
	sputs("break\n", sd);

// #ifdef HAVE_GETENV
	FILE *f = fopen(getenv("TTSCP_USER"), "rt");
	if (f) {
		while (!feof(f)) {
			*scratch = 0;
			fgets(scratch, SCRATCH_SPACE, f);
			if (*scratch) {
				sputs(scratch, sd);
				get_result();
			}
		}
		fclose(f);
	}
/// #endif

	say_data();
	trans_data();
	sputs("done\n", sd);
	get_result();

	close(sd);
	return 0;
}
