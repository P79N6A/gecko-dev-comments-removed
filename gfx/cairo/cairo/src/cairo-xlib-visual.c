


































#include "cairoint.h"

#include "cairo-xlib-private.h"








static inline int
_color_distance (unsigned short r1, unsigned short g1, unsigned short b1,
		 unsigned short r2, unsigned short g2, unsigned short b2)
{
    r1 >>= 8; g1 >>= 8; b1 >>= 8;
    r2 >>= 8; g2 >>= 8; b2 >>= 8;

    return ((r2 - r1) * (r2 - r1) +
	    (g2 - g1) * (g2 - g1) +
	    (b2 - b1) * (b2 - b1));
}

cairo_status_t
_cairo_xlib_visual_info_create (Display *dpy,
	                        int screen,
				VisualID visualid,
				cairo_xlib_visual_info_t **out)
{
    cairo_xlib_visual_info_t *info;
    Colormap colormap = DefaultColormap (dpy, screen);
    XColor color;
    int gray, red, green, blue;
    int i, index, distance, min_distance = 0;

    const unsigned short index5_to_short[5] = {
	0x0000, 0x4000, 0x8000, 0xc000, 0xffff
    };
    const unsigned short index8_to_short[8] = {
	0x0000, 0x2492, 0x4924, 0x6db6,
	0x9249, 0xb6db, 0xdb6d, 0xffff
    };

    info = malloc (sizeof (cairo_xlib_visual_info_t));
    if (info == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    info->visualid = visualid;

    

    for (gray = 0; gray < 16; gray++) {
	color.red   = (gray << 12) | (gray << 8) | (gray << 4) | gray;
	color.green = (gray << 12) | (gray << 8) | (gray << 4) | gray;
	color.blue  = (gray << 12) | (gray << 8) | (gray << 4) | gray;
	if (! XAllocColor (dpy, colormap, &color))
	    goto DONE_ALLOCATE;
    }

    


    for (red = 0; red < 5; red++) {
	for (green = 0; green < 5; green++) {
	    for (blue = 0; blue < 5; blue++) {
		color.red = index5_to_short[red];
		color.green = index5_to_short[green];
		color.blue = index5_to_short[blue];
		color.pixel = 0;
		color.flags = 0;
		color.pad = 0;
		if (! XAllocColor (dpy, colormap, &color))
		    goto DONE_ALLOCATE;
	    }
	}
    }
  DONE_ALLOCATE:

    for (i = 0; i < ARRAY_LENGTH (info->colors); i++)
	info->colors[i].pixel = i;
    XQueryColors (dpy, colormap, info->colors, ARRAY_LENGTH (info->colors));

    
    for (red = 0; red < 8; red++) {
	for (green = 0; green < 8; green++) {
	    for (blue = 0; blue < 8; blue++) {
		index = (red << 6) | (green << 3) | (blue);
		for (i = 0; i < 256; i++) {
		    distance = _color_distance (index8_to_short[red],
						index8_to_short[green],
						index8_to_short[blue],
						info->colors[i].red,
						info->colors[i].green,
						info->colors[i].blue);
		    if (i == 0 || distance < min_distance) {
			info->rgb333_to_pseudocolor[index] = info->colors[i].pixel;
			min_distance = distance;
		    }
		}
	    }
	}
    }

    *out = info;
    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_xlib_visual_info_destroy (Display *dpy, cairo_xlib_visual_info_t *info)
{
    
    free (info);
}
