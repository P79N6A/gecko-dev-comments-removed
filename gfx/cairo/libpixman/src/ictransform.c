






















#include "pixmanint.h"

#define MAX_FIXED_48_16	    ((xFixed_48_16) 0x7fffffff)
#define MIN_FIXED_48_16	    (-((xFixed_48_16) 1 << 31))

int
pixman_transform_point (pixman_transform_t	*transform,
		  pixman_vector_t	*vector)
{
    pixman_vector_t	    result;
    int		    i, j;
    xFixed_32_32    partial;
    xFixed_48_16    v;

    for (j = 0; j < 3; j++)
    {
	v = 0;
	for (i = 0; i < 3; i++)
	{
	    partial = ((xFixed_48_16) transform->matrix[j][i] *
		       (xFixed_48_16) vector->vector[i]);
	    v += partial >> 16;
	}
	if (v > MAX_FIXED_48_16 || v < MIN_FIXED_48_16)
	    return 0;
	result.vector[j] = (xFixed) v;
    }
    if (!result.vector[2])
	return 0;
    for (j = 0; j < 2; j++)
    {
	partial = (xFixed_48_16) result.vector[j] << 16;
	v = partial / result.vector[2];
	if (v > MAX_FIXED_48_16 || v < MIN_FIXED_48_16)
	    return 0;
	vector->vector[j] = (xFixed) v;
    }
    vector->vector[2] = xFixed1;
    return 1;
}
