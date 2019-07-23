





















#ifndef rasterize_span
#endif

static void
RASTERIZE_EDGES (pixman_image_t  *image,
		pixman_edge_t	*l,
		pixman_edge_t	*r,
		pixman_fixed_t		t,
		pixman_fixed_t		b)
{
    pixman_fixed_t  y = t;
    uint32_t  *line;
    uint32_t *buf = (image)->bits.bits;
    int stride = (image)->bits.rowstride;
    int width = (image)->bits.width;

    line = buf + pixman_fixed_to_int (y) * stride;

    for (;;)
    {
	pixman_fixed_t	lx;
	pixman_fixed_t      rx;
	int	lxi;
	int rxi;

	lx = l->x;
	rx = r->x;
#if N_BITS == 1
	


	lx += X_FRAC_FIRST(1);
	rx += X_FRAC_FIRST(1);
#endif
	
	if (lx < 0)
	    lx = 0;
	if (pixman_fixed_to_int (rx) >= width)
#if N_BITS == 1
	    rx = pixman_int_to_fixed (width);
#else
	    



	    rx = pixman_int_to_fixed (width) - 1;
#endif

	
	if (rx > lx)
	{

	    
	    lxi = pixman_fixed_to_int (lx);
	    rxi = pixman_fixed_to_int (rx);

#if N_BITS == 1
	    {

#ifdef WORDS_BIGENDIAN
#   define SCREEN_SHIFT_LEFT(x,n)	((x) << (n))
#   define SCREEN_SHIFT_RIGHT(x,n)	((x) >> (n))
#else
#   define SCREEN_SHIFT_LEFT(x,n)	((x) >> (n))
#   define SCREEN_SHIFT_RIGHT(x,n)	((x) << (n))
#endif

#define LEFT_MASK(x)							\
		(((x) & 0x1f) ?						\
		 SCREEN_SHIFT_RIGHT (0xffffffff, (x) & 0x1f) : 0)
#define RIGHT_MASK(x)							\
		(((32 - (x)) & 0x1f) ?					\
		 SCREEN_SHIFT_LEFT (0xffffffff, (32 - (x)) & 0x1f) : 0)
		
#define MASK_BITS(x,w,l,n,r) {						\
		    n = (w);						\
		    r = RIGHT_MASK ((x) + n);				\
		    l = LEFT_MASK (x);					\
		    if (l) {						\
			n -= 32 - ((x) & 0x1f);				\
			if (n < 0) {					\
			    n = 0;					\
			    l &= r;					\
			    r = 0;					\
			}						\
		    }							\
		    n >>= 5;						\
		}
		
		uint32_t  *a = line;
		uint32_t  startmask;
		uint32_t  endmask;
		int	    nmiddle;
		int	    width = rxi - lxi;
		int	    x = lxi;
		
		a += x >> 5;
		x &= 0x1f;
		
		MASK_BITS (x, width, startmask, nmiddle, endmask);

		if (startmask) {
		    WRITE(image, a, READ(image, a) | startmask);
		    a++;
		}
		while (nmiddle--)
		    WRITE(image, a++, 0xffffffff);
		if (endmask)
		    WRITE(image, a, READ(image, a) | endmask);
	    }
#else
	    {
		DEFINE_ALPHA(line,lxi);
		int	    lxs;
		int     rxs;

		
		lxs = RENDER_SAMPLES_X (lx, N_BITS);
		rxs = RENDER_SAMPLES_X (rx, N_BITS);

		
		if (lxi == rxi)
		{
		    ADD_ALPHA (rxs - lxs);
		}
		else
		{
		    int	xi;

		    ADD_ALPHA (N_X_FRAC(N_BITS) - lxs);
		    STEP_ALPHA;
		    for (xi = lxi + 1; xi < rxi; xi++)
		    {
			ADD_ALPHA (N_X_FRAC(N_BITS));
			STEP_ALPHA;
		    }
		    ADD_ALPHA (rxs);
		}
	    }
#endif
	}

	if (y == b)
	    break;

#if N_BITS > 1
	if (pixman_fixed_frac (y) != Y_FRAC_LAST(N_BITS))
	{
	    RENDER_EDGE_STEP_SMALL (l);
	    RENDER_EDGE_STEP_SMALL (r);
	    y += STEP_Y_SMALL(N_BITS);
	}
	else
#endif
	{
	    RENDER_EDGE_STEP_BIG (l);
	    RENDER_EDGE_STEP_BIG (r);
	    y += STEP_Y_BIG(N_BITS);
	    line += stride;
	}
    }
}

#undef rasterize_span
