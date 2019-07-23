





















#ifndef rasterizeSpan
#endif

static void
rasterizeEdges (FbBits		*buf,
		int		width,
		int		stride,
		RenderEdge	*l,
		RenderEdge	*r,
		xFixed		t,
		xFixed		b)
{
    xFixed  y = t;
    FbBits  *line;

    line = buf + xFixedToInt (y) * stride;

    for (;;)
    {
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

	    
	    lxi = xFixedToInt (lx);
	    rxi = xFixedToInt (rx);

#if N_BITS == 1
	    {
		FbBits  *a = line;
		FbBits  startmask, endmask;
		int	    nmiddle;
		int	    width = rxi - lxi;
		int	    x = lxi;

		a += x >> FB_SHIFT;
		x &= FB_MASK;

		FbMaskBits (x, width, startmask, nmiddle, endmask);
		if (startmask)
		    *a++ |= startmask;
		while (nmiddle--)
		    *a++ = FB_ALLONES;
		if (endmask)
		    *a |= endmask;
	    }
#else
	    {
		DefineAlpha(line,lxi);
		int	    lxs, rxs;

		
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
		    


		    if (rxs)
			AddAlpha (rxs);
		}
	    }
#endif
	}

	if (y == b)
	    break;

#if N_BITS > 1
	if (xFixedFrac (y) != Y_FRAC_LAST(N_BITS))
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
