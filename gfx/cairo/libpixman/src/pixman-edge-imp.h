





















#ifndef rasterizeSpan
#endif

static void
rasterizeEdges (pixman_image_t  *image,
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
		uint32_t  *a = line;
		uint32_t  startmask;
		uint32_t  endmask;
		int	    nmiddle;
		int	    width = rxi - lxi;
		int	    x = lxi;

		a += x >> FB_SHIFT;
		x &= FB_MASK;

		FbMaskBits (x, width, startmask, nmiddle, endmask);
		    if (startmask) {
			WRITE(image, a, READ(image, a) | startmask);
			a++;
		    }
		    while (nmiddle--)
			WRITE(image, a++, FB_ALLONES);
		    if (endmask)
			WRITE(image, a, READ(image, a) | endmask);
	    }
#else
	    {
		DefineAlpha(line,lxi);
		int	    lxs;
		int     rxs;

		
		lxs = RenderSamplesX (lx, N_BITS);
		rxs = RenderSamplesX (rx, N_BITS);

		
		if (lxi == rxi)
		{
		    AddAlpha (rxs - lxs);
		}
		else
		{
		    int	xi;

		    AddAlpha (N_X_FRAC(N_BITS) - lxs);
		    StepAlpha;
		    for (xi = lxi + 1; xi < rxi; xi++)
		    {
			AddAlpha (N_X_FRAC(N_BITS));
			StepAlpha;
		    }
		    AddAlpha (rxs);
		}
	    }
#endif
	}

	if (y == b)
	    break;

#if N_BITS > 1
	if (pixman_fixed_frac (y) != Y_FRAC_LAST(N_BITS))
	{
	    RenderEdgeStepSmall (l);
	    RenderEdgeStepSmall (r);
	    y += STEP_Y_SMALL(N_BITS);
	}
	else
#endif
	{
	    RenderEdgeStepBig (l);
	    RenderEdgeStepBig (r);
	    y += STEP_Y_BIG(N_BITS);
	    line += stride;
	}
    }
}

#undef rasterizeSpan
