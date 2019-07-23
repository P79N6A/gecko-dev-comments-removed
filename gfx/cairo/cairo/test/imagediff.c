
























#include <stdio.h>
#include <stdlib.h>

#include "buffer_diff.h"
#include "read_png.h"
#include "write_png.h"
#include "xmalloc.h"

int 
main (int argc, char *argv[])
{
    unsigned char *buffer_a;
    unsigned int width_a, height_a, stride_a;
    unsigned char *buffer_b;
    unsigned int width_b, height_b, stride_b;

    unsigned char *buffer;
    unsigned int width, height, stride;
    int buffer_size, total_pixels_changed;

    if (argc < 2) {
	fprintf (stderr, "Usage: %s image1.png image2.png\n", argv[0]);
	fprintf (stderr, "Computes an output image designed to present a \"visual diff\" such that even\n");
	fprintf (stderr, "small errors in single pixels are readily apparent in the output.\n");
	fprintf (stderr, "The output image is written on stdout.\n");
	exit (1);
    }

    read_png_argb32 (argv[1], &buffer_a, &width_a, &height_a, &stride_a);
    read_png_argb32 (argv[2], &buffer_b, &width_b, &height_b, &stride_b);

    if ((width_a == width_b) && (height_a == height_b) && (stride_a == stride_b))
    {
	width = width_a;
	height = height_a;
	stride = stride_a;
    } else {
	fprintf (stderr, "Error. Both images must be the same size\n");
	return 1;
    }

    buffer_size = stride * height;
    buffer = xmalloc (buffer_size);
    
    total_pixels_changed = buffer_diff (buffer_a, buffer_b, buffer,
					width_a, height_a, stride_a);


    if (total_pixels_changed)
	fprintf (stderr, "Total pixels changed: %d\n", total_pixels_changed);
    write_png_argb32 (buffer, stdout, width, height, stride);

    free (buffer);

    return 0;
}



