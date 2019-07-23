























#include "dmg_fp.h"

namespace dmg_fp {

 char *
g_fmt(register char *b, double x)
{
	register int i, k;
	register char *s;
	int decpt, j, sign;
	char *b0, *s0, *se;

	b0 = b;
#ifdef IGNORE_ZERO_SIGN
	if (!x) {
		*b++ = '0';
		*b = 0;
		goto done;
		}
#endif
	s = s0 = dtoa(x, 0, 0, &decpt, &sign, &se);
	if (sign)
		*b++ = '-';
	if (decpt == 9999)  {
		while((*b++ = *s++));
		goto done0;
		}
	if (decpt <= -4 || decpt > se - s + 5) {
		*b++ = *s++;
		if (*s) {
			*b++ = '.';
			while((*b = *s++))
				b++;
			}
		*b++ = 'e';
		
		if (--decpt < 0) {
			*b++ = '-';
			decpt = -decpt;
			}
		else
			*b++ = '+';
		for(j = 2, k = 10; 10*k <= decpt; j++, k *= 10);
		for(;;) {
			i = decpt / k;
			*b++ = i + '0';
			if (--j <= 0)
				break;
			decpt -= i*k;
			decpt *= 10;
			}
		*b = 0;
		}
	else if (decpt <= 0) {
		*b++ = '.';
		for(; decpt < 0; decpt++)
			*b++ = '0';
		while((*b++ = *s++));
		}
	else {
		while((*b = *s++)) {
			b++;
			if (--decpt == 0 && *s)
				*b++ = '.';
			}
		for(; decpt > 0; decpt--)
			*b++ = '0';
		*b = 0;
		}
 done0:
	freedtoa(s0);
#ifdef IGNORE_ZERO_SIGN
 done:
#endif
	return b0;
	}

}  
