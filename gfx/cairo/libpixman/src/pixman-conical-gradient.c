

























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include "pixman-private.h"

static void
conical_gradient_get_scanline_32 (pixman_image_t *image,
                                  int             x,
                                  int             y,
                                  int             width,
                                  uint32_t *      buffer,
                                  const uint32_t *mask,
                                  uint32_t        mask_bits)
{
    source_image_t *source = (source_image_t *)image;
    gradient_t *gradient = (gradient_t *)source;
    conical_gradient_t *conical = (conical_gradient_t *)image;
    uint32_t       *end = buffer + width;
    pixman_gradient_walker_t walker;
    pixman_bool_t affine = TRUE;
    double cx = 1.;
    double cy = 0.;
    double cz = 0.;
    double rx = x + 0.5;
    double ry = y + 0.5;
    double rz = 1.;
    double a = (conical->angle * M_PI) / (180. * 65536);

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
	rx -= conical->center.x / 65536.;
	ry -= conical->center.y / 65536.;

	while (buffer < end)
	{
	    double angle;

	    if (!mask || *mask++ & mask_bits)
	    {
		pixman_fixed_48_16_t t;

		angle = atan2 (ry, rx) + a;
		t     = (pixman_fixed_48_16_t) (angle * (65536. / (2 * M_PI)));

		*buffer = _pixman_gradient_walker_pixel (&walker, t);
	    }

	    ++buffer;
	    
	    rx += cx;
	    ry += cy;
	}
    }
    else
    {
	while (buffer < end)
	{
	    double x, y;
	    double angle;

	    if (!mask || *mask++ & mask_bits)
	    {
		pixman_fixed_48_16_t t;

		if (rz != 0)
		{
		    x = rx / rz;
		    y = ry / rz;
		}
		else
		{
		    x = y = 0.;
		}

		x -= conical->center.x / 65536.;
		y -= conical->center.y / 65536.;
		
		angle = atan2 (y, x) + a;
		t     = (pixman_fixed_48_16_t) (angle * (65536. / (2 * M_PI)));

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
conical_gradient_property_changed (pixman_image_t *image)
{
    image->common.get_scanline_32 = conical_gradient_get_scanline_32;
    image->common.get_scanline_64 = _pixman_image_get_scanline_generic_64;
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_conical_gradient (pixman_point_fixed_t *        center,
                                      pixman_fixed_t                angle,
                                      const pixman_gradient_stop_t *stops,
                                      int                           n_stops)
{
    pixman_image_t *image = _pixman_image_allocate ();
    conical_gradient_t *conical;

    if (!image)
	return NULL;

    conical = &image->conical;

    if (!_pixman_init_gradient (&conical->common, stops, n_stops))
    {
	free (image);
	return NULL;
    }

    image->type = CONICAL;
    conical->center = *center;
    conical->angle = angle;

    image->common.property_changed = conical_gradient_property_changed;

    return image;
}

