






















#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "pixman-private.h"

static void
solid_fill_get_scanline_32 (pixman_image_t *image,
                            int             x,
                            int             y,
                            int             width,
                            uint32_t *      buffer,
                            const uint32_t *mask,
                            uint32_t        mask_bits)
{
    uint32_t *end = buffer + width;
    register uint32_t color = ((solid_fill_t *)image)->color;

    while (buffer < end)
	*(buffer++) = color;

    return;
}

static source_image_class_t
solid_fill_classify (pixman_image_t *image,
                     int             x,
                     int             y,
                     int             width,
                     int             height)
{
    return (image->source.class = SOURCE_IMAGE_CLASS_HORIZONTAL);
}

static void
solid_fill_property_changed (pixman_image_t *image)
{
    image->common.get_scanline_32 = solid_fill_get_scanline_32;
    image->common.get_scanline_64 = _pixman_image_get_scanline_generic_64;
}

static uint32_t
color_to_uint32 (const pixman_color_t *color)
{
    return
        (color->alpha >> 8 << 24) |
        (color->red >> 8 << 16) |
        (color->green & 0xff00) |
        (color->blue >> 8);
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_solid_fill (pixman_color_t *color)
{
    pixman_image_t *img = _pixman_image_allocate ();

    if (!img)
	return NULL;

    img->type = SOLID;
    img->solid.color = color_to_uint32 (color);

    img->source.class = SOURCE_IMAGE_CLASS_UNKNOWN;
    img->common.classify = solid_fill_classify;
    img->common.property_changed = solid_fill_property_changed;

    return img;
}

