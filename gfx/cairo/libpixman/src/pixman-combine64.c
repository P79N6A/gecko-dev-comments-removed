


#line 1 "pixman-combine.c.template"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>

#include "pixman-private.h"

#include "pixman-combine64.h"



static void
combine_mask_ca (uint64_t *src, uint64_t *mask)
{
    uint64_t a = *mask;

    uint64_t x;
    uint32_t xa;

    if (!a)
    {
	*(src) = 0;
	return;
    }

    x = *(src);
    if (a == ~0)
    {
	x = x >> A_SHIFT;
	x |= x << G_SHIFT;
	x |= x << R_SHIFT;
	*(mask) = x;
	return;
    }

    xa = x >> A_SHIFT;
    UN16x4_MUL_UN16x4 (x, a);
    *(src) = x;
    
    UN16x4_MUL_UN16 (a, xa);
    *(mask) = a;
}

static void
combine_mask_value_ca (uint64_t *src, const uint64_t *mask)
{
    uint64_t a = *mask;
    uint64_t x;

    if (!a)
    {
	*(src) = 0;
	return;
    }

    if (a == ~0)
	return;

    x = *(src);
    UN16x4_MUL_UN16x4 (x, a);
    *(src) = x;
}

static void
combine_mask_alpha_ca (const uint64_t *src, uint64_t *mask)
{
    uint64_t a = *(mask);
    uint64_t x;

    if (!a)
	return;

    x = *(src) >> A_SHIFT;
    if (x == MASK)
	return;

    if (a == ~0)
    {
	x |= x << G_SHIFT;
	x |= x << R_SHIFT;
	*(mask) = x;
	return;
    }

    UN16x4_MUL_UN16 (a, x);
    *(mask) = a;
}













static force_inline uint64_t
combine_mask (const uint64_t *src, const uint64_t *mask, int i)
{
    uint64_t s, m;

    if (mask)
    {
	m = *(mask + i) >> A_SHIFT;

	if (!m)
	    return 0;
    }

    s = *(src + i);

    if (mask)
	UN16x4_MUL_UN16 (s, m);

    return s;
}

static void
combine_clear (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint64_t *                dest,
               const uint64_t *          src,
               const uint64_t *          mask,
               int                      width)
{
    memset (dest, 0, width * sizeof(uint64_t));
}

static void
combine_dst (pixman_implementation_t *imp,
	     pixman_op_t	      op,
	     uint64_t *		      dest,
	     const uint64_t *	      src,
	     const uint64_t *          mask,
	     int		      width)
{
    return;
}

static void
combine_src_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint64_t *                dest,
               const uint64_t *          src,
               const uint64_t *          mask,
               int                      width)
{
    int i;

    if (!mask)
	memcpy (dest, src, width * sizeof (uint64_t));
    else
    {
	for (i = 0; i < width; ++i)
	{
	    uint64_t s = combine_mask (src, mask, i);

	    *(dest + i) = s;
	}
    }
}


static void
combine_over_u (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint64_t *                dest,
                const uint64_t *          src,
                const uint64_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t ia = ALPHA_16 (~s);

	UN16x4_MUL_UN16_ADD_UN16x4 (d, ia, s);
	*(dest + i) = d;
    }
}


static void
combine_over_reverse_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t ia = ALPHA_16 (~*(dest + i));
	UN16x4_MUL_UN16_ADD_UN16x4 (s, ia, d);
	*(dest + i) = s;
    }
}


static void
combine_in_u (pixman_implementation_t *imp,
              pixman_op_t              op,
              uint64_t *                dest,
              const uint64_t *          src,
              const uint64_t *          mask,
              int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t a = ALPHA_16 (*(dest + i));
	UN16x4_MUL_UN16 (s, a);
	*(dest + i) = s;
    }
}


static void
combine_in_reverse_u (pixman_implementation_t *imp,
                      pixman_op_t              op,
                      uint64_t *                dest,
                      const uint64_t *          src,
                      const uint64_t *          mask,
                      int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t a = ALPHA_16 (s);
	UN16x4_MUL_UN16 (d, a);
	*(dest + i) = d;
    }
}


static void
combine_out_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint64_t *                dest,
               const uint64_t *          src,
               const uint64_t *          mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t a = ALPHA_16 (~*(dest + i));
	UN16x4_MUL_UN16 (s, a);
	*(dest + i) = s;
    }
}


static void
combine_out_reverse_u (pixman_implementation_t *imp,
                       pixman_op_t              op,
                       uint64_t *                dest,
                       const uint64_t *          src,
                       const uint64_t *          mask,
                       int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t a = ALPHA_16 (~s);
	UN16x4_MUL_UN16 (d, a);
	*(dest + i) = d;
    }
}




static void
combine_atop_u (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint64_t *                dest,
                const uint64_t *          src,
                const uint64_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t dest_a = ALPHA_16 (d);
	uint64_t src_ia = ALPHA_16 (~s);

	UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16 (s, dest_a, d, src_ia);
	*(dest + i) = s;
    }
}




static void
combine_atop_reverse_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t src_a = ALPHA_16 (s);
	uint64_t dest_ia = ALPHA_16 (~d);

	UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16 (s, dest_ia, d, src_a);
	*(dest + i) = s;
    }
}




static void
combine_xor_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint64_t *                dest,
               const uint64_t *          src,
               const uint64_t *          mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t src_ia = ALPHA_16 (~s);
	uint64_t dest_ia = ALPHA_16 (~d);

	UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16 (s, dest_ia, d, src_ia);
	*(dest + i) = s;
    }
}

static void
combine_add_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint64_t *                dest,
               const uint64_t *          src,
               const uint64_t *          mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	UN16x4_ADD_UN16x4 (d, s);
	*(dest + i) = d;
    }
}




static void
combine_saturate_u (pixman_implementation_t *imp,
                    pixman_op_t              op,
                    uint64_t *                dest,
                    const uint64_t *          src,
                    const uint64_t *          mask,
                    int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint32_t sa, da;

	sa = s >> A_SHIFT;
	da = ~d >> A_SHIFT;
	if (sa > da)
	{
	    sa = DIV_UN16 (da, sa);
	    UN16x4_MUL_UN16 (s, sa);
	}
	;
	UN16x4_ADD_UN16x4 (d, s);
	*(dest + i) = d;
    }
}





























static void
combine_multiply_u (pixman_implementation_t *imp,
                    pixman_op_t              op,
                    uint64_t *                dest,
                    const uint64_t *          src,
                    const uint64_t *          mask,
                    int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t ss = s;
	uint64_t src_ia = ALPHA_16 (~s);
	uint64_t dest_ia = ALPHA_16 (~d);

	UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16 (ss, dest_ia, d, src_ia);
	UN16x4_MUL_UN16x4 (d, s);
	UN16x4_ADD_UN16x4 (d, ss);

	*(dest + i) = d;
    }
}

static void
combine_multiply_ca (pixman_implementation_t *imp,
                     pixman_op_t              op,
                     uint64_t *                dest,
                     const uint64_t *          src,
                     const uint64_t *          mask,
                     int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t m = *(mask + i);
	uint64_t s = *(src + i);
	uint64_t d = *(dest + i);
	uint64_t r = d;
	uint64_t dest_ia = ALPHA_16 (~d);

	combine_mask_value_ca (&s, &m);

	UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16 (r, ~m, s, dest_ia);
	UN16x4_MUL_UN16x4 (d, s);
	UN16x4_ADD_UN16x4 (r, d);

	*(dest + i) = r;
    }
}

#define PDF_SEPARABLE_BLEND_MODE(name)					\
    static void								\
    combine_ ## name ## _u (pixman_implementation_t *imp,		\
			    pixman_op_t              op,		\
                            uint64_t *                dest,		\
			    const uint64_t *          src,		\
			    const uint64_t *          mask,		\
			    int                      width)		\
    {									\
	int i;								\
	for (i = 0; i < width; ++i) {					\
	    uint64_t s = combine_mask (src, mask, i);			\
	    uint64_t d = *(dest + i);					\
	    uint16_t sa = ALPHA_16 (s);					\
	    uint16_t isa = ~sa;						\
	    uint16_t da = ALPHA_16 (d);					\
	    uint16_t ida = ~da;						\
	    uint64_t result;						\
									\
	    result = d;							\
	    UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16 (result, isa, s, ida);	\
	    								\
	    *(dest + i) = result +					\
		(DIV_ONE_UN16 (sa * da) << A_SHIFT) +			\
		(blend_ ## name (RED_16 (d), da, RED_16 (s), sa) << R_SHIFT) + \
		(blend_ ## name (GREEN_16 (d), da, GREEN_16 (s), sa) << G_SHIFT) + \
		(blend_ ## name (BLUE_16 (d), da, BLUE_16 (s), sa));	\
	}								\
    }									\
    									\
    static void								\
    combine_ ## name ## _ca (pixman_implementation_t *imp,		\
			     pixman_op_t              op,		\
                             uint64_t *                dest,		\
			     const uint64_t *          src,		\
			     const uint64_t *          mask,		\
			     int                     width)		\
    {									\
	int i;								\
	for (i = 0; i < width; ++i) {					\
	    uint64_t m = *(mask + i);					\
	    uint64_t s = *(src + i);					\
	    uint64_t d = *(dest + i);					\
	    uint16_t da = ALPHA_16 (d);					\
	    uint16_t ida = ~da;						\
	    uint64_t result;						\
            								\
	    combine_mask_value_ca (&s, &m);				\
            								\
	    result = d;							\
	    UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16 (result, ~m, s, ida);     \
            								\
	    result +=							\
	        (DIV_ONE_UN16 (ALPHA_16 (m) * da) << A_SHIFT) +		\
	        (blend_ ## name (RED_16 (d), da, RED_16 (s), RED_16 (m)) << R_SHIFT) + \
	        (blend_ ## name (GREEN_16 (d), da, GREEN_16 (s), GREEN_16 (m)) << G_SHIFT) + \
	        (blend_ ## name (BLUE_16 (d), da, BLUE_16 (s), BLUE_16 (m))); \
	    								\
	    *(dest + i) = result;					\
	}								\
    }





static inline uint64_t
blend_screen (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    return DIV_ONE_UN16 (sca * da + dca * sa - sca * dca);
}

PDF_SEPARABLE_BLEND_MODE (screen)









static inline uint64_t
blend_overlay (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    uint64_t rca;

    if (2 * dca < da)
	rca = 2 * sca * dca;
    else
	rca = sa * da - 2 * (da - dca) * (sa - sca);
    return DIV_ONE_UN16 (rca);
}

PDF_SEPARABLE_BLEND_MODE (overlay)





static inline uint64_t
blend_darken (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    uint64_t s, d;

    s = sca * da;
    d = dca * sa;
    return DIV_ONE_UN16 (s > d ? d : s);
}

PDF_SEPARABLE_BLEND_MODE (darken)





static inline uint64_t
blend_lighten (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    uint64_t s, d;

    s = sca * da;
    d = dca * sa;
    return DIV_ONE_UN16 (s > d ? s : d);
}

PDF_SEPARABLE_BLEND_MODE (lighten)











static inline uint64_t
blend_color_dodge (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    if (sca >= sa)
    {
	return dca == 0 ? 0 : DIV_ONE_UN16 (sa * da);
    }
    else
    {
	uint64_t rca = dca * sa / (sa - sca);
	return DIV_ONE_UN16 (sa * MIN (rca, da));
    }
}

PDF_SEPARABLE_BLEND_MODE (color_dodge)











static inline uint64_t
blend_color_burn (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    if (sca == 0)
    {
	return dca < da ? 0 : DIV_ONE_UN16 (sa * da);
    }
    else
    {
	uint64_t rca = (da - dca) * sa / sca;
	return DIV_ONE_UN16 (sa * (MAX (rca, da) - rca));
    }
}

PDF_SEPARABLE_BLEND_MODE (color_burn)









static inline uint64_t
blend_hard_light (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    if (2 * sca < sa)
	return DIV_ONE_UN16 (2 * sca * dca);
    else
	return DIV_ONE_UN16 (sa * da - 2 * (da - dca) * (sa - sca));
}

PDF_SEPARABLE_BLEND_MODE (hard_light)











static inline uint64_t
blend_soft_light (uint64_t dca_org,
		  uint64_t da_org,
		  uint64_t sca_org,
		  uint64_t sa_org)
{
    double dca = dca_org * (1.0 / MASK);
    double da = da_org * (1.0 / MASK);
    double sca = sca_org * (1.0 / MASK);
    double sa = sa_org * (1.0 / MASK);
    double rca;

    if (2 * sca < sa)
    {
	if (da == 0)
	    rca = dca * sa;
	else
	    rca = dca * sa - dca * (da - dca) * (sa - 2 * sca) / da;
    }
    else if (da == 0)
    {
	rca = 0;
    }
    else if (4 * dca <= da)
    {
	rca = dca * sa +
	    (2 * sca - sa) * dca * ((16 * dca / da - 12) * dca / da + 3);
    }
    else
    {
	rca = dca * sa + (sqrt (dca * da) - dca) * (2 * sca - sa);
    }
    return rca * MASK + 0.5;
}

PDF_SEPARABLE_BLEND_MODE (soft_light)





static inline uint64_t
blend_difference (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    uint64_t dcasa = dca * sa;
    uint64_t scada = sca * da;

    if (scada < dcasa)
	return DIV_ONE_UN16 (dcasa - scada);
    else
	return DIV_ONE_UN16 (scada - dcasa);
}

PDF_SEPARABLE_BLEND_MODE (difference)









static inline uint64_t
blend_exclusion (uint64_t dca, uint64_t da, uint64_t sca, uint64_t sa)
{
    return DIV_ONE_UN16 (sca * da + dca * sa - 2 * dca * sca);
}

PDF_SEPARABLE_BLEND_MODE (exclusion)

#undef PDF_SEPARABLE_BLEND_MODE














































































































#define CH_MIN(c) (c[0] < c[1] ? (c[0] < c[2] ? c[0] : c[2]) : (c[1] < c[2] ? c[1] : c[2]))
#define CH_MAX(c) (c[0] > c[1] ? (c[0] > c[2] ? c[0] : c[2]) : (c[1] > c[2] ? c[1] : c[2]))
#define LUM(c) ((c[0] * 30 + c[1] * 59 + c[2] * 11) / 100)
#define SAT(c) (CH_MAX (c) - CH_MIN (c))

#define PDF_NON_SEPARABLE_BLEND_MODE(name)				\
    static void								\
    combine_ ## name ## _u (pixman_implementation_t *imp,		\
			    pixman_op_t op,				\
                            uint64_t *dest,				\
			    const uint64_t *src,				\
			    const uint64_t *mask,			\
			    int width)					\
    {									\
	int i;								\
	for (i = 0; i < width; ++i)					\
	{								\
	    uint64_t s = combine_mask (src, mask, i);			\
	    uint64_t d = *(dest + i);					\
	    uint16_t sa = ALPHA_16 (s);					\
	    uint16_t isa = ~sa;						\
	    uint16_t da = ALPHA_16 (d);					\
	    uint16_t ida = ~da;						\
	    uint64_t result;						\
	    uint64_t sc[3], dc[3], c[3];					\
            								\
	    result = d;							\
	    UN16x4_MUL_UN16_ADD_UN16x4_MUL_UN16 (result, isa, s, ida);	\
	    dc[0] = RED_16 (d);						\
	    sc[0] = RED_16 (s);						\
	    dc[1] = GREEN_16 (d);					\
	    sc[1] = GREEN_16 (s);					\
	    dc[2] = BLUE_16 (d);						\
	    sc[2] = BLUE_16 (s);						\
	    blend_ ## name (c, dc, da, sc, sa);				\
            								\
	    *(dest + i) = result +					\
		(DIV_ONE_UN16 (sa * da) << A_SHIFT) +			\
		(DIV_ONE_UN16 (c[0]) << R_SHIFT) +			\
		(DIV_ONE_UN16 (c[1]) << G_SHIFT) +			\
		(DIV_ONE_UN16 (c[2]));					\
	}								\
    }

static void
set_lum (uint64_t dest[3], uint64_t src[3], uint64_t sa, uint64_t lum)
{
    double a, l, min, max;
    double tmp[3];

    a = sa * (1.0 / MASK);

    l = lum * (1.0 / MASK);
    tmp[0] = src[0] * (1.0 / MASK);
    tmp[1] = src[1] * (1.0 / MASK);
    tmp[2] = src[2] * (1.0 / MASK);

    l = l - LUM (tmp);
    tmp[0] += l;
    tmp[1] += l;
    tmp[2] += l;

    
    l = LUM (tmp);
    min = CH_MIN (tmp);
    max = CH_MAX (tmp);

    if (min < 0)
    {
	if (l - min == 0.0)
	{
	    tmp[0] = 0;
	    tmp[1] = 0;
	    tmp[2] = 0;
	}
	else
	{
	    tmp[0] = l + (tmp[0] - l) * l / (l - min);
	    tmp[1] = l + (tmp[1] - l) * l / (l - min);
	    tmp[2] = l + (tmp[2] - l) * l / (l - min);
	}
    }
    if (max > a)
    {
	if (max - l == 0.0)
	{
	    tmp[0] = a;
	    tmp[1] = a;
	    tmp[2] = a;
	}
	else
	{
	    tmp[0] = l + (tmp[0] - l) * (a - l) / (max - l);
	    tmp[1] = l + (tmp[1] - l) * (a - l) / (max - l);
	    tmp[2] = l + (tmp[2] - l) * (a - l) / (max - l);
	}
    }

    dest[0] = tmp[0] * MASK + 0.5;
    dest[1] = tmp[1] * MASK + 0.5;
    dest[2] = tmp[2] * MASK + 0.5;
}

static void
set_sat (uint64_t dest[3], uint64_t src[3], uint64_t sat)
{
    int id[3];
    uint64_t min, max;

    if (src[0] > src[1])
    {
	if (src[0] > src[2])
	{
	    id[0] = 0;
	    if (src[1] > src[2])
	    {
		id[1] = 1;
		id[2] = 2;
	    }
	    else
	    {
		id[1] = 2;
		id[2] = 1;
	    }
	}
	else
	{
	    id[0] = 2;
	    id[1] = 0;
	    id[2] = 1;
	}
    }
    else
    {
	if (src[0] > src[2])
	{
	    id[0] = 1;
	    id[1] = 0;
	    id[2] = 2;
	}
	else
	{
	    id[2] = 0;
	    if (src[1] > src[2])
	    {
		id[0] = 1;
		id[1] = 2;
	    }
	    else
	    {
		id[0] = 2;
		id[1] = 1;
	    }
	}
    }

    max = dest[id[0]];
    min = dest[id[2]];
    if (max > min)
    {
	dest[id[1]] = (dest[id[1]] - min) * sat / (max - min);
	dest[id[0]] = sat;
	dest[id[2]] = 0;
    }
    else
    {
	dest[0] = dest[1] = dest[2] = 0;
    }
}





static inline void
blend_hsl_hue (uint64_t c[3],
               uint64_t dc[3],
               uint64_t da,
               uint64_t sc[3],
               uint64_t sa)
{
    c[0] = sc[0] * da;
    c[1] = sc[1] * da;
    c[2] = sc[2] * da;
    set_sat (c, c, SAT (dc) * sa);
    set_lum (c, c, sa * da, LUM (dc) * sa);
}

PDF_NON_SEPARABLE_BLEND_MODE (hsl_hue)





static inline void
blend_hsl_saturation (uint64_t c[3],
                      uint64_t dc[3],
                      uint64_t da,
                      uint64_t sc[3],
                      uint64_t sa)
{
    c[0] = dc[0] * sa;
    c[1] = dc[1] * sa;
    c[2] = dc[2] * sa;
    set_sat (c, c, SAT (sc) * da);
    set_lum (c, c, sa * da, LUM (dc) * sa);
}

PDF_NON_SEPARABLE_BLEND_MODE (hsl_saturation)





static inline void
blend_hsl_color (uint64_t c[3],
                 uint64_t dc[3],
                 uint64_t da,
                 uint64_t sc[3],
                 uint64_t sa)
{
    c[0] = sc[0] * da;
    c[1] = sc[1] * da;
    c[2] = sc[2] * da;
    set_lum (c, c, sa * da, LUM (dc) * sa);
}

PDF_NON_SEPARABLE_BLEND_MODE (hsl_color)





static inline void
blend_hsl_luminosity (uint64_t c[3],
                      uint64_t dc[3],
                      uint64_t da,
                      uint64_t sc[3],
                      uint64_t sa)
{
    c[0] = dc[0] * sa;
    c[1] = dc[1] * sa;
    c[2] = dc[2] * sa;
    set_lum (c, c, sa * da, LUM (sc) * da);
}

PDF_NON_SEPARABLE_BLEND_MODE (hsl_luminosity)

#undef SAT
#undef LUM
#undef CH_MAX
#undef CH_MIN
#undef PDF_NON_SEPARABLE_BLEND_MODE


























#define COMBINE_A_OUT 1
#define COMBINE_A_IN  2
#define COMBINE_B_OUT 4
#define COMBINE_B_IN  8

#define COMBINE_CLEAR   0
#define COMBINE_A       (COMBINE_A_OUT | COMBINE_A_IN)
#define COMBINE_B       (COMBINE_B_OUT | COMBINE_B_IN)
#define COMBINE_A_OVER  (COMBINE_A_OUT | COMBINE_B_OUT | COMBINE_A_IN)
#define COMBINE_B_OVER  (COMBINE_A_OUT | COMBINE_B_OUT | COMBINE_B_IN)
#define COMBINE_A_ATOP  (COMBINE_B_OUT | COMBINE_A_IN)
#define COMBINE_B_ATOP  (COMBINE_A_OUT | COMBINE_B_IN)
#define COMBINE_XOR     (COMBINE_A_OUT | COMBINE_B_OUT)


static uint16_t
combine_disjoint_out_part (uint16_t a, uint16_t b)
{
    

    b = ~b;                 
    if (b >= a)             
	return MASK;        
    return DIV_UN16 (b, a);     
}


static uint16_t
combine_disjoint_in_part (uint16_t a, uint16_t b)
{
    
    
    

    b = ~b;                 
    if (b >= a)             
	return 0;           
    return ~DIV_UN16(b, a);    
}


static uint16_t
combine_conjoint_out_part (uint16_t a, uint16_t b)
{
    
    

    

    if (b >= a)             
	return 0x00;        
    return ~DIV_UN16(b, a);    
}


static uint16_t
combine_conjoint_in_part (uint16_t a, uint16_t b)
{
    

    if (b >= a)             
	return MASK;        
    return DIV_UN16 (b, a);     
}

#define GET_COMP(v, i)   ((uint32_t) (uint16_t) ((v) >> i))

#define ADD(x, y, i, t)							\
    ((t) = GET_COMP (x, i) + GET_COMP (y, i),				\
     (uint64_t) ((uint16_t) ((t) | (0 - ((t) >> G_SHIFT)))) << (i))

#define GENERIC(x, y, i, ax, ay, t, u, v)				\
    ((t) = (MUL_UN16 (GET_COMP (y, i), ay, (u)) +			\
            MUL_UN16 (GET_COMP (x, i), ax, (v))),			\
     (uint64_t) ((uint16_t) ((t) |					\
                           (0 - ((t) >> G_SHIFT)))) << (i))

static void
combine_disjoint_general_u (uint64_t *      dest,
                            const uint64_t *src,
                            const uint64_t *mask,
                            int            width,
                            uint16_t        combine)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t m, n, o, p;
	uint32_t Fa, Fb, t, u, v;
	uint16_t sa = s >> A_SHIFT;
	uint16_t da = d >> A_SHIFT;

	switch (combine & COMBINE_A)
	{
	default:
	    Fa = 0;
	    break;

	case COMBINE_A_OUT:
	    Fa = combine_disjoint_out_part (sa, da);
	    break;

	case COMBINE_A_IN:
	    Fa = combine_disjoint_in_part (sa, da);
	    break;

	case COMBINE_A:
	    Fa = MASK;
	    break;
	}

	switch (combine & COMBINE_B)
	{
	default:
	    Fb = 0;
	    break;

	case COMBINE_B_OUT:
	    Fb = combine_disjoint_out_part (da, sa);
	    break;

	case COMBINE_B_IN:
	    Fb = combine_disjoint_in_part (da, sa);
	    break;

	case COMBINE_B:
	    Fb = MASK;
	    break;
	}
	m = GENERIC (s, d, 0, Fa, Fb, t, u, v);
	n = GENERIC (s, d, G_SHIFT, Fa, Fb, t, u, v);
	o = GENERIC (s, d, R_SHIFT, Fa, Fb, t, u, v);
	p = GENERIC (s, d, A_SHIFT, Fa, Fb, t, u, v);
	s = m | n | o | p;
	*(dest + i) = s;
    }
}

static void
combine_disjoint_over_u (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint32_t a = s >> A_SHIFT;

	if (s != 0x00)
	{
	    uint64_t d = *(dest + i);
	    a = combine_disjoint_out_part (d >> A_SHIFT, a);
	    UN16x4_MUL_UN16_ADD_UN16x4 (d, a, s);

	    *(dest + i) = d;
	}
    }
}

static void
combine_disjoint_in_u (pixman_implementation_t *imp,
                       pixman_op_t              op,
                       uint64_t *                dest,
                       const uint64_t *          src,
                       const uint64_t *          mask,
                       int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_A_IN);
}

static void
combine_disjoint_in_reverse_u (pixman_implementation_t *imp,
                               pixman_op_t              op,
                               uint64_t *                dest,
                               const uint64_t *          src,
                               const uint64_t *          mask,
                               int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_B_IN);
}

static void
combine_disjoint_out_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_A_OUT);
}

static void
combine_disjoint_out_reverse_u (pixman_implementation_t *imp,
                                pixman_op_t              op,
                                uint64_t *                dest,
                                const uint64_t *          src,
                                const uint64_t *          mask,
                                int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_B_OUT);
}

static void
combine_disjoint_atop_u (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_A_ATOP);
}

static void
combine_disjoint_atop_reverse_u (pixman_implementation_t *imp,
                                 pixman_op_t              op,
                                 uint64_t *                dest,
                                 const uint64_t *          src,
                                 const uint64_t *          mask,
                                 int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_B_ATOP);
}

static void
combine_disjoint_xor_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    combine_disjoint_general_u (dest, src, mask, width, COMBINE_XOR);
}

static void
combine_conjoint_general_u (uint64_t *      dest,
                            const uint64_t *src,
                            const uint64_t *mask,
                            int            width,
                            uint16_t        combine)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = combine_mask (src, mask, i);
	uint64_t d = *(dest + i);
	uint64_t m, n, o, p;
	uint32_t Fa, Fb, t, u, v;
	uint16_t sa = s >> A_SHIFT;
	uint16_t da = d >> A_SHIFT;

	switch (combine & COMBINE_A)
	{
	default:
	    Fa = 0;
	    break;

	case COMBINE_A_OUT:
	    Fa = combine_conjoint_out_part (sa, da);
	    break;

	case COMBINE_A_IN:
	    Fa = combine_conjoint_in_part (sa, da);
	    break;

	case COMBINE_A:
	    Fa = MASK;
	    break;
	}

	switch (combine & COMBINE_B)
	{
	default:
	    Fb = 0;
	    break;

	case COMBINE_B_OUT:
	    Fb = combine_conjoint_out_part (da, sa);
	    break;

	case COMBINE_B_IN:
	    Fb = combine_conjoint_in_part (da, sa);
	    break;

	case COMBINE_B:
	    Fb = MASK;
	    break;
	}

	m = GENERIC (s, d, 0, Fa, Fb, t, u, v);
	n = GENERIC (s, d, G_SHIFT, Fa, Fb, t, u, v);
	o = GENERIC (s, d, R_SHIFT, Fa, Fb, t, u, v);
	p = GENERIC (s, d, A_SHIFT, Fa, Fb, t, u, v);

	s = m | n | o | p;

	*(dest + i) = s;
    }
}

static void
combine_conjoint_over_u (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_A_OVER);
}

static void
combine_conjoint_over_reverse_u (pixman_implementation_t *imp,
                                 pixman_op_t              op,
                                 uint64_t *                dest,
                                 const uint64_t *          src,
                                 const uint64_t *          mask,
                                 int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_B_OVER);
}

static void
combine_conjoint_in_u (pixman_implementation_t *imp,
                       pixman_op_t              op,
                       uint64_t *                dest,
                       const uint64_t *          src,
                       const uint64_t *          mask,
                       int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_A_IN);
}

static void
combine_conjoint_in_reverse_u (pixman_implementation_t *imp,
                               pixman_op_t              op,
                               uint64_t *                dest,
                               const uint64_t *          src,
                               const uint64_t *          mask,
                               int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_B_IN);
}

static void
combine_conjoint_out_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_A_OUT);
}

static void
combine_conjoint_out_reverse_u (pixman_implementation_t *imp,
                                pixman_op_t              op,
                                uint64_t *                dest,
                                const uint64_t *          src,
                                const uint64_t *          mask,
                                int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_B_OUT);
}

static void
combine_conjoint_atop_u (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_A_ATOP);
}

static void
combine_conjoint_atop_reverse_u (pixman_implementation_t *imp,
                                 pixman_op_t              op,
                                 uint64_t *                dest,
                                 const uint64_t *          src,
                                 const uint64_t *          mask,
                                 int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_B_ATOP);
}

static void
combine_conjoint_xor_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    combine_conjoint_general_u (dest, src, mask, width, COMBINE_XOR);
}





static void
combine_clear_ca (pixman_implementation_t *imp,
                  pixman_op_t              op,
                  uint64_t *                dest,
                  const uint64_t *          src,
                  const uint64_t *          mask,
                  int                      width)
{
    memset (dest, 0, width * sizeof(uint64_t));
}

static void
combine_src_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint64_t *                dest,
                const uint64_t *          src,
                const uint64_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);

	combine_mask_value_ca (&s, &m);

	*(dest + i) = s;
    }
}

static void
combine_over_ca (pixman_implementation_t *imp,
                 pixman_op_t              op,
                 uint64_t *                dest,
                 const uint64_t *          src,
                 const uint64_t *          mask,
                 int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t a;

	combine_mask_ca (&s, &m);

	a = ~m;
	if (a)
	{
	    uint64_t d = *(dest + i);
	    UN16x4_MUL_UN16x4_ADD_UN16x4 (d, a, s);
	    s = d;
	}

	*(dest + i) = s;
    }
}

static void
combine_over_reverse_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t d = *(dest + i);
	uint64_t a = ~d >> A_SHIFT;

	if (a)
	{
	    uint64_t s = *(src + i);
	    uint64_t m = *(mask + i);

	    UN16x4_MUL_UN16x4 (s, m);
	    UN16x4_MUL_UN16_ADD_UN16x4 (s, a, d);

	    *(dest + i) = s;
	}
    }
}

static void
combine_in_ca (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint64_t *                dest,
               const uint64_t *          src,
               const uint64_t *          mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t d = *(dest + i);
	uint32_t a = d >> A_SHIFT;
	uint64_t s = 0;

	if (a)
	{
	    uint64_t m = *(mask + i);

	    s = *(src + i);
	    combine_mask_value_ca (&s, &m);

	    if (a != MASK)
		UN16x4_MUL_UN16 (s, a);
	}

	*(dest + i) = s;
    }
}

static void
combine_in_reverse_ca (pixman_implementation_t *imp,
                       pixman_op_t              op,
                       uint64_t *                dest,
                       const uint64_t *          src,
                       const uint64_t *          mask,
                       int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t a;

	combine_mask_alpha_ca (&s, &m);

	a = m;
	if (a != ~0)
	{
	    uint64_t d = 0;

	    if (a)
	    {
		d = *(dest + i);
		UN16x4_MUL_UN16x4 (d, a);
	    }

	    *(dest + i) = d;
	}
    }
}

static void
combine_out_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint64_t *                dest,
                const uint64_t *          src,
                const uint64_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t d = *(dest + i);
	uint32_t a = ~d >> A_SHIFT;
	uint64_t s = 0;

	if (a)
	{
	    uint64_t m = *(mask + i);

	    s = *(src + i);
	    combine_mask_value_ca (&s, &m);

	    if (a != MASK)
		UN16x4_MUL_UN16 (s, a);
	}

	*(dest + i) = s;
    }
}

static void
combine_out_reverse_ca (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t a;

	combine_mask_alpha_ca (&s, &m);

	a = ~m;
	if (a != ~0)
	{
	    uint64_t d = 0;

	    if (a)
	    {
		d = *(dest + i);
		UN16x4_MUL_UN16x4 (d, a);
	    }

	    *(dest + i) = d;
	}
    }
}

static void
combine_atop_ca (pixman_implementation_t *imp,
                 pixman_op_t              op,
                 uint64_t *                dest,
                 const uint64_t *          src,
                 const uint64_t *          mask,
                 int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t d = *(dest + i);
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t ad;
	uint32_t as = d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	ad = ~m;

	UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16 (d, ad, s, as);

	*(dest + i) = d;
    }
}

static void
combine_atop_reverse_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t d = *(dest + i);
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t ad;
	uint32_t as = ~d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	ad = m;

	UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16 (d, ad, s, as);

	*(dest + i) = d;
    }
}

static void
combine_xor_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint64_t *                dest,
                const uint64_t *          src,
                const uint64_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t d = *(dest + i);
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t ad;
	uint32_t as = ~d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	ad = ~m;

	UN16x4_MUL_UN16x4_ADD_UN16x4_MUL_UN16 (d, ad, s, as);

	*(dest + i) = d;
    }
}

static void
combine_add_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint64_t *                dest,
                const uint64_t *          src,
                const uint64_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s = *(src + i);
	uint64_t m = *(mask + i);
	uint64_t d = *(dest + i);

	combine_mask_value_ca (&s, &m);

	UN16x4_ADD_UN16x4 (d, s);

	*(dest + i) = d;
    }
}

static void
combine_saturate_ca (pixman_implementation_t *imp,
                     pixman_op_t              op,
                     uint64_t *                dest,
                     const uint64_t *          src,
                     const uint64_t *          mask,
                     int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s, d;
	uint32_t sa, sr, sg, sb, da;
	uint32_t t, u, v;
	uint64_t m, n, o, p;

	d = *(dest + i);
	s = *(src + i);
	m = *(mask + i);

	combine_mask_ca (&s, &m);

	sa = (m >> A_SHIFT);
	sr = (m >> R_SHIFT) & MASK;
	sg = (m >> G_SHIFT) & MASK;
	sb =  m             & MASK;
	da = ~d >> A_SHIFT;

	if (sb <= da)
	    m = ADD (s, d, 0, t);
	else
	    m = GENERIC (s, d, 0, (da << G_SHIFT) / sb, MASK, t, u, v);

	if (sg <= da)
	    n = ADD (s, d, G_SHIFT, t);
	else
	    n = GENERIC (s, d, G_SHIFT, (da << G_SHIFT) / sg, MASK, t, u, v);

	if (sr <= da)
	    o = ADD (s, d, R_SHIFT, t);
	else
	    o = GENERIC (s, d, R_SHIFT, (da << G_SHIFT) / sr, MASK, t, u, v);

	if (sa <= da)
	    p = ADD (s, d, A_SHIFT, t);
	else
	    p = GENERIC (s, d, A_SHIFT, (da << G_SHIFT) / sa, MASK, t, u, v);

	*(dest + i) = m | n | o | p;
    }
}

static void
combine_disjoint_general_ca (uint64_t *      dest,
                             const uint64_t *src,
                             const uint64_t *mask,
                             int            width,
                             uint16_t        combine)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s, d;
	uint64_t m, n, o, p;
	uint64_t Fa, Fb;
	uint32_t t, u, v;
	uint64_t sa;
	uint16_t da;

	s = *(src + i);
	m = *(mask + i);
	d = *(dest + i);
	da = d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	sa = m;

	switch (combine & COMBINE_A)
	{
	default:
	    Fa = 0;
	    break;

	case COMBINE_A_OUT:
	    m = (uint64_t)combine_disjoint_out_part ((uint16_t) (sa >> 0), da);
	    n = (uint64_t)combine_disjoint_out_part ((uint16_t) (sa >> G_SHIFT), da) << G_SHIFT;
	    o = (uint64_t)combine_disjoint_out_part ((uint16_t) (sa >> R_SHIFT), da) << R_SHIFT;
	    p = (uint64_t)combine_disjoint_out_part ((uint16_t) (sa >> A_SHIFT), da) << A_SHIFT;
	    Fa = m | n | o | p;
	    break;

	case COMBINE_A_IN:
	    m = (uint64_t)combine_disjoint_in_part ((uint16_t) (sa >> 0), da);
	    n = (uint64_t)combine_disjoint_in_part ((uint16_t) (sa >> G_SHIFT), da) << G_SHIFT;
	    o = (uint64_t)combine_disjoint_in_part ((uint16_t) (sa >> R_SHIFT), da) << R_SHIFT;
	    p = (uint64_t)combine_disjoint_in_part ((uint16_t) (sa >> A_SHIFT), da) << A_SHIFT;
	    Fa = m | n | o | p;
	    break;

	case COMBINE_A:
	    Fa = ~0;
	    break;
	}

	switch (combine & COMBINE_B)
	{
	default:
	    Fb = 0;
	    break;

	case COMBINE_B_OUT:
	    m = (uint64_t)combine_disjoint_out_part (da, (uint16_t) (sa >> 0));
	    n = (uint64_t)combine_disjoint_out_part (da, (uint16_t) (sa >> G_SHIFT)) << G_SHIFT;
	    o = (uint64_t)combine_disjoint_out_part (da, (uint16_t) (sa >> R_SHIFT)) << R_SHIFT;
	    p = (uint64_t)combine_disjoint_out_part (da, (uint16_t) (sa >> A_SHIFT)) << A_SHIFT;
	    Fb = m | n | o | p;
	    break;

	case COMBINE_B_IN:
	    m = (uint64_t)combine_disjoint_in_part (da, (uint16_t) (sa >> 0));
	    n = (uint64_t)combine_disjoint_in_part (da, (uint16_t) (sa >> G_SHIFT)) << G_SHIFT;
	    o = (uint64_t)combine_disjoint_in_part (da, (uint16_t) (sa >> R_SHIFT)) << R_SHIFT;
	    p = (uint64_t)combine_disjoint_in_part (da, (uint16_t) (sa >> A_SHIFT)) << A_SHIFT;
	    Fb = m | n | o | p;
	    break;

	case COMBINE_B:
	    Fb = ~0;
	    break;
	}
	m = GENERIC (s, d, 0, GET_COMP (Fa, 0), GET_COMP (Fb, 0), t, u, v);
	n = GENERIC (s, d, G_SHIFT, GET_COMP (Fa, G_SHIFT), GET_COMP (Fb, G_SHIFT), t, u, v);
	o = GENERIC (s, d, R_SHIFT, GET_COMP (Fa, R_SHIFT), GET_COMP (Fb, R_SHIFT), t, u, v);
	p = GENERIC (s, d, A_SHIFT, GET_COMP (Fa, A_SHIFT), GET_COMP (Fb, A_SHIFT), t, u, v);

	s = m | n | o | p;

	*(dest + i) = s;
    }
}

static void
combine_disjoint_over_ca (pixman_implementation_t *imp,
                          pixman_op_t              op,
                          uint64_t *                dest,
                          const uint64_t *          src,
                          const uint64_t *          mask,
                          int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_A_OVER);
}

static void
combine_disjoint_in_ca (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_A_IN);
}

static void
combine_disjoint_in_reverse_ca (pixman_implementation_t *imp,
                                pixman_op_t              op,
                                uint64_t *                dest,
                                const uint64_t *          src,
                                const uint64_t *          mask,
                                int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_B_IN);
}

static void
combine_disjoint_out_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_A_OUT);
}

static void
combine_disjoint_out_reverse_ca (pixman_implementation_t *imp,
                                 pixman_op_t              op,
                                 uint64_t *                dest,
                                 const uint64_t *          src,
                                 const uint64_t *          mask,
                                 int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_B_OUT);
}

static void
combine_disjoint_atop_ca (pixman_implementation_t *imp,
                          pixman_op_t              op,
                          uint64_t *                dest,
                          const uint64_t *          src,
                          const uint64_t *          mask,
                          int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_A_ATOP);
}

static void
combine_disjoint_atop_reverse_ca (pixman_implementation_t *imp,
                                  pixman_op_t              op,
                                  uint64_t *                dest,
                                  const uint64_t *          src,
                                  const uint64_t *          mask,
                                  int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_B_ATOP);
}

static void
combine_disjoint_xor_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_disjoint_general_ca (dest, src, mask, width, COMBINE_XOR);
}

static void
combine_conjoint_general_ca (uint64_t *      dest,
                             const uint64_t *src,
                             const uint64_t *mask,
                             int            width,
                             uint16_t        combine)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint64_t s, d;
	uint64_t m, n, o, p;
	uint64_t Fa, Fb;
	uint32_t t, u, v;
	uint64_t sa;
	uint16_t da;

	s = *(src + i);
	m = *(mask + i);
	d = *(dest + i);
	da = d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	sa = m;

	switch (combine & COMBINE_A)
	{
	default:
	    Fa = 0;
	    break;

	case COMBINE_A_OUT:
	    m = (uint64_t)combine_conjoint_out_part ((uint16_t) (sa >> 0), da);
	    n = (uint64_t)combine_conjoint_out_part ((uint16_t) (sa >> G_SHIFT), da) << G_SHIFT;
	    o = (uint64_t)combine_conjoint_out_part ((uint16_t) (sa >> R_SHIFT), da) << R_SHIFT;
	    p = (uint64_t)combine_conjoint_out_part ((uint16_t) (sa >> A_SHIFT), da) << A_SHIFT;
	    Fa = m | n | o | p;
	    break;

	case COMBINE_A_IN:
	    m = (uint64_t)combine_conjoint_in_part ((uint16_t) (sa >> 0), da);
	    n = (uint64_t)combine_conjoint_in_part ((uint16_t) (sa >> G_SHIFT), da) << G_SHIFT;
	    o = (uint64_t)combine_conjoint_in_part ((uint16_t) (sa >> R_SHIFT), da) << R_SHIFT;
	    p = (uint64_t)combine_conjoint_in_part ((uint16_t) (sa >> A_SHIFT), da) << A_SHIFT;
	    Fa = m | n | o | p;
	    break;

	case COMBINE_A:
	    Fa = ~0;
	    break;
	}

	switch (combine & COMBINE_B)
	{
	default:
	    Fb = 0;
	    break;

	case COMBINE_B_OUT:
	    m = (uint64_t)combine_conjoint_out_part (da, (uint16_t) (sa >> 0));
	    n = (uint64_t)combine_conjoint_out_part (da, (uint16_t) (sa >> G_SHIFT)) << G_SHIFT;
	    o = (uint64_t)combine_conjoint_out_part (da, (uint16_t) (sa >> R_SHIFT)) << R_SHIFT;
	    p = (uint64_t)combine_conjoint_out_part (da, (uint16_t) (sa >> A_SHIFT)) << A_SHIFT;
	    Fb = m | n | o | p;
	    break;

	case COMBINE_B_IN:
	    m = (uint64_t)combine_conjoint_in_part (da, (uint16_t) (sa >> 0));
	    n = (uint64_t)combine_conjoint_in_part (da, (uint16_t) (sa >> G_SHIFT)) << G_SHIFT;
	    o = (uint64_t)combine_conjoint_in_part (da, (uint16_t) (sa >> R_SHIFT)) << R_SHIFT;
	    p = (uint64_t)combine_conjoint_in_part (da, (uint16_t) (sa >> A_SHIFT)) << A_SHIFT;
	    Fb = m | n | o | p;
	    break;

	case COMBINE_B:
	    Fb = ~0;
	    break;
	}
	m = GENERIC (s, d, 0, GET_COMP (Fa, 0), GET_COMP (Fb, 0), t, u, v);
	n = GENERIC (s, d, G_SHIFT, GET_COMP (Fa, G_SHIFT), GET_COMP (Fb, G_SHIFT), t, u, v);
	o = GENERIC (s, d, R_SHIFT, GET_COMP (Fa, R_SHIFT), GET_COMP (Fb, R_SHIFT), t, u, v);
	p = GENERIC (s, d, A_SHIFT, GET_COMP (Fa, A_SHIFT), GET_COMP (Fb, A_SHIFT), t, u, v);

	s = m | n | o | p;

	*(dest + i) = s;
    }
}

static void
combine_conjoint_over_ca (pixman_implementation_t *imp,
                          pixman_op_t              op,
                          uint64_t *                dest,
                          const uint64_t *          src,
                          const uint64_t *          mask,
                          int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_A_OVER);
}

static void
combine_conjoint_over_reverse_ca (pixman_implementation_t *imp,
                                  pixman_op_t              op,
                                  uint64_t *                dest,
                                  const uint64_t *          src,
                                  const uint64_t *          mask,
                                  int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_B_OVER);
}

static void
combine_conjoint_in_ca (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint64_t *                dest,
                        const uint64_t *          src,
                        const uint64_t *          mask,
                        int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_A_IN);
}

static void
combine_conjoint_in_reverse_ca (pixman_implementation_t *imp,
                                pixman_op_t              op,
                                uint64_t *                dest,
                                const uint64_t *          src,
                                const uint64_t *          mask,
                                int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_B_IN);
}

static void
combine_conjoint_out_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_A_OUT);
}

static void
combine_conjoint_out_reverse_ca (pixman_implementation_t *imp,
                                 pixman_op_t              op,
                                 uint64_t *                dest,
                                 const uint64_t *          src,
                                 const uint64_t *          mask,
                                 int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_B_OUT);
}

static void
combine_conjoint_atop_ca (pixman_implementation_t *imp,
                          pixman_op_t              op,
                          uint64_t *                dest,
                          const uint64_t *          src,
                          const uint64_t *          mask,
                          int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_A_ATOP);
}

static void
combine_conjoint_atop_reverse_ca (pixman_implementation_t *imp,
                                  pixman_op_t              op,
                                  uint64_t *                dest,
                                  const uint64_t *          src,
                                  const uint64_t *          mask,
                                  int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_B_ATOP);
}

static void
combine_conjoint_xor_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint64_t *                dest,
                         const uint64_t *          src,
                         const uint64_t *          mask,
                         int                      width)
{
    combine_conjoint_general_ca (dest, src, mask, width, COMBINE_XOR);
}

void
_pixman_setup_combiner_functions_64 (pixman_implementation_t *imp)
{
    
    imp->combine_64[PIXMAN_OP_CLEAR] = combine_clear;
    imp->combine_64[PIXMAN_OP_SRC] = combine_src_u;
    imp->combine_64[PIXMAN_OP_DST] = combine_dst;
    imp->combine_64[PIXMAN_OP_OVER] = combine_over_u;
    imp->combine_64[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_u;
    imp->combine_64[PIXMAN_OP_IN] = combine_in_u;
    imp->combine_64[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_u;
    imp->combine_64[PIXMAN_OP_OUT] = combine_out_u;
    imp->combine_64[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_u;
    imp->combine_64[PIXMAN_OP_ATOP] = combine_atop_u;
    imp->combine_64[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_u;
    imp->combine_64[PIXMAN_OP_XOR] = combine_xor_u;
    imp->combine_64[PIXMAN_OP_ADD] = combine_add_u;
    imp->combine_64[PIXMAN_OP_SATURATE] = combine_saturate_u;

    
    imp->combine_64[PIXMAN_OP_DISJOINT_CLEAR] = combine_clear;
    imp->combine_64[PIXMAN_OP_DISJOINT_SRC] = combine_src_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_DST] = combine_dst;
    imp->combine_64[PIXMAN_OP_DISJOINT_OVER] = combine_disjoint_over_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_OVER_REVERSE] = combine_saturate_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_IN] = combine_disjoint_in_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_IN_REVERSE] = combine_disjoint_in_reverse_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_OUT] = combine_disjoint_out_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_OUT_REVERSE] = combine_disjoint_out_reverse_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_ATOP] = combine_disjoint_atop_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_ATOP_REVERSE] = combine_disjoint_atop_reverse_u;
    imp->combine_64[PIXMAN_OP_DISJOINT_XOR] = combine_disjoint_xor_u;

    
    imp->combine_64[PIXMAN_OP_CONJOINT_CLEAR] = combine_clear;
    imp->combine_64[PIXMAN_OP_CONJOINT_SRC] = combine_src_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_DST] = combine_dst;
    imp->combine_64[PIXMAN_OP_CONJOINT_OVER] = combine_conjoint_over_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_OVER_REVERSE] = combine_conjoint_over_reverse_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_IN] = combine_conjoint_in_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_IN_REVERSE] = combine_conjoint_in_reverse_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_OUT] = combine_conjoint_out_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_OUT_REVERSE] = combine_conjoint_out_reverse_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_ATOP] = combine_conjoint_atop_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_ATOP_REVERSE] = combine_conjoint_atop_reverse_u;
    imp->combine_64[PIXMAN_OP_CONJOINT_XOR] = combine_conjoint_xor_u;

    imp->combine_64[PIXMAN_OP_MULTIPLY] = combine_multiply_u;
    imp->combine_64[PIXMAN_OP_SCREEN] = combine_screen_u;
    imp->combine_64[PIXMAN_OP_OVERLAY] = combine_overlay_u;
    imp->combine_64[PIXMAN_OP_DARKEN] = combine_darken_u;
    imp->combine_64[PIXMAN_OP_LIGHTEN] = combine_lighten_u;
    imp->combine_64[PIXMAN_OP_COLOR_DODGE] = combine_color_dodge_u;
    imp->combine_64[PIXMAN_OP_COLOR_BURN] = combine_color_burn_u;
    imp->combine_64[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_u;
    imp->combine_64[PIXMAN_OP_SOFT_LIGHT] = combine_soft_light_u;
    imp->combine_64[PIXMAN_OP_DIFFERENCE] = combine_difference_u;
    imp->combine_64[PIXMAN_OP_EXCLUSION] = combine_exclusion_u;
    imp->combine_64[PIXMAN_OP_HSL_HUE] = combine_hsl_hue_u;
    imp->combine_64[PIXMAN_OP_HSL_SATURATION] = combine_hsl_saturation_u;
    imp->combine_64[PIXMAN_OP_HSL_COLOR] = combine_hsl_color_u;
    imp->combine_64[PIXMAN_OP_HSL_LUMINOSITY] = combine_hsl_luminosity_u;

    
    imp->combine_64_ca[PIXMAN_OP_CLEAR] = combine_clear_ca;
    imp->combine_64_ca[PIXMAN_OP_SRC] = combine_src_ca;
    
    imp->combine_64_ca[PIXMAN_OP_OVER] = combine_over_ca;
    imp->combine_64_ca[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_IN] = combine_in_ca;
    imp->combine_64_ca[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_OUT] = combine_out_ca;
    imp->combine_64_ca[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_ATOP] = combine_atop_ca;
    imp->combine_64_ca[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_XOR] = combine_xor_ca;
    imp->combine_64_ca[PIXMAN_OP_ADD] = combine_add_ca;
    imp->combine_64_ca[PIXMAN_OP_SATURATE] = combine_saturate_ca;

    
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_CLEAR] = combine_clear_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_SRC] = combine_src_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_DST] = combine_dst;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_OVER] = combine_disjoint_over_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_OVER_REVERSE] = combine_saturate_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_IN] = combine_disjoint_in_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_IN_REVERSE] = combine_disjoint_in_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_OUT] = combine_disjoint_out_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_OUT_REVERSE] = combine_disjoint_out_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_ATOP] = combine_disjoint_atop_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_ATOP_REVERSE] = combine_disjoint_atop_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_DISJOINT_XOR] = combine_disjoint_xor_ca;

    
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_CLEAR] = combine_clear_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_SRC] = combine_src_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_DST] = combine_dst;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_OVER] = combine_conjoint_over_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_OVER_REVERSE] = combine_conjoint_over_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_IN] = combine_conjoint_in_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_IN_REVERSE] = combine_conjoint_in_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_OUT] = combine_conjoint_out_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_OUT_REVERSE] = combine_conjoint_out_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_ATOP] = combine_conjoint_atop_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_ATOP_REVERSE] = combine_conjoint_atop_reverse_ca;
    imp->combine_64_ca[PIXMAN_OP_CONJOINT_XOR] = combine_conjoint_xor_ca;

    imp->combine_64_ca[PIXMAN_OP_MULTIPLY] = combine_multiply_ca;
    imp->combine_64_ca[PIXMAN_OP_SCREEN] = combine_screen_ca;
    imp->combine_64_ca[PIXMAN_OP_OVERLAY] = combine_overlay_ca;
    imp->combine_64_ca[PIXMAN_OP_DARKEN] = combine_darken_ca;
    imp->combine_64_ca[PIXMAN_OP_LIGHTEN] = combine_lighten_ca;
    imp->combine_64_ca[PIXMAN_OP_COLOR_DODGE] = combine_color_dodge_ca;
    imp->combine_64_ca[PIXMAN_OP_COLOR_BURN] = combine_color_burn_ca;
    imp->combine_64_ca[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_ca;
    imp->combine_64_ca[PIXMAN_OP_SOFT_LIGHT] = combine_soft_light_ca;
    imp->combine_64_ca[PIXMAN_OP_DIFFERENCE] = combine_difference_ca;
    imp->combine_64_ca[PIXMAN_OP_EXCLUSION] = combine_exclusion_ca;

    
    imp->combine_64_ca[PIXMAN_OP_HSL_HUE] = combine_dst;
    imp->combine_64_ca[PIXMAN_OP_HSL_SATURATION] = combine_dst;
    imp->combine_64_ca[PIXMAN_OP_HSL_COLOR] = combine_dst;
    imp->combine_64_ca[PIXMAN_OP_HSL_LUMINOSITY] = combine_dst;
}

