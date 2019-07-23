
























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include "pixman-private.h"

typedef struct
{
    uint32_t        left_ag;
    uint32_t        left_rb;
    uint32_t        right_ag;
    uint32_t        right_rb;
    int32_t       left_x;
    int32_t       right_x;
    int32_t       stepper;

    pixman_gradient_stop_t	*stops;
    int                      num_stops;
    unsigned int             spread;

    int		  need_reset;
} GradientWalker;

static void
_gradient_walker_init (GradientWalker  *walker,
		       gradient_t      *gradient,
		       unsigned int     spread)
{
    walker->num_stops = gradient->n_stops;
    walker->stops     = gradient->stops;
    walker->left_x    = 0;
    walker->right_x   = 0x10000;
    walker->stepper   = 0;
    walker->left_ag   = 0;
    walker->left_rb   = 0;
    walker->right_ag  = 0;
    walker->right_rb  = 0;
    walker->spread    = spread;

    walker->need_reset = TRUE;
}

static void
_gradient_walker_reset (GradientWalker  *walker,
                        pixman_fixed_32_32_t     pos)
{
    int32_t                  x, left_x, right_x;
    pixman_color_t          *left_c, *right_c;
    int                      n, count = walker->num_stops;
    pixman_gradient_stop_t *      stops = walker->stops;

    static const pixman_color_t   transparent_black = { 0, 0, 0, 0 };

    switch (walker->spread)
    {
    case PIXMAN_REPEAT_NORMAL:
	x = (int32_t)pos & 0xFFFF;
	for (n = 0; n < count; n++)
	    if (x < stops[n].x)
		break;
	if (n == 0) {
	    left_x =  stops[count-1].x - 0x10000;
	    left_c = &stops[count-1].color;
	} else {
	    left_x =  stops[n-1].x;
	    left_c = &stops[n-1].color;
	}

	if (n == count) {
	    right_x =  stops[0].x + 0x10000;
	    right_c = &stops[0].color;
	} else {
	    right_x =  stops[n].x;
	    right_c = &stops[n].color;
	}
	left_x  += (pos - x);
	right_x += (pos - x);
	break;

    case PIXMAN_REPEAT_PAD:
	for (n = 0; n < count; n++)
	    if (pos < stops[n].x)
		break;

	if (n == 0) {
	    left_x =  INT32_MIN;
	    left_c = &stops[0].color;
	} else {
	    left_x =  stops[n-1].x;
	    left_c = &stops[n-1].color;
	}

	if (n == count) {
	    right_x =  INT32_MAX;
	    right_c = &stops[n-1].color;
	} else {
	    right_x =  stops[n].x;
	    right_c = &stops[n].color;
	}
	break;

    case PIXMAN_REPEAT_REFLECT:
	x = (int32_t)pos & 0xFFFF;
	if ((int32_t)pos & 0x10000)
	    x = 0x10000 - x;
	for (n = 0; n < count; n++)
	    if (x < stops[n].x)
		break;

	if (n == 0) {
	    left_x =  -stops[0].x;
	    left_c = &stops[0].color;
	} else {
	    left_x =  stops[n-1].x;
	    left_c = &stops[n-1].color;
	}

	if (n == count) {
	    right_x = 0x20000 - stops[n-1].x;
	    right_c = &stops[n-1].color;
	} else {
	    right_x =  stops[n].x;
	    right_c = &stops[n].color;
	}

	if ((int32_t)pos & 0x10000) {
	    pixman_color_t  *tmp_c;
	    int32_t          tmp_x;

	    tmp_x   = 0x10000 - right_x;
	    right_x = 0x10000 - left_x;
	    left_x  = tmp_x;

	    tmp_c   = right_c;
	    right_c = left_c;
	    left_c  = tmp_c;

	    x = 0x10000 - x;
	}
	left_x  += (pos - x);
	right_x += (pos - x);
	break;

    default:  
	for (n = 0; n < count; n++)
	    if (pos < stops[n].x)
		break;

	if (n == 0)
	{
	    left_x  =  INT32_MIN;
	    right_x =  stops[0].x;
	    left_c  = right_c = (pixman_color_t*) &transparent_black;
	}
	else if (n == count)
	{
	    left_x  = stops[n-1].x;
	    right_x = INT32_MAX;
	    left_c  = right_c = (pixman_color_t*) &transparent_black;
	}
	else
	{
	    left_x  =  stops[n-1].x;
	    right_x =  stops[n].x;
	    left_c  = &stops[n-1].color;
	    right_c = &stops[n].color;
	}
    }

    walker->left_x   = left_x;
    walker->right_x  = right_x;
    walker->left_ag  = ((left_c->alpha >> 8) << 16)   | (left_c->green >> 8);
    walker->left_rb  = ((left_c->red & 0xff00) << 8)  | (left_c->blue >> 8);
    walker->right_ag = ((right_c->alpha >> 8) << 16)  | (right_c->green >> 8);
    walker->right_rb = ((right_c->red & 0xff00) << 8) | (right_c->blue >> 8);

    if ( walker->left_x == walker->right_x                ||
	 ( walker->left_ag == walker->right_ag &&
	   walker->left_rb == walker->right_rb )   )
    {
	walker->stepper = 0;
    }
    else
    {
	int32_t width = right_x - left_x;
	walker->stepper = ((1 << 24) + width/2)/width;
    }

    walker->need_reset = FALSE;
}

#define  GRADIENT_WALKER_NEED_RESET(w,x)				\
    ( (w)->need_reset || (x) < (w)->left_x || (x) >= (w)->right_x)



static uint32_t
_gradient_walker_pixel (GradientWalker  *walker,
                        pixman_fixed_32_32_t     x)
{
    int  dist, idist;
    uint32_t  t1, t2, a, color;

    if (GRADIENT_WALKER_NEED_RESET (walker, x))
        _gradient_walker_reset (walker, x);

    dist  = ((int)(x - walker->left_x)*walker->stepper) >> 16;
    idist = 256 - dist;

    
    t1 = walker->left_rb*idist + walker->right_rb*dist;
    t1 = (t1 >> 8) & 0xff00ff;

    t2  = walker->left_ag*idist + walker->right_ag*dist;
    t2 &= 0xff00ff00;

    color = t2 & 0xff000000;
    a     = t2 >> 24;

    t1  = t1*a + 0x800080;
    t1  = (t1 + ((t1 >> 8) & 0xff00ff)) >> 8;

    t2  = (t2 >> 8)*a + 0x800080;
    t2  = (t2 + ((t2 >> 8) & 0xff00ff));

    return (color | (t1 & 0xff00ff) | (t2 & 0xff00));
}

void pixmanFetchSourcePict(source_image_t * pict, int x, int y, int width,
                           uint32_t *buffer, uint32_t *mask, uint32_t maskBits)
{
#if 0
    SourcePictPtr   pGradient = pict->pSourcePict;
#endif
    GradientWalker  walker;
    uint32_t       *end = buffer + width;
    gradient_t	    *gradient;

    if (pict->common.type == SOLID)
    {
	register uint32_t color = ((solid_fill_t *)pict)->color;

	while (buffer < end)
	    *(buffer++) = color;

	return;
    }

    gradient = (gradient_t *)pict;

    _gradient_walker_init (&walker, gradient, pict->common.repeat);

    if (pict->common.type == LINEAR) {
	pixman_vector_t v, unit;
	pixman_fixed_32_32_t l;
	pixman_fixed_48_16_t dx, dy, a, b, off;
	linear_gradient_t *linear = (linear_gradient_t *)pict;

        
        v.vector[0] = pixman_int_to_fixed(x) + pixman_fixed_1/2;
        v.vector[1] = pixman_int_to_fixed(y) + pixman_fixed_1/2;
        v.vector[2] = pixman_fixed_1;
        if (pict->common.transform) {
            if (!pixman_transform_point_3d (pict->common.transform, &v))
                return;
            unit.vector[0] = pict->common.transform->matrix[0][0];
            unit.vector[1] = pict->common.transform->matrix[1][0];
            unit.vector[2] = pict->common.transform->matrix[2][0];
        } else {
            unit.vector[0] = pixman_fixed_1;
            unit.vector[1] = 0;
            unit.vector[2] = 0;
        }

        dx = linear->p2.x - linear->p1.x;
        dy = linear->p2.y - linear->p1.y;
        l = dx*dx + dy*dy;
        if (l != 0) {
            a = (dx << 32) / l;
            b = (dy << 32) / l;
            off = (-a*linear->p1.x - b*linear->p1.y)>>16;
        }
        if (l == 0  || (unit.vector[2] == 0 && v.vector[2] == pixman_fixed_1)) {
            pixman_fixed_48_16_t inc, t;
            
            if (l == 0) {
                t = 0;
                inc = 0;
            } else {
                t = ((a*v.vector[0] + b*v.vector[1]) >> 16) + off;
                inc = (a * unit.vector[0] + b * unit.vector[1]) >> 16;
            }

	    if (pict->class == SOURCE_IMAGE_CLASS_VERTICAL)
	    {
		register uint32_t color;

		color = _gradient_walker_pixel( &walker, t );
		while (buffer < end)
		    *(buffer++) = color;
	    }
	    else
	    {
                if (!mask) {
                    while (buffer < end)
                    {
			*(buffer) = _gradient_walker_pixel (&walker, t);
                        buffer += 1;
                        t      += inc;
                    }
                } else {
                    while (buffer < end) {
                        if (*mask++ & maskBits)
                        {
			    *(buffer) = _gradient_walker_pixel (&walker, t);
                        }
                        buffer += 1;
                        t      += inc;
                    }
                }
	    }
	}
	else 
	{
	    pixman_fixed_48_16_t t;

	    if (pict->class == SOURCE_IMAGE_CLASS_VERTICAL)
	    {
		register uint32_t color;

		if (v.vector[2] == 0)
		{
		    t = 0;
		}
		else
		{
		    pixman_fixed_48_16_t x, y;

		    x = ((pixman_fixed_48_16_t) v.vector[0] << 16) / v.vector[2];
		    y = ((pixman_fixed_48_16_t) v.vector[1] << 16) / v.vector[2];
		    t = ((a * x + b * y) >> 16) + off;
		}

 		color = _gradient_walker_pixel( &walker, t );
		while (buffer < end)
		    *(buffer++) = color;
	    }
	    else
	    {
		while (buffer < end)
		{
		    if (!mask || *mask++ & maskBits)
		    {
			if (v.vector[2] == 0) {
			    t = 0;
			} else {
			    pixman_fixed_48_16_t x, y;
			    x = ((pixman_fixed_48_16_t)v.vector[0] << 16) / v.vector[2];
			    y = ((pixman_fixed_48_16_t)v.vector[1] << 16) / v.vector[2];
			    t = ((a*x + b*y) >> 16) + off;
			}
			*(buffer) = _gradient_walker_pixel (&walker, t);
		    }
		    ++buffer;
		    v.vector[0] += unit.vector[0];
		    v.vector[1] += unit.vector[1];
		    v.vector[2] += unit.vector[2];
		}
            }
        }
    } else {



















































































































        
        pixman_bool_t affine = TRUE;
        double cx = 1.;
        double cy = 0.;
        double cz = 0.;
	double rx = x + 0.5;
	double ry = y + 0.5;
        double rz = 1.;

        if (pict->common.transform) {
            pixman_vector_t v;
            
            v.vector[0] = pixman_int_to_fixed(x) + pixman_fixed_1/2;
            v.vector[1] = pixman_int_to_fixed(y) + pixman_fixed_1/2;
            v.vector[2] = pixman_fixed_1;
            if (!pixman_transform_point_3d (pict->common.transform, &v))
                return;

            cx = pict->common.transform->matrix[0][0]/65536.;
            cy = pict->common.transform->matrix[1][0]/65536.;
            cz = pict->common.transform->matrix[2][0]/65536.;
            rx = v.vector[0]/65536.;
            ry = v.vector[1]/65536.;
            rz = v.vector[2]/65536.;
            affine = pict->common.transform->matrix[2][0] == 0 && v.vector[2] == pixman_fixed_1;
        }

        if (pict->common.type == RADIAL) {
	    radial_gradient_t *radial = (radial_gradient_t *)pict;
            if (affine) {
                while (buffer < end) {
		    if (!mask || *mask++ & maskBits)
		    {
			double pdx, pdy;
			double B, C;
			double det;
			double c1x = radial->c1.x / 65536.0;
			double c1y = radial->c1.y / 65536.0;
			double r1  = radial->c1.radius / 65536.0;
                        pixman_fixed_48_16_t t;

			pdx = rx - c1x;
			pdy = ry - c1y;

			B = -2 * (  pdx * radial->cdx
				    + pdy * radial->cdy
				    + r1 * radial->dr);
			C = (pdx * pdx + pdy * pdy - r1 * r1);

                        det = (B * B) - (4 * radial->A * C);
			if (det < 0.0)
			    det = 0.0;

			if (radial->A < 0)
			    t = (pixman_fixed_48_16_t) ((- B - sqrt(det)) / (2.0 * radial->A) * 65536);
			else
			    t = (pixman_fixed_48_16_t) ((- B + sqrt(det)) / (2.0 * radial->A) * 65536);

			*(buffer) = _gradient_walker_pixel (&walker, t);
		    }
		    ++buffer;

                    rx += cx;
                    ry += cy;
                }
            } else {
		
                while (buffer < end) {
		    if (!mask || *mask++ & maskBits)
		    {
			double pdx, pdy;
			double B, C;
			double det;
			double c1x = radial->c1.x / 65536.0;
			double c1y = radial->c1.y / 65536.0;
			double r1  = radial->c1.radius / 65536.0;
                        pixman_fixed_48_16_t t;
			double x, y;

			if (rz != 0) {
			    x = rx/rz;
			    y = ry/rz;
			} else {
			    x = y = 0.;
			}

			pdx = x - c1x;
			pdy = y - c1y;

			B = -2 * (  pdx * radial->cdx
				    + pdy * radial->cdy
				    + r1 * radial->dr);
			C = (pdx * pdx + pdy * pdy - r1 * r1);

                        det = (B * B) - (4 * radial->A * C);
			if (det < 0.0)
			    det = 0.0;

			if (radial->A < 0)
			    t = (pixman_fixed_48_16_t) ((- B - sqrt(det)) / (2.0 * radial->A) * 65536);
			else
			    t = (pixman_fixed_48_16_t) ((- B + sqrt(det)) / (2.0 * radial->A) * 65536);

			*(buffer) = _gradient_walker_pixel (&walker, t);
		    }
		    ++buffer;

                    rx += cx;
                    ry += cy;
		    rz += cz;
                }
            }
        } else  {
	    conical_gradient_t *conical = (conical_gradient_t *)pict;
            double a = conical->angle/(180.*65536);
            if (affine) {
                rx -= conical->center.x/65536.;
                ry -= conical->center.y/65536.;

                while (buffer < end) {
		    double angle;

                    if (!mask || *mask++ & maskBits)
		    {
                        pixman_fixed_48_16_t   t;

                        angle = atan2(ry, rx) + a;
			t     = (pixman_fixed_48_16_t) (angle * (65536. / (2*M_PI)));

			*(buffer) = _gradient_walker_pixel (&walker, t);
		    }

                    ++buffer;
                    rx += cx;
                    ry += cy;
                }
            } else {
                while (buffer < end) {
                    double x, y;
                    double angle;

                    if (!mask || *mask++ & maskBits)
                    {
			pixman_fixed_48_16_t  t;

			if (rz != 0) {
			    x = rx/rz;
			    y = ry/rz;
			} else {
			    x = y = 0.;
			}
			x -= conical->center.x/65536.;
			y -= conical->center.y/65536.;
			angle = atan2(y, x) + a;
			t     = (pixman_fixed_48_16_t) (angle * (65536. / (2*M_PI)));

			*(buffer) = _gradient_walker_pixel (&walker, t);
		    }

                    ++buffer;
                    rx += cx;
                    ry += cy;
                    rz += cz;
                }
            }
        }
    }
}
