FORDEL 

class wavefm {};


class t_any
{
   protected:
	union
	{
		wavefm *w;
	} u;
   public:
	virtual ~t_any() {};
};

class t_wavefm : t_any
{
   public:
	t_wavefm(wavefm *w) { ut.w = w; }
	virtual ~t_wavefm() { delete ut.w; }
};

inb->bus()
inb->data.w