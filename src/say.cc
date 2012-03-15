/*
 *	(c) 1998-01 Jirka Hanika <geo@cuni.cz>
 *
 *	This single source file src/say.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precedence.
 *
 *	This file implements a simple TTSCP client. See doc/english/ttscp.sgml
 *	for a preliminary technical specification.
 *
 *	This file is almost a plain C source file.  We compile it with a C++
 *	compiler just to avoid additional configure complexity.
 */

#define THIS_IS_A_TTSCP_CLIENT

#include "config.h"	/* You can usually remove this item */

#ifdef HAVE_WINSVC_H
	bool start_service();
	bool stop_service();
#else
	bool start_service() { return true; }
	bool stop_service() { return true; }
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#define TTSCP_PORT	8778

const char *COMMENT_LINES = "#;\r\n";
const char *WHITESPACE = " \t\r";

const char *output_file = NULL;

const char *charset = "8859-2";

bool chunking = false;
bool show_segments = false;
bool wavfile = false;
bool wavstdout = false;
bool traditional = true;  

struct segment {
	short code; char nothing; char ll;
	int f,e,t;
};

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

int size;

char *get_data()
{
	char *b = NULL;
	size = 0;
	while (sgets(scratch, SCRATCH_SPACE, ctrld)) {
		scratch[SCRATCH_SPACE] = 0;
		if (strchr("2468", *scratch)) { 	/* all done, write result */
			if (*scratch != '2') shriek(scratch);
			if (!size) shriek("No processed data received");
			b[size] = 0;
			return b;
		}
		if (!strncmp(scratch, "123 ", 4)) {
			int count;
			sgets(scratch, SCRATCH_SPACE, ctrld);
			scratch[SCRATCH_SPACE] = 0;
			sscanf(scratch, "%d", &count);
			b = size ? (char *)realloc(b, size + count + 1) : (char *)malloc(count + 1);
			int limit = size + count;
			while (size < limit)
				size += yread(datad, b + size, limit - size);
		}
	}
	if (size) shriek("Disconnect during transmit");
	else shriek("Disconnect before transmit");
	return NULL;
}

void say_data()
{
	if (!data) data = strdup("No.");
	sputs("strm $", ctrld);
	sputs(dh, ctrld);
	if (chunking) sputs(":chunk", ctrld);
	if (traditional) sputs(":raw:rules:diphs:synth:", ctrld);
	else sputs(":raw:rules:dump:syn:", ctrld);
	if (wavfile || wavstdout) sputs("$", ctrld), sputs(dh, ctrld);
	else sputs("#localsound", ctrld);
	sputs("\r\n", ctrld);
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", (int)strlen(data));
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	sputs(data, datad);
	if (get_result(ctrld) > 2) shriek("Could not set up a stream");
	char *b;
	if (wavfile || wavstdout) {
		b = get_data();
		FILE *f = wavstdout ? stdout : fopen(output_file, "wb");
		if (!f || !fwrite(b, size, 1, f)) shriek("Could not write waveform");
		if (!wavstdout) fclose(f);
		free(b);
		return;
	}
	if (get_result(ctrld) > 2) shriek("Could not apply a stream");
}

void trans_data()
{
	if (!data) data = strdup("No.");
	sputs("strm $", ctrld);
	sputs(dh, ctrld);
	if (show_segments) sputs(traditional ? ":raw:rules:diphs:$"
					     : ":raw:rules:dump:$", ctrld);
	else sputs(":raw:rules:print:$", ctrld);
	sputs(dh, ctrld);
	sputs("\r\n", ctrld);
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", (int)strlen(data));
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	sputs(data, datad);
	get_result(ctrld);
	
	if (show_segments) {
		segment *b = (segment *)get_data();
		if (traditional)
			for (int i=1; i<b[0].code; i++)
				printf("%4d - %3d %3d %3d\n", b[i].code, b[i].f, b[i].e, b[i].t);
		else printf("%s\n", (char *)b);
	} else {
		char *b = get_data();
		printf("%s\n", b);
	}
}

void send_option(const char *name, const char *value)
{
	xmit_option(name, value, ctrld);
	get_result(ctrld);
}

#define CMD_LINE_OPT "-"
#define CMD_LINE_VAL '='

void dump_help()
{
	printf("usage: say [options] ['Text to be processed.']\n");
	printf(" -b  bare format (no frills)\n");
	printf(" -c  casual pronunciation\n");
	printf(" -d  show segments\n");
	printf(" -k  shutdown Epos\n");
	printf(" -l  list available languages and voices\n");
	printf(" -m  write the waveform in mu law format to ./said.vox\n");
	printf(" -o  write the waveform to the standard output\n");
#ifdef HAVE_UNISTD_H
	printf(" -r  reread Epos configuration\n");
#endif
	printf(" -s  use the SAMPA-based (MBROLA compatible) synthesizer interface\n");
	printf(" -t  use the traditional lower level synthesizer interface\n");
	printf(" -u  use utterance chunking\n");
	printf(" -w  write the waveform to ./said.wav\n");
}

void send_cmd_line(int argc, char **argv)
{
	char *ar;
	char *j = NULL;
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
				case 'd': show_segments = true; break;
//				case 'd': send_option("show_segments", "true"); break;
				case 'H': send_option("long_help", "true");	/* fall through */
				case 'h': dump_help(); exit(0);
				case 'k': FILE *f;
					  f = fopen("/var/run/epos.pwd", "r");
					  if (!f) {
					  	stop_service();
					  	shriek("Cannot open /var/run/epos.pwd");
					  }
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
				case 'm': send_option("ulaw", "true");
					  send_option("wave_header", "false");
					  wavfile = true;
					  output_file = "said.vox"; break;
				case 'o': wavstdout = true; break;
				case 'p': send_option("pausing", "true"); break;
#ifdef HAVE_UNISTD_H
				case 'r': system("killall -HUP epos");
					  do sleep(1); while(just_connect_socket(0, 8778) == -1);
					  exit(0);
#endif
				case 's': traditional = false; break;
				case 't': traditional = true; break;
				case 'u': chunking = true; break;
				case 'v': send_option("version", "true"); break;
				case 'w': send_option("wave_header", "true");
					  wavfile = true;
					  output_file = "said.wav"; break;
				case 'D':
					send_option("use_debug", "true");
			//		if (!scfg->use_dbg) cfg->use_dbg=true;
			//		else if (scfg->warnings)
			//			scfg->always_dbg--;
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

/*
 *	main() implements what most TTSCP clients do: it opens two TTSCP connections,
 *	converts one of them to a data connection dependent on the other one.
 *	Then, commands in a file found using the TTSCP_USER environment variable
 *	are transmitted and synthesis and transcription procedures invoked.
 *	Last, general cleanup is done (the connections are gracefully closed.)
 *
 *	Note that the connection establishment code is less intuitive than
 *	it could be because of paralelism oriented code ordering.
 */

int main(int argc, char **argv)
{
#ifdef HAVE_WINSOCK
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
	charset = "cp1250";
#endif
	start_service();		/* Windows NT etc. only */

	ctrld = connect_socket(0, 0);
	datad = connect_socket(0, 0);
	sputs("data ", datad);
	ch = get_handle(ctrld);
	sputs(ch, datad);
	sputs("\r\n", datad);
	free(ch);
	sputs("setl charset ",ctrld);
	sputs(charset, ctrld);
	sputs("\r\n", ctrld);
	get_result(ctrld);
	send_cmd_line(argc, argv);
	dh = get_handle(datad);
	get_result(datad);

// #ifdef HAVE_GETENV
	FILE *f = NULL;
	char *ttscp_user_config = getenv("TTSCP_USER");
	if (ttscp_user_config) f = fopen(ttscp_user_config, "rt");
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

	if (!wavstdout) trans_data();
	say_data();
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


