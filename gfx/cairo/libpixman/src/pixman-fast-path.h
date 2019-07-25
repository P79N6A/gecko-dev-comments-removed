
























#ifndef PIXMAN_FAST_PATH_H__
#define PIXMAN_FAST_PATH_H__

#include "pixman-private.h"

#define PIXMAN_REPEAT_COVER -1

static force_inline pixman_bool_t
repeat (pixman_repeat_t repeat, int *c, int size)
{
    if (repeat == PIXMAN_REPEAT_NONE)
    {
	if (*c < 0 || *c >= size)
	    return FALSE;
    }
    else if (repeat == PIXMAN_REPEAT_NORMAL)
    {
	while (*c >= size)
	    *c -= size;
	while (*c < 0)
	    *c += size;
    }
    else if (repeat == PIXMAN_REPEAT_PAD)
    {
	*c = CLIP (*c, 0, size - 1);
    }
    else 
    {
	*c = MOD (*c, size * 2);
	if (*c >= size)
	    *c = size * 2 - *c - 1;
    }
    return TRUE;
}













static force_inline void
pad_repeat_get_scanline_bounds (int32_t         source_image_width,
				pixman_fixed_t  vx,
				pixman_fixed_t  unit_x,
				int32_t *       width,
				int32_t *       left_pad,
				int32_t *       right_pad)
{
    int64_t max_vx = (int64_t) source_image_width << 16;
    int64_t tmp;
    if (vx < 0)
    {
	tmp = ((int64_t) unit_x - 1 - vx) / unit_x;
	if (tmp > *width)
	{
	    *left_pad = *width;
	    *width = 0;
	}
	else
	{
	    *left_pad = (int32_t) tmp;
	    *width -= (int32_t) tmp;
	}
    }
    else
    {
	*left_pad = 0;
    }
    tmp = ((int64_t) unit_x - 1 - vx + max_vx) / unit_x - *left_pad;
    if (tmp < 0)
    {
	*right_pad = *width;
	*width = 0;
    }
    else if (tmp >= *width)
    {
	*right_pad = 0;
    }
    else
    {
	*right_pad = *width - (int32_t) tmp;
	*width = (int32_t) tmp;
    }
}
















#define GET_8888_ALPHA(s) ((s) >> 24)
 

#define GET_0565_ALPHA(s) 0xff

#define FAST_NEAREST_SCANLINE(scanline_func_name, SRC_FORMAT, DST_FORMAT,			\
			      src_type_t, dst_type_t, OP, repeat_mode)				\
static force_inline void									\
scanline_func_name (dst_type_t     *dst,							\
		    src_type_t     *src,							\
		    int32_t         w,								\
		    pixman_fixed_t  vx,								\
		    pixman_fixed_t  unit_x,							\
		    pixman_fixed_t  max_vx)							\
{												\
	uint32_t   d;										\
	src_type_t s1, s2;									\
	uint8_t    a1, a2;									\
	int        x1, x2;									\
												\
	if (PIXMAN_OP_ ## OP != PIXMAN_OP_SRC && PIXMAN_OP_ ## OP != PIXMAN_OP_OVER)		\
	    abort();										\
												\
	while ((w -= 2) >= 0)									\
	{											\
	    x1 = vx >> 16;									\
	    vx += unit_x;									\
	    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	    {											\
		/* This works because we know that unit_x is positive */			\
		while (vx >= max_vx)								\
		    vx -= max_vx;								\
	    }											\
	    s1 = src[x1];									\
												\
	    x2 = vx >> 16;									\
	    vx += unit_x;									\
	    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	    {											\
		/* This works because we know that unit_x is positive */			\
		while (vx >= max_vx)								\
		    vx -= max_vx;								\
	    }											\
	    s2 = src[x2];									\
												\
	    if (PIXMAN_OP_ ## OP == PIXMAN_OP_OVER)						\
	    {											\
		a1 = GET_ ## SRC_FORMAT ## _ALPHA(s1);						\
		a2 = GET_ ## SRC_FORMAT ## _ALPHA(s2);						\
												\
		if (a1 == 0xff)									\
		{										\
		    *dst = CONVERT_ ## SRC_FORMAT ## _TO_ ## DST_FORMAT (s1);			\
		}										\
		else if (s1)									\
		{										\
		    d = CONVERT_ ## DST_FORMAT ## _TO_8888 (*dst);				\
		    s1 = CONVERT_ ## SRC_FORMAT ## _TO_8888 (s1);				\
		    a1 ^= 0xff;									\
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, a1, s1);					\
		    *dst = CONVERT_8888_TO_ ## DST_FORMAT (d);					\
		}										\
		dst++;										\
												\
		if (a2 == 0xff)									\
		{										\
		    *dst = CONVERT_ ## SRC_FORMAT ## _TO_ ## DST_FORMAT (s2);			\
		}										\
		else if (s2)									\
		{										\
		    d = CONVERT_## DST_FORMAT ## _TO_8888 (*dst);				\
		    s2 = CONVERT_## SRC_FORMAT ## _TO_8888 (s2);				\
		    a2 ^= 0xff;									\
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, a2, s2);					\
		    *dst = CONVERT_8888_TO_ ## DST_FORMAT (d);					\
		}										\
		dst++;										\
	    }											\
	    else /* PIXMAN_OP_SRC */								\
	    {											\
		*dst++ = CONVERT_ ## SRC_FORMAT ## _TO_ ## DST_FORMAT (s1);			\
		*dst++ = CONVERT_ ## SRC_FORMAT ## _TO_ ## DST_FORMAT (s2);			\
	    }											\
	}											\
												\
	if (w & 1)										\
	{											\
	    x1 = vx >> 16;									\
	    s1 = src[x1];									\
												\
	    if (PIXMAN_OP_ ## OP == PIXMAN_OP_OVER)						\
	    {											\
		a1 = GET_ ## SRC_FORMAT ## _ALPHA(s1);						\
												\
		if (a1 == 0xff)									\
		{										\
		    *dst = CONVERT_ ## SRC_FORMAT ## _TO_ ## DST_FORMAT (s1);			\
		}										\
		else if (s1)									\
		{										\
		    d = CONVERT_## DST_FORMAT ## _TO_8888 (*dst);				\
		    s1 = CONVERT_ ## SRC_FORMAT ## _TO_8888 (s1);				\
		    a1 ^= 0xff;									\
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, a1, s1);					\
		    *dst = CONVERT_8888_TO_ ## DST_FORMAT (d);					\
		}										\
		dst++;										\
	    }											\
	    else /* PIXMAN_OP_SRC */								\
	    {											\
		*dst++ = CONVERT_ ## SRC_FORMAT ## _TO_ ## DST_FORMAT (s1);			\
	    }											\
	}											\
}

#define FAST_NEAREST_MAINLOOP(scale_func_name, scanline_func, src_type_t, dst_type_t,		\
			      repeat_mode)							\
static void											\
fast_composite_scaled_nearest_ ## scale_func_name (pixman_implementation_t *imp,		\
						   pixman_op_t              op,			\
						   pixman_image_t *         src_image,		\
						   pixman_image_t *         mask_image,		\
						   pixman_image_t *         dst_image,		\
						   int32_t                  src_x,		\
						   int32_t                  src_y,		\
						   int32_t                  mask_x,		\
						   int32_t                  mask_y,		\
						   int32_t                  dst_x,		\
						   int32_t                  dst_y,		\
						   int32_t                  width,		\
						   int32_t                  height)		\
{												\
    dst_type_t *dst_line;									\
    src_type_t *src_first_line;									\
    int       y;										\
    pixman_fixed_t max_vx = max_vx; /* suppress uninitialized variable warning */		\
    pixman_fixed_t max_vy;									\
    pixman_vector_t v;										\
    pixman_fixed_t vx, vy;									\
    pixman_fixed_t unit_x, unit_y;								\
    int32_t left_pad, right_pad;								\
												\
    src_type_t *src;										\
    dst_type_t *dst;										\
    int       src_stride, dst_stride;								\
												\
    PIXMAN_IMAGE_GET_LINE (dst_image, dst_x, dst_y, dst_type_t, dst_stride, dst_line, 1);	\
    /* pass in 0 instead of src_x and src_y because src_x and src_y need to be			\
     * transformed from destination space to source space */					\
    PIXMAN_IMAGE_GET_LINE (src_image, 0, 0, src_type_t, src_stride, src_first_line, 1);		\
												\
    /* reference point is the center of the pixel */						\
    v.vector[0] = pixman_int_to_fixed (src_x) + pixman_fixed_1 / 2;				\
    v.vector[1] = pixman_int_to_fixed (src_y) + pixman_fixed_1 / 2;				\
    v.vector[2] = pixman_fixed_1;								\
												\
    if (!pixman_transform_point_3d (src_image->common.transform, &v))				\
	return;											\
												\
    unit_x = src_image->common.transform->matrix[0][0];						\
    unit_y = src_image->common.transform->matrix[1][1];						\
												\
    /* Round down to closest integer, ensuring that 0.5 rounds to 0, not 1 */			\
    v.vector[0] -= pixman_fixed_e;								\
    v.vector[1] -= pixman_fixed_e;								\
												\
    vx = v.vector[0];										\
    vy = v.vector[1];										\
												\
    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)					\
    {												\
	/* Clamp repeating positions inside the actual samples */				\
	max_vx = src_image->bits.width << 16;							\
	max_vy = src_image->bits.height << 16;							\
												\
	repeat (PIXMAN_REPEAT_NORMAL, &vx, max_vx);						\
	repeat (PIXMAN_REPEAT_NORMAL, &vy, max_vy);						\
    }												\
												\
    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD ||					\
	PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NONE)					\
    {												\
	pad_repeat_get_scanline_bounds (src_image->bits.width, vx, unit_x,			\
					&width, &left_pad, &right_pad);				\
	vx += left_pad * unit_x;								\
    }												\
												\
    while (--height >= 0)									\
    {												\
	dst = dst_line;										\
	dst_line += dst_stride;									\
												\
	y = vy >> 16;										\
	vy += unit_y;										\
	if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	    repeat (PIXMAN_REPEAT_NORMAL, &vy, max_vy);						\
	if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD)					\
	{											\
	    repeat (PIXMAN_REPEAT_PAD, &y, src_image->bits.height);				\
	    src = src_first_line + src_stride * y;						\
	    if (left_pad > 0)									\
	    {											\
		scanline_func (dst, src, left_pad, 0, 0, 0);					\
	    }											\
	    if (width > 0)									\
	    {											\
		scanline_func (dst + left_pad, src, width, vx, unit_x, 0);			\
	    }											\
	    if (right_pad > 0)									\
	    {											\
		scanline_func (dst + left_pad + width, src + src_image->bits.width - 1,		\
			        right_pad, 0, 0, 0);						\
	    }											\
	}											\
	else if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NONE)				\
	{											\
	    static src_type_t zero = 0;								\
	    if (y < 0 || y >= src_image->bits.height)						\
	    {											\
		scanline_func (dst, &zero, left_pad + width + right_pad, 0, 0, 0);		\
		continue;									\
	    }											\
	    src = src_first_line + src_stride * y;						\
	    if (left_pad > 0)									\
	    {											\
		scanline_func (dst, &zero, left_pad, 0, 0, 0);					\
	    }											\
	    if (width > 0)									\
	    {											\
		scanline_func (dst + left_pad, src, width, vx, unit_x, 0);			\
	    }											\
	    if (right_pad > 0)									\
	    {											\
		scanline_func (dst + left_pad + width, &zero, right_pad, 0, 0, 0);		\
	    }											\
	}											\
	else											\
	{											\
	    src = src_first_line + src_stride * y;						\
	    scanline_func (dst, src, width, vx, unit_x, max_vx);				\
	}											\
    }												\
}

#define FAST_NEAREST(scale_func_name, SRC_FORMAT, DST_FORMAT,				\
		     src_type_t, dst_type_t, OP, repeat_mode)				\
    FAST_NEAREST_SCANLINE(scaled_nearest_scanline_ ## scale_func_name ## _ ## OP,	\
			  SRC_FORMAT, DST_FORMAT, src_type_t, dst_type_t,		\
			  OP, repeat_mode)						\
    FAST_NEAREST_MAINLOOP(scale_func_name##_##OP,					\
			  scaled_nearest_scanline_ ## scale_func_name ## _ ## OP,	\
			  src_type_t, dst_type_t, repeat_mode)				\
											\
    extern int no_such_variable


#define SCALED_NEAREST_FLAGS						\
    (FAST_PATH_SCALE_TRANSFORM	|					\
     FAST_PATH_NO_ALPHA_MAP	|					\
     FAST_PATH_NEAREST_FILTER	|					\
     FAST_PATH_NO_ACCESSORS	|					\
     FAST_PATH_NARROW_FORMAT)

#define SIMPLE_NEAREST_FAST_PATH_NORMAL(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _normal ## _ ## op,	\
    }

#define SIMPLE_NEAREST_FAST_PATH_PAD(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_NEAREST_FAST_PATH_NONE(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_NEAREST_FAST_PATH_COVER(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_NEAREST_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP,		\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _cover ## _ ## op,	\
    }


#define SIMPLE_NEAREST_FAST_PATH(op,s,d,func)				\
    SIMPLE_NEAREST_FAST_PATH_COVER (op,s,d,func),			\
    SIMPLE_NEAREST_FAST_PATH_NONE (op,s,d,func),			\
    SIMPLE_NEAREST_FAST_PATH_PAD (op,s,d,func),				\
    SIMPLE_NEAREST_FAST_PATH_NORMAL (op,s,d,func)

#endif
