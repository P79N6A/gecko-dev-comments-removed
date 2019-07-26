


























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>
#include <float.h>

#include "pixman-private.h"



#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 6)
#undef force_inline
#define force_inline __inline__
#endif

typedef float (* combine_channel_t) (float sa, float s, float da, float d);

static force_inline void
combine_inner (pixman_bool_t component,
	       float *dest, const float *src, const float *mask, int n_pixels,
	       combine_channel_t combine_a, combine_channel_t combine_c)
{
    int i;

    if (!mask)
    {
	for (i = 0; i < 4 * n_pixels; i += 4)
	{
	    float sa = src[i + 0];
	    float sr = src[i + 1];
	    float sg = src[i + 2];
	    float sb = src[i + 3];
	    
	    float da = dest[i + 0];
	    float dr = dest[i + 1];
	    float dg = dest[i + 2];
	    float db = dest[i + 3];					
	    
	    dest[i + 0] = combine_a (sa, sa, da, da);
	    dest[i + 1] = combine_c (sa, sr, da, dr);
	    dest[i + 2] = combine_c (sa, sg, da, dg);
	    dest[i + 3] = combine_c (sa, sb, da, db);
	}
    }
    else
    {
	for (i = 0; i < 4 * n_pixels; i += 4)
	{
	    float sa, sr, sg, sb;
	    float ma, mr, mg, mb;
	    float da, dr, dg, db;
	    
	    sa = src[i + 0];
	    sr = src[i + 1];
	    sg = src[i + 2];
	    sb = src[i + 3];
	    
	    if (component)
	    {
		ma = mask[i + 0];
		mr = mask[i + 1];
		mg = mask[i + 2];
		mb = mask[i + 3];

		sr *= mr;
		sg *= mg;
		sb *= mb;

		ma *= sa;
		mr *= sa;
		mg *= sa;
		mb *= sa;
		
		sa = ma;
	    }
	    else
	    {
		ma = mask[i + 0];

		sa *= ma;
		sr *= ma;
		sg *= ma;
		sb *= ma;

		ma = mr = mg = mb = sa;
	    }
	    
	    da = dest[i + 0];
	    dr = dest[i + 1];
	    dg = dest[i + 2];
	    db = dest[i + 3];
	    
	    dest[i + 0] = combine_a (ma, sa, da, da);
	    dest[i + 1] = combine_c (mr, sr, da, dr);
	    dest[i + 2] = combine_c (mg, sg, da, dg);
	    dest[i + 3] = combine_c (mb, sb, da, db);
	}
    }
}

#define MAKE_COMBINER(name, component, combine_a, combine_c)		\
    static void								\
    combine_ ## name ## _float (pixman_implementation_t *imp,		\
				pixman_op_t              op,		\
				float                   *dest,		\
				const float             *src,		\
				const float             *mask,		\
				int		         n_pixels)	\
    {									\
	combine_inner (component, dest, src, mask, n_pixels,		\
		       combine_a, combine_c);				\
    }

#define MAKE_COMBINERS(name, combine_a, combine_c)			\
    MAKE_COMBINER(name ## _ca, TRUE, combine_a, combine_c)		\
    MAKE_COMBINER(name ## _u, FALSE, combine_a, combine_c)





typedef enum
{
    ZERO,
    ONE,
    SRC_ALPHA,
    DEST_ALPHA,
    INV_SA,
    INV_DA,
    SA_OVER_DA,
    DA_OVER_SA,
    INV_SA_OVER_DA,
    INV_DA_OVER_SA,
    ONE_MINUS_SA_OVER_DA,
    ONE_MINUS_DA_OVER_SA,
    ONE_MINUS_INV_DA_OVER_SA,
    ONE_MINUS_INV_SA_OVER_DA
} combine_factor_t;

#define CLAMP(f)					\
    (((f) < 0)? 0 : (((f) > 1.0) ? 1.0 : (f)))

static force_inline float
get_factor (combine_factor_t factor, float sa, float da)
{
    float f = -1;

    switch (factor)
    {
    case ZERO:
	f = 0.0f;
	break;

    case ONE:
	f = 1.0f;
	break;

    case SRC_ALPHA:
	f = sa;
	break;

    case DEST_ALPHA:
	f = da;
	break;

    case INV_SA:
	f = 1 - sa;
	break;

    case INV_DA:
	f = 1 - da;
	break;

    case SA_OVER_DA:
	if (da == 0.0f)
	    f = 1.0f;
	else
	    f = CLAMP (sa / da);
	break;

    case DA_OVER_SA:
	if (sa == 0.0f)
	    f = 1.0f;
	else
	    f = CLAMP (da / sa);
	break;

    case INV_SA_OVER_DA:
	if (da == 0.0f)
	    f = 1.0f;
	else
	    f = CLAMP ((1.0f - sa) / da);
	break;

    case INV_DA_OVER_SA:
	if (sa == 0.0f)
	    f = 1.0f;
	else
	    f = CLAMP ((1.0f - da) / sa);
	break;

    case ONE_MINUS_SA_OVER_DA:
	if (da == 0.0f)
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - sa / da);
	break;

    case ONE_MINUS_DA_OVER_SA:
	if (sa == 0.0f)
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - da / sa);
	break;

    case ONE_MINUS_INV_DA_OVER_SA:
	if (sa == 0.0f)
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - (1.0f - da) / sa);
	break;

    case ONE_MINUS_INV_SA_OVER_DA:
	if (da == 0.0f)
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - (1.0f - sa) / da);
	break;
    }

    return f;
}

#define MAKE_PD_COMBINERS(name, a, b)					\
    static float force_inline						\
    pd_combine_ ## name (float sa, float s, float da, float d)		\
    {									\
	const float fa = get_factor (a, sa, da);			\
	const float fb = get_factor (b, sa, da);			\
									\
	return MIN (1.0f, s * fa + d * fb);				\
    }									\
    									\
    MAKE_COMBINERS(name, pd_combine_ ## name, pd_combine_ ## name)

MAKE_PD_COMBINERS (clear,			ZERO,				ZERO)
MAKE_PD_COMBINERS (src,				ONE,				ZERO)
MAKE_PD_COMBINERS (dst,				ZERO,				ONE)
MAKE_PD_COMBINERS (over,			ONE,				INV_SA)
MAKE_PD_COMBINERS (over_reverse,		INV_DA,				ONE)
MAKE_PD_COMBINERS (in,				DEST_ALPHA,			ZERO)
MAKE_PD_COMBINERS (in_reverse,			ZERO,				SRC_ALPHA)
MAKE_PD_COMBINERS (out,				INV_DA,				ZERO)
MAKE_PD_COMBINERS (out_reverse,			ZERO,				INV_SA)
MAKE_PD_COMBINERS (atop,			DEST_ALPHA,			INV_SA)
MAKE_PD_COMBINERS (atop_reverse,		INV_DA,				SRC_ALPHA)
MAKE_PD_COMBINERS (xor,				INV_DA,				INV_SA)
MAKE_PD_COMBINERS (add,				ONE,				ONE)

MAKE_PD_COMBINERS (saturate,			INV_DA_OVER_SA,			ONE)

MAKE_PD_COMBINERS (disjoint_clear,		ZERO,				ZERO)
MAKE_PD_COMBINERS (disjoint_src,		ONE,				ZERO)
MAKE_PD_COMBINERS (disjoint_dst,		ZERO,				ONE)
MAKE_PD_COMBINERS (disjoint_over,		ONE,				INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_over_reverse,	INV_DA_OVER_SA,			ONE)
MAKE_PD_COMBINERS (disjoint_in,			ONE_MINUS_INV_DA_OVER_SA,	ZERO)
MAKE_PD_COMBINERS (disjoint_in_reverse,		ZERO,				ONE_MINUS_INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_out,		INV_DA_OVER_SA,			ZERO)
MAKE_PD_COMBINERS (disjoint_out_reverse,	ZERO,				INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_atop,		ONE_MINUS_INV_DA_OVER_SA,	INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_atop_reverse,	INV_DA_OVER_SA,			ONE_MINUS_INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_xor,		INV_DA_OVER_SA,			INV_SA_OVER_DA)

MAKE_PD_COMBINERS (conjoint_clear,		ZERO,				ZERO)
MAKE_PD_COMBINERS (conjoint_src,		ONE,				ZERO)
MAKE_PD_COMBINERS (conjoint_dst,		ZERO,				ONE)
MAKE_PD_COMBINERS (conjoint_over,		ONE,				ONE_MINUS_SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_over_reverse,	ONE_MINUS_DA_OVER_SA,		ONE)
MAKE_PD_COMBINERS (conjoint_in,			DA_OVER_SA,			ZERO)
MAKE_PD_COMBINERS (conjoint_in_reverse,		ZERO,				SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_out,		ONE_MINUS_DA_OVER_SA,		ZERO)
MAKE_PD_COMBINERS (conjoint_out_reverse,	ZERO,				ONE_MINUS_SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_atop,		DA_OVER_SA,			ONE_MINUS_SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_atop_reverse,	ONE_MINUS_DA_OVER_SA,		SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_xor,		ONE_MINUS_DA_OVER_SA,		ONE_MINUS_SA_OVER_DA)

























#define MAKE_SEPARABLE_PDF_COMBINERS(name)				\
    static force_inline float						\
    combine_ ## name ## _a (float sa, float s, float da, float d)	\
    {									\
	return da + sa - da * sa;					\
    }									\
    									\
    static force_inline float						\
    combine_ ## name ## _c (float sa, float s, float da, float d)	\
    {									\
	float f = (1 - sa) * d + (1 - da) * s;				\
									\
	return f + blend_ ## name (sa, s, da, d);			\
    }									\
    									\
    MAKE_COMBINERS (name, combine_ ## name ## _a, combine_ ## name ## _c)

static force_inline float
blend_multiply (float sa, float s, float da, float d)
{
    return d * s;
}

static force_inline float
blend_screen (float sa, float s, float da, float d)
{
    return d * sa + s * da - s * d;
}

static force_inline float
blend_overlay (float sa, float s, float da, float d)
{
    if (2 * d < da)
	return 2 * s * d;
    else
	return sa * da - 2 * (da - d) * (sa - s);
}

static force_inline float
blend_darken (float sa, float s, float da, float d)
{
    s = s * da;
    d = d * sa;

    if (s > d)
	return d;
    else
	return s;
}

static force_inline float
blend_lighten (float sa, float s, float da, float d)
{
    s = s * da;
    d = d * sa;

    if (s > d)
	return s;
    else
	return d;
}

static force_inline float
blend_color_dodge (float sa, float s, float da, float d)
{
    if (d == 0.0f)
	return 0.0f;
    else if (d * sa >= sa * da - s * da)
	return sa * da;
    else if (sa - s == 0.0f)
	return sa * da;
    else
	return sa * sa * d / (sa - s);
}

static force_inline float
blend_color_burn (float sa, float s, float da, float d)
{
    if (d >= da)
	return sa * da;
    else if (sa * (da - d) >= s * da)
	return 0.0f;
    else if (s == 0.0f)
	return 0.0f;
    else
	return sa * (da - sa * (da - d) / s);
}

static force_inline float
blend_hard_light (float sa, float s, float da, float d)
{
    if (2 * s < sa)
	return 2 * s * d;
    else
	return sa * da - 2 * (da - d) * (sa - s);
}

static force_inline float
blend_soft_light (float sa, float s, float da, float d)
{
    if (2 * s < sa)
    {
	if (da == 0.0f)
	    return d * sa;
	else
	    return d * sa - d * (da - d) * (sa - 2 * s) / da;
    }
    else
    {
	if (da == 0.0f)
	{
	    return 0.0f;
	}
	else
	{
	    if (4 * d <= da)
		return d * sa + (2 * s - sa) * d * ((16 * d / da - 12) * d / da + 3);
	    else
		return d * sa + (sqrtf (d * da) - d) * (2 * s - sa);
	}
    }
}

static force_inline float
blend_difference (float sa, float s, float da, float d)
{
    float dsa = d * sa;
    float sda = s * da;

    if (sda < dsa)
	return dsa - sda;
    else
	return sda - dsa;
}

static force_inline float
blend_exclusion (float sa, float s, float da, float d)
{
    return s * da + d * sa - 2 * d * s;
}

MAKE_SEPARABLE_PDF_COMBINERS (multiply)
MAKE_SEPARABLE_PDF_COMBINERS (screen)
MAKE_SEPARABLE_PDF_COMBINERS (overlay)
MAKE_SEPARABLE_PDF_COMBINERS (darken)
MAKE_SEPARABLE_PDF_COMBINERS (lighten)
MAKE_SEPARABLE_PDF_COMBINERS (color_dodge)
MAKE_SEPARABLE_PDF_COMBINERS (color_burn)
MAKE_SEPARABLE_PDF_COMBINERS (hard_light)
MAKE_SEPARABLE_PDF_COMBINERS (soft_light)
MAKE_SEPARABLE_PDF_COMBINERS (difference)
MAKE_SEPARABLE_PDF_COMBINERS (exclusion)
















































































































typedef struct
{
    float	r;
    float	g;
    float	b;
} rgb_t;

static force_inline float
minf (float a, float b)
{
    return a < b? a : b;
}

static force_inline float
maxf (float a, float b)
{
    return a > b? a : b;
}

static force_inline float
channel_min (const rgb_t *c)
{
    return minf (minf (c->r, c->g), c->b);
}

static force_inline float
channel_max (const rgb_t *c)
{
    return maxf (maxf (c->r, c->g), c->b);
}

static force_inline float
get_lum (const rgb_t *c)
{
    return c->r * 0.3f + c->g * 0.59f + c->b * 0.11f;
}

static force_inline float
get_sat (const rgb_t *c)
{
    return channel_max (c) - channel_min (c);
}

static void
clip_color (rgb_t *color, float a)
{
    float l = get_lum (color);
    float n = channel_min (color);
    float x = channel_max (color);

    if (n < 0.0f)
    {
	if ((l - n) < 4 * FLT_EPSILON)
	{
	    color->r = 0.0f;
	    color->g = 0.0f;
	    color->b = 0.0f;
	}
	else
	{
	    color->r = l + (((color->r - l) * l) / (l - n));
	    color->g = l + (((color->g - l) * l) / (l - n));
	    color->b = l + (((color->b - l) * l) / (l - n));
	}
    }
    if (x > a)
    {
	if ((x - l) < 4 * FLT_EPSILON)
	{
	    color->r = a;
	    color->g = a;
	    color->b = a;
	}
	else
	{
	    color->r = l + (((color->r - l) * (a - l) / (x - l)));
	    color->g = l + (((color->g - l) * (a - l) / (x - l)));
	    color->b = l + (((color->b - l) * (a - l) / (x - l)));
	}
    }
}

static void
set_lum (rgb_t *color, float sa, float l)
{
    float d = l - get_lum (color);

    color->r = color->r + d;
    color->g = color->g + d;
    color->b = color->b + d;

    clip_color (color, sa);
}

static void
set_sat (rgb_t *src, float sat)
{
    float *max, *mid, *min;

    if (src->r > src->g)
    {
	if (src->r > src->b)
	{
	    max = &(src->r);

	    if (src->g > src->b)
	    {
		mid = &(src->g);
		min = &(src->b);
	    }
	    else
	    {
		mid = &(src->b);
		min = &(src->g);
	    }
	}
	else
	{
	    max = &(src->b);
	    mid = &(src->r);
	    min = &(src->g);
	}
    }
    else
    {
	if (src->r > src->b)
	{
	    max = &(src->g);
	    mid = &(src->r);
	    min = &(src->b);
	}
	else
	{
	    min = &(src->r);

	    if (src->g > src->b)
	    {
		max = &(src->g);
		mid = &(src->b);
	    }
	    else
	    {
		max = &(src->b);
		mid = &(src->g);
	    }
	}
    }

    if (*max > *min)
    {
	*mid = (((*mid - *min) * sat) / (*max - *min));
	*max = sat;
    }
    else
    {
	*mid = *max = 0.0f;
    }

    *min = 0.0f;
}





static force_inline void
blend_hsl_hue (rgb_t *res,
	       const rgb_t *dest, float da,
	       const rgb_t *src, float sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_sat (res, get_sat (dest) * sa);
    set_lum (res, sa * da, get_lum (dest) * sa);
}





static force_inline void
blend_hsl_saturation (rgb_t *res,
		      const rgb_t *dest, float da,
		      const rgb_t *src, float sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_sat (res, get_sat (src) * da);
    set_lum (res, sa * da, get_lum (dest) * sa);
}





static force_inline void
blend_hsl_color (rgb_t *res,
		 const rgb_t *dest, float da,
		 const rgb_t *src, float sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_lum (res, sa * da, get_lum (dest) * sa);
}





static force_inline void
blend_hsl_luminosity (rgb_t *res,
		      const rgb_t *dest, float da,
		      const rgb_t *src, float sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_lum (res, sa * da, get_lum (src) * da);
}

#define MAKE_NON_SEPARABLE_PDF_COMBINERS(name)				\
    static void								\
    combine_ ## name ## _u_float (pixman_implementation_t *imp,		\
				  pixman_op_t              op,		\
				  float                   *dest,	\
				  const float             *src,		\
				  const float             *mask,	\
				  int		           n_pixels)	\
    {									\
    	int i;								\
									\
	for (i = 0; i < 4 * n_pixels; i += 4)				\
	{								\
	    float sa, da;						\
	    rgb_t sc, dc, rc;						\
									\
	    sa = src[i + 0];						\
	    sc.r = src[i + 1];						\
	    sc.g = src[i + 2];						\
	    sc.b = src[i + 3];						\
									\
	    da = dest[i + 0];						\
	    dc.r = dest[i + 1];						\
	    dc.g = dest[i + 2];						\
	    dc.b = dest[i + 3];						\
									\
	    if (mask)							\
	    {								\
		float ma = mask[i + 0];					\
									\
		/* Component alpha is not supported for HSL modes */	\
		sa *= ma;						\
		sc.r *= ma;						\
		sc.g *= ma;						\
		sc.g *= ma;						\
	    }								\
									\
	    blend_ ## name (&rc, &dc, da, &sc, sa);			\
									\
	    dest[i + 0] = sa + da - sa * da;				\
	    dest[i + 1] = (1 - sa) * dc.r + (1 - da) * sc.r + rc.r;	\
	    dest[i + 2] = (1 - sa) * dc.g + (1 - da) * sc.g + rc.g;	\
	    dest[i + 3] = (1 - sa) * dc.b + (1 - da) * sc.b + rc.b;	\
	}								\
    }

MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_hue)
MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_saturation)
MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_color)
MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_luminosity)

void
_pixman_setup_combiner_functions_float (pixman_implementation_t *imp)
{
    
    imp->combine_float[PIXMAN_OP_CLEAR] = combine_clear_u_float;
    imp->combine_float[PIXMAN_OP_SRC] = combine_src_u_float;
    imp->combine_float[PIXMAN_OP_DST] = combine_dst_u_float;
    imp->combine_float[PIXMAN_OP_OVER] = combine_over_u_float;
    imp->combine_float[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_u_float;
    imp->combine_float[PIXMAN_OP_IN] = combine_in_u_float;
    imp->combine_float[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_u_float;
    imp->combine_float[PIXMAN_OP_OUT] = combine_out_u_float;
    imp->combine_float[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_u_float;
    imp->combine_float[PIXMAN_OP_ATOP] = combine_atop_u_float;
    imp->combine_float[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_u_float;
    imp->combine_float[PIXMAN_OP_XOR] = combine_xor_u_float;
    imp->combine_float[PIXMAN_OP_ADD] = combine_add_u_float;
    imp->combine_float[PIXMAN_OP_SATURATE] = combine_saturate_u_float;

    
    imp->combine_float[PIXMAN_OP_DISJOINT_CLEAR] = combine_disjoint_clear_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_SRC] = combine_disjoint_src_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_DST] = combine_disjoint_dst_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OVER] = combine_disjoint_over_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OVER_REVERSE] = combine_disjoint_over_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_IN] = combine_disjoint_in_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_IN_REVERSE] = combine_disjoint_in_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OUT] = combine_disjoint_out_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OUT_REVERSE] = combine_disjoint_out_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_ATOP] = combine_disjoint_atop_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_ATOP_REVERSE] = combine_disjoint_atop_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_XOR] = combine_disjoint_xor_u_float;

    
    imp->combine_float[PIXMAN_OP_CONJOINT_CLEAR] = combine_conjoint_clear_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_SRC] = combine_conjoint_src_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_DST] = combine_conjoint_dst_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OVER] = combine_conjoint_over_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OVER_REVERSE] = combine_conjoint_over_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_IN] = combine_conjoint_in_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_IN_REVERSE] = combine_conjoint_in_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OUT] = combine_conjoint_out_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OUT_REVERSE] = combine_conjoint_out_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_ATOP] = combine_conjoint_atop_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_ATOP_REVERSE] = combine_conjoint_atop_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_XOR] = combine_conjoint_xor_u_float;

    
    imp->combine_float[PIXMAN_OP_MULTIPLY] = combine_multiply_u_float;
    imp->combine_float[PIXMAN_OP_SCREEN] = combine_screen_u_float;
    imp->combine_float[PIXMAN_OP_OVERLAY] = combine_overlay_u_float;
    imp->combine_float[PIXMAN_OP_DARKEN] = combine_darken_u_float;
    imp->combine_float[PIXMAN_OP_LIGHTEN] = combine_lighten_u_float;
    imp->combine_float[PIXMAN_OP_COLOR_DODGE] = combine_color_dodge_u_float;
    imp->combine_float[PIXMAN_OP_COLOR_BURN] = combine_color_burn_u_float;
    imp->combine_float[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_u_float;
    imp->combine_float[PIXMAN_OP_SOFT_LIGHT] = combine_soft_light_u_float;
    imp->combine_float[PIXMAN_OP_DIFFERENCE] = combine_difference_u_float;
    imp->combine_float[PIXMAN_OP_EXCLUSION] = combine_exclusion_u_float;

    imp->combine_float[PIXMAN_OP_HSL_HUE] = combine_hsl_hue_u_float;
    imp->combine_float[PIXMAN_OP_HSL_SATURATION] = combine_hsl_saturation_u_float;
    imp->combine_float[PIXMAN_OP_HSL_COLOR] = combine_hsl_color_u_float;
    imp->combine_float[PIXMAN_OP_HSL_LUMINOSITY] = combine_hsl_luminosity_u_float;

    
    imp->combine_float_ca[PIXMAN_OP_CLEAR] = combine_clear_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SRC] = combine_src_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DST] = combine_dst_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OVER] = combine_over_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_IN] = combine_in_ca_float;
    imp->combine_float_ca[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OUT] = combine_out_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_ATOP] = combine_atop_ca_float;
    imp->combine_float_ca[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_XOR] = combine_xor_ca_float;
    imp->combine_float_ca[PIXMAN_OP_ADD] = combine_add_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SATURATE] = combine_saturate_ca_float;

    
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_CLEAR] = combine_disjoint_clear_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_SRC] = combine_disjoint_src_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_DST] = combine_disjoint_dst_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OVER] = combine_disjoint_over_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OVER_REVERSE] = combine_disjoint_over_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_IN] = combine_disjoint_in_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_IN_REVERSE] = combine_disjoint_in_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OUT] = combine_disjoint_out_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OUT_REVERSE] = combine_disjoint_out_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_ATOP] = combine_disjoint_atop_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_ATOP_REVERSE] = combine_disjoint_atop_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_XOR] = combine_disjoint_xor_ca_float;

    
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_CLEAR] = combine_conjoint_clear_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_SRC] = combine_conjoint_src_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_DST] = combine_conjoint_dst_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OVER] = combine_conjoint_over_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OVER_REVERSE] = combine_conjoint_over_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_IN] = combine_conjoint_in_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_IN_REVERSE] = combine_conjoint_in_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OUT] = combine_conjoint_out_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OUT_REVERSE] = combine_conjoint_out_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_ATOP] = combine_conjoint_atop_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_ATOP_REVERSE] = combine_conjoint_atop_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_XOR] = combine_conjoint_xor_ca_float;

    
    imp->combine_float_ca[PIXMAN_OP_MULTIPLY] = combine_multiply_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SCREEN] = combine_screen_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OVERLAY] = combine_overlay_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DARKEN] = combine_darken_ca_float;
    imp->combine_float_ca[PIXMAN_OP_LIGHTEN] = combine_lighten_ca_float;
    imp->combine_float_ca[PIXMAN_OP_COLOR_DODGE] = combine_color_dodge_ca_float;
    imp->combine_float_ca[PIXMAN_OP_COLOR_BURN] = combine_color_burn_ca_float;
    imp->combine_float_ca[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SOFT_LIGHT] = combine_soft_light_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DIFFERENCE] = combine_difference_ca_float;
    imp->combine_float_ca[PIXMAN_OP_EXCLUSION] = combine_exclusion_ca_float;

    
    imp->combine_float_ca[PIXMAN_OP_HSL_HUE] = combine_dst_u_float;
    imp->combine_float_ca[PIXMAN_OP_HSL_SATURATION] = combine_dst_u_float;
    imp->combine_float_ca[PIXMAN_OP_HSL_COLOR] = combine_dst_u_float;
    imp->combine_float_ca[PIXMAN_OP_HSL_LUMINOSITY] = combine_dst_u_float;
}
