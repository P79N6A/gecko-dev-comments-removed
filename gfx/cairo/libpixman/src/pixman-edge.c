






















#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "pixman.h"
#include "pixman-private.h"

#ifdef PIXMAN_FB_ACCESSORS
#define PIXMAN_RASTERIZE_EDGES pixman_rasterize_edges_accessors
#else
#define PIXMAN_RASTERIZE_EDGES pixman_rasterize_edges_no_accessors
#endif





#define N_BITS	4
#define rasterizeEdges	fbRasterizeEdges4

#if BITMAP_BIT_ORDER == LSBFirst
#define Shift4(o)	((o) << 2)
#else
#define Shift4(o)	((1-(o)) << 2)
#endif

#define Get4(x,o)	(((x) >> Shift4(o)) & 0xf)
#define Put4(x,o,v)	(((x) & ~(0xf << Shift4(o))) | (((v) & 0xf) << Shift4(o)))

#define DefineAlpha(line,x)			     \
    uint8_t   *__ap = (uint8_t *) line + ((x) >> 1); \
    int	    __ao = (x) & 1

#define StepAlpha	((__ap += __ao), (__ao ^= 1))

#define AddAlpha(a) {						\
	uint8_t   __o = READ(__ap);				\
	uint8_t   __a = (a) + Get4(__o, __ao);			\
	WRITE(__ap, Put4 (__o, __ao, __a | (0 - ((__a) >> 4))));	\
    }

#include "pixman-edge-imp.h"

#undef AddAlpha
#undef StepAlpha
#undef DefineAlpha
#undef rasterizeEdges
#undef N_BITS






#define N_BITS 1
#define rasterizeEdges	fbRasterizeEdges1

#include "pixman-edge-imp.h"

#undef rasterizeEdges
#undef N_BITS





static inline uint8_t
clip255 (int x)
{
    if (x > 255) return 255;
    return x;
}

#define add_saturate_8(buf,val,length)				\
    do {							\
	int i__ = (length);					\
	uint8_t *buf__ = (buf);					\
	int val__ = (val);					\
								\
	while (i__--)						\
	{							\
	    WRITE((buf__), clip255 (READ((buf__)) + (val__)));	\
	    (buf__)++;						\
	}							\
    } while (0)












static void
fbRasterizeEdges8 (pixman_image_t       *image,
		   pixman_edge_t	*l,
		   pixman_edge_t	*r,
		   pixman_fixed_t	t,
		   pixman_fixed_t	b)
{
    pixman_fixed_t  y = t;
    uint32_t  *line;
    int fill_start = -1, fill_end = -1;
    int fill_size = 0;
    uint32_t *buf = (image)->bits.bits;		
    int32_t stride = (image)->bits.rowstride;	
    int32_t width = (image)->bits.width;
    
    line = buf + pixman_fixed_to_int (y) * stride;
    
    for (;;)
    {
        uint8_t *ap = (uint8_t *) line;
	pixman_fixed_t	lx, rx;
	int	lxi, rxi;
	
	
	lx = l->x;
	if (lx < 0)
	    lx = 0;
	rx = r->x;
	if (pixman_fixed_to_int (rx) >= width)
	    rx = pixman_int_to_fixed (width);
	
	
	if (rx > lx)
	{
            int lxs, rxs;
	    
	    
	    lxi = pixman_fixed_to_int (lx);
	    rxi = pixman_fixed_to_int (rx);
	    
            
            lxs = RenderSamplesX (lx, 8);
            rxs = RenderSamplesX (rx, 8);
	    
            
	    if (lxi == rxi)
	    {
		WRITE(ap +lxi, clip255 (READ(ap + lxi) + rxs - lxs));
	    }
	    else
	    {
		WRITE(ap + lxi, clip255 (READ(ap + lxi) + N_X_FRAC(8) - lxs));
		
		
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
		    WRITE(ap + rxi, clip255 (READ(ap + rxi) + rxs));
	    }
	}
	
	if (y == b) {
            
            if (fill_start != fill_end) {
		if (fill_size == N_Y_FRAC(8))
		{
		    MEMSET_WRAPPED (ap + fill_start, 0xff, fill_end - fill_start);
		}
		else
		{
		    add_saturate_8 (ap + fill_start, fill_size * N_X_FRAC(8),
				    fill_end - fill_start);
		}
            }
	    break;
        }
	
	if (pixman_fixed_frac (y) != Y_FRAC_LAST(8))
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
		    MEMSET_WRAPPED (ap + fill_start, 0xff, fill_end - fill_start);
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

#ifndef PIXMAN_FB_ACCESSORS
static
#endif
void
PIXMAN_RASTERIZE_EDGES (pixman_image_t *image,
			pixman_edge_t	*l,
			pixman_edge_t	*r,
			pixman_fixed_t	t,
			pixman_fixed_t	b)
{
    switch (PIXMAN_FORMAT_BPP (image->bits.format))
    {
    case 1:
	fbRasterizeEdges1 (image, l, r, t, b);
	break;
    case 4:
	fbRasterizeEdges4 (image, l, r, t, b);
	break;
    case 8:
	fbRasterizeEdges8 (image, l, r, t, b);
	break;
    }
}

#ifndef PIXMAN_FB_ACCESSORS

void
pixman_rasterize_edges (pixman_image_t *image,
			pixman_edge_t	*l,
			pixman_edge_t	*r,
			pixman_fixed_t	t,
			pixman_fixed_t	b)
{
    if (image->common.read_func	|| image->common.write_func)
	pixman_rasterize_edges_accessors (image, l, r, t, b);
    else
	pixman_rasterize_edges_no_accessors (image, l, r, t, b);
}

#endif
