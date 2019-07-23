


















































#include "fdlibm.h"
#include <errno.h>

#ifdef __STDC__
	double fd_ldexp(double value, int exp)
#else
	double fd_ldexp(value, exp)
	double value; int exp;
#endif
{
	if(!fd_finite(value)||value==0.0) return value;
	value = fd_scalbn(value,exp);
	if(!fd_finite(value)||value==0.0) errno = ERANGE;
	return value;
}
