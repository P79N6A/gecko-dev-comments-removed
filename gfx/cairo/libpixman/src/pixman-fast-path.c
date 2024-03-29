
























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "pixman-private.h"
#include "pixman-combine32.h"
#include "pixman-inlines.h"

static force_inline uint32_t
fetch_24 (uint8_t *a)
{
    if (((uintptr_t)a) & 1)
    {
#ifdef WORDS_BIGENDIAN
	return (*a << 16) | (*(uint16_t *)(a + 1));
#else
	return *a | (*(uint16_t *)(a + 1) << 8);
#endif
    }
    else
    {
#ifdef WORDS_BIGENDIAN
	return (*(uint16_t *)a << 8) | *(a + 2);
#else
	return *(uint16_t *)a | (*(a + 2) << 16);
#endif
    }
}

static force_inline void
store_24 (uint8_t *a,
          uint32_t v)
{
    if (((uintptr_t)a) & 1)
    {
#ifdef WORDS_BIGENDIAN
	*a = (uint8_t) (v >> 16);
	*(uint16_t *)(a + 1) = (uint16_t) (v);
#else
	*a = (uint8_t) (v);
	*(uint16_t *)(a + 1) = (uint16_t) (v >> 8);
#endif
    }
    else
    {
#ifdef WORDS_BIGENDIAN
	*(uint16_t *)a = (uint16_t)(v >> 8);
	*(a + 2) = (uint8_t)v;
#else
	*(uint16_t *)a = (uint16_t)v;
	*(a + 2) = (uint8_t)(v >> 16);
#endif
    }
}

static force_inline uint32_t
over (uint32_t src,
      uint32_t dest)
{
    uint32_t a = ~src >> 24;

    UN8x4_MUL_UN8_ADD_UN8x4 (dest, a, src);

    return dest;
}

static force_inline uint32_t
in (uint32_t x,
    uint8_t  y)
{
    uint16_t a = y;

    UN8x4_MUL_UN8 (x, a);

    return x;
}






static void
fast_composite_over_x888_8_8888 (pixman_implementation_t *imp,
                                 pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t    *src, *src_line;
    uint32_t    *dst, *dst_line;
    uint8_t     *mask, *mask_line;
    int src_stride, mask_stride, dst_stride;
    uint8_t m;
    uint32_t s, d;
    int32_t w;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint8_t, mask_stride, mask_line, 1);
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint32_t, src_stride, src_line, 1);

    while (height--)
    {
	src = src_line;
	src_line += src_stride;
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;

	w = width;
	while (w--)
	{
	    m = *mask++;
	    if (m)
	    {
		s = *src | 0xff000000;

		if (m == 0xff)
		{
		    *dst = s;
		}
		else
		{
		    d = in (s, m);
		    *dst = over (d, *dst);
		}
	    }
	    src++;
	    dst++;
	}
    }
}

static void
fast_composite_in_n_8_8 (pixman_implementation_t *imp,
                         pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src, srca;
    uint8_t     *dst_line, *dst;
    uint8_t     *mask_line, *mask, m;
    int dst_stride, mask_stride;
    int32_t w;
    uint16_t t;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    srca = src >> 24;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint8_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint8_t, mask_stride, mask_line, 1);

    if (srca == 0xff)
    {
	while (height--)
	{
	    dst = dst_line;
	    dst_line += dst_stride;
	    mask = mask_line;
	    mask_line += mask_stride;
	    w = width;

	    while (w--)
	    {
		m = *mask++;

		if (m == 0)
		    *dst = 0;
		else if (m != 0xff)
		    *dst = MUL_UN8 (m, *dst, t);

		dst++;
	    }
	}
    }
    else
    {
	while (height--)
	{
	    dst = dst_line;
	    dst_line += dst_stride;
	    mask = mask_line;
	    mask_line += mask_stride;
	    w = width;

	    while (w--)
	    {
		m = *mask++;
		m = MUL_UN8 (m, srca, t);

		if (m == 0)
		    *dst = 0;
		else if (m != 0xff)
		    *dst = MUL_UN8 (m, *dst, t);

		dst++;
	    }
	}
    }
}

static void
fast_composite_in_8_8 (pixman_implementation_t *imp,
                       pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint8_t     *dst_line, *dst;
    uint8_t     *src_line, *src;
    int dst_stride, src_stride;
    int32_t w;
    uint8_t s;
    uint16_t t;

    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint8_t, src_stride, src_line, 1);
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint8_t, dst_stride, dst_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;

	    if (s == 0)
		*dst = 0;
	    else if (s != 0xff)
		*dst = MUL_UN8 (s, *dst, t);

	    dst++;
	}
    }
}

static void
fast_composite_over_n_8_8888 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src, srca;
    uint32_t    *dst_line, *dst, d;
    uint8_t     *mask_line, *mask, m;
    int dst_stride, mask_stride;
    int32_t w;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    srca = src >> 24;
    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint8_t, mask_stride, mask_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    m = *mask++;
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		    *dst = src;
		else
		    *dst = over (src, *dst);
	    }
	    else if (m)
	    {
		d = in (src, m);
		*dst = over (d, *dst);
	    }
	    dst++;
	}
    }
}

static void
fast_composite_add_n_8888_8888_ca (pixman_implementation_t *imp,
				   pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src, s;
    uint32_t    *dst_line, *dst, d;
    uint32_t    *mask_line, *mask, ma;
    int dst_stride, mask_stride;
    int32_t w;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint32_t, mask_stride, mask_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    ma = *mask++;

	    if (ma)
	    {
		d = *dst;
		s = src;

		UN8x4_MUL_UN8x4_ADD_UN8x4 (s, ma, d);

		*dst = s;
	    }

	    dst++;
	}
    }
}

static void
fast_composite_over_n_8888_8888_ca (pixman_implementation_t *imp,
                                    pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src, srca, s;
    uint32_t    *dst_line, *dst, d;
    uint32_t    *mask_line, *mask, ma;
    int dst_stride, mask_stride;
    int32_t w;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    srca = src >> 24;
    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint32_t, mask_stride, mask_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    ma = *mask++;
	    if (ma == 0xffffffff)
	    {
		if (srca == 0xff)
		    *dst = src;
		else
		    *dst = over (src, *dst);
	    }
	    else if (ma)
	    {
		d = *dst;
		s = src;

		UN8x4_MUL_UN8x4 (s, ma);
		UN8x4_MUL_UN8 (ma, srca);
		ma = ~ma;
		UN8x4_MUL_UN8x4_ADD_UN8x4 (d, ma, s);

		*dst = d;
	    }

	    dst++;
	}
    }
}

static void
fast_composite_over_n_8_0888 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src, srca;
    uint8_t     *dst_line, *dst;
    uint32_t d;
    uint8_t     *mask_line, *mask, m;
    int dst_stride, mask_stride;
    int32_t w;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    srca = src >> 24;
    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint8_t, dst_stride, dst_line, 3);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint8_t, mask_stride, mask_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    m = *mask++;
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		{
		    d = src;
		}
		else
		{
		    d = fetch_24 (dst);
		    d = over (src, d);
		}
		store_24 (dst, d);
	    }
	    else if (m)
	    {
		d = over (in (src, m), fetch_24 (dst));
		store_24 (dst, d);
	    }
	    dst += 3;
	}
    }
}

static void
fast_composite_over_n_8_0565 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src, srca;
    uint16_t    *dst_line, *dst;
    uint32_t d;
    uint8_t     *mask_line, *mask, m;
    int dst_stride, mask_stride;
    int32_t w;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    srca = src >> 24;
    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint16_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint8_t, mask_stride, mask_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    m = *mask++;
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		{
		    d = src;
		}
		else
		{
		    d = *dst;
		    d = over (src, convert_0565_to_0888 (d));
		}
		*dst = convert_8888_to_0565 (d);
	    }
	    else if (m)
	    {
		d = *dst;
		d = over (in (src, m), convert_0565_to_0888 (d));
		*dst = convert_8888_to_0565 (d);
	    }
	    dst++;
	}
    }
}

static void
fast_composite_over_n_8888_0565_ca (pixman_implementation_t *imp,
                                    pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t  src, srca, s;
    uint16_t  src16;
    uint16_t *dst_line, *dst;
    uint32_t  d;
    uint32_t *mask_line, *mask, ma;
    int dst_stride, mask_stride;
    int32_t w;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    srca = src >> 24;
    if (src == 0)
	return;

    src16 = convert_8888_to_0565 (src);

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint16_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint32_t, mask_stride, mask_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    ma = *mask++;
	    if (ma == 0xffffffff)
	    {
		if (srca == 0xff)
		{
		    *dst = src16;
		}
		else
		{
		    d = *dst;
		    d = over (src, convert_0565_to_0888 (d));
		    *dst = convert_8888_to_0565 (d);
		}
	    }
	    else if (ma)
	    {
		d = *dst;
		d = convert_0565_to_0888 (d);

		s = src;

		UN8x4_MUL_UN8x4 (s, ma);
		UN8x4_MUL_UN8 (ma, srca);
		ma = ~ma;
		UN8x4_MUL_UN8x4_ADD_UN8x4 (d, ma, s);

		*dst = convert_8888_to_0565 (d);
	    }
	    dst++;
	}
    }
}

static void
fast_composite_over_8888_8888 (pixman_implementation_t *imp,
                               pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t    *dst_line, *dst;
    uint32_t    *src_line, *src, s;
    int dst_stride, src_stride;
    uint8_t a;
    int32_t w;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint32_t, src_stride, src_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    a = s >> 24;
	    if (a == 0xff)
		*dst = s;
	    else if (s)
		*dst = over (s, *dst);
	    dst++;
	}
    }
}

static void
fast_composite_src_x888_8888 (pixman_implementation_t *imp,
			      pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t    *dst_line, *dst;
    uint32_t    *src_line, *src;
    int dst_stride, src_stride;
    int32_t w;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint32_t, src_stride, src_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	    *dst++ = (*src++) | 0xff000000;
    }
}

#if 0
static void
fast_composite_over_8888_0888 (pixman_implementation_t *imp,
			       pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint8_t     *dst_line, *dst;
    uint32_t d;
    uint32_t    *src_line, *src, s;
    uint8_t a;
    int dst_stride, src_stride;
    int32_t w;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint8_t, dst_stride, dst_line, 3);
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint32_t, src_stride, src_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    a = s >> 24;
	    if (a)
	    {
		if (a == 0xff)
		    d = s;
		else
		    d = over (s, fetch_24 (dst));

		store_24 (dst, d);
	    }
	    dst += 3;
	}
    }
}
#endif

static void
fast_composite_over_8888_0565 (pixman_implementation_t *imp,
                               pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint16_t    *dst_line, *dst;
    uint32_t d;
    uint32_t    *src_line, *src, s;
    uint8_t a;
    int dst_stride, src_stride;
    int32_t w;

    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint32_t, src_stride, src_line, 1);
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint16_t, dst_stride, dst_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    a = s >> 24;
	    if (s)
	    {
		if (a == 0xff)
		{
		    d = s;
		}
		else
		{
		    d = *dst;
		    d = over (s, convert_0565_to_0888 (d));
		}
		*dst = convert_8888_to_0565 (d);
	    }
	    dst++;
	}
    }
}

static void
fast_composite_add_8_8 (pixman_implementation_t *imp,
			pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint8_t     *dst_line, *dst;
    uint8_t     *src_line, *src;
    int dst_stride, src_stride;
    int32_t w;
    uint8_t s, d;
    uint16_t t;

    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint8_t, src_stride, src_line, 1);
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint8_t, dst_stride, dst_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    if (s)
	    {
		if (s != 0xff)
		{
		    d = *dst;
		    t = d + s;
		    s = t | (0 - (t >> 8));
		}
		*dst = s;
	    }
	    dst++;
	}
    }
}

static void
fast_composite_add_0565_0565 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint16_t    *dst_line, *dst;
    uint32_t	d;
    uint16_t    *src_line, *src;
    uint32_t	s;
    int dst_stride, src_stride;
    int32_t w;

    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint16_t, src_stride, src_line, 1);
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint16_t, dst_stride, dst_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    if (s)
	    {
		d = *dst;
		s = convert_0565_to_8888 (s);
		if (d)
		{
		    d = convert_0565_to_8888 (d);
		    UN8x4_ADD_UN8x4 (s, d);
		}
		*dst = convert_8888_to_0565 (s);
	    }
	    dst++;
	}
    }
}

static void
fast_composite_add_8888_8888 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t    *dst_line, *dst;
    uint32_t    *src_line, *src;
    int dst_stride, src_stride;
    int32_t w;
    uint32_t s, d;

    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, uint32_t, src_stride, src_line, 1);
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    s = *src++;
	    if (s)
	    {
		if (s != 0xffffffff)
		{
		    d = *dst;
		    if (d)
			UN8x4_ADD_UN8x4 (s, d);
		}
		*dst = s;
	    }
	    dst++;
	}
    }
}

static void
fast_composite_add_n_8_8 (pixman_implementation_t *imp,
			  pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint8_t     *dst_line, *dst;
    uint8_t     *mask_line, *mask;
    int dst_stride, mask_stride;
    int32_t w;
    uint32_t src;
    uint8_t sa;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint8_t, dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, uint8_t, mask_stride, mask_line, 1);
    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);
    sa = (src >> 24);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;

	while (w--)
	{
	    uint16_t tmp;
	    uint16_t a;
	    uint32_t m, d;
	    uint32_t r;

	    a = *mask++;
	    d = *dst;

	    m = MUL_UN8 (sa, a, tmp);
	    r = ADD_UN8 (m, d, tmp);

	    *dst++ = r;
	}
    }
}

#ifdef WORDS_BIGENDIAN
#define CREATE_BITMASK(n) (0x80000000 >> (n))
#define UPDATE_BITMASK(n) ((n) >> 1)
#else
#define CREATE_BITMASK(n) (1 << (n))
#define UPDATE_BITMASK(n) ((n) << 1)
#endif

#define TEST_BIT(p, n)					\
    (*((p) + ((n) >> 5)) & CREATE_BITMASK ((n) & 31))
#define SET_BIT(p, n)							\
    do { *((p) + ((n) >> 5)) |= CREATE_BITMASK ((n) & 31); } while (0);

static void
fast_composite_add_1_1 (pixman_implementation_t *imp,
			pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t     *dst_line, *dst;
    uint32_t     *src_line, *src;
    int           dst_stride, src_stride;
    int32_t       w;

    PIXMAN_IMAGE_GET_LINE (src_image, 0, src_y, uint32_t,
                           src_stride, src_line, 1);
    PIXMAN_IMAGE_GET_LINE (dest_image, 0, dest_y, uint32_t,
                           dst_stride, dst_line, 1);

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	while (w--)
	{
	    



	    if (TEST_BIT (src, src_x + w))
		SET_BIT (dst, dest_x + w);
	}
    }
}

static void
fast_composite_over_n_1_8888 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t     src, srca;
    uint32_t    *dst, *dst_line;
    uint32_t    *mask, *mask_line;
    int          mask_stride, dst_stride;
    uint32_t     bitcache, bitmask;
    int32_t      w;

    if (width <= 0)
	return;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);
    srca = src >> 24;
    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t,
                           dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, 0, mask_y, uint32_t,
                           mask_stride, mask_line, 1);
    mask_line += mask_x >> 5;

    if (srca == 0xff)
    {
	while (height--)
	{
	    dst = dst_line;
	    dst_line += dst_stride;
	    mask = mask_line;
	    mask_line += mask_stride;
	    w = width;

	    bitcache = *mask++;
	    bitmask = CREATE_BITMASK (mask_x & 31);

	    while (w--)
	    {
		if (bitmask == 0)
		{
		    bitcache = *mask++;
		    bitmask = CREATE_BITMASK (0);
		}
		if (bitcache & bitmask)
		    *dst = src;
		bitmask = UPDATE_BITMASK (bitmask);
		dst++;
	    }
	}
    }
    else
    {
	while (height--)
	{
	    dst = dst_line;
	    dst_line += dst_stride;
	    mask = mask_line;
	    mask_line += mask_stride;
	    w = width;

	    bitcache = *mask++;
	    bitmask = CREATE_BITMASK (mask_x & 31);

	    while (w--)
	    {
		if (bitmask == 0)
		{
		    bitcache = *mask++;
		    bitmask = CREATE_BITMASK (0);
		}
		if (bitcache & bitmask)
		    *dst = over (src, *dst);
		bitmask = UPDATE_BITMASK (bitmask);
		dst++;
	    }
	}
    }
}

static void
fast_composite_over_n_1_0565 (pixman_implementation_t *imp,
                              pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t     src, srca;
    uint16_t    *dst, *dst_line;
    uint32_t    *mask, *mask_line;
    int          mask_stride, dst_stride;
    uint32_t     bitcache, bitmask;
    int32_t      w;
    uint32_t     d;
    uint16_t     src565;

    if (width <= 0)
	return;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);
    srca = src >> 24;
    if (src == 0)
	return;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint16_t,
                           dst_stride, dst_line, 1);
    PIXMAN_IMAGE_GET_LINE (mask_image, 0, mask_y, uint32_t,
                           mask_stride, mask_line, 1);
    mask_line += mask_x >> 5;

    if (srca == 0xff)
    {
	src565 = convert_8888_to_0565 (src);
	while (height--)
	{
	    dst = dst_line;
	    dst_line += dst_stride;
	    mask = mask_line;
	    mask_line += mask_stride;
	    w = width;

	    bitcache = *mask++;
	    bitmask = CREATE_BITMASK (mask_x & 31);

	    while (w--)
	    {
		if (bitmask == 0)
		{
		    bitcache = *mask++;
		    bitmask = CREATE_BITMASK (0);
		}
		if (bitcache & bitmask)
		    *dst = src565;
		bitmask = UPDATE_BITMASK (bitmask);
		dst++;
	    }
	}
    }
    else
    {
	while (height--)
	{
	    dst = dst_line;
	    dst_line += dst_stride;
	    mask = mask_line;
	    mask_line += mask_stride;
	    w = width;

	    bitcache = *mask++;
	    bitmask = CREATE_BITMASK (mask_x & 31);

	    while (w--)
	    {
		if (bitmask == 0)
		{
		    bitcache = *mask++;
		    bitmask = CREATE_BITMASK (0);
		}
		if (bitcache & bitmask)
		{
		    d = over (src, convert_0565_to_0888 (*dst));
		    *dst = convert_8888_to_0565 (d);
		}
		bitmask = UPDATE_BITMASK (bitmask);
		dst++;
	    }
	}
    }
}





static void
fast_composite_solid_fill (pixman_implementation_t *imp,
                           pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t src;

    src = _pixman_image_get_solid (imp, src_image, dest_image->bits.format);

    if (dest_image->bits.format == PIXMAN_a1)
    {
	src = src >> 31;
    }
    else if (dest_image->bits.format == PIXMAN_a8)
    {
	src = src >> 24;
    }
    else if (dest_image->bits.format == PIXMAN_r5g6b5 ||
             dest_image->bits.format == PIXMAN_b5g6r5)
    {
	src = convert_8888_to_0565 (src);
    }

    pixman_fill (dest_image->bits.bits, dest_image->bits.rowstride,
                 PIXMAN_FORMAT_BPP (dest_image->bits.format),
                 dest_x, dest_y,
                 width, height,
                 src);
}

static void
fast_composite_src_memcpy (pixman_implementation_t *imp,
			   pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    int bpp = PIXMAN_FORMAT_BPP (dest_image->bits.format) / 8;
    uint32_t n_bytes = width * bpp;
    int dst_stride, src_stride;
    uint8_t    *dst;
    uint8_t    *src;

    src_stride = src_image->bits.rowstride * 4;
    dst_stride = dest_image->bits.rowstride * 4;

    src = (uint8_t *)src_image->bits.bits + src_y * src_stride + src_x * bpp;
    dst = (uint8_t *)dest_image->bits.bits + dest_y * dst_stride + dest_x * bpp;

    while (height--)
    {
	memcpy (dst, src, n_bytes);

	dst += dst_stride;
	src += src_stride;
    }
}

FAST_NEAREST (8888_8888_cover, 8888, 8888, uint32_t, uint32_t, SRC, COVER)
FAST_NEAREST (8888_8888_none, 8888, 8888, uint32_t, uint32_t, SRC, NONE)
FAST_NEAREST (8888_8888_pad, 8888, 8888, uint32_t, uint32_t, SRC, PAD)
FAST_NEAREST (8888_8888_normal, 8888, 8888, uint32_t, uint32_t, SRC, NORMAL)
FAST_NEAREST (x888_8888_cover, x888, 8888, uint32_t, uint32_t, SRC, COVER)
FAST_NEAREST (x888_8888_pad, x888, 8888, uint32_t, uint32_t, SRC, PAD)
FAST_NEAREST (x888_8888_normal, x888, 8888, uint32_t, uint32_t, SRC, NORMAL)
FAST_NEAREST (8888_8888_cover, 8888, 8888, uint32_t, uint32_t, OVER, COVER)
FAST_NEAREST (8888_8888_none, 8888, 8888, uint32_t, uint32_t, OVER, NONE)
FAST_NEAREST (8888_8888_pad, 8888, 8888, uint32_t, uint32_t, OVER, PAD)
FAST_NEAREST (8888_8888_normal, 8888, 8888, uint32_t, uint32_t, OVER, NORMAL)
FAST_NEAREST (8888_565_cover, 8888, 0565, uint32_t, uint16_t, SRC, COVER)
FAST_NEAREST (8888_565_none, 8888, 0565, uint32_t, uint16_t, SRC, NONE)
FAST_NEAREST (8888_565_pad, 8888, 0565, uint32_t, uint16_t, SRC, PAD)
FAST_NEAREST (8888_565_normal, 8888, 0565, uint32_t, uint16_t, SRC, NORMAL)
FAST_NEAREST (565_565_normal, 0565, 0565, uint16_t, uint16_t, SRC, NORMAL)
FAST_NEAREST (8888_565_cover, 8888, 0565, uint32_t, uint16_t, OVER, COVER)
FAST_NEAREST (8888_565_none, 8888, 0565, uint32_t, uint16_t, OVER, NONE)
FAST_NEAREST (8888_565_pad, 8888, 0565, uint32_t, uint16_t, OVER, PAD)
FAST_NEAREST (8888_565_normal, 8888, 0565, uint32_t, uint16_t, OVER, NORMAL)

static force_inline void
scaled_bilinear_scanline_8888_565_OVER (uint16_t *       dst,
                                        const uint32_t * mask,
                                        const uint32_t * src_top,
                                        const uint32_t * src_bottom,
                                        int32_t          w,
                                        int              wt,
                                        int              wb,
                                        pixman_fixed_t   vx,
                                        pixman_fixed_t   unit_x,
                                        pixman_fixed_t   max_vx,
                                        pixman_bool_t    zero_src)
{
    while ((w -= 1) >= 0)
    {
	uint32_t tl = src_top [pixman_fixed_to_int (vx)];
	uint32_t tr = src_top [pixman_fixed_to_int (vx) + 1];
	uint32_t bl = src_bottom [pixman_fixed_to_int (vx)];
	uint32_t br = src_bottom [pixman_fixed_to_int (vx) + 1];
	uint32_t src, result;
	uint16_t d;
	d = *dst;
	src = bilinear_interpolation (tl, tr,
				      bl, br,
				      pixman_fixed_to_bilinear_weight(vx),
				      wb);
	vx += unit_x;
	result = over (src, convert_0565_to_0888 (d));
	*dst++ = convert_8888_to_0565 (result);
    }
}

static force_inline void
scaled_bilinear_scanline_8888_8888_OVER (uint32_t *       dst,
                                         const uint32_t * mask,
                                         const uint32_t * src_top,
                                         const uint32_t * src_bottom,
                                         int32_t          w,
                                         int              wt,
                                         int              wb,
                                         pixman_fixed_t   vx,
                                         pixman_fixed_t   unit_x,
                                         pixman_fixed_t   max_vx,
                                         pixman_bool_t    zero_src)
{
    while ((w -= 1) >= 0)
    {
	uint32_t tl = src_top [pixman_fixed_to_int (vx)];
	uint32_t tr = src_top [pixman_fixed_to_int (vx) + 1];
	uint32_t bl = src_bottom [pixman_fixed_to_int (vx)];
	uint32_t br = src_bottom [pixman_fixed_to_int (vx) + 1];
	uint32_t src;
	uint32_t d;
	uint32_t result;
	d = *dst;
	src = bilinear_interpolation (tl, tr,
				      bl, br,
				      pixman_fixed_to_bilinear_weight(vx),
				      wb);
	vx += unit_x;
	*dst++ = over (src, d);
    }
}

#ifndef LOWER_QUALITY_INTERPOLATION

static force_inline void
scaled_bilinear_scanline_565_565_SRC (uint16_t *       dst,
				      const uint32_t * mask,
				      const uint16_t * src_top,
				      const uint16_t * src_bottom,
				      int32_t          w,
				      int              wt,
				      int              wb,
				      pixman_fixed_t   vx,
				      pixman_fixed_t   unit_x,
				      pixman_fixed_t   max_vx,
				      pixman_bool_t    zero_src)
{
    while ((w -= 1) >= 0)
    {
	uint16_t tl = src_top [pixman_fixed_to_int (vx)];
	uint16_t tr = src_top [pixman_fixed_to_int (vx) + 1];
	uint16_t bl = src_bottom [pixman_fixed_to_int (vx)];
	uint16_t br = src_bottom [pixman_fixed_to_int (vx) + 1];
	uint32_t d;
	d = bilinear_interpolation(convert_0565_to_8888 (tl),
				   convert_0565_to_8888 (tr),
				   convert_0565_to_8888 (bl),
				   convert_0565_to_8888 (br),
				   pixman_fixed_to_bilinear_weight (vx),
				   wb);
	vx += unit_x;
	*dst++ = convert_8888_to_0565 (d);
    }
}

#else













#define GREEN_MASK (((1 << 6) - 1) << 5)

static inline uint32_t
expand_rgb_565 (uint16_t c) {
    return ((c & GREEN_MASK) << 16) | (c & ~GREEN_MASK);
}

static inline uint16_t
compact_rgb_565 (uint32_t c) {
    return ((c >> 16) & GREEN_MASK) | (c & ~GREEN_MASK);
}

static inline uint16_t
bilinear_interpolation_565(uint16_t tl, uint16_t tr,
			   uint16_t bl, uint16_t br,
			   int x, int y)
{
    int xy;
    uint32_t a00 = expand_rgb_565 (tl);
    uint32_t a01 = expand_rgb_565 (tr);
    uint32_t a10 = expand_rgb_565 (bl);
    uint32_t a11 = expand_rgb_565 (br);

    xy = (x * y) >> 3;
    return compact_rgb_565 ((a00 * (32 - 2*y - 2*x + xy) +
			     a01 * (2*x - xy) +
			     a10 * (2*y - xy) +
			     a11 * xy) >> 5);
}

static force_inline void
scaled_bilinear_scanline_565_565_SRC (uint16_t *       dst,
				      const uint32_t * mask,
				      const uint16_t * src_top,
				      const uint16_t * src_bottom,
				      int32_t          w,
				      int              wt,
				      int              wb,
				      pixman_fixed_t   vx,
				      pixman_fixed_t   unit_x,
				      pixman_fixed_t   max_vx,
				      pixman_bool_t    zero_src)
{
    while ((w -= 1) >= 0)
    {
	uint16_t tl = src_top [pixman_fixed_to_int (vx)];
	uint16_t tr = src_top [pixman_fixed_to_int (vx) + 1];
	uint16_t bl = src_bottom [pixman_fixed_to_int (vx)];
	uint16_t br = src_bottom [pixman_fixed_to_int (vx) + 1];

        uint16_t d = bilinear_interpolation_565 (tl, tr, bl, br,
						 pixman_fixed_to_bilinear_weight(vx),
						 wb);
        vx += unit_x;
        *dst++ = d;
    }
}

#endif

FAST_BILINEAR_MAINLOOP_COMMON (565_565_cover_SRC,
			       scaled_bilinear_scanline_565_565_SRC, NULL,
			       uint16_t, uint32_t, uint16_t,
			       COVER, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (565_565_pad_SRC,
			       scaled_bilinear_scanline_565_565_SRC, NULL,
			       uint16_t, uint32_t, uint16_t,
			       PAD, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (565_565_none_SRC,
			       scaled_bilinear_scanline_565_565_SRC, NULL,
			       uint16_t, uint32_t, uint16_t,
			       NONE, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (565_565_normal_SRC,
			       scaled_bilinear_scanline_565_565_SRC, NULL,
			       uint16_t, uint32_t, uint16_t,
			       NORMAL, FLAG_NONE)

FAST_BILINEAR_MAINLOOP_COMMON (8888_565_cover_OVER,
			       scaled_bilinear_scanline_8888_565_OVER, NULL,
			       uint32_t, uint32_t, uint16_t,
			       COVER, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (8888_565_pad_OVER,
			       scaled_bilinear_scanline_8888_565_OVER, NULL,
			       uint32_t, uint32_t, uint16_t,
			       PAD, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (8888_565_none_OVER,
			       scaled_bilinear_scanline_8888_565_OVER, NULL,
			       uint32_t, uint32_t, uint16_t,
			       NONE, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (8888_565_normal_OVER,
			       scaled_bilinear_scanline_8888_565_OVER, NULL,
			       uint32_t, uint32_t, uint16_t,
			       NORMAL, FLAG_NONE)

FAST_BILINEAR_MAINLOOP_COMMON (8888_8888_cover_OVER,
			       scaled_bilinear_scanline_8888_8888_OVER, NULL,
			       uint32_t, uint32_t, uint32_t,
			       COVER, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (8888_8888_pad_OVER,
			       scaled_bilinear_scanline_8888_8888_OVER, NULL,
			       uint32_t, uint32_t, uint32_t,
			       PAD, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (8888_8888_none_OVER,
			       scaled_bilinear_scanline_8888_8888_OVER, NULL,
			       uint32_t, uint32_t, uint32_t,
			       NONE, FLAG_NONE)
FAST_BILINEAR_MAINLOOP_COMMON (8888_8888_normal_OVER,
			       scaled_bilinear_scanline_8888_8888_OVER, NULL,
			       uint32_t, uint32_t, uint32_t,
			       NORMAL, FLAG_NONE)

#define REPEAT_MIN_WIDTH    32

static void
fast_composite_tiled_repeat (pixman_implementation_t *imp,
			     pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    pixman_composite_func_t func;
    pixman_format_code_t mask_format;
    uint32_t src_flags, mask_flags;
    int32_t sx, sy;
    int32_t width_remain;
    int32_t num_pixels;
    int32_t src_width;
    int32_t i, j;
    pixman_image_t extended_src_image;
    uint32_t extended_src[REPEAT_MIN_WIDTH * 2];
    pixman_bool_t need_src_extension;
    uint32_t *src_line;
    int32_t src_stride;
    int32_t src_bpp;
    pixman_composite_info_t info2 = *info;

    src_flags = (info->src_flags & ~FAST_PATH_NORMAL_REPEAT) |
		    FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;

    if (mask_image)
    {
	mask_format = mask_image->common.extended_format_code;
	mask_flags = info->mask_flags;
    }
    else
    {
	mask_format = PIXMAN_null;
	mask_flags = FAST_PATH_IS_OPAQUE;
    }

    _pixman_implementation_lookup_composite (
	imp->toplevel, info->op,
	src_image->common.extended_format_code, src_flags,
	mask_format, mask_flags,
	dest_image->common.extended_format_code, info->dest_flags,
	&imp, &func);

    src_bpp = PIXMAN_FORMAT_BPP (src_image->bits.format);

    if (src_image->bits.width < REPEAT_MIN_WIDTH		&&
	(src_bpp == 32 || src_bpp == 16 || src_bpp == 8)	&&
	!src_image->bits.indexed)
    {
	sx = src_x;
	sx = MOD (sx, src_image->bits.width);
	sx += width;
	src_width = 0;

	while (src_width < REPEAT_MIN_WIDTH && src_width <= sx)
	    src_width += src_image->bits.width;

	src_stride = (src_width * (src_bpp >> 3) + 3) / (int) sizeof (uint32_t);

	
	_pixman_bits_image_init (&extended_src_image, src_image->bits.format,
				 src_width, 1, &extended_src[0], src_stride,
				 FALSE);
	_pixman_image_validate (&extended_src_image);

	info2.src_image = &extended_src_image;
	need_src_extension = TRUE;
    }
    else
    {
	src_width = src_image->bits.width;
	need_src_extension = FALSE;
    }

    sx = src_x;
    sy = src_y;

    while (--height >= 0)
    {
	sx = MOD (sx, src_width);
	sy = MOD (sy, src_image->bits.height);

	if (need_src_extension)
	{
	    if (src_bpp == 32)
	    {
		PIXMAN_IMAGE_GET_LINE (src_image, 0, sy, uint32_t, src_stride, src_line, 1);

		for (i = 0; i < src_width; )
		{
		    for (j = 0; j < src_image->bits.width; j++, i++)
			extended_src[i] = src_line[j];
		}
	    }
	    else if (src_bpp == 16)
	    {
		uint16_t *src_line_16;

		PIXMAN_IMAGE_GET_LINE (src_image, 0, sy, uint16_t, src_stride,
				       src_line_16, 1);
		src_line = (uint32_t*)src_line_16;

		for (i = 0; i < src_width; )
		{
		    for (j = 0; j < src_image->bits.width; j++, i++)
			((uint16_t*)extended_src)[i] = ((uint16_t*)src_line)[j];
		}
	    }
	    else if (src_bpp == 8)
	    {
		uint8_t *src_line_8;

		PIXMAN_IMAGE_GET_LINE (src_image, 0, sy, uint8_t, src_stride,
				       src_line_8, 1);
		src_line = (uint32_t*)src_line_8;

		for (i = 0; i < src_width; )
		{
		    for (j = 0; j < src_image->bits.width; j++, i++)
			((uint8_t*)extended_src)[i] = ((uint8_t*)src_line)[j];
		}
	    }

	    info2.src_y = 0;
	}
	else
	{
	    info2.src_y = sy;
	}

	width_remain = width;

	while (width_remain > 0)
	{
	    num_pixels = src_width - sx;

	    if (num_pixels > width_remain)
		num_pixels = width_remain;

	    info2.src_x = sx;
	    info2.width = num_pixels;
	    info2.height = 1;

	    func (imp, &info2);

	    width_remain -= num_pixels;
	    info2.mask_x += num_pixels;
	    info2.dest_x += num_pixels;
	    sx = 0;
	}

	sx = src_x;
	sy++;
	info2.mask_x = info->mask_x;
	info2.mask_y++;
	info2.dest_x = info->dest_x;
	info2.dest_y++;
    }

    if (need_src_extension)
	_pixman_image_fini (&extended_src_image);
}


static force_inline void
scaled_nearest_scanline_565_565_SRC (uint16_t *       dst,
				     const uint16_t * src,
				     int32_t          w,
				     pixman_fixed_t   vx,
				     pixman_fixed_t   unit_x,
				     pixman_fixed_t   max_vx,
				     pixman_bool_t    fully_transparent_src)
{
    uint16_t tmp1, tmp2, tmp3, tmp4;
    while ((w -= 4) >= 0)
    {
	tmp1 = *(src + pixman_fixed_to_int (vx));
	vx += unit_x;
	tmp2 = *(src + pixman_fixed_to_int (vx));
	vx += unit_x;
	tmp3 = *(src + pixman_fixed_to_int (vx));
	vx += unit_x;
	tmp4 = *(src + pixman_fixed_to_int (vx));
	vx += unit_x;
	*dst++ = tmp1;
	*dst++ = tmp2;
	*dst++ = tmp3;
	*dst++ = tmp4;
    }
    if (w & 2)
    {
	tmp1 = *(src + pixman_fixed_to_int (vx));
	vx += unit_x;
	tmp2 = *(src + pixman_fixed_to_int (vx));
	vx += unit_x;
	*dst++ = tmp1;
	*dst++ = tmp2;
    }
    if (w & 1)
	*dst = *(src + pixman_fixed_to_int (vx));
}

FAST_NEAREST_MAINLOOP (565_565_cover_SRC,
		       scaled_nearest_scanline_565_565_SRC,
		       uint16_t, uint16_t, COVER)
FAST_NEAREST_MAINLOOP (565_565_none_SRC,
		       scaled_nearest_scanline_565_565_SRC,
		       uint16_t, uint16_t, NONE)
FAST_NEAREST_MAINLOOP (565_565_pad_SRC,
		       scaled_nearest_scanline_565_565_SRC,
		       uint16_t, uint16_t, PAD)

static force_inline uint32_t
fetch_nearest (pixman_repeat_t src_repeat,
	       pixman_format_code_t format,
	       uint32_t *src, int x, int src_width)
{
    if (repeat (src_repeat, &x, src_width))
    {
	if (format == PIXMAN_x8r8g8b8 || format == PIXMAN_x8b8g8r8)
	    return *(src + x) | 0xff000000;
	else
	    return *(src + x);
    }
    else
    {
	return 0;
    }
}

static force_inline void
combine_over (uint32_t s, uint32_t *dst)
{
    if (s)
    {
	uint8_t ia = 0xff - (s >> 24);

	if (ia)
	    UN8x4_MUL_UN8_ADD_UN8x4 (*dst, ia, s);
	else
	    *dst = s;
    }
}

static force_inline void
combine_src (uint32_t s, uint32_t *dst)
{
    *dst = s;
}

static void
fast_composite_scaled_nearest (pixman_implementation_t *imp,
			       pixman_composite_info_t *info)
{
    PIXMAN_COMPOSITE_ARGS (info);
    uint32_t       *dst_line;
    uint32_t       *src_line;
    int             dst_stride, src_stride;
    int		    src_width, src_height;
    pixman_repeat_t src_repeat;
    pixman_fixed_t unit_x, unit_y;
    pixman_format_code_t src_format;
    pixman_vector_t v;
    pixman_fixed_t vy;

    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, uint32_t, dst_stride, dst_line, 1);
    


    PIXMAN_IMAGE_GET_LINE (src_image, 0, 0, uint32_t, src_stride, src_line, 1);

    
    v.vector[0] = pixman_int_to_fixed (src_x) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (src_y) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    if (!pixman_transform_point_3d (src_image->common.transform, &v))
	return;

    unit_x = src_image->common.transform->matrix[0][0];
    unit_y = src_image->common.transform->matrix[1][1];

    
    v.vector[0] -= pixman_fixed_e;
    v.vector[1] -= pixman_fixed_e;

    src_height = src_image->bits.height;
    src_width = src_image->bits.width;
    src_repeat = src_image->common.repeat;
    src_format = src_image->bits.format;

    vy = v.vector[1];
    while (height--)
    {
        pixman_fixed_t vx = v.vector[0];
	int y = pixman_fixed_to_int (vy);
	uint32_t *dst = dst_line;

	dst_line += dst_stride;

        

        vy += unit_y;

	if (!repeat (src_repeat, &y, src_height))
	{
	    if (op == PIXMAN_OP_SRC)
		memset (dst, 0, sizeof (*dst) * width);
	}
	else
	{
	    int w = width;

	    uint32_t *src = src_line + y * src_stride;

	    while (w >= 2)
	    {
		uint32_t s1, s2;
		int x1, x2;

		x1 = pixman_fixed_to_int (vx);
		vx += unit_x;

		x2 = pixman_fixed_to_int (vx);
		vx += unit_x;

		w -= 2;

		s1 = fetch_nearest (src_repeat, src_format, src, x1, src_width);
		s2 = fetch_nearest (src_repeat, src_format, src, x2, src_width);

		if (op == PIXMAN_OP_OVER)
		{
		    combine_over (s1, dst++);
		    combine_over (s2, dst++);
		}
		else
		{
		    combine_src (s1, dst++);
		    combine_src (s2, dst++);
		}
	    }

	    while (w--)
	    {
		uint32_t s;
		int x;

		x = pixman_fixed_to_int (vx);
		vx += unit_x;

		s = fetch_nearest (src_repeat, src_format, src, x, src_width);

		if (op == PIXMAN_OP_OVER)
		    combine_over (s, dst++);
		else
		    combine_src (s, dst++);
	    }
	}
    }
}

#define CACHE_LINE_SIZE 64

#define FAST_SIMPLE_ROTATE(suffix, pix_type)                                  \
                                                                              \
static void                                                                   \
blt_rotated_90_trivial_##suffix (pix_type       *dst,                         \
				 int             dst_stride,                  \
				 const pix_type *src,                         \
				 int             src_stride,                  \
				 int             w,                           \
				 int             h)                           \
{                                                                             \
    int x, y;                                                                 \
    for (y = 0; y < h; y++)                                                   \
    {                                                                         \
	const pix_type *s = src + (h - y - 1);                                \
	pix_type *d = dst + dst_stride * y;                                   \
	for (x = 0; x < w; x++)                                               \
	{                                                                     \
	    *d++ = *s;                                                        \
	    s += src_stride;                                                  \
	}                                                                     \
    }                                                                         \
}                                                                             \
                                                                              \
static void                                                                   \
blt_rotated_270_trivial_##suffix (pix_type       *dst,                        \
				  int             dst_stride,                 \
				  const pix_type *src,                        \
				  int             src_stride,                 \
				  int             w,                          \
				  int             h)                          \
{                                                                             \
    int x, y;                                                                 \
    for (y = 0; y < h; y++)                                                   \
    {                                                                         \
	const pix_type *s = src + src_stride * (w - 1) + y;                   \
	pix_type *d = dst + dst_stride * y;                                   \
	for (x = 0; x < w; x++)                                               \
	{                                                                     \
	    *d++ = *s;                                                        \
	    s -= src_stride;                                                  \
	}                                                                     \
    }                                                                         \
}                                                                             \
                                                                              \
static void                                                                   \
blt_rotated_90_##suffix (pix_type       *dst,                                 \
			 int             dst_stride,                          \
			 const pix_type *src,                                 \
			 int             src_stride,                          \
			 int             W,                                   \
			 int             H)                                   \
{                                                                             \
    int x;                                                                    \
    int leading_pixels = 0, trailing_pixels = 0;                              \
    const int TILE_SIZE = CACHE_LINE_SIZE / sizeof(pix_type);                 \
                                                                              \
    /*                                                                        \
     * split processing into handling destination as TILE_SIZExH cache line   \
     * aligned vertical stripes (optimistically assuming that destination     \
     * stride is a multiple of cache line, if not - it will be just a bit     \
     * slower)                                                                \
     */                                                                       \
                                                                              \
    if ((uintptr_t)dst & (CACHE_LINE_SIZE - 1))                               \
    {                                                                         \
	leading_pixels = TILE_SIZE - (((uintptr_t)dst &                       \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
	if (leading_pixels > W)                                               \
	    leading_pixels = W;                                               \
                                                                              \
	/* unaligned leading part NxH (where N < TILE_SIZE) */                \
	blt_rotated_90_trivial_##suffix (                                     \
	    dst,                                                              \
	    dst_stride,                                                       \
	    src,                                                              \
	    src_stride,                                                       \
	    leading_pixels,                                                   \
	    H);                                                               \
	                                                                      \
	dst += leading_pixels;                                                \
	src += leading_pixels * src_stride;                                   \
	W -= leading_pixels;                                                  \
    }                                                                         \
                                                                              \
    if ((uintptr_t)(dst + W) & (CACHE_LINE_SIZE - 1))                         \
    {                                                                         \
	trailing_pixels = (((uintptr_t)(dst + W) &                            \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
	if (trailing_pixels > W)                                              \
	    trailing_pixels = W;                                              \
	W -= trailing_pixels;                                                 \
    }                                                                         \
                                                                              \
    for (x = 0; x < W; x += TILE_SIZE)                                        \
    {                                                                         \
	/* aligned middle part TILE_SIZExH */                                 \
	blt_rotated_90_trivial_##suffix (                                     \
	    dst + x,                                                          \
	    dst_stride,                                                       \
	    src + src_stride * x,                                             \
	    src_stride,                                                       \
	    TILE_SIZE,                                                        \
	    H);                                                               \
    }                                                                         \
                                                                              \
    if (trailing_pixels)                                                      \
    {                                                                         \
	/* unaligned trailing part NxH (where N < TILE_SIZE) */               \
	blt_rotated_90_trivial_##suffix (                                     \
	    dst + W,                                                          \
	    dst_stride,                                                       \
	    src + W * src_stride,                                             \
	    src_stride,                                                       \
	    trailing_pixels,                                                  \
	    H);                                                               \
    }                                                                         \
}                                                                             \
                                                                              \
static void                                                                   \
blt_rotated_270_##suffix (pix_type       *dst,                                \
			  int             dst_stride,                         \
			  const pix_type *src,                                \
			  int             src_stride,                         \
			  int             W,                                  \
			  int             H)                                  \
{                                                                             \
    int x;                                                                    \
    int leading_pixels = 0, trailing_pixels = 0;                              \
    const int TILE_SIZE = CACHE_LINE_SIZE / sizeof(pix_type);                 \
                                                                              \
    /*                                                                        \
     * split processing into handling destination as TILE_SIZExH cache line   \
     * aligned vertical stripes (optimistically assuming that destination     \
     * stride is a multiple of cache line, if not - it will be just a bit     \
     * slower)                                                                \
     */                                                                       \
                                                                              \
    if ((uintptr_t)dst & (CACHE_LINE_SIZE - 1))                               \
    {                                                                         \
	leading_pixels = TILE_SIZE - (((uintptr_t)dst &                       \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
	if (leading_pixels > W)                                               \
	    leading_pixels = W;                                               \
                                                                              \
	/* unaligned leading part NxH (where N < TILE_SIZE) */                \
	blt_rotated_270_trivial_##suffix (                                    \
	    dst,                                                              \
	    dst_stride,                                                       \
	    src + src_stride * (W - leading_pixels),                          \
	    src_stride,                                                       \
	    leading_pixels,                                                   \
	    H);                                                               \
	                                                                      \
	dst += leading_pixels;                                                \
	W -= leading_pixels;                                                  \
    }                                                                         \
                                                                              \
    if ((uintptr_t)(dst + W) & (CACHE_LINE_SIZE - 1))                         \
    {                                                                         \
	trailing_pixels = (((uintptr_t)(dst + W) &                            \
			    (CACHE_LINE_SIZE - 1)) / sizeof(pix_type));       \
	if (trailing_pixels > W)                                              \
	    trailing_pixels = W;                                              \
	W -= trailing_pixels;                                                 \
	src += trailing_pixels * src_stride;                                  \
    }                                                                         \
                                                                              \
    for (x = 0; x < W; x += TILE_SIZE)                                        \
    {                                                                         \
	/* aligned middle part TILE_SIZExH */                                 \
	blt_rotated_270_trivial_##suffix (                                    \
	    dst + x,                                                          \
	    dst_stride,                                                       \
	    src + src_stride * (W - x - TILE_SIZE),                           \
	    src_stride,                                                       \
	    TILE_SIZE,                                                        \
	    H);                                                               \
    }                                                                         \
                                                                              \
    if (trailing_pixels)                                                      \
    {                                                                         \
	/* unaligned trailing part NxH (where N < TILE_SIZE) */               \
	blt_rotated_270_trivial_##suffix (                                    \
	    dst + W,                                                          \
	    dst_stride,                                                       \
	    src - trailing_pixels * src_stride,                               \
	    src_stride,                                                       \
	    trailing_pixels,                                                  \
	    H);                                                               \
    }                                                                         \
}                                                                             \
                                                                              \
static void                                                                   \
fast_composite_rotate_90_##suffix (pixman_implementation_t *imp,              \
				   pixman_composite_info_t *info)	      \
{									      \
    PIXMAN_COMPOSITE_ARGS (info);					      \
    pix_type       *dst_line;						      \
    pix_type       *src_line;                                                 \
    int             dst_stride, src_stride;                                   \
    int             src_x_t, src_y_t;                                         \
                                                                              \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, pix_type,              \
			   dst_stride, dst_line, 1);                          \
    src_x_t = -src_y + pixman_fixed_to_int (                                  \
				src_image->common.transform->matrix[0][2] +   \
				pixman_fixed_1 / 2 - pixman_fixed_e) - height;\
    src_y_t = src_x + pixman_fixed_to_int (                                   \
				src_image->common.transform->matrix[1][2] +   \
				pixman_fixed_1 / 2 - pixman_fixed_e);         \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x_t, src_y_t, pix_type,             \
			   src_stride, src_line, 1);                          \
    blt_rotated_90_##suffix (dst_line, dst_stride, src_line, src_stride,      \
			     width, height);                                  \
}                                                                             \
                                                                              \
static void                                                                   \
fast_composite_rotate_270_##suffix (pixman_implementation_t *imp,             \
				    pixman_composite_info_t *info)            \
{                                                                             \
    PIXMAN_COMPOSITE_ARGS (info);					      \
    pix_type       *dst_line;						      \
    pix_type       *src_line;                                                 \
    int             dst_stride, src_stride;                                   \
    int             src_x_t, src_y_t;                                         \
                                                                              \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, pix_type,              \
			   dst_stride, dst_line, 1);                          \
    src_x_t = src_y + pixman_fixed_to_int (                                   \
				src_image->common.transform->matrix[0][2] +   \
				pixman_fixed_1 / 2 - pixman_fixed_e);         \
    src_y_t = -src_x + pixman_fixed_to_int (                                  \
				src_image->common.transform->matrix[1][2] +   \
				pixman_fixed_1 / 2 - pixman_fixed_e) - width; \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x_t, src_y_t, pix_type,             \
			   src_stride, src_line, 1);                          \
    blt_rotated_270_##suffix (dst_line, dst_stride, src_line, src_stride,     \
			      width, height);                                 \
}

FAST_SIMPLE_ROTATE (8, uint8_t)
FAST_SIMPLE_ROTATE (565, uint16_t)
FAST_SIMPLE_ROTATE (8888, uint32_t)

static const pixman_fast_path_t c_fast_paths[] =
{
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, r5g6b5, fast_composite_over_n_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, b5g6r5, fast_composite_over_n_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, r8g8b8, fast_composite_over_n_8_0888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, b8g8r8, fast_composite_over_n_8_0888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, a8r8g8b8, fast_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, x8r8g8b8, fast_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, a8b8g8r8, fast_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, x8b8g8r8, fast_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a1, a8r8g8b8, fast_composite_over_n_1_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a1, x8r8g8b8, fast_composite_over_n_1_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a1, a8b8g8r8, fast_composite_over_n_1_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a1, x8b8g8r8, fast_composite_over_n_1_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a1, r5g6b5,   fast_composite_over_n_1_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid, a1, b5g6r5,   fast_composite_over_n_1_0565),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, a8r8g8b8, fast_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, x8r8g8b8, fast_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, r5g6b5, fast_composite_over_n_8888_0565_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, a8b8g8r8, fast_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, x8b8g8r8, fast_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, b5g6r5, fast_composite_over_n_8888_0565_ca),
    PIXMAN_STD_FAST_PATH (OVER, x8r8g8b8, a8, x8r8g8b8, fast_composite_over_x888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, x8r8g8b8, a8, a8r8g8b8, fast_composite_over_x888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, x8b8g8r8, a8, x8b8g8r8, fast_composite_over_x888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, x8b8g8r8, a8, a8b8g8r8, fast_composite_over_x888_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, a8r8g8b8, fast_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, x8r8g8b8, fast_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, r5g6b5, fast_composite_over_8888_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, a8b8g8r8, fast_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, x8b8g8r8, fast_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, b5g6r5, fast_composite_over_8888_0565),
    PIXMAN_STD_FAST_PATH (ADD, r5g6b5, null, r5g6b5, fast_composite_add_0565_0565),
    PIXMAN_STD_FAST_PATH (ADD, b5g6r5, null, b5g6r5, fast_composite_add_0565_0565),
    PIXMAN_STD_FAST_PATH (ADD, a8r8g8b8, null, a8r8g8b8, fast_composite_add_8888_8888),
    PIXMAN_STD_FAST_PATH (ADD, a8b8g8r8, null, a8b8g8r8, fast_composite_add_8888_8888),
    PIXMAN_STD_FAST_PATH (ADD, a8, null, a8, fast_composite_add_8_8),
    PIXMAN_STD_FAST_PATH (ADD, a1, null, a1, fast_composite_add_1_1),
    PIXMAN_STD_FAST_PATH_CA (ADD, solid, a8r8g8b8, a8r8g8b8, fast_composite_add_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH (ADD, solid, a8, a8, fast_composite_add_n_8_8),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, a8r8g8b8, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, x8r8g8b8, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, a8b8g8r8, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, x8b8g8r8, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, a1, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, a8, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, solid, null, r5g6b5, fast_composite_solid_fill),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, a8r8g8b8, fast_composite_src_x888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, a8b8g8r8, fast_composite_src_x888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, x8r8g8b8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, a8r8g8b8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, x8r8g8b8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, x8b8g8r8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, a8b8g8r8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, x8b8g8r8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, b8g8r8a8, null, b8g8r8x8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, b8g8r8a8, null, b8g8r8a8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, b8g8r8x8, null, b8g8r8x8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, r5g6b5, null, r5g6b5, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5, null, b5g6r5, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, r8g8b8, null, r8g8b8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, b8g8r8, null, b8g8r8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, x1r5g5b5, null, x1r5g5b5, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, a1r5g5b5, null, x1r5g5b5, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (SRC, a8, null, a8, fast_composite_src_memcpy),
    PIXMAN_STD_FAST_PATH (IN, a8, null, a8, fast_composite_in_8_8),
    PIXMAN_STD_FAST_PATH (IN, solid, a8, a8, fast_composite_in_n_8_8),

    SIMPLE_NEAREST_FAST_PATH (SRC, x8r8g8b8, x8r8g8b8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8r8g8b8, x8r8g8b8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, x8b8g8r8, x8b8g8r8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8b8g8r8, x8b8g8r8, 8888_8888),

    SIMPLE_NEAREST_FAST_PATH (SRC, a8r8g8b8, a8r8g8b8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8b8g8r8, a8b8g8r8, 8888_8888),

    SIMPLE_NEAREST_FAST_PATH (SRC, x8r8g8b8, r5g6b5, 8888_565),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8r8g8b8, r5g6b5, 8888_565),

    SIMPLE_NEAREST_FAST_PATH (SRC, r5g6b5, r5g6b5, 565_565),

    SIMPLE_NEAREST_FAST_PATH_COVER (SRC, x8r8g8b8, a8r8g8b8, x888_8888),
    SIMPLE_NEAREST_FAST_PATH_COVER (SRC, x8b8g8r8, a8b8g8r8, x888_8888),
    SIMPLE_NEAREST_FAST_PATH_PAD (SRC, x8r8g8b8, a8r8g8b8, x888_8888),
    SIMPLE_NEAREST_FAST_PATH_PAD (SRC, x8b8g8r8, a8b8g8r8, x888_8888),
    SIMPLE_NEAREST_FAST_PATH_NORMAL (SRC, x8r8g8b8, a8r8g8b8, x888_8888),
    SIMPLE_NEAREST_FAST_PATH_NORMAL (SRC, x8b8g8r8, a8b8g8r8, x888_8888),

    SIMPLE_NEAREST_FAST_PATH (OVER, a8r8g8b8, x8r8g8b8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (OVER, a8b8g8r8, x8b8g8r8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8, 8888_8888),
    SIMPLE_NEAREST_FAST_PATH (OVER, a8b8g8r8, a8b8g8r8, 8888_8888),

    SIMPLE_NEAREST_FAST_PATH (OVER, a8r8g8b8, r5g6b5, 8888_565),

#define NEAREST_FAST_PATH(op,s,d)		\
    {   PIXMAN_OP_ ## op,			\
	PIXMAN_ ## s, SCALED_NEAREST_FLAGS,	\
	PIXMAN_null, 0,				\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,	\
	fast_composite_scaled_nearest,		\
    }

    NEAREST_FAST_PATH (SRC, x8r8g8b8, x8r8g8b8),
    NEAREST_FAST_PATH (SRC, a8r8g8b8, x8r8g8b8),
    NEAREST_FAST_PATH (SRC, x8b8g8r8, x8b8g8r8),
    NEAREST_FAST_PATH (SRC, a8b8g8r8, x8b8g8r8),

    NEAREST_FAST_PATH (SRC, x8r8g8b8, a8r8g8b8),
    NEAREST_FAST_PATH (SRC, a8r8g8b8, a8r8g8b8),
    NEAREST_FAST_PATH (SRC, x8b8g8r8, a8b8g8r8),
    NEAREST_FAST_PATH (SRC, a8b8g8r8, a8b8g8r8),

    NEAREST_FAST_PATH (OVER, x8r8g8b8, x8r8g8b8),
    NEAREST_FAST_PATH (OVER, a8r8g8b8, x8r8g8b8),
    NEAREST_FAST_PATH (OVER, x8b8g8r8, x8b8g8r8),
    NEAREST_FAST_PATH (OVER, a8b8g8r8, x8b8g8r8),

    NEAREST_FAST_PATH (OVER, x8r8g8b8, a8r8g8b8),
    NEAREST_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8),
    NEAREST_FAST_PATH (OVER, x8b8g8r8, a8b8g8r8),
    NEAREST_FAST_PATH (OVER, a8b8g8r8, a8b8g8r8),

#define SIMPLE_ROTATE_FLAGS(angle)					  \
    (FAST_PATH_ROTATE_ ## angle ## _TRANSFORM	|			  \
     FAST_PATH_NEAREST_FILTER			|			  \
     FAST_PATH_SAMPLES_COVER_CLIP_NEAREST	|			  \
     FAST_PATH_STANDARD_FLAGS)

#define SIMPLE_ROTATE_FAST_PATH(op,s,d,suffix)				  \
    {   PIXMAN_OP_ ## op,						  \
	PIXMAN_ ## s, SIMPLE_ROTATE_FLAGS (90),				  \
	PIXMAN_null, 0,							  \
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				  \
	fast_composite_rotate_90_##suffix,				  \
    },									  \
    {   PIXMAN_OP_ ## op,						  \
	PIXMAN_ ## s, SIMPLE_ROTATE_FLAGS (270),			  \
	PIXMAN_null, 0,							  \
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				  \
	fast_composite_rotate_270_##suffix,				  \
    }

    SIMPLE_ROTATE_FAST_PATH (SRC, a8r8g8b8, a8r8g8b8, 8888),
    SIMPLE_ROTATE_FAST_PATH (SRC, a8r8g8b8, x8r8g8b8, 8888),
    SIMPLE_ROTATE_FAST_PATH (SRC, x8r8g8b8, x8r8g8b8, 8888),
    SIMPLE_ROTATE_FAST_PATH (SRC, r5g6b5, r5g6b5, 565),
    SIMPLE_ROTATE_FAST_PATH (SRC, a8, a8, 8),

    
    {	PIXMAN_OP_any,
	PIXMAN_any,
	(FAST_PATH_STANDARD_FLAGS | FAST_PATH_ID_TRANSFORM | FAST_PATH_BITS_IMAGE |
	 FAST_PATH_NORMAL_REPEAT),
	PIXMAN_any, 0,
	PIXMAN_any, FAST_PATH_STD_DEST_FLAGS,
	fast_composite_tiled_repeat
    },

    SIMPLE_BILINEAR_FAST_PATH (SRC, r5g6b5, r5g6b5, 565_565),
    SIMPLE_BILINEAR_FAST_PATH (OVER, a8r8g8b8, r5g6b5, 8888_565),
    SIMPLE_BILINEAR_FAST_PATH (OVER, a8r8g8b8, a8r8g8b8, 8888_8888),

    {   PIXMAN_OP_NONE	},
};

#ifdef WORDS_BIGENDIAN
#define A1_FILL_MASK(n, offs) (((1U << (n)) - 1) << (32 - (offs) - (n)))
#else
#define A1_FILL_MASK(n, offs) (((1U << (n)) - 1) << (offs))
#endif

static force_inline void
pixman_fill1_line (uint32_t *dst, int offs, int width, int v)
{
    if (offs)
    {
	int leading_pixels = 32 - offs;
	if (leading_pixels >= width)
	{
	    if (v)
		*dst |= A1_FILL_MASK (width, offs);
	    else
		*dst &= ~A1_FILL_MASK (width, offs);
	    return;
	}
	else
	{
	    if (v)
		*dst++ |= A1_FILL_MASK (leading_pixels, offs);
	    else
		*dst++ &= ~A1_FILL_MASK (leading_pixels, offs);
	    width -= leading_pixels;
	}
    }
    while (width >= 32)
    {
	if (v)
	    *dst++ = 0xFFFFFFFF;
	else
	    *dst++ = 0;
	width -= 32;
    }
    if (width > 0)
    {
	if (v)
	    *dst |= A1_FILL_MASK (width, 0);
	else
	    *dst &= ~A1_FILL_MASK (width, 0);
    }
}

static void
pixman_fill1 (uint32_t *bits,
              int       stride,
              int       x,
              int       y,
              int       width,
              int       height,
              uint32_t  filler)
{
    uint32_t *dst = bits + y * stride + (x >> 5);
    int offs = x & 31;

    if (filler & 1)
    {
	while (height--)
	{
	    pixman_fill1_line (dst, offs, width, 1);
	    dst += stride;
	}
    }
    else
    {
	while (height--)
	{
	    pixman_fill1_line (dst, offs, width, 0);
	    dst += stride;
	}
    }
}

static void
pixman_fill8 (uint32_t *bits,
              int       stride,
              int       x,
              int       y,
              int       width,
              int       height,
              uint32_t  filler)
{
    int byte_stride = stride * (int) sizeof (uint32_t);
    uint8_t *dst = (uint8_t *) bits;
    uint8_t v = filler & 0xff;
    int i;

    dst = dst + y * byte_stride + x;

    while (height--)
    {
	for (i = 0; i < width; ++i)
	    dst[i] = v;

	dst += byte_stride;
    }
}

static void
pixman_fill16 (uint32_t *bits,
               int       stride,
               int       x,
               int       y,
               int       width,
               int       height,
               uint32_t  filler)
{
    int short_stride =
	(stride * (int)sizeof (uint32_t)) / (int)sizeof (uint16_t);
    uint16_t *dst = (uint16_t *)bits;
    uint16_t v = filler & 0xffff;
    int i;

    dst = dst + y * short_stride + x;

    while (height--)
    {
	for (i = 0; i < width; ++i)
	    dst[i] = v;

	dst += short_stride;
    }
}

static void
pixman_fill32 (uint32_t *bits,
               int       stride,
               int       x,
               int       y,
               int       width,
               int       height,
               uint32_t  filler)
{
    int i;

    bits = bits + y * stride + x;

    while (height--)
    {
	for (i = 0; i < width; ++i)
	    bits[i] = filler;

	bits += stride;
    }
}

static pixman_bool_t
fast_path_fill (pixman_implementation_t *imp,
                uint32_t *               bits,
                int                      stride,
                int                      bpp,
                int                      x,
                int                      y,
                int                      width,
                int                      height,
                uint32_t		 filler)
{
    switch (bpp)
    {
    case 1:
	pixman_fill1 (bits, stride, x, y, width, height, filler);
	break;

    case 8:
	pixman_fill8 (bits, stride, x, y, width, height, filler);
	break;

    case 16:
	pixman_fill16 (bits, stride, x, y, width, height, filler);
	break;

    case 32:
	pixman_fill32 (bits, stride, x, y, width, height, filler);
	break;

    default:
	return FALSE;
    }

    return TRUE;
}



static uint32_t *
fast_fetch_r5g6b5 (pixman_iter_t *iter, const uint32_t *mask)
{
    int32_t w = iter->width;
    uint32_t *dst = iter->buffer;
    const uint16_t *src = (const uint16_t *)iter->bits;

    iter->bits += iter->stride;

    
    if (w > 0 && ((uintptr_t)src & 3))
    {
	*dst++ = convert_0565_to_8888 (*src++);
	w--;
    }
    
    while ((w -= 2) >= 0)
    {
	uint32_t sr, sb, sg, t0, t1;
	uint32_t s = *(const uint32_t *)src;
	src += 2;
	sr = (s >> 8) & 0x00F800F8;
	sb = (s << 3) & 0x00F800F8;
	sg = (s >> 3) & 0x00FC00FC;
	sr |= sr >> 5;
	sb |= sb >> 5;
	sg |= sg >> 6;
	t0 = ((sr << 16) & 0x00FF0000) | ((sg << 8) & 0x0000FF00) |
	     (sb & 0xFF) | 0xFF000000;
	t1 = (sr & 0x00FF0000) | ((sg >> 8) & 0x0000FF00) |
	     (sb >> 16) | 0xFF000000;
#ifdef WORDS_BIGENDIAN
	*dst++ = t1;
	*dst++ = t0;
#else
	*dst++ = t0;
	*dst++ = t1;
#endif
    }
    if (w & 1)
    {
	*dst = convert_0565_to_8888 (*src);
    }

    return iter->buffer;
}

static uint32_t *
fast_dest_fetch_noop (pixman_iter_t *iter, const uint32_t *mask)
{
    iter->bits += iter->stride;
    return iter->buffer;
}




static force_inline uint32_t
convert_8888_to_0565_workaround (uint32_t s, uint32_t x1F001F)
{
    uint32_t a, b;
    a = (s >> 3) & x1F001F;
    b = s & 0xFC00;
    a |= a >> 5;
    a |= b >> 5;
    return a;
}

static void
fast_write_back_r5g6b5 (pixman_iter_t *iter)
{
    int32_t w = iter->width;
    uint16_t *dst = (uint16_t *)(iter->bits - iter->stride);
    const uint32_t *src = iter->buffer;
    
    static volatile uint32_t volatile_x1F001F = 0x1F001F;
    uint32_t x1F001F = volatile_x1F001F;

    while ((w -= 4) >= 0)
    {
	uint32_t s1 = *src++;
	uint32_t s2 = *src++;
	uint32_t s3 = *src++;
	uint32_t s4 = *src++;
	*dst++ = convert_8888_to_0565_workaround (s1, x1F001F);
	*dst++ = convert_8888_to_0565_workaround (s2, x1F001F);
	*dst++ = convert_8888_to_0565_workaround (s3, x1F001F);
	*dst++ = convert_8888_to_0565_workaround (s4, x1F001F);
    }
    if (w & 2)
    {
	*dst++ = convert_8888_to_0565_workaround (*src++, x1F001F);
	*dst++ = convert_8888_to_0565_workaround (*src++, x1F001F);
    }
    if (w & 1)
    {
	*dst = convert_8888_to_0565_workaround (*src, x1F001F);
    }
}

typedef struct
{
    pixman_format_code_t	format;
    pixman_iter_get_scanline_t	get_scanline;
    pixman_iter_write_back_t	write_back;
} fetcher_info_t;

static const fetcher_info_t fetchers[] =
{
    { PIXMAN_r5g6b5, fast_fetch_r5g6b5, fast_write_back_r5g6b5 },
    { PIXMAN_null }
};

static pixman_bool_t
fast_src_iter_init (pixman_implementation_t *imp, pixman_iter_t *iter)
{
    pixman_image_t *image = iter->image;

#define FLAGS								\
    (FAST_PATH_STANDARD_FLAGS | FAST_PATH_ID_TRANSFORM |		\
     FAST_PATH_BITS_IMAGE | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST)

    if (iter->iter_flags & ITER_16)
	    return FALSE;

    if ((iter->iter_flags & ITER_NARROW)			&&
	(iter->image_flags & FLAGS) == FLAGS)
    {
	const fetcher_info_t *f;

	for (f = &fetchers[0]; f->format != PIXMAN_null; f++)
	{
	    if (image->common.extended_format_code == f->format)
	    {
		uint8_t *b = (uint8_t *)image->bits.bits;
		int s = image->bits.rowstride * 4;

		iter->bits = b + s * iter->y + iter->x * PIXMAN_FORMAT_BPP (f->format) / 8;
		iter->stride = s;

		iter->get_scanline = f->get_scanline;
		return TRUE;
	    }
	}
    }

    return FALSE;
}

static pixman_bool_t
fast_dest_iter_init (pixman_implementation_t *imp, pixman_iter_t *iter)
{
    pixman_image_t *image = iter->image;

    if (iter->iter_flags & ITER_16)
	    return FALSE;

    if ((iter->iter_flags & ITER_NARROW)		&&
	(iter->image_flags & FAST_PATH_STD_DEST_FLAGS) == FAST_PATH_STD_DEST_FLAGS)
    {
	const fetcher_info_t *f;

	for (f = &fetchers[0]; f->format != PIXMAN_null; f++)
	{
	    if (image->common.extended_format_code == f->format)
	    {
		uint8_t *b = (uint8_t *)image->bits.bits;
		int s = image->bits.rowstride * 4;

		iter->bits = b + s * iter->y + iter->x * PIXMAN_FORMAT_BPP (f->format) / 8;
		iter->stride = s;

		if ((iter->iter_flags & (ITER_IGNORE_RGB | ITER_IGNORE_ALPHA)) ==
		    (ITER_IGNORE_RGB | ITER_IGNORE_ALPHA))
		{
		    iter->get_scanline = fast_dest_fetch_noop;
		}
		else
		{
		    iter->get_scanline = f->get_scanline;
		}
		iter->write_back = f->write_back;
		return TRUE;
	    }
	}
    }
    return FALSE;
}


pixman_implementation_t *
_pixman_implementation_create_fast_path (pixman_implementation_t *fallback)
{
    pixman_implementation_t *imp = _pixman_implementation_create (fallback, c_fast_paths);

    imp->fill = fast_path_fill;
    imp->src_iter_init = fast_src_iter_init;
    imp->dest_iter_init = fast_dest_iter_init;

    return imp;
}
