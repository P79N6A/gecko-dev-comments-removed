
























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"
#include "pixman-arm-common.h"

#if 0 

void
pixman_composite_add_8_8_asm_armv6 (int32_t  width,
				    int32_t  height,
				    uint8_t *dst_line,
				    int32_t  dst_stride,
				    uint8_t *src_line,
				    int32_t  src_stride)
{
    uint8_t *dst, *src;
    int32_t w;
    uint8_t s, d;

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;

	


	while (w && (((unsigned long)dst & 3) || ((unsigned long)src & 3)))
	{
	    s = *src;
	    d = *dst;
	    asm ("uqadd8 %0, %1, %2" : "+r" (d) : "r" (s));
	    *dst = d;

	    dst++;
	    src++;
	    w--;
	}

	while (w >= 4)
	{
	    asm ("uqadd8 %0, %1, %2"
		 : "=r" (*(uint32_t*)dst)
		 : "r" (*(uint32_t*)src), "r" (*(uint32_t*)dst));
	    dst += 4;
	    src += 4;
	    w -= 4;
	}

	while (w)
	{
	    s = *src;
	    d = *dst;
	    asm ("uqadd8 %0, %1, %2" : "+r" (d) : "r" (s));
	    *dst = d;

	    dst++;
	    src++;
	    w--;
	}
    }

}

void
pixman_composite_over_8888_8888_asm_armv6 (int32_t   width,
                                           int32_t   height,
                                           uint32_t *dst_line,
                                           int32_t   dst_stride,
                                           uint32_t *src_line,
                                           int32_t   src_stride)
{
    uint32_t    *dst;
    uint32_t    *src;
    int32_t w;
    uint32_t component_half = 0x800080;
    uint32_t upper_component_mask = 0xff00ff00;
    uint32_t alpha_mask = 0xff;

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;


	asm volatile (
	    "cmp %[w], #0\n\t"
	    "beq 2f\n\t"
	    "1:\n\t"
	    
	    "ldr r5, [%[src]], #4\n\t"
#ifdef inner_branch
	    



	    "cmp r5, #0\n\t"
	    "beq 3f\n\t"

	    
	    "sub r8, %[alpha_mask], r5, lsr #24\n\t"

	    "ldr r4, [%[dest]] \n\t"

#else
	    "ldr r4, [%[dest]] \n\t"

	    
	    "sub r8, %[alpha_mask], r5, lsr #24\n\t"
#endif
	    "uxtb16 r6, r4\n\t"
	    "uxtb16 r7, r4, ror #8\n\t"

	    
	    "mla r6, r6, r8, %[component_half]\n\t"
	    "mla r7, r7, r8, %[component_half]\n\t"

	    "uxtab16 r6, r6, r6, ror #8\n\t"
	    "uxtab16 r7, r7, r7, ror #8\n\t"

	    
	    "and r7, r7, %[upper_component_mask]\n\t"
	    "uxtab16 r6, r7, r6, ror #8\n\t"

	    "uqadd8 r5, r6, r5\n\t"

#ifdef inner_branch
	    "3:\n\t"

#endif
	    "str r5, [%[dest]], #4\n\t"
	    
	    "subs	%[w], %[w], #1\n\t"
	    "bne	1b\n\t"
	    "2:\n\t"
	    : [w] "+r" (w), [dest] "+r" (dst), [src] "+r" (src)
	    : [component_half] "r" (component_half), [upper_component_mask] "r" (upper_component_mask),
	      [alpha_mask] "r" (alpha_mask)
	    : "r4", "r5", "r6", "r7", "r8", "cc", "memory"
	    );
    }
}

void
pixman_composite_over_8888_n_8888_asm_armv6 (int32_t   width,
                                             int32_t   height,
                                             uint32_t *dst_line,
                                             int32_t   dst_stride,
                                             uint32_t *src_line,
                                             int32_t   src_stride,
                                             uint32_t  mask)
{
    uint32_t *dst;
    uint32_t *src;
    int32_t w;
    uint32_t component_half = 0x800080;
    uint32_t alpha_mask = 0xff;

    mask = (mask) >> 24;

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	src = src_line;
	src_line += src_stride;
	w = width;


	asm volatile (
	    "cmp %[w], #0\n\t"
	    "beq 2f\n\t"
	    "1:\n\t"
	    
	    "ldr r5, [%[src]], #4\n\t"
#ifdef inner_branch
	    



	    "cmp r5, #0\n\t"
	    "beq 3f\n\t"

#endif
	    "ldr r4, [%[dest]] \n\t"

	    "uxtb16 r6, r5\n\t"
	    "uxtb16 r7, r5, ror #8\n\t"

	    
	    "mla r6, r6, %[mask_alpha], %[component_half]\n\t"
	    "mla r7, r7, %[mask_alpha], %[component_half]\n\t"

	    "uxtab16 r6, r6, r6, ror #8\n\t"
	    "uxtab16 r7, r7, r7, ror #8\n\t"

	    "uxtb16 r6, r6, ror #8\n\t"
	    "uxtb16 r7, r7, ror #8\n\t"

	    
	    "orr r5, r6, r7, lsl #8\n\t"

	    "uxtb16 r6, r4\n\t"
	    "uxtb16 r7, r4, ror #8\n\t"

	    
	    "sub r8, %[alpha_mask], r5, lsr #24\n\t"

	    
	    "mla r6, r6, r8, %[component_half]\n\t"
	    "mla r7, r7, r8, %[component_half]\n\t"

	    "uxtab16 r6, r6, r6, ror #8\n\t"
	    "uxtab16 r7, r7, r7, ror #8\n\t"

	    "uxtb16 r6, r6, ror #8\n\t"
	    "uxtb16 r7, r7, ror #8\n\t"

	    
	    "orr r6, r6, r7, lsl #8\n\t"

	    "uqadd8 r5, r6, r5\n\t"

#ifdef inner_branch
	    "3:\n\t"

#endif
	    "str r5, [%[dest]], #4\n\t"
	    
	    "subs	%[w], %[w], #1\n\t"
	    "bne	1b\n\t"
	    "2:\n\t"
	    : [w] "+r" (w), [dest] "+r" (dst), [src] "+r" (src)
	    : [component_half] "r" (component_half), [mask_alpha] "r" (mask),
	      [alpha_mask] "r" (alpha_mask)
	    : "r4", "r5", "r6", "r7", "r8", "r9", "cc", "memory"
	    );
    }
}

void
pixman_composite_over_n_8_8888_asm_armv6 (int32_t   width,
                                          int32_t   height,
                                          uint32_t *dst_line,
                                          int32_t   dst_stride,
                                          uint32_t  src,
                                          int32_t   unused,
                                          uint8_t  *mask_line,
                                          int32_t   mask_stride)
{
    uint32_t  srca;
    uint32_t *dst;
    uint8_t  *mask;
    int32_t w;

    srca = src >> 24;

    uint32_t component_mask = 0xff00ff;
    uint32_t component_half = 0x800080;

    uint32_t src_hi = (src >> 8) & component_mask;
    uint32_t src_lo = src & component_mask;

    while (height--)
    {
	dst = dst_line;
	dst_line += dst_stride;
	mask = mask_line;
	mask_line += mask_stride;
	w = width;


	asm volatile (
	    "cmp %[w], #0\n\t"
	    "beq 2f\n\t"
	    "1:\n\t"
	    
	    "ldrb r5, [%[mask]], #1\n\t"
#ifdef inner_branch
	    



	    "cmp r5, #0\n\t"
	    "beq 3f\n\t"

#endif
	    "ldr r4, [%[dest]] \n\t"

	    
	    "mla r6, %[src_lo], r5, %[component_half]\n\t"
	    "mla r7, %[src_hi], r5, %[component_half]\n\t"

	    "uxtab16 r6, r6, r6, ror #8\n\t"
	    "uxtab16 r7, r7, r7, ror #8\n\t"

	    "uxtb16 r6, r6, ror #8\n\t"
	    "uxtb16 r7, r7, ror #8\n\t"

	    
	    "orr r5, r6, r7, lsl #8\n\t"

	    "uxtb16 r6, r4\n\t"
	    "uxtb16 r7, r4, ror #8\n\t"

	    


	    "mvn r8, r5\n\t"
	    "mov r8, r8, lsr #24\n\t"

	    
	    "mla r6, r6, r8, %[component_half]\n\t"
	    "mla r7, r7, r8, %[component_half]\n\t"

	    "uxtab16 r6, r6, r6, ror #8\n\t"
	    "uxtab16 r7, r7, r7, ror #8\n\t"

	    "uxtb16 r6, r6, ror #8\n\t"
	    "uxtb16 r7, r7, ror #8\n\t"

	    
	    "orr r6, r6, r7, lsl #8\n\t"

	    "uqadd8 r5, r6, r5\n\t"

#ifdef inner_branch
	    "3:\n\t"

#endif
	    "str r5, [%[dest]], #4\n\t"
	    
	    "subs	%[w], %[w], #1\n\t"
	    "bne	1b\n\t"
	    "2:\n\t"
	    : [w] "+r" (w), [dest] "+r" (dst), [src] "+r" (src), [mask] "+r" (mask)
	    : [component_half] "r" (component_half),
	      [src_hi] "r" (src_hi), [src_lo] "r" (src_lo)
	    : "r4", "r5", "r6", "r7", "r8", "cc", "memory");
    }
}

#endif

PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, add_8_8,
                                   uint8_t, 1, uint8_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, over_8888_8888,
                                   uint32_t, 1, uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST (armv6, over_8888_n_8888,
                                     uint32_t, 1, uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST (armv6, over_n_8_8888,
                                      uint8_t, 1, uint32_t, 1)

static const pixman_fast_path_t arm_simd_fast_paths[] =
{
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, a8r8g8b8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, x8r8g8b8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, a8b8g8r8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, x8b8g8r8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid, a8r8g8b8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid, x8r8g8b8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, solid, a8b8g8r8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, solid, x8b8g8r8, armv6_composite_over_8888_n_8888),

    PIXMAN_STD_FAST_PATH (ADD, a8, null, a8, armv6_composite_add_8_8),

    PIXMAN_STD_FAST_PATH (OVER, solid, a8, a8r8g8b8, armv6_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, x8r8g8b8, armv6_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, a8b8g8r8, armv6_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, x8b8g8r8, armv6_composite_over_n_8_8888),

    { PIXMAN_OP_NONE },
};

pixman_implementation_t *
_pixman_implementation_create_arm_simd (void)
{
    pixman_implementation_t *general = _pixman_implementation_create_fast_path ();
    pixman_implementation_t *imp = _pixman_implementation_create (general, arm_simd_fast_paths);

    return imp;
}
