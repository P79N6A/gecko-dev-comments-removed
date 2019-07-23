























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "pixman-private.h"

pixman_bool_t
pixman_multiply_overflows_int (unsigned int a,
                               unsigned int b)
{
    return a >= INT32_MAX / b;
}

pixman_bool_t
pixman_addition_overflows_int (unsigned int a,
                               unsigned int b)
{
    return a > INT32_MAX - b;
}

void *
pixman_malloc_ab (unsigned int a,
                  unsigned int b)
{
    if (a >= INT32_MAX / b)
	return NULL;

    return malloc (a * b);
}

void *
pixman_malloc_abc (unsigned int a,
                   unsigned int b,
                   unsigned int c)
{
    if (a >= INT32_MAX / b)
	return NULL;
    else if (a * b >= INT32_MAX / c)
	return NULL;
    else
	return malloc (a * b * c);
}





static inline uint64_t
expand16 (const uint8_t val, int nbits)
{
    
    uint16_t result = (uint16_t)val << (16 - nbits);

    if (nbits == 0)
	return 0;

    


    while (nbits < 16)
    {
	result |= result >> nbits;
	nbits *= 2;
    }

    return result;
}








void
pixman_expand (uint64_t *           dst,
               const uint32_t *     src,
               pixman_format_code_t format,
               int                  width)
{
    



    const int a_size = PIXMAN_FORMAT_A (format),
              r_size = PIXMAN_FORMAT_R (format),
              g_size = PIXMAN_FORMAT_G (format),
              b_size = PIXMAN_FORMAT_B (format);
    const int a_shift = 32 - a_size,
              r_shift = 24 - r_size,
              g_shift = 16 - g_size,
              b_shift =  8 - b_size;
    const uint8_t a_mask = ~(~0 << a_size),
                  r_mask = ~(~0 << r_size),
                  g_mask = ~(~0 << g_size),
                  b_mask = ~(~0 << b_size);
    int i;

    


    for (i = width - 1; i >= 0; i--)
    {
	const uint32_t pixel = src[i];
	const uint8_t a = (pixel >> a_shift) & a_mask,
	              r = (pixel >> r_shift) & r_mask,
	              g = (pixel >> g_shift) & g_mask,
	              b = (pixel >> b_shift) & b_mask;
	const uint64_t a16 = a_size ? expand16 (a, a_size) : 0xffff,
	               r16 = expand16 (r, r_size),
	               g16 = expand16 (g, g_size),
	               b16 = expand16 (b, b_size);

	dst[i] = a16 << 48 | r16 << 32 | g16 << 16 | b16;
    }
}





void
pixman_contract (uint32_t *      dst,
                 const uint64_t *src,
                 int             width)
{
    int i;

    


    for (i = 0; i < width; i++)
    {
	const uint8_t a = src[i] >> 56,
	              r = src[i] >> 40,
	              g = src[i] >> 24,
	              b = src[i] >> 8;

	dst[i] = a << 24 | r << 16 | g << 8 | b;
    }
}

#define N_TMP_BOXES (16)

pixman_bool_t
pixman_region16_copy_from_region32 (pixman_region16_t *dst,
                                    pixman_region32_t *src)
{
    int n_boxes, i;
    pixman_box32_t *boxes32;
    pixman_box16_t *boxes16;
    pixman_bool_t retval;

    boxes32 = pixman_region32_rectangles (src, &n_boxes);

    boxes16 = pixman_malloc_ab (n_boxes, sizeof (pixman_box16_t));

    if (!boxes16)
	return FALSE;

    for (i = 0; i < n_boxes; ++i)
    {
	boxes16[i].x1 = boxes32[i].x1;
	boxes16[i].y1 = boxes32[i].y1;
	boxes16[i].x2 = boxes32[i].x2;
	boxes16[i].y2 = boxes32[i].y2;
    }

    pixman_region_fini (dst);
    retval = pixman_region_init_rects (dst, boxes16, n_boxes);
    free (boxes16);
    return retval;
}

pixman_bool_t
pixman_region32_copy_from_region16 (pixman_region32_t *dst,
                                    pixman_region16_t *src)
{
    int n_boxes, i;
    pixman_box16_t *boxes16;
    pixman_box32_t *boxes32;
    pixman_box32_t tmp_boxes[N_TMP_BOXES];
    pixman_bool_t retval;

    boxes16 = pixman_region_rectangles (src, &n_boxes);

    if (n_boxes > N_TMP_BOXES)
	boxes32 = pixman_malloc_ab (n_boxes, sizeof (pixman_box32_t));
    else
	boxes32 = tmp_boxes;

    if (!boxes32)
	return FALSE;

    for (i = 0; i < n_boxes; ++i)
    {
	boxes32[i].x1 = boxes16[i].x1;
	boxes32[i].y1 = boxes16[i].y1;
	boxes32[i].x2 = boxes16[i].x2;
	boxes32[i].y2 = boxes16[i].y2;
    }

    pixman_region32_fini (dst);
    retval = pixman_region32_init_rects (dst, boxes32, n_boxes);

    if (boxes32 != tmp_boxes)
	free (boxes32);

    return retval;
}

#ifdef DEBUG

void
_pixman_log_error (const char *function, const char *message)
{
    static int n_messages = 0;

    if (n_messages < 10)
    {
	fprintf (stderr,
		 "*** BUG ***\n"
		 "In %s: %s\n"
		 "Set a breakpoint on '_pixman_log_error' to debug\n\n",
                 function, message);

	n_messages++;
    }
}

#endif
