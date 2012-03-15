//enum mclass { mc_f, mc_i, mc_t };

#define mclass FIT_IDX

/*
 *	More comments should eventually come here.  These markers are attached
 *	to units, in an unlimited number.  At present, they carry prosodic
 *	information (only intensity and pitch, not duration) relating either
 *	to the unit, or to a specified point within it.
 *
 *	This was added between 2.4 and 2.5.
 */

class marker
{
	friend class unit;

	mclass quant;
	bool extent;
	int par;
	float pos;
	marker *next;

   public:
	marker();
	~marker();
	marker(mclass mc, bool e, int p, marker *m, float po) { quant = mc; extent = e; par = p; next = m; pos = po; };
	marker *derived();
	void merge(marker *ma, marker **fixup);

	bool operator < (marker ma);
	
	int write_ssif(char *whither);

	void *operator new(size_t size);
	void operator delete(void *ptr);
};

marker::marker()
{
	next = NULL;
}

marker::~marker()
{
	if (next) delete next;
	next = NULL;
}

marker *marker::derived()
{
	if (!this) return NULL;
	marker *nm = new marker;
	nm->next = next->derived();
	nm->quant = quant;
	nm->extent = extent;
	nm->par = par;
	nm->pos = pos;
	return nm;
}

void
marker::merge(marker *ma, marker **fixup)	// FIXME: ma == NULL
{
	if (!this) {
		*fixup = ma;
		return;
	}
	if (*this < *ma) {
		if (!next) next = ma;
		else next->merge(ma, &next);
	} else {
		if (quant == ma->quant && pos == ma->pos && extent && ma->extent) {
			par += ma->par;		// merge compatible extent markers
			marker *nm = ma->next;
			ma->next = NULL;
			delete ma;
			ma = nm;
			if (nm) merge(nm, fixup);
		} else {
			*fixup = ma;
			if (!ma->next) ma->next = this;
			else ma->next->merge(this, &ma->next);
		}
	}
}

bool
marker::operator < (marker ma)
{
	if (extent == ma.extent) return pos < ma.pos;
	else return !extent;
}


int
marker::write_ssif(char *whither)
{
	if (quant == Q_FREQ) return sprintf(whither, "(%d,%d) ", (int)(pos * 100),
		this_voice->init_f + par, extent?"yes":"no");
	else return 0;
}
