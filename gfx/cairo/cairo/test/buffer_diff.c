
























#include "buffer_diff.h"






int
buffer_diff (char *buf_a, char *buf_b, char *buf_diff,
	    int width, int height, int stride)
{
    int x, y;
    int total_pixels_changed = 0;
    unsigned char *row_a, *row_b, *row;

    for (y = 0; y < height; y++)
    {
	row_a = buf_a + y * stride;
	row_b = buf_b + y * stride;
	row = buf_diff + y * stride;
	for (x = 0; x < width; x++)
	{
	    int channel;
	    unsigned char value_a, value_b;
	    int pixel_changed = 0;
	    for (channel = 0; channel < 4; channel++)
	    {
		double diff;
		value_a = row_a[x * 4 + channel];
		value_b = row_b[x * 4 + channel];
		if (value_a != value_b)
		    pixel_changed = 1;
		diff = value_a - value_b;
		row[x * 4 + channel] = 128 + diff / 3.0;
	    }
	    if (pixel_changed) {
		total_pixels_changed++;
	    } else {
		row[x*4+0] = 0;
		row[x*4+1] = 0;
		row[x*4+2] = 0;
	    }
	    row[x * 4 + 3] = 0xff; 
	}
    }

    return total_pixels_changed;
}
