

char *charset = "8859-2";

int size;
const char *other_traffic_prefix = "Unexpected: ";

bool debug_ttscp = true;
int ctrld, datad;		/* file descriptors for the control and data connections */
char *data = NULL;

char *ch;
char *dh;

int send_to_epos(const char *buffer, int socket)
{
	if (debug_ttscp && socket == ctrld)
		printf("%s", buffer);
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
			case '4': //printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return 4;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
					exit(0);
				  } /* else fall through */
			case '8': //printf("%s\n", scratch+strspn(scratch, "0123456789x "));
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