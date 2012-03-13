#define LOCALSOUNDAGENT "localsound"
#define LOCALSOUNDFILE	"/dev/dsp"

int localsound = -1;

int special_io(const char *name, DATA_TYPE intype)
{
	if (intype == T_INPUT || strcmp(name, LOCALSOUNDAGENT))
		shriek(415, fmt("Bad stream component %s", name));
//	if (...) shriek(453, "Not allowed to use localsound");		// FIXME
	if (localsound != -1) return localsound;
	int r = open(LOCALSOUNDFILE, O_WRONLY | O_NONBLOCK);
	if (r == -1) shriek(462, fmt("Could not open localsound device, error %d", errno));
	localsound = r;
	return r;
}
