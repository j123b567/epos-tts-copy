/*
 *	(c) 1998-2005 Jirka Hanika <geo@cuni.cz>
 *
 *	This single source file src/say-epos.cc, but NOT THE REST OF THIS PACKAGE,
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

#include "common.h"

#ifdef HAVE_WINSVC_H
	bool start_service();
	bool stop_service();
#else
	bool start_service() { return true; }
	bool stop_service() { return true; }
#endif

#ifndef HAVE_TERMINATE

void terminate(void)
{
	abort();
}

#endif

const char *COMMENT_LINES = "#;\r\n";
const char *WHITESPACE = " \t\r";

const char *charset = "8859-2";

const char *other_traffic_prefix = "Unexpected: ";

#define STDIN_BUFF_SIZE  550000

bool debug_ttscp = false;

int ctrld, datad;		/* file descriptors for the control and data connections */
char *data = NULL;
__int16 *f0, *delka, pocet_slov, f0_delka;

#define MAX_CHAR 256

char *ch;
char *dh;

extern void shriek(char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
	exit(1);
}

extern void shriek(int, char *txt)
{
	shriek(txt);
}


// #include "exc.h"
#include "client.cc"
#include <malloc.h>

int send_to_epos(const char *buffer, int socket)
{
	return sputs(buffer, socket);
}

int get_result(int sd)
{
	while (sgets(scratch, scfg->scratch_size, sd)) {
		scratch[scfg->scratch_size] = 0;
		if (debug_ttscp && sd == ctrld)
			printf("Received: %s\n", scratch);
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
		char *o = scratch+strspn(scratch, "0123456789 -");
		if (*scratch && *o) printf("%s%s\n", other_traffic_prefix, o);
	}
	return 8;	/* guessing */
}

int size;

char *get_data()
{
	char *b = NULL;
	size = 0;
	while (sgets(scratch, scfg->scratch_size, ctrld)) {
		scratch[scfg->scratch_size] = 0;
		if (debug_ttscp) printf("Received: %s\n", scratch);
		if (strchr("2468", *scratch)) { 	/* all done, write result */
			if (*scratch != '2') shriek(scratch);
			if (!size) shriek("No processed data received");
			b[size] = 0;
			return b;
		}
		if (!strncmp(scratch, "123 ", 4)) {
			int count;
			sgets(scratch, scfg->scratch_size, ctrld);
			scratch[scfg->scratch_size] = 0;
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

int f0_mean(__int16 *f0, int n){
	int mean = 0;
	for (int i=0; i<n; mean += f0[i++]);
	return mean/(double)i;
}

void send_modul(char *list, char *input){
	send_to_epos("strm $", ctrld);
	send_to_epos(dh, ctrld);
	send_to_epos(list, ctrld);
	send_to_epos("$",ctrld);
	send_to_epos(dh, ctrld);
	send_to_epos("\r\n", ctrld);
	send_to_epos("appl ", ctrld);
	sprintf(scratch, "%d", (int)strlen(input));
	send_to_epos(scratch, ctrld);
	send_to_epos("\r\n", ctrld);
	send_to_epos(input, datad);
	
	if (get_result(ctrld) > 2) shriek("Could not set up a stream");
}

void send_modul(char *list){
	send_modul(list, data);
}

char *make_transp(char *mbr, char *trans){
	char phone[10], prosod[20], *p, *new_mbr;
	int *length, i = 0, j, buf1, buf2;
	bool watch_out=false;

	length = (int*)calloc(2*pocet_slov, sizeof(*length));
	for (p=mbr, j=0; *p; p = strchr(p, 0x0A)+1){
		sscanf(p, "%s %d", phone, &buf1);
		length[i] += buf1;
		if (watch_out && *trans != *phone && *phone != '_')
			shriek("Pravdepodobna chyba pri korekci delek slov!");
		else
			watch_out = *trans == *phone ? false : true;
		j++;
		if ( *(++trans) == ' ' && *phone != '_') {
			length[++i] = j;
			trans++;
			j = 0;
			i++;
		}
	}

	// prepocitani delek a f0
	if ( *data == '-' ) f0 = f0 + delka[-1]/15;
	new_mbr = (char*)calloc(4096, sizeof(*new_mbr)); // FIXME> skutecna velikost se muze lisit
	buf2 = f0_mean(f0, _msize(f0)/2);
	for (p=mbr, i=0, j=0; *p; p = strchr(p, 0x0A)+1){
		sscanf(p, "%s %d %s", phone, &buf1, prosod);
		buf1 = delka[i/2]*(double)buf1/(double)length[i];
		sprintf(new_mbr+strlen(new_mbr), "%s %d (99,%d)\n", phone, buf1, 100+buf2-f0_mean(f0, buf1/15));
		f0 += buf1/15;
		if (++j == length[i+1]){
			j = 0;
			i +=2;
		}
		if (strlen(new_mbr) > 4000) //FIXME> skutecna velikost je jina
			new_mbr = (char*)realloc(new_mbr, 2*4096);
	}
	free(mbr);
	return new_mbr;
}
void say_data(char **argv)
{
	char *txt, *mmry, *b, *hold, *wave;

	if (!data) shriek("NO DATA");
	
	// at first, get the transcription
	send_modul(":raw:rules:print:");
	//send_modul(":raw:");
	txt = get_data();

	for( char *p = txt-1; *(++p); ) {
		if ( strspn(p, "^%@") )
			memmove(p, p+1, strlen(p));
		if ( *p == '\''){
			*p = '?';
			if ( p[-1] != ' ' ) { memmove(p+1, p, strlen(p)); *p = ' ';}
			p[1] = p[3] = '?';
			*p = ' ';
		}
		if ( strspn(p, "αινσϊ") ){
			switch(*p){
			case 'α': *p = 'a'; break;
			case 'ι': *p = 'e'; break;
			case 'ν': *p = 'i'; break;
			case 'σ': *p = 'o'; break;
			case 'ϊ': *p = 'u'; break;
			}
		}
	}

	// then get the TDPMBROLA
	send_modul(":raw:rules:dump:");
	b = get_data();

	b = make_transp(b, txt);
	
	send_modul(":raw:rules:diphs:");
	segment *segs = (segment *)get_data();
				
	int i = 0;
	mmry = (char*)calloc(strlen(b)+segs->code*5, 1);
	*mmry = ';';

	itoa(segs->code, mmry+1, 10);
	for (i = 1; i < segs->code; i++){
		mmry[strlen(mmry)] = ',';
		itoa(segs[i].code, mmry+strlen(mmry), 10);
	}
	mmry[strlen(mmry)] = 0x0A;
	memmove(mmry+strlen(mmry), b, strlen(b)+1);
	free(b);
	free(txt);
	txt = data;
	data = mmry;
	
	// synthetize
	send_modul(":syn:");
	wave = get_data();
	
	b = (char*)calloc(strlen(argv[1])+10, sizeof(*b));
	strncat(b, argv[1], strrchr(argv[1], (int)'\\')-argv[1]+1);
	strcat(b, "output.wav");

	FILE *f = fopen(b, "wb");
	if (!size || !wave) shriek("Could not get waveform");
	if (!f || !fwrite(wave, size, 1, f)) shriek("Could not write waveform");
	fclose(f);
	free(b);
	free(wave);
	free(data);
	data = txt;
	return;
}


#ifdef HAVE_UNISTD_H
void restart_epos()
{
	int timeout;
	int d = just_connect_socket(0, 8778);
	int w;
	if (d == -1) {
		shriek("Not running at port 8778");
	}
	system("killall -HUP eposd");
	signal(SIGPIPE, SIG_IGN);
	timeout = 30;
	do {
		usleep(100000);
		w = send_to_epos(" ", d);
		if (!timeout--) {
			shriek("Restart not attempted");
		}
	} while (w != -1);

	timeout = 30;
	do usleep(100000); while(just_connect_socket(0, 8778) == -1 && timeout--);
	if (timeout == -1) shriek("Restart attempted, but timed out");
}
#endif

void send_option(const char *name, const char *value)
{
	if (debug_ttscp) {
		printf("setl %s %s\n", name, value);
	}
	xmit_option(name, value, ctrld);
	get_result(ctrld);
}

#define CMD_LINE_OPT "-"
#define CMD_LINE_VAL '='

void dump_help()
{
	printf("\npouziti: libetdp [options] soubor_s_prepisem [soubor_s_f0]\n");
	printf("options:\n");
	printf("\t-S \"davkovy_soubor_txt\" [davkovy_soubor_f0]\n\tpouziti davkoveho souboru obsahujici cesty k souborum s prepisem\n");
	printf("\tpokud neni zadan soubor \"davkovy_soubor_f0\" hledaji se soubory \"f0.txt\" ve stejnem adresari\n");
}

void send_cmd_line(int argc, char **argv)
{
	FILE *f_txt, *f_f0;
	int start, end, i=0, data_len, delka_len;

	if (argc < 2) shriek("Malo vstupnich parametru.");

	f_txt = fopen(argv[0], "rb");
	f_f0 = fopen(argv[1], "rb");
	if (!f_txt || !f_f0) shriek("Nepodarilo se nacist vstupni soubory.");

	// vytvor vetu
	f0_delka = 0;
	pocet_slov = -1;
	data_len = _msize(data);
	delka_len = _msize(delka)/2;
	do {
		fscanf(f_txt, "%d %d", &start, &end);
		fscanf(f_txt, "%s %s", data + strlen(data));
		data[strlen(data)] = ' ';
		delka[++pocet_slov] = 10*(end-start);
		f0_delka += delka[pocet_slov];
		if ( strlen(data) > data_len-100 ){
			data = (char*)realloc(data, 4096+data_len);
			data_len = _msize(data);
		}
		if ( pocet_slov >= delka_len ){
			delka = (__int16*)realloc(delka, delka_len+40);
			delka_len = _msize(delka)/2;
		}
	}
	while(!feof(f_txt));
	if ( *data == '-') {
		pocet_slov--;
		delka++;
	}
	
	f0_delka /= 15;
	f0 = (__int16*)calloc(f0_delka+5, sizeof(*f0));
	i=0;
	while(!feof(f_f0)) {
		fscanf(f_f0, "%d", f0 + i++);
		if (i >= f0_delka)
			shriek("Pocet segmentu f0 presahuje alokaci!");
	}

	if (data) 
		for(char *p=data; *p; p++) {
			if (*p == '\n' || *p == '\r') *p=' ';
			if (*p == '_') memmove(p, p+1, strlen(p));
			if (data && strspn(data, ",.!?:;+=-*/&^%#$_<>{}()[]|\\~`' \04\t\"") == strlen(data))
				shriek("Input text too funny");
		}
	fclose(f_txt);
	fclose(f_f0);
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
	FILE *f_scp_txt = NULL, *f_scp_f0 = NULL;
	char *file_txt, *file_f0, *argn[]={NULL, NULL};

	if (argc > 1 && !strcmp(argv[1], "--help")) {
		dump_help();
		exit(0);
	}

#ifdef HAVE_WINSOCK
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
	charset = "cp1250";
#endif
	start_service();		/* Windows NT etc. only */

	ctrld = connect_socket(0, 0);
	datad = connect_socket(0, 0);
	send_to_epos("data ", datad);
	ch = get_handle(ctrld);
	send_to_epos(ch, datad);
	send_to_epos("\r\n", datad);
	free(ch);
	send_to_epos("setl charset ",ctrld);
	send_to_epos(charset, ctrld);
	send_to_epos("\r\n", ctrld);
	get_result(ctrld);
	dh = get_handle(datad);
	get_result(datad);
	send_option("voice", "tdp-mbr");
	send_option("init_t", "100");

	data = (char*)calloc(4096, 1);
	//f0 = (__int16 *)calloc(400, sizeof(*f0));
	delka = (__int16 *)calloc(40, sizeof(*delka));

	if ( !strcmp(argv[1], "-S") ) {
		if ( !(f_scp_txt = fopen(argv[2], "rb")) )
			shriek( "Nepodarilo se nacist seznam souboru s textem -S !soubor.scp!");
		if ( argc > 3 && !(f_scp_f0 = fopen(argv[3], "rb")) )
			shriek( "Nepodarilo se nacist seznam souboru s f0 -S 'souborTXT.scp' !souborF0.scp!");

		argn[0] = (char*)calloc(MAX_CHAR, sizeof(*argn[0]));
		argn[1] = (char*)calloc(MAX_CHAR, sizeof(*argn[0]));

		while ( !feof(f_scp_txt) || !feof(f_scp_f0) ) {
			fscanf(f_scp_txt, "%s", argn[0]);
			if (f_scp_f0)
				fscanf(f_scp_f0, "%s", argn[0]);
			else{
				strncat(argn[1], argn[0], strrchr(argn[0], (int)'\\')-argn[0]+1);
				strcat(argn[1], "f0.txt");
			}
		send_cmd_line(2, argn);
		say_data(argn);
		memset(data, 0, strlen(data));
		free(f0);
		memset(delka, 0, _msize(delka)/2);
		memset(argn[1], 0, MAX_CHAR);
		}
	}
	else {
		send_cmd_line(argc-1, argv);
		say_data(argv);
	}
	
	send_to_epos("delh ", ctrld);
	send_to_epos(dh, ctrld);
	send_to_epos("\r\ndone\r\n", ctrld);
	get_result(ctrld);
	//get_result(ctrld);
	close(datad);
	close(ctrld);
	return 0;
}


