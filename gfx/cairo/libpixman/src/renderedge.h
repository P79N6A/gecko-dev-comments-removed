





















#ifndef _RENDEREDGE_H_
#define _RENDEREDGE_H_

#include "pixman-xserver-compat.h"








































#define MAX_ALPHA(n)	((1 << (n)) - 1)
#define N_Y_FRAC(n)	((n) == 1 ? 1 : (1 << ((n)/2)) - 1)
#define N_X_FRAC(n)	((1 << ((n)/2)) + 1)

#define STEP_Y_SMALL(n)	(xFixed1 / N_Y_FRAC(n))
#define STEP_Y_BIG(n)	(xFixed1 - (N_Y_FRAC(n) - 1) * STEP_Y_SMALL(n))

#define Y_FRAC_FIRST(n)	(STEP_Y_SMALL(n) / 2)
#define Y_FRAC_LAST(n)	(Y_FRAC_FIRST(n) + (N_Y_FRAC(n) - 1) * STEP_Y_SMALL(n))

#define STEP_X_SMALL(n)	(xFixed1 / N_X_FRAC(n))
#define STEP_X_BIG(n)	(xFixed1 - (N_X_FRAC(n) - 1) * STEP_X_SMALL(n))

#define X_FRAC_FIRST(n)	(STEP_X_SMALL(n) / 2)
#define X_FRAC_LAST(n)	(X_FRAC_FIRST(n) + (N_X_FRAC(n) - 1) * STEP_X_SMALL(n))

#define RenderSamplesX(x,n)	((n) == 1 ? 0 : (xFixedFrac (x) + X_FRAC_FIRST(n)) / STEP_X_SMALL(n))







typedef struct {
    xFixed   x;
    xFixed   e;
    xFixed   stepx;
    xFixed   signdx;
    xFixed   dy;
    xFixed   dx;

    xFixed   stepx_small;
    xFixed   stepx_big;
    xFixed   dx_small;
    xFixed   dx_big;
} RenderEdge;




#define RenderEdgeStepSmall(edge) { \
    (edge)->x += (edge)->stepx_small;   \
    (edge)->e += (edge)->dx_small;	    \
    if ((edge)->e > 0)		    \
    {				    \
	(edge)->e -= (edge)->dy;	    \
	(edge)->x += (edge)->signdx;    \
    }				    \
}




#define RenderEdgeStepBig(edge) {   \
    (edge)->x += (edge)->stepx_big;	    \
    (edge)->e += (edge)->dx_big;	    \
    if ((edge)->e > 0)		    \
    {				    \
	(edge)->e -= (edge)->dy;	    \
	(edge)->x += (edge)->signdx;    \
    }				    \
}

pixman_private xFixed
RenderSampleCeilY (xFixed y, int bpp);

pixman_private xFixed
RenderSampleFloorY (xFixed y, int bpp);

pixman_private void
RenderEdgeStep (RenderEdge *e, int n);

pixman_private void
RenderEdgeInit (RenderEdge	*e,
		int		bpp,
		xFixed		y_start,
		xFixed		x_top,
		xFixed		y_top,
		xFixed		x_bot,
		xFixed		y_bot);

pixman_private void
RenderLineFixedEdgeInit (RenderEdge *e,
			 int	    bpp,
			 xFixed	    y,
			 const pixman_line_fixed_t *line,
			 int	    x_off,
			 int	    y_off);

#endif 
