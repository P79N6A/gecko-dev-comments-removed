





















#include "pixmanint.h"

#include "pixman-xserver-compat.h"






xFixed
RenderSampleCeilY (xFixed y, int n)
{
    xFixed   f = xFixedFrac(y);
    xFixed   i = xFixedFloor(y);

    f = ((f + Y_FRAC_FIRST(n)) / STEP_Y_SMALL(n)) * STEP_Y_SMALL(n) + Y_FRAC_FIRST(n);
    if (f > Y_FRAC_LAST(n))
    {
	f = Y_FRAC_FIRST(n);
	i += xFixed1;
    }
    return (i | f);
}

#define _div(a,b)    ((a) >= 0 ? (a) / (b) : -((-(a) + (b) - 1) / (b)))





xFixed
RenderSampleFloorY (xFixed y, int n)
{
    xFixed   f = xFixedFrac(y);
    xFixed   i = xFixedFloor (y);

    f = _div(f - Y_FRAC_FIRST(n), STEP_Y_SMALL(n)) * STEP_Y_SMALL(n) + Y_FRAC_FIRST(n);
    if (f < Y_FRAC_FIRST(n))
    {
	f = Y_FRAC_LAST(n);
	i -= xFixed1;
    }
    return (i | f);
}




void
RenderEdgeStep (RenderEdge *e, int n)
{
    xFixed_48_16	ne;

    e->x += n * e->stepx;

    ne = e->e + n * (xFixed_48_16) e->dx;

    if (n >= 0)
    {
	if (ne > 0)
	{
	    int nx = (ne + e->dy - 1) / e->dy;
	    e->e = ne - nx * (xFixed_48_16) e->dy;
	    e->x += nx * e->signdx;
	}
    }
    else
    {
	if (ne <= -e->dy)
	{
	    int nx = (-ne) / e->dy;
	    e->e = ne + nx * (xFixed_48_16) e->dy;
	    e->x -= nx * e->signdx;
	}
    }
}





static void
_RenderEdgeMultiInit (RenderEdge *e, int n, xFixed *stepx_p, xFixed *dx_p)
{
    xFixed	stepx;
    xFixed_48_16	ne;

    ne = n * (xFixed_48_16) e->dx;
    stepx = n * e->stepx;
    if (ne > 0)
    {
	int nx = ne / e->dy;
	ne -= nx * e->dy;
	stepx += nx * e->signdx;
    }
    *dx_p = ne;
    *stepx_p = stepx;
}





void
RenderEdgeInit (RenderEdge	*e,
		int		n,
		xFixed		y_start,
		xFixed		x_top,
		xFixed		y_top,
		xFixed		x_bot,
		xFixed		y_bot)
{
    xFixed	dx, dy;

    e->x = x_top;
    e->e = 0;
    dx = x_bot - x_top;
    dy = y_bot - y_top;
    e->dy = dy;
    if (dy)
    {
	if (dx >= 0)
	{
	    e->signdx = 1;
	    e->stepx = dx / dy;
	    e->dx = dx % dy;
	    e->e = -dy;
	}
	else
	{
	    e->signdx = -1;
	    e->stepx = -(-dx / dy);
	    e->dx = -dx % dy;
	    e->e = 0;
	}

	_RenderEdgeMultiInit (e, STEP_Y_SMALL(n), &e->stepx_small, &e->dx_small);
	_RenderEdgeMultiInit (e, STEP_Y_BIG(n), &e->stepx_big, &e->dx_big);
    }
    RenderEdgeStep (e, y_start - y_top);
}





void
RenderLineFixedEdgeInit (RenderEdge *e,
			 int	    n,
			 xFixed	    y,
			 const xLineFixed *line,
			 int	    x_off,
			 int	    y_off)
{
    xFixed	x_off_fixed = IntToxFixed(x_off);
    xFixed	y_off_fixed = IntToxFixed(y_off);
    const xPointFixed *top, *bot;

    if (line->p1.y <= line->p2.y)
    {
	top = &line->p1;
	bot = &line->p2;
    }
    else
    {
	top = &line->p2;
	bot = &line->p1;
    }
    RenderEdgeInit (e, n, y,
		    top->x + x_off_fixed,
		    top->y + y_off_fixed,
		    bot->x + x_off_fixed,
		    bot->y + y_off_fixed);
}
