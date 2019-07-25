


































#include "cairoint.h"

#include "cairo-xlib-private.h"

#include "cairo-error-private.h"








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
    int i, j, distance, min_distance = 0;
    XColor colors[256];
    unsigned short cube_index_to_short[CUBE_SIZE];
    unsigned short ramp_index_to_short[RAMP_SIZE];
    unsigned char  gray_to_pseudocolor[RAMP_SIZE];

    for (i = 0; i < CUBE_SIZE; i++)
	cube_index_to_short[i] = (0xffff * i + ((CUBE_SIZE-1)>>1)) / (CUBE_SIZE-1);
    for (i = 0; i < RAMP_SIZE; i++)
	ramp_index_to_short[i] = (0xffff * i + ((RAMP_SIZE-1)>>1)) / (RAMP_SIZE-1);

    info = malloc (sizeof (cairo_xlib_visual_info_t));
    if (unlikely (info == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    info->visualid = visualid;

    


    for (gray = 0; gray < RAMP_SIZE; gray++) {
	color.red = color.green = color.blue = ramp_index_to_short[gray];
	if (! XAllocColor (dpy, colormap, &color))
	    goto DONE_ALLOCATE;
    }

    


    for (red = 0; red < CUBE_SIZE; red++) {
	for (green = 0; green < CUBE_SIZE; green++) {
	    for (blue = 0; blue < CUBE_SIZE; blue++) {
		color.red = cube_index_to_short[red];
		color.green = cube_index_to_short[green];
		color.blue = cube_index_to_short[blue];
		color.pixel = 0;
		color.flags = 0;
		color.pad = 0;
		if (! XAllocColor (dpy, colormap, &color))
		    goto DONE_ALLOCATE;
	    }
	}
    }
  DONE_ALLOCATE:

    for (i = 0; i < ARRAY_LENGTH (colors); i++)
	colors[i].pixel = i;
    XQueryColors (dpy, colormap, colors, ARRAY_LENGTH (colors));

    
    for (gray = 0; gray < RAMP_SIZE; gray++) {
	for (i = 0; i < 256; i++) {
	    distance = _color_distance (ramp_index_to_short[gray],
					ramp_index_to_short[gray],
					ramp_index_to_short[gray],
					colors[i].red,
					colors[i].green,
					colors[i].blue);
	    if (i == 0 || distance < min_distance) {
		gray_to_pseudocolor[gray] = colors[i].pixel;
		min_distance = distance;
		if (!min_distance)
		    break;
	    }
	}
    }
    for (red = 0; red < CUBE_SIZE; red++) {
	for (green = 0; green < CUBE_SIZE; green++) {
	    for (blue = 0; blue < CUBE_SIZE; blue++) {
		for (i = 0; i < 256; i++) {
		    distance = _color_distance (cube_index_to_short[red],
						cube_index_to_short[green],
						cube_index_to_short[blue],
						colors[i].red,
						colors[i].green,
						colors[i].blue);
		    if (i == 0 || distance < min_distance) {
			info->cube_to_pseudocolor[red][green][blue] = colors[i].pixel;
			min_distance = distance;
			if (!min_distance)
			    break;
		    }
		}
	    }
	}
    }

    for (i = 0, j = 0; i < 256; i++) {
	if (j < CUBE_SIZE - 1 && (((i<<8)+i) - (int)cube_index_to_short[j]) > ((int)cube_index_to_short[j+1] - ((i<<8)+i)))
	    j++;
	info->field8_to_cube[i] = j;

	info->dither8_to_cube[i] = ((int)i - 128) / (CUBE_SIZE - 1);
    }
    for (i = 0, j = 0; i < 256; i++) {
	if (j < RAMP_SIZE - 1 && (((i<<8)+i) - (int)ramp_index_to_short[j]) > ((int)ramp_index_to_short[j+1] - ((i<<8)+i)))
	    j++;
	info->gray8_to_pseudocolor[i] = gray_to_pseudocolor[j];
    }

    for (i = 0; i < 256; i++) {
	info->colors[i].a = 0xff;
	info->colors[i].r = colors[i].red   >> 8;
	info->colors[i].g = colors[i].green >> 8;
	info->colors[i].b = colors[i].blue  >> 8;
    }

    *out = info;
    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_xlib_visual_info_destroy (cairo_xlib_visual_info_t *info)
{
    
    free (info);
}
