













#ifndef _LIBP_ALIGN_H_
#define _LIBP_ALIGN_H_




union max_align
{
	char   c;
	short  s;
	long   l;
	int    i;
	float  f;
	double d;
	void * v;
	void (*q)(void);
};

typedef union max_align max_align_t;

#endif

