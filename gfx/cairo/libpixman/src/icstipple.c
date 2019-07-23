























#include "pixman-xserver-compat.h"

#ifndef ICNOPIXADDR





#define LaneCases1(c,a)	    case c: \
				while (n--) { (void)FbLaneCase(c,a); a++; } \
				break
#define LaneCases2(c,a)	    LaneCases1(c,a); LaneCases1(c+1,a)
#define LaneCases4(c,a)	    LaneCases2(c,a); LaneCases2(c+2,a)
#define LaneCases8(c,a)	    LaneCases4(c,a); LaneCases4(c+4,a)
#define LaneCases16(c,a)    LaneCases8(c,a); LaneCases8(c+8,a)
#define LaneCases32(c,a)    LaneCases16(c,a); LaneCases16(c+16,a)
#define LaneCases64(c,a)    LaneCases32(c,a); LaneCases32(c+32,a)
#define LaneCases128(c,a)   LaneCases64(c,a); LaneCases64(c+64,a)
#define LaneCases256(c,a)   LaneCases128(c,a); LaneCases128(c+128,a)

#if FB_SHIFT == 6
#define LaneCases(a)	    LaneCases256(0,a)
#endif

#if FB_SHIFT == 5
#define LaneCases(a)	    LaneCases16(0,a)
#endif





void
fbTransparentSpan (FbBits   *dst,
		   FbBits   stip,
		   FbBits   fgxor,
		   int	    n)
{
    FbStip  s;

    s  = ((FbStip) (stip      ) & 0x01);
    s |= ((FbStip) (stip >>  8) & 0x02);
    s |= ((FbStip) (stip >> 16) & 0x04);
    s |= ((FbStip) (stip >> 24) & 0x08);
#if FB_SHIFT > 5
    s |= ((FbStip) (stip >> 32) & 0x10);
    s |= ((FbStip) (stip >> 40) & 0x20);
    s |= ((FbStip) (stip >> 48) & 0x40);
    s |= ((FbStip) (stip >> 56) & 0x80);
#endif
    switch (s) {
	LaneCases(dst);
    }
}
#endif
