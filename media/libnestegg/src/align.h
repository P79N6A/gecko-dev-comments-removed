













#ifndef _LIBP_ALIGN_H_
#define _LIBP_ALIGN_H_

#ifdef _MSC_VER




typedef double max_align_t;

#else




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

#endif

