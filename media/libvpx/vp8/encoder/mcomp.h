










#ifndef __INC_MCOMP_H
#define __INC_MCOMP_H

#include "block.h"
#include "vp8/common/variance.h"

#ifdef VP8_ENTROPY_STATS
extern void init_mv_ref_counts();
extern void accum_mv_refs(MB_PREDICTION_MODE, const int near_mv_ref_cts[4]);
#endif





#define MAX_MVSEARCH_STEPS 8


#define MAX_FULL_PEL_VAL ((1 << (MAX_MVSEARCH_STEPS)) - 1)


#define MAX_FIRST_STEP (1 << (MAX_MVSEARCH_STEPS-1))

extern void print_mode_context(void);
extern int vp8_mv_bit_cost(int_mv *mv, int_mv *ref, int *mvcost[2], int Weight);
extern void vp8_init_dsmotion_compensation(MACROBLOCK *x, int stride);
extern void vp8_init3smotion_compensation(MACROBLOCK *x,  int stride);


extern int vp8_hex_search
(
    MACROBLOCK *x,
    BLOCK *b,
    BLOCKD *d,
    int_mv *ref_mv,
    int_mv *best_mv,
    int search_param,
    int error_per_bit,
    const vp8_variance_fn_ptr_t *vf,
    int *mvsadcost[2],
    int *mvcost[2],
    int_mv *center_mv
);

typedef int (fractional_mv_step_fp)
    (MACROBLOCK *x, BLOCK *b, BLOCKD *d, int_mv *bestmv, int_mv *ref_mv,
     int error_per_bit, const vp8_variance_fn_ptr_t *vfp, int *mvcost[2],
     int *distortion, unsigned int *sse);

extern fractional_mv_step_fp vp8_find_best_sub_pixel_step_iteratively;
extern fractional_mv_step_fp vp8_find_best_sub_pixel_step;
extern fractional_mv_step_fp vp8_find_best_half_pixel_step;
extern fractional_mv_step_fp vp8_skip_fractional_mv_step;

typedef int (*vp8_full_search_fn_t)
    (
     MACROBLOCK *x,
     BLOCK *b,
     BLOCKD *d,
     int_mv *ref_mv,
     int sad_per_bit,
     int distance,
     vp8_variance_fn_ptr_t *fn_ptr,
     int *mvcost[2],
     int_mv *center_mv
    );

typedef int (*vp8_refining_search_fn_t)
    (
     MACROBLOCK *x,
     BLOCK *b,
     BLOCKD *d,
     int_mv *ref_mv,
     int sad_per_bit,
     int distance,
     vp8_variance_fn_ptr_t *fn_ptr,
     int *mvcost[2],
     int_mv *center_mv
    );

typedef int (*vp8_diamond_search_fn_t)
    (
     MACROBLOCK *x,
     BLOCK *b,
     BLOCKD *d,
     int_mv *ref_mv,
     int_mv *best_mv,
     int search_param,
     int sad_per_bit,
     int *num00,
     vp8_variance_fn_ptr_t *fn_ptr,
     int *mvcost[2],
     int_mv *center_mv
    );

#endif
