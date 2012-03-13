/*
 *	(c) 1998-99 Jirka Hanika <geo@ff.cuni.cz>
 *
 *	This single source file src/say.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precendence.
 *
 *	This file implements a simple TTSCP client. See doc/english/ttscp.sgml
 *	for a preliminary technical specification.
 *
 *	This file is almost a plain C source file.  We compile it with a C++
 *	compiler just to avoid additional configure complexity.
 */

#define THIS_IS_A_TTSCP_CLIENT

#include "config.h"	/* You can usually remove this item */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TTSCP_PORT	8778

const char *COMMENT_LINES = "#;\r\n";
const char *WHITESPACE = " \t";

const char *output_file = "#localsound";

#define STDIN_BUFF_SIZE  550000

int ctrld, datad;		/* file descriptors for the control and data connections */
char *data = NULL;

char *ch;
char *dh;

void shriek(char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
	exit(1);
}

void shriek(int, char *txt)
{
	shriek(txt);
}


#define EPOS_COMMON_H	// this is a lie
#include "exc.h"
#include "client.cc"

int get_result(int sd)
{
	while (sgets(scratch, SCRATCH_SPACE, sd)) {
		scratch[SCRATCH_SPACE] = 0;
//		printf("Received: %s\n", scratch);
		switch(*scratch) {
			case '1': continue;
			case '2': return 2;
			case '3': break;
			case '4': printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return 4;
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
		char *o = scratch+strspn(scratch, "0123456789 ");
		if (*scratch && *o) printf("%s\n", o);
	}
	return 8;	/* guessing */
}

void say_data()
{
	if (!data) data = "No.";
	sputs("strm $", ctrld);
	sputs(dh, ctrld);
	sputs(":chunk:raw:rules:diphs:synth:", ctrld);
	sputs(output_file, ctrld);
	sputs("\r\n", ctrld);
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", strlen(data));
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	sputs(data, datad);
	if (get_result(ctrld) > 2) shriek("Could not set up a stream");
	if (get_result(ctrld) > 2) shriek("Could not apply a stream to text");
}

void trans_data()
{
	if (!data) data = "No.";
	sputs("strm $", ctrld);
	sputs(dh, ctrld);
	sputs(":raw:rules:print:$", ctrld);
	sputs(dh, ctrld);
	sputs("\r\n", ctrld);
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", strlen(data));
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	sputs(data, datad);
	get_result(ctrld);

	while (sgets(scratch, SCRATCH_SPACE, ctrld)) {
		scratch[SCRATCH_SPACE] = 0;
		if (*scratch > '1') return;	// guessing
		if (!strncmp(scratch, "122 ", 4)) {
			int count;
			sgets(scratch, SCRATCH_SPACE, ctrld);
			scratch[SCRATCH_SPACE] = 0;
			sscanf(scratch, "%d", &count);
			char *b=(char *)malloc(count+1);
			b[read(datad, b, count)] = 0;
			printf("%s\n", b);
		}
	}

	get_result(ctrld);
}

void send_option(char *name, char *value)
{
	xmit_option(name, value, ctrld);
	get_result(ctrld);
}

#define CMD_LINE_OPT "-"
#define CMD_LINE_VAL '='

void dump_help()
{
	printf("usage: say [options] ['text to be processed']\n");
	printf(" -b  bare format (no frills)\n");
	printf(" -c  casual pronunciation\n");
	printf(" -i  ironic pronunciation\n");
	printf(" -k  shutdown Epos\n");
	printf(" -l  list available languages and voices\n");
	printf(" -w  write output to said.wav (in the \"pseudo root\" directory)\n");
}

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
				case 'h': dump_help(); exit(0);
				case 'i': send_option("irony", "true"); break;
				case 'k': FILE *f;
					  f = fopen("/var/run/epos.pwd", "r");
					  if (!f) shriek("Cannot open /var/run/epos.pwd");
					  sputs("pass ", ctrld);
					  scratch[fread(scratch, 1, 1024, f)] = 0;
					  sputs(scratch, ctrld);
					  sputs("down\r\n", ctrld);
					  get_result(ctrld);
					  get_result(ctrld);
					  exit(0);
				case 'l': sputs("show languages\r\nshow voices\r\n", ctrld);
					  printf("Languages available:\n");
					  get_result(ctrld);
					  printf("Voices available for the current language:\n");
					  get_result(ctrld);
					  exit(0);
			//	case 'n': cfg->rules_file="nnet.rul";
			//		  if (this_lang)
			//			this_lang->rules_file = cfg->rules_file;
			//		  cfg->neuronet=true; break;
				case 'p': send_option("pausing", "true"); break;
				case 's': send_option("play_diph", "true"); break;
				case 'v': send_option("version", "true"); break;
				case 'w': send_option("wave_header", "true");
					  output_file = "/said.wav"; break;
				case 'D':
					send_option("use_debug", "true");
			//		if (!cfg->use_dbg) cfg->use_dbg=true;
			//		else if (cfg->warnings)
			//			cfg->always_dbg--;
					break;
				default : shriek("Unknown short option");
			}
			if (j==ar+1) {			//dash only
				if (data) free(data);
				data = (char *)malloc(STDIN_BUFF_SIZE);
				fread(data,1,STDIN_BUFF_SIZE,stdin);
			}
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
#ifdef HAVE_WINSOCK2_H
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
#endif
	ctrld = connect_socket(0, TTSCP_PORT);
	ch = get_handle(ctrld);
	datad = connect_socket(0, TTSCP_PORT);
	sputs("data ", datad);
	sputs(ch, datad);
	sputs("\r\n", datad);
	free(ch);
	dh = get_handle(datad);
	get_result(datad);
	send_cmd_line(argc, argv);
//	sputs("intr\r\n", ctrld);

// #ifdef HAVE_GETENV
	FILE *f = fopen(getenv("TTSCP_USER"), "rt");
	if (f) {
		while (!feof(f)) {
			*scratch = 0;
			fgets(scratch, SCRATCH_SPACE, f);
			if (*scratch && !strchr(COMMENT_LINES, scratch[strspn(scratch, WHITESPACE)])) {
				sputs(scratch, ctrld);
				get_result(ctrld);
			}
		}
		fclose(f);
	}
/// #endif

	say_data();
	trans_data();
//	printf("\n");
	sputs("delh ", ctrld);
	sputs(dh, ctrld);
	sputs("\r\ndone\r\n", ctrld);
	get_result(ctrld);
	get_result(ctrld);
	close(datad);
	close(ctrld);
	return 0;
}

#ifndef HAVE_STRDUP

char *strdup(const char*src)
{
	return strcpy((char *)malloc(strlen(src)+1), src);
}

#endif   // ifdef HAVE_STRDUP

#ifndef HAVE_TERMINATE

void terminate(void)
{
	abort();
}

#endif

