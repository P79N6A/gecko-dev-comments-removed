





















#include <string.h>
#include "pixman-xserver-compat.h"

#ifdef RENDER





#define N_BITS	4
#define rasterizeEdges	fbRasterizeEdges4

#if BITMAP_BIT_ORDER == LSBFirst
#define Shift4(o)	((o) << 2)
#else
#define Shift4(o)	((1-(o)) << 2)
#endif

#define Get4(x,o)	(((x) >> Shift4(o)) & 0xf)
#define Put4(x,o,v)	(((x) & ~(0xf << Shift4(o))) | (((v) & 0xf) << Shift4(o)))

#define DefineAlpha(line,x) \
    CARD8   *__ap = (CARD8 *) line + ((x) >> 1); \
    int	    __ao = (x) & 1

#define StepAlpha	((__ap += __ao), (__ao ^= 1))

#define AddAlpha(a) {						\
    CARD8   __o = *__ap;					\
    CARD8   __a = (a) + Get4(__o, __ao);			\
    *__ap = Put4 (__o, __ao, __a | (0 - ((__a) >> 4)));		\
}

#include "fbedgeimp.h"

#undef AddAlpha
#undef StepAlpha
#undef DefineAlpha
#undef rasterizeEdges
#undef N_BITS





#define N_BITS 1
#define rasterizeEdges	fbRasterizeEdges1

#include "fbedgeimp.h"

#undef rasterizeEdges
#undef N_BITS





static INLINE CARD8
clip255 (int x)
{
    if (x > 255) return 255;
    return x;
}

static INLINE void
add_saturate_8 (CARD8 *buf, int value, int length)
{
    while (length--)
    {
        *buf = clip255 (*buf + value);
        buf++;
    }
}












static void
fbRasterizeEdges8 (FbBits	*buf,
		   int		width,
		   int		stride,
		   RenderEdge	*l,
		   RenderEdge	*r,
		   xFixed	t,
		   xFixed	b)
{
    xFixed  y = t;
    FbBits  *line;
    int fill_start = -1, fill_end = -1;
    int fill_size = 0;

    line = buf + xFixedToInt (y) * stride;

    for (;;)
    {
        CARD8 *ap = (CARD8 *) line;
	xFixed	lx, rx;
	int	lxi, rxi;

	
	lx = l->x;
	if (lx < 0)
	    lx = 0;
	rx = r->x;
	if (xFixedToInt (rx) >= width)
	    rx = IntToxFixed (width);

	
	if (rx > lx)
	{
            int lxs, rxs;

	    
	    lxi = xFixedToInt (lx);
	    rxi = xFixedToInt (rx);

            
            lxs = RenderSamplesX (lx, 8);
            rxs = RenderSamplesX (rx, 8);

            
            if (lxi == rxi)
            {
                ap[lxi] = clip255 (ap[lxi] + rxs - lxs);
            }
            else
            {
                ap[lxi] = clip255 (ap[lxi] + N_X_FRAC(8) - lxs);

                
                lxi++;

                

                if (rxi - lxi > 4)
                {
                    if (fill_start < 0)
                    {
                        fill_start = lxi;
                        fill_end = rxi;
                        fill_size++;
                    }
                    else
                    {
                        if (lxi >= fill_end || rxi < fill_start)
                        {
                            
                            add_saturate_8 (ap + fill_start,
                                            fill_size * N_X_FRAC(8),
                                            fill_end - fill_start);
                            fill_start = lxi;
                            fill_end = rxi;
                            fill_size = 1;
                        }
                        else
                        {
                            
                            if (lxi > fill_start)
                            {
                                add_saturate_8 (ap + fill_start,
                                                fill_size * N_X_FRAC(8),
                                                lxi - fill_start);
                                fill_start = lxi;
                            }
                            else if (lxi < fill_start)
                            {
                                add_saturate_8 (ap + lxi, N_X_FRAC(8),
                                                fill_start - lxi);
                            }

                            
                            if (rxi < fill_end)
                            {
                                add_saturate_8 (ap + rxi,
                                                fill_size * N_X_FRAC(8),
                                                fill_end - rxi);
                                fill_end = rxi;
                            }
                            else if (fill_end < rxi)
                            {
                                add_saturate_8 (ap + fill_end,
                                                N_X_FRAC(8),
                                                rxi - fill_end);
                            }
                            fill_size++;
                        }
                    }
                }
                else
                {
                    add_saturate_8 (ap + lxi, N_X_FRAC(8), rxi - lxi);
                }

                


                if (rxs)
                    ap[rxi] = clip255 (ap[rxi] + rxs);
            }
	}

	if (y == b) {
            
            if (fill_start != fill_end) {
                if (fill_size == N_Y_FRAC(8))
                {
                    memset (ap + fill_start, 0xff, fill_end - fill_start);
                }
                else
                {
                    add_saturate_8 (ap + fill_start, fill_size * N_X_FRAC(8),
                                    fill_end - fill_start);
                }
            }
	    break;
        }

	if (xFixedFrac (y) != Y_FRAC_LAST(8))
	{
	    RenderEdgeStepSmall (l);
	    RenderEdgeStepSmall (r);
	    y += STEP_Y_SMALL(8);
	}
	else
	{
	    RenderEdgeStepBig (l);
	    RenderEdgeStepBig (r);
	    y += STEP_Y_BIG(8);
            if (fill_start != fill_end)
            {
                if (fill_size == N_Y_FRAC(8))
                {
                    memset (ap + fill_start, 0xff, fill_end - fill_start);
                }
                else
                {
                    add_saturate_8 (ap + fill_start, fill_size * N_X_FRAC(8),
                                    fill_end - fill_start);
                }
                fill_start = fill_end = -1;
                fill_size = 0;
            }
	    line += stride;
	}
    }
}

void
fbRasterizeEdges (FbBits	*buf,
		  int		bpp,
		  int		width,
		  int		stride,
		  RenderEdge	*l,
		  RenderEdge	*r,
		  xFixed	t,
		  xFixed	b)
{
    switch (bpp) {
    case 1:
	fbRasterizeEdges1 (buf, width, stride, l, r, t, b);
	break;
    case 4:
	fbRasterizeEdges4 (buf, width, stride, l, r, t, b);
	break;
    case 8:
	fbRasterizeEdges8 (buf, width, stride, l, r, t, b);
	break;
    }
}

#endif 
