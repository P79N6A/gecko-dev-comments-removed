



























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <math.h>
#include "pixman-private.h"

static void
radial_gradient_get_scanline_32 (pixman_image_t *image,
                                 int             x,
                                 int             y,
                                 int             width,
                                 uint32_t *      buffer,
                                 const uint32_t *mask,
                                 uint32_t        mask_bits)
{
    


















































































































    gradient_t *gradient = (gradient_t *)image;
    source_image_t *source = (source_image_t *)image;
    radial_gradient_t *radial = (radial_gradient_t *)image;
    uint32_t *end = buffer + width;
    pixman_gradient_walker_t walker;
    pixman_bool_t affine = TRUE;
    double cx = 1.;
    double cy = 0.;
    double cz = 0.;
    double rx = x + 0.5;
    double ry = y + 0.5;
    double rz = 1.;

    _pixman_gradient_walker_init (&walker, gradient, source->common.repeat);

    if (source->common.transform)
    {
	pixman_vector_t v;
	
	v.vector[0] = pixman_int_to_fixed (x) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed (y) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;
	
	if (!pixman_transform_point_3d (source->common.transform, &v))
	    return;

	cx = source->common.transform->matrix[0][0] / 65536.;
	cy = source->common.transform->matrix[1][0] / 65536.;
	cz = source->common.transform->matrix[2][0] / 65536.;
	
	rx = v.vector[0] / 65536.;
	ry = v.vector[1] / 65536.;
	rz = v.vector[2] / 65536.;

	affine =
	    source->common.transform->matrix[2][0] == 0 &&
	    v.vector[2] == pixman_fixed_1;
    }

    if (affine)
    {
	


























	double r1   = radial->c1.radius / 65536.;
	double r1sq = r1 * r1;
	double pdx  = rx - radial->c1.x / 65536.;
	double pdy  = ry - radial->c1.y / 65536.;
	double A = radial->A;
	double invA = -65536. / (2. * A);
	double A4 = -4. * A;
	double B  = -2. * (pdx*radial->cdx + pdy*radial->cdy + r1*radial->dr);
	double cB = -2. *  (cx*radial->cdx +  cy*radial->cdy);
	pixman_bool_t invert = A * radial->dr < 0;

	while (buffer < end)
	{
	    if (!mask || *mask++ & mask_bits)
	    {
		pixman_fixed_48_16_t t;
		double det = B * B + A4 * (pdx * pdx + pdy * pdy - r1sq);
		if (det <= 0.)
		    t = (pixman_fixed_48_16_t) (B * invA);
		else if (invert)
		    t = (pixman_fixed_48_16_t) ((B + sqrt (det)) * invA);
		else
		    t = (pixman_fixed_48_16_t) ((B - sqrt (det)) * invA);

		*buffer = _pixman_gradient_walker_pixel (&walker, t);
	    }
	    ++buffer;

	    pdx += cx;
	    pdy += cy;
	    B += cB;
	}
    }
    else
    {
	
	while (buffer < end)
	{
	    if (!mask || *mask++ & mask_bits)
	    {
		double pdx, pdy;
		double B, C;
		double det;
		double c1x = radial->c1.x / 65536.0;
		double c1y = radial->c1.y / 65536.0;
		double r1  = radial->c1.radius / 65536.0;
		pixman_fixed_48_16_t t;
		double x, y;

		if (rz != 0)
		{
		    x = rx / rz;
		    y = ry / rz;
		}
		else
		{
		    x = y = 0.;
		}

		pdx = x - c1x;
		pdy = y - c1y;

		B = -2 * (pdx * radial->cdx +
			  pdy * radial->cdy +
			  r1 * radial->dr);
		C = (pdx * pdx + pdy * pdy - r1 * r1);

		det = (B * B) - (4 * radial->A * C);
		if (det < 0.0)
		    det = 0.0;

		if (radial->A * radial->dr < 0)
		    t = (pixman_fixed_48_16_t) ((-B - sqrt (det)) / (2.0 * radial->A) * 65536);
		else
		    t = (pixman_fixed_48_16_t) ((-B + sqrt (det)) / (2.0 * radial->A) * 65536);

		*buffer = _pixman_gradient_walker_pixel (&walker, t);
	    }
	    
	    ++buffer;

	    rx += cx;
	    ry += cy;
	    rz += cz;
	}
    }
}

static void
radial_gradient_property_changed (pixman_image_t *image)
{
    image->common.get_scanline_32 = radial_gradient_get_scanline_32;
    image->common.get_scanline_64 = _pixman_image_get_scanline_generic_64;
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_radial_gradient (pixman_point_fixed_t *        inner,
                                     pixman_point_fixed_t *        outer,
                                     pixman_fixed_t                inner_radius,
                                     pixman_fixed_t                outer_radius,
                                     const pixman_gradient_stop_t *stops,
                                     int                           n_stops)
{
    pixman_image_t *image;
    radial_gradient_t *radial;

    return_val_if_fail (n_stops >= 2, NULL);

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
    radial->cdx = pixman_fixed_to_double (radial->c2.x - radial->c1.x);
    radial->cdy = pixman_fixed_to_double (radial->c2.y - radial->c1.y);
    radial->dr = pixman_fixed_to_double (radial->c2.radius - radial->c1.radius);
    radial->A = (radial->cdx * radial->cdx +
		 radial->cdy * radial->cdy -
		 radial->dr  * radial->dr);

    image->common.property_changed = radial_gradient_property_changed;

    return image;
}

