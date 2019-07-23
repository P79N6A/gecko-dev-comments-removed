






















#include "pixmanint.h"

#ifdef ICINT_NEED_IC_ONES

int
_FbOnes (unsigned int mask)
{
    register int y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
}
#endif

void
pixman_color_to_pixel (const pixman_format_t	*format,
		const pixman_color_t	*color,
		pixman_bits_t		*pixel)
{
    uint32_t	    r, g, b, a;

    r = color->red >> (16 - _FbOnes (format->redMask));
    g = color->green >> (16 - _FbOnes (format->greenMask));
    b = color->blue >> (16 - _FbOnes (format->blueMask));
    a = color->alpha >> (16 - _FbOnes (format->alphaMask));
    r = r << format->red;
    g = g << format->green;
    b = b << format->blue;
    a = a << format->alpha;
    *pixel = r|g|b|a;
}

static uint16_t
FbFillColor (uint32_t pixel, int bits)
{
    while (bits < 16)
    {
	pixel |= pixel << bits;
	bits <<= 1;
    }
    return (uint16_t) pixel;
}

void
pixman_pixel_to_color (const pixman_format_t	*format,
		const pixman_bits_t	pixel,
		pixman_color_t		*color)
{
    uint32_t	    r, g, b, a;

    r = (pixel >> format->red) & format->redMask;
    g = (pixel >> format->green) & format->greenMask;
    b = (pixel >> format->blue) & format->blueMask;
    a = (pixel >> format->alpha) & format->alphaMask;
    color->red = FbFillColor (r, _FbOnes (format->redMask));
    color->green = FbFillColor (r, _FbOnes (format->greenMask));
    color->blue = FbFillColor (r, _FbOnes (format->blueMask));
    color->alpha = FbFillColor (r, _FbOnes (format->alphaMask));
}
