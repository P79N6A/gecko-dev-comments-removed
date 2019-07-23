


















































#include "fdlibm.h"

#ifdef __STDC__
	int fd_matherr(struct exception *x)
#else
	int fd_matherr(x)
	struct exception *x;
#endif
{
	int n=0;
	if(x->arg1!=x->arg1) return 0;
	return n;
}
