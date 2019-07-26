









#include <assert.h>

#include "error_concealment.h"
#include "onyxd_int.h"
#include "decodemv.h"
#include "vpx_mem/vpx_mem.h"
#include "vp8/common/findnearmv.h"

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

#define FLOOR(x,q) ((x) & -(1 << (q)))

#define NUM_NEIGHBORS 20

typedef struct ec_position
{
    int row;
    int col;
} EC_POS;








static const int weights_q7[5][5] = {
       {  0,   128,    64,    43,    32 },
       {128,    91,    57,    40,    31 },
       { 64,    57,    45,    36,    29 },
       { 43,    40,    36,    30,    26 },
       { 32,    31,    29,    26,    23 }
};

int vp8_alloc_overlap_lists(VP8D_COMP *pbi)
{
    if (pbi->overlaps != NULL)
    {
        vpx_free(pbi->overlaps);
        pbi->overlaps = NULL;
    }

    pbi->overlaps = vpx_calloc(pbi->common.mb_rows * pbi->common.mb_cols,
                               sizeof(MB_OVERLAP));

    if (pbi->overlaps == NULL)
        return -1;

    return 0;
}

void vp8_de_alloc_overlap_lists(VP8D_COMP *pbi)
{
    vpx_free(pbi->overlaps);
    pbi->overlaps = NULL;
}


static void assign_overlap(OVERLAP_NODE* overlaps,
                           union b_mode_info *bmi,
                           int overlap)
{
    int i;
    if (overlap <= 0)
        return;
    

    for (i = 0; i < MAX_OVERLAPS; i++)
    {
        if (overlaps[i].bmi == NULL)
        {
            overlaps[i].bmi = bmi;
            overlaps[i].overlap = overlap;
            break;
        }
    }
}






static int block_overlap(int b1_row, int b1_col, int b2_row, int b2_col)
{
    const int int_top = MAX(b1_row, b2_row); 
    const int int_left = MAX(b1_col, b2_col); 
    


    const int int_right = MIN(b1_col + (4<<3), b2_col + (4<<3)); 
    const int int_bottom = MIN(b1_row + (4<<3), b2_row + (4<<3)); 
    return (int_bottom - int_top) * (int_right - int_left);
}







static void calculate_overlaps_mb(B_OVERLAP *b_overlaps, union b_mode_info *bmi,
                                  int new_row, int new_col,
                                  int mb_row, int mb_col,
                                  int first_blk_row, int first_blk_col)
{
    



    
    const int rel_ol_blk_row = first_blk_row - mb_row * 4;
    const int rel_ol_blk_col = first_blk_col - mb_col * 4;
    


    const int blk_idx = MAX(rel_ol_blk_row,0) * 4 + MAX(rel_ol_blk_col,0);
    
    B_OVERLAP *b_ol_ul = &(b_overlaps[blk_idx]);

    


    
    int end_row = MIN(4 + mb_row * 4 - first_blk_row, 2);
    int end_col = MIN(4 + mb_col * 4 - first_blk_col, 2);
    int row, col;

    


    if (new_row >= 0 && (new_row & 0x1F) == 0)
        end_row = 1;
    if (new_col >= 0 && (new_col & 0x1F) == 0)
        end_col = 1;

    


    if (new_row < (mb_row*16)<<3)
        end_row = 1;
    if (new_col < (mb_col*16)<<3)
        end_col = 1;

    for (row = 0; row < end_row; ++row)
    {
        for (col = 0; col < end_col; ++col)
        {
            
            const int overlap = block_overlap(new_row, new_col,
                                                  (((first_blk_row + row) *
                                                      4) << 3),
                                                  (((first_blk_col + col) *
                                                      4) << 3));
            assign_overlap(b_ol_ul[row * 4 + col].overlaps, bmi, overlap);
        }
    }
}

void vp8_calculate_overlaps(MB_OVERLAP *overlap_ul,
                            int mb_rows, int mb_cols,
                            union b_mode_info *bmi,
                            int b_row, int b_col)
{
    MB_OVERLAP *mb_overlap;
    int row, col, rel_row, rel_col;
    int new_row, new_col;
    int end_row, end_col;
    int overlap_b_row, overlap_b_col;
    int overlap_mb_row, overlap_mb_col;

    
    row = (4 * b_row) << 3; 
    col = (4 * b_col) << 3; 

    
    new_row = row - bmi->mv.as_mv.row;
    new_col = col - bmi->mv.as_mv.col;

    if (new_row >= ((16*mb_rows) << 3) || new_col >= ((16*mb_cols) << 3))
    {
        
        return;
    }

    if (new_row <= (-4 << 3) || new_col <= (-4 << 3))
    {
        
        return;
    }
    
    overlap_b_row = FLOOR(new_row / 4, 3) >> 3;
    overlap_b_col = FLOOR(new_col / 4, 3) >> 3;

    


    overlap_mb_row = FLOOR((overlap_b_row << 3) / 4, 3) >> 3;
    overlap_mb_col = FLOOR((overlap_b_col << 3) / 4, 3) >> 3;

    end_row = MIN(mb_rows - overlap_mb_row, 2);
    end_col = MIN(mb_cols - overlap_mb_col, 2);

    
    
    if (abs(new_row - ((16*overlap_mb_row) << 3)) < ((3*4) << 3))
        end_row = 1;
    
    if (abs(new_col - ((16*overlap_mb_col) << 3)) < ((3*4) << 3))
        end_col = 1;

    
    for (rel_row = 0; rel_row < end_row; ++rel_row)
    {
        for (rel_col = 0; rel_col < end_col; ++rel_col)
        {
            if (overlap_mb_row + rel_row < 0 ||
                overlap_mb_col + rel_col < 0)
                continue;
            mb_overlap = overlap_ul + (overlap_mb_row + rel_row) * mb_cols +
                 overlap_mb_col + rel_col;

            calculate_overlaps_mb(mb_overlap->overlaps, bmi,
                                  new_row, new_col,
                                  overlap_mb_row + rel_row,
                                  overlap_mb_col + rel_col,
                                  overlap_b_row + rel_row,
                                  overlap_b_col + rel_col);
        }
    }
}





static void estimate_mv(const OVERLAP_NODE *overlaps, union b_mode_info *bmi)
{
    int i;
    int overlap_sum = 0;
    int row_acc = 0;
    int col_acc = 0;

    bmi->mv.as_int = 0;
    for (i=0; i < MAX_OVERLAPS; ++i)
    {
        if (overlaps[i].bmi == NULL)
            break;
        col_acc += overlaps[i].overlap * overlaps[i].bmi->mv.as_mv.col;
        row_acc += overlaps[i].overlap * overlaps[i].bmi->mv.as_mv.row;
        overlap_sum += overlaps[i].overlap;
    }
    if (overlap_sum > 0)
    {
        
        bmi->mv.as_mv.col = col_acc / overlap_sum;
        bmi->mv.as_mv.row = row_acc / overlap_sum;
    }
    else
    {
        bmi->mv.as_mv.col = 0;
        bmi->mv.as_mv.row = 0;
    }
}




static void estimate_mb_mvs(const B_OVERLAP *block_overlaps,
                            MODE_INFO *mi,
                            int mb_to_left_edge,
                            int mb_to_right_edge,
                            int mb_to_top_edge,
                            int mb_to_bottom_edge)
{
    int row, col;
    int non_zero_count = 0;
    MV * const filtered_mv = &(mi->mbmi.mv.as_mv);
    union b_mode_info * const bmi = mi->bmi;
    filtered_mv->col = 0;
    filtered_mv->row = 0;
    mi->mbmi.need_to_clamp_mvs = 0;
    for (row = 0; row < 4; ++row)
    {
        int this_b_to_top_edge = mb_to_top_edge + ((row*4)<<3);
        int this_b_to_bottom_edge = mb_to_bottom_edge - ((row*4)<<3);
        for (col = 0; col < 4; ++col)
        {
            int i = row * 4 + col;
            int this_b_to_left_edge = mb_to_left_edge + ((col*4)<<3);
            int this_b_to_right_edge = mb_to_right_edge - ((col*4)<<3);
            
            
            estimate_mv(block_overlaps[i].overlaps, &(bmi[i]));
            mi->mbmi.need_to_clamp_mvs |= vp8_check_mv_bounds(
                                                         &bmi[i].mv,
                                                         this_b_to_left_edge,
                                                         this_b_to_right_edge,
                                                         this_b_to_top_edge,
                                                         this_b_to_bottom_edge);
            if (bmi[i].mv.as_int != 0)
            {
                ++non_zero_count;
                filtered_mv->col += bmi[i].mv.as_mv.col;
                filtered_mv->row += bmi[i].mv.as_mv.row;
            }
        }
    }
    if (non_zero_count > 0)
    {
        filtered_mv->col /= non_zero_count;
        filtered_mv->row /= non_zero_count;
    }
}

static void calc_prev_mb_overlaps(MB_OVERLAP *overlaps, MODE_INFO *prev_mi,
                                    int mb_row, int mb_col,
                                    int mb_rows, int mb_cols)
{
    int sub_row;
    int sub_col;
    for (sub_row = 0; sub_row < 4; ++sub_row)
    {
        for (sub_col = 0; sub_col < 4; ++sub_col)
        {
            vp8_calculate_overlaps(
                                overlaps, mb_rows, mb_cols,
                                &(prev_mi->bmi[sub_row * 4 + sub_col]),
                                4 * mb_row + sub_row,
                                4 * mb_col + sub_col);
        }
    }
}



static void estimate_missing_mvs(MB_OVERLAP *overlaps,
                                 MODE_INFO *mi, MODE_INFO *prev_mi,
                                 int mb_rows, int mb_cols,
                                 unsigned int first_corrupt)
{
    int mb_row, mb_col;
    vpx_memset(overlaps, 0, sizeof(MB_OVERLAP) * mb_rows * mb_cols);
    
    for (mb_row = 0; mb_row < mb_rows; ++mb_row)
    {
        for (mb_col = 0; mb_col < mb_cols; ++mb_col)
        {
            


            if (prev_mi->mbmi.ref_frame == LAST_FRAME)
            {
                calc_prev_mb_overlaps(overlaps, prev_mi,
                                      mb_row, mb_col,
                                      mb_rows, mb_cols);
            }
            ++prev_mi;
        }
        ++prev_mi;
    }

    mb_row = first_corrupt / mb_cols;
    mb_col = first_corrupt - mb_row * mb_cols;
    mi += mb_row*(mb_cols + 1) + mb_col;
    


    for (; mb_row < mb_rows; ++mb_row)
    {
        int mb_to_top_edge = -((mb_row * 16)) << 3;
        int mb_to_bottom_edge = ((mb_rows - 1 - mb_row) * 16) << 3;
        for (; mb_col < mb_cols; ++mb_col)
        {
            int mb_to_left_edge = -((mb_col * 16) << 3);
            int mb_to_right_edge = ((mb_cols - 1 - mb_col) * 16) << 3;
            const B_OVERLAP *block_overlaps =
                    overlaps[mb_row*mb_cols + mb_col].overlaps;
            mi->mbmi.ref_frame = LAST_FRAME;
            mi->mbmi.mode = SPLITMV;
            mi->mbmi.uv_mode = DC_PRED;
            mi->mbmi.partitioning = 3;
            mi->mbmi.segment_id = 0;
            estimate_mb_mvs(block_overlaps,
                            mi,
                            mb_to_left_edge,
                            mb_to_right_edge,
                            mb_to_top_edge,
                            mb_to_bottom_edge);
            ++mi;
        }
        mb_col = 0;
        ++mi;
    }
}

void vp8_estimate_missing_mvs(VP8D_COMP *pbi)
{
    VP8_COMMON * const pc = &pbi->common;
    estimate_missing_mvs(pbi->overlaps,
                         pc->mi, pc->prev_mi,
                         pc->mb_rows, pc->mb_cols,
                         pbi->mvs_corrupt_from_mb);
}

static void assign_neighbor(EC_BLOCK *neighbor, MODE_INFO *mi, int block_idx)
{
    assert(mi->mbmi.ref_frame < MAX_REF_FRAMES);
    neighbor->ref_frame = mi->mbmi.ref_frame;
    neighbor->mv = mi->bmi[block_idx].mv.as_mv;
}









static void find_neighboring_blocks(MODE_INFO *mi,
                                    EC_BLOCK *neighbors,
                                    int mb_row, int mb_col,
                                    int mb_rows, int mb_cols,
                                    int mi_stride)
{
    int i = 0;
    int j;
    if (mb_row > 0)
    {
        
        if (mb_col > 0)
            assign_neighbor(&neighbors[i], mi - mi_stride - 1, 15);
        ++i;
        
        for (j = 12; j < 16; ++j, ++i)
            assign_neighbor(&neighbors[i], mi - mi_stride, j);
    }
    else
        i += 5;
    if (mb_col < mb_cols - 1)
    {
        
        if (mb_row > 0)
            assign_neighbor(&neighbors[i], mi - mi_stride + 1, 12);
        ++i;
        
        for (j = 0; j <= 12; j += 4, ++i)
            assign_neighbor(&neighbors[i], mi + 1, j);
    }
    else
        i += 5;
    if (mb_row < mb_rows - 1)
    {
        
        if (mb_col < mb_cols - 1)
            assign_neighbor(&neighbors[i], mi + mi_stride + 1, 0);
        ++i;
        
        for (j = 0; j < 4; ++j, ++i)
            assign_neighbor(&neighbors[i], mi + mi_stride, j);
    }
    else
        i += 5;
    if (mb_col > 0)
    {
        
        if (mb_row < mb_rows - 1)
            assign_neighbor(&neighbors[i], mi + mi_stride - 1, 4);
        ++i;
        
        for (j = 3; j < 16; j += 4, ++i)
        {
            assign_neighbor(&neighbors[i], mi - 1, j);
        }
    }
    else
        i += 5;
    assert(i == 20);
}




static void interpolate_mvs(MACROBLOCKD *mb,
                         EC_BLOCK *neighbors,
                         MV_REFERENCE_FRAME dom_ref_frame)
{
    int row, col, i;
    MODE_INFO * const mi = mb->mode_info_context;
    



    const EC_POS neigh_pos[NUM_NEIGHBORS] = {
                                        {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3},
                                        {-1,4}, {0,4}, {1,4}, {2,4}, {3,4},
                                        {4,4}, {4,3}, {4,2}, {4,1}, {4,0},
                                        {4,-1}, {3,-1}, {2,-1}, {1,-1}, {0,-1}
                                      };
    mi->mbmi.need_to_clamp_mvs = 0;
    for (row = 0; row < 4; ++row)
    {
        int mb_to_top_edge = mb->mb_to_top_edge + ((row*4)<<3);
        int mb_to_bottom_edge = mb->mb_to_bottom_edge - ((row*4)<<3);
        for (col = 0; col < 4; ++col)
        {
            int mb_to_left_edge = mb->mb_to_left_edge + ((col*4)<<3);
            int mb_to_right_edge = mb->mb_to_right_edge - ((col*4)<<3);
            int w_sum = 0;
            int mv_row_sum = 0;
            int mv_col_sum = 0;
            int_mv * const mv = &(mi->bmi[row*4 + col].mv);
            mv->as_int = 0;
            for (i = 0; i < NUM_NEIGHBORS; ++i)
            {
                


                const int w = weights_q7[abs(row - neigh_pos[i].row)]
                                        [abs(col - neigh_pos[i].col)];
                if (neighbors[i].ref_frame != dom_ref_frame)
                    continue;
                w_sum += w;
                
                mv_row_sum += w*neighbors[i].mv.row;
                mv_col_sum += w*neighbors[i].mv.col;
            }
            if (w_sum > 0)
            {
                



                mv->as_mv.row = mv_row_sum / w_sum;
                mv->as_mv.col = mv_col_sum / w_sum;
                mi->mbmi.need_to_clamp_mvs |= vp8_check_mv_bounds(
                                                            mv,
                                                            mb_to_left_edge,
                                                            mb_to_right_edge,
                                                            mb_to_top_edge,
                                                            mb_to_bottom_edge);
            }
        }
    }
}

void vp8_interpolate_motion(MACROBLOCKD *mb,
                        int mb_row, int mb_col,
                        int mb_rows, int mb_cols,
                        int mi_stride)
{
    
    EC_BLOCK neighbors[NUM_NEIGHBORS];
    int i;
    
    for (i = 0; i < NUM_NEIGHBORS; ++i)
    {
        neighbors[i].ref_frame = MAX_REF_FRAMES;
        neighbors[i].mv.row = neighbors[i].mv.col = 0;
    }
    find_neighboring_blocks(mb->mode_info_context,
                                neighbors,
                                mb_row, mb_col,
                                mb_rows, mb_cols,
                                mb->mode_info_stride);
    

    interpolate_mvs(mb, neighbors, LAST_FRAME);

    mb->mode_info_context->mbmi.ref_frame = LAST_FRAME;
    mb->mode_info_context->mbmi.mode = SPLITMV;
    mb->mode_info_context->mbmi.uv_mode = DC_PRED;
    mb->mode_info_context->mbmi.partitioning = 3;
    mb->mode_info_context->mbmi.segment_id = 0;
}

void vp8_conceal_corrupt_mb(MACROBLOCKD *xd)
{
    


    


}
