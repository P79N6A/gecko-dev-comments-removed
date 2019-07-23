























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
    uint32_t stride = (image)->bits.rowstride;
    uint32_t width = (image)->bits.width;
    
    line = buf + pixman_fixed_to_int (y) * stride;
    
    for (;;)
    {
	pixman_fixed_t	lx;
	pixman_fixed_t      rx;
	int	lxi;
	int rxi;
	
	
	lx = l->x;
	if (lx < 0)
	    lx = 0;
	rx = r->x;
	if (pixman_fixed_to_int (rx) >= width)
	    rx = pixman_int_to_fixed (width);
	
	
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
			WRITE(a, READ(a) | startmask);
			a++;
		    }
		    while (nmiddle--)
			WRITE(a++, FB_ALLONES);
		    if (endmask)
			WRITE(a, READ(a) | endmask);
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
		    



		    if (rxs != 0)
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
