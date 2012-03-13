/*
 *	(c) 1998 Jirka Hanika <geo@ff.cuni.cz>
 *
 *	This single source file src/client.h, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precendence.
 *
 *	This file implements supporting functions for a simple TTSCP client.
 *	See doc/english/ttscp.doc for a preliminary technical specification.
 *
 *	Note: the functions declared here often trash the scratch buffer.
 */



/*
 *	The sgets() and sputs() routines provide a slow get line and put line
 *	interface, especially on TTSCP control connections. They are suitable
 *	for the client. They block until receipt data, which is a problem for
 *	tcpsyn.
 */

int sgets(char *buffer, int buffer_size, int sd);
int sputs(const char *buffer, int sd);

/*
 *	The just_connect_socket() routine returns -1 if it cannot return a connected
 *	socket. The connect_socket() routine additionally checks if the remote side
 *	announces the TTSCP protocol of an acceptable version and calls shriek(674)
 *	if it doesn't.
 *
 *	Byte order: host byte order for the port number, network byte order for the addr.
 *	This is because the address has typically been acquired through gethostbyname(),
 *	while the port number is probably constant or directly specified by the user.
 */

int just_connect_socket(unsigned int ipaddr, int port);	// returns -1 if not connected
int connect_socket(unsigned int ipaddr, int port);		// as above, check protocol, shriek if bad

/*
 *	getaddrbyname() converts an Internet host name (or address in the dotted
 *	format) to the host byte order IP address.
 */

int getaddrbyname(const char *inet_name);

/*
 *	xmit_option() send the "set" command appropriate for setting a named
 *	option to a specified value. 
 */
void xmit_option(char *name, char *value, int sd);

/*
 *	send_appl() sends an apply command and blocks until it is processed.
 *	It returns the number of bytes to be then read from the data connection.
 *	send_appl() doesn't ever touch the data connection.
 */

int send_appl(int bytes, int ctrld);

/*
 *	data_conn() turns sd into a data connection.
 *	It returns strdup(data connection handle).
 */

char *data_conn(int sd);

/*
 *	sync_finish_command() can be used to wait for the completion code
 *	for a command. It will block until it is received.
 *	FIXME: it doesn't treat errors and such very well.
 */

void sync_finish_command(int ctrld);	// wait for the completion code

/*
 *	make_rnd_passwd() generates a random password or handle consisting
 *	of lowercase and uppercase characters, digits, dash and underscore.
 */

void make_rnd_passwd(char *buffer, int size);

