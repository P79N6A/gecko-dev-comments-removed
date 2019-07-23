
























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"
#include "pixman-arm-simd-asm.h"

static const pixman_fast_path_t arm_simd_fast_path_array[] =
{
    { PIXMAN_OP_OVER, PIXMAN_a8r8g8b8, PIXMAN_null,     PIXMAN_a8r8g8b8, arm_composite_over_8888_8888    },
    { PIXMAN_OP_OVER, PIXMAN_a8r8g8b8, PIXMAN_null,     PIXMAN_x8r8g8b8, arm_composite_over_8888_8888    },
    { PIXMAN_OP_OVER, PIXMAN_a8b8g8r8, PIXMAN_null,     PIXMAN_a8b8g8r8, arm_composite_over_8888_8888    },
    { PIXMAN_OP_OVER, PIXMAN_a8b8g8r8, PIXMAN_null,     PIXMAN_x8b8g8r8, arm_composite_over_8888_8888    },
    { PIXMAN_OP_OVER, PIXMAN_a8r8g8b8, PIXMAN_solid,    PIXMAN_a8r8g8b8, arm_composite_over_8888_n_8888  },
    { PIXMAN_OP_OVER, PIXMAN_a8r8g8b8, PIXMAN_solid,    PIXMAN_x8r8g8b8, arm_composite_over_8888_n_8888  },

    { PIXMAN_OP_ADD, PIXMAN_a8,        PIXMAN_null,     PIXMAN_a8,       arm_composite_add_8000_8000     },

    { PIXMAN_OP_OVER, PIXMAN_solid,    PIXMAN_a8,       PIXMAN_a8r8g8b8, arm_composite_over_n_8_8888     },
    { PIXMAN_OP_OVER, PIXMAN_solid,    PIXMAN_a8,       PIXMAN_x8r8g8b8, arm_composite_over_n_8_8888     },
    { PIXMAN_OP_OVER, PIXMAN_solid,    PIXMAN_a8,       PIXMAN_a8b8g8r8, arm_composite_over_n_8_8888     },
    { PIXMAN_OP_OVER, PIXMAN_solid,    PIXMAN_a8,       PIXMAN_x8b8g8r8, arm_composite_over_n_8_8888     },

    { PIXMAN_OP_SRC,  PIXMAN_x8r8g8b8, PIXMAN_null,     PIXMAN_r5g6b5,   arm_composite_src_8888_0565    },
    { PIXMAN_OP_SRC,  PIXMAN_x8r8g8b8, PIXMAN_null,     PIXMAN_b5g6r5,   arm_composite_src_8888_0565    },

    { PIXMAN_OP_NONE },
};

const pixman_fast_path_t *const arm_simd_fast_paths = arm_simd_fast_path_array;

static void
arm_simd_composite (pixman_implementation_t *imp,
                    pixman_op_t              op,
                    pixman_image_t *         src,
                    pixman_image_t *         mask,
                    pixman_image_t *         dest,
                    int32_t                  src_x,
                    int32_t                  src_y,
                    int32_t                  mask_x,
                    int32_t                  mask_y,
                    int32_t                  dest_x,
                    int32_t                  dest_y,
                    int32_t                  width,
                    int32_t                  height)
{
    if (_pixman_run_fast_path (arm_simd_fast_paths, imp,
                               op, src, mask, dest,
                               src_x, src_y,
                               mask_x, mask_y,
                               dest_x, dest_y,
                               width, height))
    {
	return;
    }

    _pixman_implementation_composite (imp->delegate, op,
                                      src, mask, dest,
                                      src_x, src_y,
                                      mask_x, mask_y,
                                      dest_x, dest_y,
                                      width, height);
}

pixman_implementation_t *
_pixman_implementation_create_arm_simd (void)
{
    pixman_implementation_t *general = _pixman_implementation_create_fast_path ();
    pixman_implementation_t *imp = _pixman_implementation_create (general);

    imp->composite = arm_simd_composite;

    return imp;
}
