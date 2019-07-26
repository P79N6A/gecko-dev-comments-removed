




























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <math.h>
#include "pixman-private.h"

#include "pixman-dither.h"

static inline pixman_fixed_32_32_t
dot (pixman_fixed_48_16_t x1,
     pixman_fixed_48_16_t y1,
     pixman_fixed_48_16_t z1,
     pixman_fixed_48_16_t x2,
     pixman_fixed_48_16_t y2,
     pixman_fixed_48_16_t z2)
{
    



    return x1 * x2 + y1 * y2 + z1 * z2;
}

static inline double
fdot (double x1,
      double y1,
      double z1,
      double x2,
      double y2,
      double z2)
{
    





    return x1 * x2 + y1 * y2 + z1 * z2;
}

static uint32_t
radial_compute_color (double                    a,
		      double                    b,
		      double                    c,
		      double                    inva,
		      double                    dr,
		      double                    mindr,
		      pixman_gradient_walker_t *walker,
		      pixman_repeat_t           repeat)
{
    















    double discr;

    if (a == 0)
    {
	double t;

	if (b == 0)
	    return 0;

	t = pixman_fixed_1 / 2 * c / b;
	if (repeat == PIXMAN_REPEAT_NONE)
	{
	    if (0 <= t && t <= pixman_fixed_1)
		return _pixman_gradient_walker_pixel (walker, t);
	}
	else
	{
	    if (t * dr >= mindr)
		return _pixman_gradient_walker_pixel (walker, t);
	}

	return 0;
    }

    discr = fdot (b, a, 0, b, -c, 0);
    if (discr >= 0)
    {
	double sqrtdiscr, t0, t1;

	sqrtdiscr = sqrt (discr);
	t0 = (b + sqrtdiscr) * inva;
	t1 = (b - sqrtdiscr) * inva;

	










	if (repeat == PIXMAN_REPEAT_NONE)
	{
	    if (0 <= t0 && t0 <= pixman_fixed_1)
		return _pixman_gradient_walker_pixel (walker, t0);
	    else if (0 <= t1 && t1 <= pixman_fixed_1)
		return _pixman_gradient_walker_pixel (walker, t1);
	}
	else
	{
	    if (t0 * dr >= mindr)
		return _pixman_gradient_walker_pixel (walker, t0);
	    else if (t1 * dr >= mindr)
		return _pixman_gradient_walker_pixel (walker, t1);
	}
    }

    return 0;
}

static uint32_t *
radial_get_scanline_narrow (pixman_iter_t *iter, const uint32_t *mask)
{
    
















































































    pixman_image_t *image = iter->image;
    int x = iter->x;
    int y = iter->y;
    int width = iter->width;
    uint32_t *buffer = iter->buffer;

    gradient_t *gradient = (gradient_t *)image;
    radial_gradient_t *radial = (radial_gradient_t *)image;
    uint32_t *end = buffer + width;
    pixman_gradient_walker_t walker;
    pixman_vector_t v, unit;

    
    v.vector[0] = pixman_int_to_fixed (x) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (y) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    _pixman_gradient_walker_init (&walker, gradient, image->common.repeat);

    if (image->common.transform)
    {
	if (!pixman_transform_point_3d (image->common.transform, &v))
	    return iter->buffer;

	unit.vector[0] = image->common.transform->matrix[0][0];
	unit.vector[1] = image->common.transform->matrix[1][0];
	unit.vector[2] = image->common.transform->matrix[2][0];
    }
    else
    {
	unit.vector[0] = pixman_fixed_1;
	unit.vector[1] = 0;
	unit.vector[2] = 0;
    }

    if (unit.vector[2] == 0 && v.vector[2] == pixman_fixed_1)
    {
	



















	pixman_fixed_32_32_t b, db, c, dc, ddc;

	
	v.vector[0] -= radial->c1.x;
	v.vector[1] -= radial->c1.y;

	








	b = dot (v.vector[0], v.vector[1], radial->c1.radius,
		 radial->delta.x, radial->delta.y, radial->delta.radius);
	db = dot (unit.vector[0], unit.vector[1], 0,
		  radial->delta.x, radial->delta.y, 0);

	c = dot (v.vector[0], v.vector[1],
		 -((pixman_fixed_48_16_t) radial->c1.radius),
		 v.vector[0], v.vector[1], radial->c1.radius);
	dc = dot (2 * (pixman_fixed_48_16_t) v.vector[0] + unit.vector[0],
		  2 * (pixman_fixed_48_16_t) v.vector[1] + unit.vector[1],
		  0,
		  unit.vector[0], unit.vector[1], 0);
	ddc = 2 * dot (unit.vector[0], unit.vector[1], 0,
		       unit.vector[0], unit.vector[1], 0);

	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		*buffer = radial_compute_color (radial->a, b, c,
						radial->inva,
						radial->delta.radius,
						radial->mindr,
						&walker,
						image->common.repeat);
	    }

	    b += db;
	    c += dc;
	    dc += ddc;
	    ++buffer;
	}
    }
    else
    {
	
	


	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		if (v.vector[2] != 0)
		{
		    double pdx, pdy, invv2, b, c;

		    invv2 = 1. * pixman_fixed_1 / v.vector[2];

		    pdx = v.vector[0] * invv2 - radial->c1.x;
		    

		    pdy = v.vector[1] * invv2 - radial->c1.y;
		    

		    b = fdot (pdx, pdy, radial->c1.radius,
			      radial->delta.x, radial->delta.y,
			      radial->delta.radius);
		    

		    c = fdot (pdx, pdy, -radial->c1.radius,
			      pdx, pdy, radial->c1.radius);
		    

		    *buffer = radial_compute_color (radial->a, b, c,
						    radial->inva,
						    radial->delta.radius,
						    radial->mindr,
						    &walker,
						    image->common.repeat);
		}
		else
		{
		    *buffer = 0;
		}
	    }

	    ++buffer;

	    v.vector[0] += unit.vector[0];
	    v.vector[1] += unit.vector[1];
	    v.vector[2] += unit.vector[2];
	}
    }

    iter->y++;
    return iter->buffer;
}

static uint32_t *
radial_get_scanline_16 (pixman_iter_t *iter, const uint32_t *mask)
{
    
















































































    pixman_image_t *image = iter->image;
    int x = iter->x;
    int y = iter->y;
    int width = iter->width;
    uint16_t *buffer = iter->buffer;
    pixman_bool_t toggle = ((x ^ y) & 1);

    gradient_t *gradient = (gradient_t *)image;
    radial_gradient_t *radial = (radial_gradient_t *)image;
    uint16_t *end = buffer + width;
    pixman_gradient_walker_t walker;
    pixman_vector_t v, unit;

    
    v.vector[0] = pixman_int_to_fixed (x) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (y) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    _pixman_gradient_walker_init (&walker, gradient, image->common.repeat);

    if (image->common.transform)
    {
	if (!pixman_transform_point_3d (image->common.transform, &v))
	    return iter->buffer;

	unit.vector[0] = image->common.transform->matrix[0][0];
	unit.vector[1] = image->common.transform->matrix[1][0];
	unit.vector[2] = image->common.transform->matrix[2][0];
    }
    else
    {
	unit.vector[0] = pixman_fixed_1;
	unit.vector[1] = 0;
	unit.vector[2] = 0;
    }

    if (unit.vector[2] == 0 && v.vector[2] == pixman_fixed_1)
    {
	



















	pixman_fixed_32_32_t b, db, c, dc, ddc;

	
	v.vector[0] -= radial->c1.x;
	v.vector[1] -= radial->c1.y;

	








	b = dot (v.vector[0], v.vector[1], radial->c1.radius,
		 radial->delta.x, radial->delta.y, radial->delta.radius);
	db = dot (unit.vector[0], unit.vector[1], 0,
		  radial->delta.x, radial->delta.y, 0);

	c = dot (v.vector[0], v.vector[1],
		 -((pixman_fixed_48_16_t) radial->c1.radius),
		 v.vector[0], v.vector[1], radial->c1.radius);
	dc = dot (2 * (pixman_fixed_48_16_t) v.vector[0] + unit.vector[0],
		  2 * (pixman_fixed_48_16_t) v.vector[1] + unit.vector[1],
		  0,
		  unit.vector[0], unit.vector[1], 0);
	ddc = 2 * dot (unit.vector[0], unit.vector[1], 0,
		       unit.vector[0], unit.vector[1], 0);

	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		*buffer = dither_8888_to_0565(
			  radial_compute_color (radial->a, b, c,
						radial->inva,
						radial->delta.radius,
						radial->mindr,
						&walker,
						image->common.repeat),
			  toggle);
	    }

	    toggle ^= 1;
	    b += db;
	    c += dc;
	    dc += ddc;
	    ++buffer;
	}
    }
    else
    {
	
	


	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		if (v.vector[2] != 0)
		{
		    double pdx, pdy, invv2, b, c;

		    invv2 = 1. * pixman_fixed_1 / v.vector[2];

		    pdx = v.vector[0] * invv2 - radial->c1.x;
		    

		    pdy = v.vector[1] * invv2 - radial->c1.y;
		    

		    b = fdot (pdx, pdy, radial->c1.radius,
			      radial->delta.x, radial->delta.y,
			      radial->delta.radius);
		    

		    c = fdot (pdx, pdy, -radial->c1.radius,
			      pdx, pdy, radial->c1.radius);
		    

		    *buffer = dither_8888_to_0565 (
			      radial_compute_color (radial->a, b, c,
						    radial->inva,
						    radial->delta.radius,
						    radial->mindr,
						    &walker,
						    image->common.repeat),
			      toggle);
		}
		else
		{
		    *buffer = 0;
		}
	    }

	    ++buffer;
	    toggle ^= 1;

	    v.vector[0] += unit.vector[0];
	    v.vector[1] += unit.vector[1];
	    v.vector[2] += unit.vector[2];
	}
    }

    iter->y++;
    return iter->buffer;
}
static uint32_t *
radial_get_scanline_wide (pixman_iter_t *iter, const uint32_t *mask)
{
    uint32_t *buffer = radial_get_scanline_narrow (iter, NULL);

    pixman_expand_to_float (
	(argb_t *)buffer, buffer, PIXMAN_a8r8g8b8, iter->width);

    return buffer;
}

void
_pixman_radial_gradient_iter_init (pixman_image_t *image, pixman_iter_t *iter)
{
    if (iter->iter_flags & ITER_16)
	iter->get_scanline = radial_get_scanline_16;
    else if (iter->iter_flags & ITER_NARROW)
	iter->get_scanline = radial_get_scanline_narrow;
    else
	iter->get_scanline = radial_get_scanline_wide;
}


PIXMAN_EXPORT pixman_image_t *
pixman_image_create_radial_gradient (const pixman_point_fixed_t *  inner,
                                     const pixman_point_fixed_t *  outer,
                                     pixman_fixed_t                inner_radius,
                                     pixman_fixed_t                outer_radius,
                                     const pixman_gradient_stop_t *stops,
                                     int                           n_stops)
{
    pixman_image_t *image;
    radial_gradient_t *radial;

    image = _pixman_image_allocate ();

    if (!image)
	return NULL;

    radial = &image->radial;

    if (!_pixman_init_gradient (&radial->common, stops, n_stops))
    {
	free (image);
	return NULL;
    }

    image->type = RADIAL;

    radial->c1.x = inner->x;
    radial->c1.y = inner->y;
    radial->c1.radius = inner_radius;
    radial->c2.x = outer->x;
    radial->c2.y = outer->y;
    radial->c2.radius = outer_radius;

    
    radial->delta.x = radial->c2.x - radial->c1.x;
    radial->delta.y = radial->c2.y - radial->c1.y;
    radial->delta.radius = radial->c2.radius - radial->c1.radius;

    

    radial->a = dot (radial->delta.x, radial->delta.y, -radial->delta.radius,
		     radial->delta.x, radial->delta.y, radial->delta.radius);
    if (radial->a != 0)
	radial->inva = 1. * pixman_fixed_1 / radial->a;

    radial->mindr = -1. * pixman_fixed_1 * radial->c1.radius;

    return image;
}
