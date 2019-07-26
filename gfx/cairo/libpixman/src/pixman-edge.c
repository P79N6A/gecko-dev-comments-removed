





















#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "pixman-private.h"
#include "pixman-accessor.h"




#define RENDER_EDGE_STEP_SMALL(edge)					\
    {									\
	edge->x += edge->stepx_small;					\
	edge->e += edge->dx_small;					\
	if (edge->e > 0)						\
	{								\
	    edge->e -= edge->dy;					\
	    edge->x += edge->signdx;					\
	}								\
    }




#define RENDER_EDGE_STEP_BIG(edge)					\
    {									\
	edge->x += edge->stepx_big;					\
	edge->e += edge->dx_big;					\
	if (edge->e > 0)						\
	{								\
	    edge->e -= edge->dy;					\
	    edge->x += edge->signdx;					\
	}								\
    }

#ifdef PIXMAN_FB_ACCESSORS
#define PIXMAN_RASTERIZE_EDGES pixman_rasterize_edges_accessors
#else
#define PIXMAN_RASTERIZE_EDGES pixman_rasterize_edges_no_accessors
#endif





#define N_BITS  4
#define RASTERIZE_EDGES rasterize_edges_4

#ifndef WORDS_BIGENDIAN
#define SHIFT_4(o)      ((o) << 2)
#else
#define SHIFT_4(o)      ((1 - (o)) << 2)
#endif

#define GET_4(x, o)      (((x) >> SHIFT_4 (o)) & 0xf)
#define PUT_4(x, o, v)							\
    (((x) & ~(0xf << SHIFT_4 (o))) | (((v) & 0xf) << SHIFT_4 (o)))

#define DEFINE_ALPHA(line, x)						\
    uint8_t   *__ap = (uint8_t *) line + ((x) >> 1);			\
    int __ao = (x) & 1

#define STEP_ALPHA      ((__ap += __ao), (__ao ^= 1))

#define ADD_ALPHA(a)							\
    {									\
        uint8_t __o = READ (image, __ap);				\
        uint8_t __a = (a) + GET_4 (__o, __ao);				\
        WRITE (image, __ap, PUT_4 (__o, __ao, __a | (0 - ((__a) >> 4)))); \
    }

#include "pixman-edge-imp.h"

#undef ADD_ALPHA
#undef STEP_ALPHA
#undef DEFINE_ALPHA
#undef RASTERIZE_EDGES
#undef N_BITS






#define N_BITS 1
#define RASTERIZE_EDGES rasterize_edges_1

#include "pixman-edge-imp.h"

#undef RASTERIZE_EDGES
#undef N_BITS





static force_inline uint8_t
clip255 (int x)
{
    if (x > 255)
	return 255;

    return x;
}

#define ADD_SATURATE_8(buf, val, length)				\
    do									\
    {									\
        int i__ = (length);						\
        uint8_t *buf__ = (buf);						\
        int val__ = (val);						\
									\
        while (i__--)							\
        {								\
            WRITE (image, (buf__), clip255 (READ (image, (buf__)) + (val__))); \
            (buf__)++;							\
	}								\
    } while (0)












static void
rasterize_edges_8 (pixman_image_t *image,
                   pixman_edge_t * l,
                   pixman_edge_t * r,
                   pixman_fixed_t  t,
                   pixman_fixed_t  b)
{
    pixman_fixed_t y = t;
    uint32_t  *line;
    int fill_start = -1, fill_end = -1;
    int fill_size = 0;
    uint32_t *buf = (image)->bits.bits;
    int stride = (image)->bits.rowstride;
    int width = (image)->bits.width;

    line = buf + pixman_fixed_to_int (y) * stride;

    for (;;)
    {
        uint8_t *ap = (uint8_t *) line;
        pixman_fixed_t lx, rx;
        int lxi, rxi;

        
        lx = l->x;
        if (lx < 0)
	    lx = 0;

        rx = r->x;

        if (pixman_fixed_to_int (rx) >= width)
	{
	    



	    rx = pixman_int_to_fixed (width) - 1;
	}

        
        if (rx > lx)
        {
            int lxs, rxs;

            
            lxi = pixman_fixed_to_int (lx);
            rxi = pixman_fixed_to_int (rx);

            
            lxs = RENDER_SAMPLES_X (lx, 8);
            rxs = RENDER_SAMPLES_X (rx, 8);

            
            if (lxi == rxi)
            {
                WRITE (image, ap + lxi,
		       clip255 (READ (image, ap + lxi) + rxs - lxs));
	    }
            else
            {
                WRITE (image, ap + lxi,
		       clip255 (READ (image, ap + lxi) + N_X_FRAC (8) - lxs));

                
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
                            
                            ADD_SATURATE_8 (ap + fill_start,
                                            fill_size * N_X_FRAC (8),
                                            fill_end - fill_start);
                            fill_start = lxi;
                            fill_end = rxi;
                            fill_size = 1;
			}
                        else
                        {
                            
                            if (lxi > fill_start)
                            {
                                ADD_SATURATE_8 (ap + fill_start,
                                                fill_size * N_X_FRAC (8),
                                                lxi - fill_start);
                                fill_start = lxi;
			    }
                            else if (lxi < fill_start)
                            {
                                ADD_SATURATE_8 (ap + lxi, N_X_FRAC (8),
                                                fill_start - lxi);
			    }

                            
                            if (rxi < fill_end)
                            {
                                ADD_SATURATE_8 (ap + rxi,
                                                fill_size * N_X_FRAC (8),
                                                fill_end - rxi);
                                fill_end = rxi;
			    }
                            else if (fill_end < rxi)
                            {
                                ADD_SATURATE_8 (ap + fill_end,
                                                N_X_FRAC (8),
                                                rxi - fill_end);
			    }
                            fill_size++;
			}
		    }
		}
                else
                {
                    ADD_SATURATE_8 (ap + lxi, N_X_FRAC (8), rxi - lxi);
		}

                WRITE (image, ap + rxi, clip255 (READ (image, ap + rxi) + rxs));
	    }
	}

        if (y == b)
        {
            
            if (fill_start != fill_end)
            {
                if (fill_size == N_Y_FRAC (8))
                {
                    MEMSET_WRAPPED (image, ap + fill_start,
				    0xff, fill_end - fill_start);
		}
                else
                {
                    ADD_SATURATE_8 (ap + fill_start, fill_size * N_X_FRAC (8),
                                    fill_end - fill_start);
		}
	    }
            break;
	}

        if (pixman_fixed_frac (y) != Y_FRAC_LAST (8))
        {
            RENDER_EDGE_STEP_SMALL (l);
            RENDER_EDGE_STEP_SMALL (r);
            y += STEP_Y_SMALL (8);
	}
        else
        {
            RENDER_EDGE_STEP_BIG (l);
            RENDER_EDGE_STEP_BIG (r);
            y += STEP_Y_BIG (8);
            if (fill_start != fill_end)
            {
                if (fill_size == N_Y_FRAC (8))
                {
                    MEMSET_WRAPPED (image, ap + fill_start,
				    0xff, fill_end - fill_start);
		}
                else
                {
                    ADD_SATURATE_8 (ap + fill_start, fill_size * N_X_FRAC (8),
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
                        pixman_edge_t * l,
                        pixman_edge_t * r,
                        pixman_fixed_t  t,
                        pixman_fixed_t  b)
{
    switch (PIXMAN_FORMAT_BPP (image->bits.format))
    {
    case 1:
	rasterize_edges_1 (image, l, r, t, b);
	break;

    case 4:
	rasterize_edges_4 (image, l, r, t, b);
	break;

    case 8:
	rasterize_edges_8 (image, l, r, t, b);
	break;

    default:
        break;
    }
}

#ifndef PIXMAN_FB_ACCESSORS

PIXMAN_EXPORT void
pixman_rasterize_edges (pixman_image_t *image,
                        pixman_edge_t * l,
                        pixman_edge_t * r,
                        pixman_fixed_t  t,
                        pixman_fixed_t  b)
{
    return_if_fail (image->type == BITS);
    return_if_fail (PIXMAN_FORMAT_TYPE (image->bits.format) == PIXMAN_TYPE_A);
    
    if (image->bits.read_func || image->bits.write_func)
	pixman_rasterize_edges_accessors (image, l, r, t, b);
    else
	pixman_rasterize_edges_no_accessors (image, l, r, t, b);
}

#endif
