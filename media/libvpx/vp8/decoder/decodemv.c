










#include "treereader.h"
#include "vp8/common/entropymv.h"
#include "vp8/common/entropymode.h"
#include "onyxd_int.h"
#include "vp8/common/findnearmv.h"

#if CONFIG_DEBUG
#include <assert.h>
#endif
static B_PREDICTION_MODE read_bmode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_bmode_tree, p);

    return (B_PREDICTION_MODE)i;
}

static MB_PREDICTION_MODE read_ymode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_ymode_tree, p);

    return (MB_PREDICTION_MODE)i;
}

static MB_PREDICTION_MODE read_kf_ymode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_kf_ymode_tree, p);

    return (MB_PREDICTION_MODE)i;
}

static MB_PREDICTION_MODE read_uv_mode(vp8_reader *bc, const vp8_prob *p)
{
    const int i = vp8_treed_read(bc, vp8_uv_mode_tree, p);

    return (MB_PREDICTION_MODE)i;
}

static void read_kf_modes(VP8D_COMP *pbi, MODE_INFO *mi)
{
    vp8_reader *const bc = & pbi->mbc[8];
    const int mis = pbi->common.mode_info_stride;

    mi->mbmi.ref_frame = INTRA_FRAME;
    mi->mbmi.mode = read_kf_ymode(bc, vp8_kf_ymode_prob);

    if (mi->mbmi.mode == B_PRED)
    {
        int i = 0;
        mi->mbmi.is_4x4 = 1;

        do
        {
            const B_PREDICTION_MODE A = above_block_mode(mi, i, mis);
            const B_PREDICTION_MODE L = left_block_mode(mi, i);

            mi->bmi[i].as_mode =
                read_bmode(bc, vp8_kf_bmode_prob [A] [L]);
        }
        while (++i < 16);
    }

    mi->mbmi.uv_mode = read_uv_mode(bc, vp8_kf_uv_mode_prob);
}

static int read_mvcomponent(vp8_reader *r, const MV_CONTEXT *mvc)
{
    const vp8_prob *const p = (const vp8_prob *) mvc;
    int x = 0;

    if (vp8_read(r, p [mvpis_short]))  
    {
        int i = 0;

        do
        {
            x += vp8_read(r, p [MVPbits + i]) << i;
        }
        while (++i < 3);

        i = mvlong_width - 1;  

        do
        {
            x += vp8_read(r, p [MVPbits + i]) << i;
        }
        while (--i > 3);

        if (!(x & 0xFFF0)  ||  vp8_read(r, p [MVPbits + 3]))
            x += 8;
    }
    else   
        x = vp8_treed_read(r, vp8_small_mvtree, p + MVPshort);

    if (x  &&  vp8_read(r, p [MVPsign]))
        x = -x;

    return x;
}

static void read_mv(vp8_reader *r, MV *mv, const MV_CONTEXT *mvc)
{
    mv->row = (short)(read_mvcomponent(r,   mvc) * 2);
    mv->col = (short)(read_mvcomponent(r, ++mvc) * 2);
}


static void read_mvcontexts(vp8_reader *bc, MV_CONTEXT *mvc)
{
    int i = 0;

    do
    {
        const vp8_prob *up = vp8_mv_update_probs[i].prob;
        vp8_prob *p = (vp8_prob *)(mvc + i);
        vp8_prob *const pstop = p + MVPcount;

        do
        {
            if (vp8_read(bc, *up++))
            {
                const vp8_prob x = (vp8_prob)vp8_read_literal(bc, 7);

                *p = x ? x << 1 : 1;
            }
        }
        while (++p < pstop);
    }
    while (++i < 2);
}

static const unsigned char mbsplit_fill_count[4] = {8, 8, 4, 1};
static const unsigned char mbsplit_fill_offset[4][16] = {
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15},
    { 0,  1,  4,  5,  8,  9, 12, 13,  2,  3,   6,  7, 10, 11, 14, 15},
    { 0,  1,  4,  5,  2,  3,  6,  7,  8,  9,  12, 13, 10, 11, 14, 15},
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15}
};


static void mb_mode_mv_init(VP8D_COMP *pbi)
{
    vp8_reader *const bc = & pbi->mbc[8];
    MV_CONTEXT *const mvc = pbi->common.fc.mvc;

#if CONFIG_ERROR_CONCEALMENT
    


    pbi->mvs_corrupt_from_mb = UINT_MAX;
#endif
    
    pbi->common.mb_no_coeff_skip = (int)vp8_read_bit(bc);

    pbi->prob_skip_false = 0;
    if (pbi->common.mb_no_coeff_skip)
        pbi->prob_skip_false = (vp8_prob)vp8_read_literal(bc, 8);

    if(pbi->common.frame_type != KEY_FRAME)
    {
        pbi->prob_intra = (vp8_prob)vp8_read_literal(bc, 8);
        pbi->prob_last  = (vp8_prob)vp8_read_literal(bc, 8);
        pbi->prob_gf    = (vp8_prob)vp8_read_literal(bc, 8);

        if (vp8_read_bit(bc))
        {
            int i = 0;

            do
            {
                pbi->common.fc.ymode_prob[i] =
                    (vp8_prob) vp8_read_literal(bc, 8);
            }
            while (++i < 4);
        }

        if (vp8_read_bit(bc))
        {
            int i = 0;

            do
            {
                pbi->common.fc.uv_mode_prob[i] =
                    (vp8_prob) vp8_read_literal(bc, 8);
            }
            while (++i < 3);
        }

        read_mvcontexts(bc, mvc);
    }
}

const vp8_prob vp8_sub_mv_ref_prob3 [8][VP8_SUBMVREFS-1] =
{
    { 147, 136, 18 },   
    { 223, 1  , 34 },   
    { 106, 145, 1  },   
    { 208, 1  , 1  },   
    { 179, 121, 1  },   
    { 223, 1  , 34 },   
    { 179, 121, 1  },   
    { 208, 1  , 1  }    
};

static
const vp8_prob * get_sub_mv_ref_prob(const int left, const int above)
{
    int lez = (left == 0);
    int aez = (above == 0);
    int lea = (left == above);
    const vp8_prob * prob;

    prob = vp8_sub_mv_ref_prob3[(aez << 2) |
                                (lez << 1) |
                                (lea)];

    return prob;
}

static void decode_split_mv(vp8_reader *const bc, MODE_INFO *mi,
                        const MODE_INFO *left_mb, const MODE_INFO *above_mb,
                        MB_MODE_INFO *mbmi, int_mv best_mv,
                        MV_CONTEXT *const mvc, int mb_to_left_edge,
                        int mb_to_right_edge, int mb_to_top_edge,
                        int mb_to_bottom_edge)
{
    int s;      
    int num_p;  

    int j = 0;

    s = 3;
    num_p = 16;
    if( vp8_read(bc, 110) )
    {
        s = 2;
        num_p = 4;
        if( vp8_read(bc, 111) )
        {
            s = vp8_read(bc, 150);
            num_p = 2;
        }
    }

    do  
    {
        int_mv leftmv, abovemv;
        int_mv blockmv;
        int k;  

        const vp8_prob *prob;
        k = vp8_mbsplit_offset[s][j];

        if (!(k & 3))
        {
            
            if(left_mb->mbmi.mode != SPLITMV)
                leftmv.as_int =  left_mb->mbmi.mv.as_int;
            else
                leftmv.as_int =  (left_mb->bmi + k + 4 - 1)->mv.as_int;
        }
        else
            leftmv.as_int =  (mi->bmi + k - 1)->mv.as_int;

        if (!(k >> 2))
        {
            
            if(above_mb->mbmi.mode != SPLITMV)
                abovemv.as_int =  above_mb->mbmi.mv.as_int;
            else
                abovemv.as_int =  (above_mb->bmi + k + 16 - 4)->mv.as_int;
        }
        else
            abovemv.as_int = (mi->bmi + k - 4)->mv.as_int;

        prob = get_sub_mv_ref_prob(leftmv.as_int, abovemv.as_int);

        if( vp8_read(bc, prob[0]) )
        {
            if( vp8_read(bc, prob[1]) )
            {
                blockmv.as_int = 0;
                if( vp8_read(bc, prob[2]) )
                {
                    blockmv.as_mv.row = read_mvcomponent(bc, &mvc[0]) * 2;
                    blockmv.as_mv.row += best_mv.as_mv.row;
                    blockmv.as_mv.col = read_mvcomponent(bc, &mvc[1]) * 2;
                    blockmv.as_mv.col += best_mv.as_mv.col;
                }
            }
            else
            {
                blockmv.as_int = abovemv.as_int;
            }
        }
        else
        {
            blockmv.as_int = leftmv.as_int;
        }

        mbmi->need_to_clamp_mvs |= vp8_check_mv_bounds(&blockmv,
                                                  mb_to_left_edge,
                                                  mb_to_right_edge,
                                                  mb_to_top_edge,
                                                  mb_to_bottom_edge);

        {
            


            const unsigned char *fill_offset;
            unsigned int fill_count = mbsplit_fill_count[s];

            fill_offset = &mbsplit_fill_offset[s]
                             [(unsigned char)j * mbsplit_fill_count[s]];

            do {
                mi->bmi[ *fill_offset].mv.as_int = blockmv.as_int;
                fill_offset++;
            }while (--fill_count);
        }

    }
    while (++j < num_p);

    mbmi->partitioning = s;
}

static void read_mb_modes_mv(VP8D_COMP *pbi, MODE_INFO *mi, MB_MODE_INFO *mbmi)
{
    vp8_reader *const bc = & pbi->mbc[8];
    mbmi->ref_frame = (MV_REFERENCE_FRAME) vp8_read(bc, pbi->prob_intra);
    if (mbmi->ref_frame)    
    {
        enum {CNT_INTRA, CNT_NEAREST, CNT_NEAR, CNT_SPLITMV};
        int cnt[4];
        int *cntx = cnt;
        int_mv near_mvs[4];
        int_mv *nmv = near_mvs;
        const int mis = pbi->mb.mode_info_stride;
        const MODE_INFO *above = mi - mis;
        const MODE_INFO *left = mi - 1;
        const MODE_INFO *aboveleft = above - 1;
        int *ref_frame_sign_bias = pbi->common.ref_frame_sign_bias;

        mbmi->need_to_clamp_mvs = 0;

        if (vp8_read(bc, pbi->prob_last))
        {
            mbmi->ref_frame =
                (MV_REFERENCE_FRAME)((int)(2 + vp8_read(bc, pbi->prob_gf)));
        }

        
        nmv[0].as_int = nmv[1].as_int = nmv[2].as_int = 0;
        cnt[0] = cnt[1] = cnt[2] = cnt[3] = 0;

        
        if (above->mbmi.ref_frame != INTRA_FRAME)
        {
            if (above->mbmi.mv.as_int)
            {
                (++nmv)->as_int = above->mbmi.mv.as_int;
                mv_bias(ref_frame_sign_bias[above->mbmi.ref_frame],
                        mbmi->ref_frame, nmv, ref_frame_sign_bias);
                ++cntx;
            }

            *cntx += 2;
        }

        
        if (left->mbmi.ref_frame != INTRA_FRAME)
        {
            if (left->mbmi.mv.as_int)
            {
                int_mv this_mv;

                this_mv.as_int = left->mbmi.mv.as_int;
                mv_bias(ref_frame_sign_bias[left->mbmi.ref_frame],
                        mbmi->ref_frame, &this_mv, ref_frame_sign_bias);

                if (this_mv.as_int != nmv->as_int)
                {
                    (++nmv)->as_int = this_mv.as_int;
                    ++cntx;
                }

                *cntx += 2;
            }
            else
                cnt[CNT_INTRA] += 2;
        }

        
        if (aboveleft->mbmi.ref_frame != INTRA_FRAME)
        {
            if (aboveleft->mbmi.mv.as_int)
            {
                int_mv this_mv;

                this_mv.as_int = aboveleft->mbmi.mv.as_int;
                mv_bias(ref_frame_sign_bias[aboveleft->mbmi.ref_frame],
                        mbmi->ref_frame, &this_mv, ref_frame_sign_bias);

                if (this_mv.as_int != nmv->as_int)
                {
                    (++nmv)->as_int = this_mv.as_int;
                    ++cntx;
                }

                *cntx += 1;
            }
            else
                cnt[CNT_INTRA] += 1;
        }

        if( vp8_read(bc, vp8_mode_contexts [cnt[CNT_INTRA]] [0]) )
        {

            
            
            cnt[CNT_NEAREST] += ( (cnt[CNT_SPLITMV] > 0) &
                (nmv->as_int == near_mvs[CNT_NEAREST].as_int));

            
            if (cnt[CNT_NEAR] > cnt[CNT_NEAREST])
            {
                int tmp;
                tmp = cnt[CNT_NEAREST];
                cnt[CNT_NEAREST] = cnt[CNT_NEAR];
                cnt[CNT_NEAR] = tmp;
                tmp = near_mvs[CNT_NEAREST].as_int;
                near_mvs[CNT_NEAREST].as_int = near_mvs[CNT_NEAR].as_int;
                near_mvs[CNT_NEAR].as_int = tmp;
            }

            if( vp8_read(bc, vp8_mode_contexts [cnt[CNT_NEAREST]] [1]) )
            {

                if( vp8_read(bc, vp8_mode_contexts [cnt[CNT_NEAR]] [2]) )
                {
                    int mb_to_top_edge;
                    int mb_to_bottom_edge;
                    int mb_to_left_edge;
                    int mb_to_right_edge;
                    MV_CONTEXT *const mvc = pbi->common.fc.mvc;
                    int near_index;

                    mb_to_top_edge = pbi->mb.mb_to_top_edge;
                    mb_to_bottom_edge = pbi->mb.mb_to_bottom_edge;
                    mb_to_top_edge -= LEFT_TOP_MARGIN;
                    mb_to_bottom_edge += RIGHT_BOTTOM_MARGIN;
                    mb_to_right_edge = pbi->mb.mb_to_right_edge;
                    mb_to_right_edge += RIGHT_BOTTOM_MARGIN;
                    mb_to_left_edge = pbi->mb.mb_to_left_edge;
                    mb_to_left_edge -= LEFT_TOP_MARGIN;

                    
                    near_index = CNT_INTRA +
                        (cnt[CNT_NEAREST] >= cnt[CNT_INTRA]);

                    vp8_clamp_mv2(&near_mvs[near_index], &pbi->mb);

                    cnt[CNT_SPLITMV] = ((above->mbmi.mode == SPLITMV)
                                        + (left->mbmi.mode == SPLITMV)) * 2
                                       + (aboveleft->mbmi.mode == SPLITMV);

                    if( vp8_read(bc, vp8_mode_contexts [cnt[CNT_SPLITMV]] [3]) )
                    {
                        decode_split_mv(bc, mi, left, above,
                                                    mbmi,
                                                    near_mvs[near_index],
                                                    mvc, mb_to_left_edge,
                                                    mb_to_right_edge,
                                                    mb_to_top_edge,
                                                    mb_to_bottom_edge);
                        mbmi->mv.as_int = mi->bmi[15].mv.as_int;
                        mbmi->mode =  SPLITMV;
                        mbmi->is_4x4 = 1;
                    }
                    else
                    {
                        int_mv *const mbmi_mv = & mbmi->mv;
                        read_mv(bc, &mbmi_mv->as_mv, (const MV_CONTEXT *) mvc);
                        mbmi_mv->as_mv.row += near_mvs[near_index].as_mv.row;
                        mbmi_mv->as_mv.col += near_mvs[near_index].as_mv.col;

                        




                        mbmi->need_to_clamp_mvs =
                            vp8_check_mv_bounds(mbmi_mv, mb_to_left_edge,
                                                mb_to_right_edge,
                                                mb_to_top_edge,
                                                mb_to_bottom_edge);
                        mbmi->mode =  NEWMV;
                    }
                }
                else
                {
                    mbmi->mode =  NEARMV;
                    mbmi->mv.as_int = near_mvs[CNT_NEAR].as_int;
                    vp8_clamp_mv2(&mbmi->mv, &pbi->mb);
                }
            }
            else
            {
                mbmi->mode =  NEARESTMV;
                mbmi->mv.as_int = near_mvs[CNT_NEAREST].as_int;
                vp8_clamp_mv2(&mbmi->mv, &pbi->mb);
            }
        }
        else
        {
            mbmi->mode =  ZEROMV;
            mbmi->mv.as_int = 0;
        }

#if CONFIG_ERROR_CONCEALMENT
        if(pbi->ec_enabled && (mbmi->mode != SPLITMV))
        {
            mi->bmi[ 0].mv.as_int =
            mi->bmi[ 1].mv.as_int =
            mi->bmi[ 2].mv.as_int =
            mi->bmi[ 3].mv.as_int =
            mi->bmi[ 4].mv.as_int =
            mi->bmi[ 5].mv.as_int =
            mi->bmi[ 6].mv.as_int =
            mi->bmi[ 7].mv.as_int =
            mi->bmi[ 8].mv.as_int =
            mi->bmi[ 9].mv.as_int =
            mi->bmi[10].mv.as_int =
            mi->bmi[11].mv.as_int =
            mi->bmi[12].mv.as_int =
            mi->bmi[13].mv.as_int =
            mi->bmi[14].mv.as_int =
            mi->bmi[15].mv.as_int = mbmi->mv.as_int;
        }
#endif
    }
    else
    {
        
        mbmi->mv.as_int = 0;

        
        if ((mbmi->mode = read_ymode(bc, pbi->common.fc.ymode_prob)) == B_PRED)
        {
            int j = 0;
            mbmi->is_4x4 = 1;
            do
            {
                mi->bmi[j].as_mode = read_bmode(bc, pbi->common.fc.bmode_prob);
            }
            while (++j < 16);
        }

        mbmi->uv_mode = read_uv_mode(bc, pbi->common.fc.uv_mode_prob);
    }

}

static void read_mb_features(vp8_reader *r, MB_MODE_INFO *mi, MACROBLOCKD *x)
{
    
    if (x->segmentation_enabled && x->update_mb_segmentation_map)
    {
        
        if (vp8_read(r, x->mb_segment_tree_probs[0]))
            mi->segment_id =
                (unsigned char)(2 + vp8_read(r, x->mb_segment_tree_probs[2]));
        else
            mi->segment_id =
                (unsigned char)(vp8_read(r, x->mb_segment_tree_probs[1]));
    }
}

static void decode_mb_mode_mvs(VP8D_COMP *pbi, MODE_INFO *mi,
                               MB_MODE_INFO *mbmi)
{
    (void)mbmi;

    



    if (pbi->mb.update_mb_segmentation_map)
        read_mb_features(&pbi->mbc[8], &mi->mbmi, &pbi->mb);
    else if(pbi->common.frame_type == KEY_FRAME)
        mi->mbmi.segment_id = 0;

    

    if (pbi->common.mb_no_coeff_skip)
        mi->mbmi.mb_skip_coeff = vp8_read(&pbi->mbc[8], pbi->prob_skip_false);
    else
        mi->mbmi.mb_skip_coeff = 0;

    mi->mbmi.is_4x4 = 0;
    if(pbi->common.frame_type == KEY_FRAME)
        read_kf_modes(pbi, mi);
    else
        read_mb_modes_mv(pbi, mi, &mi->mbmi);

}

void vp8_decode_mode_mvs(VP8D_COMP *pbi)
{
    MODE_INFO *mi = pbi->common.mi;
    int mb_row = -1;
    int mb_to_right_edge_start;

    mb_mode_mv_init(pbi);

    pbi->mb.mb_to_top_edge = 0;
    pbi->mb.mb_to_bottom_edge = ((pbi->common.mb_rows - 1) * 16) << 3;
    mb_to_right_edge_start = ((pbi->common.mb_cols - 1) * 16) << 3;

    while (++mb_row < pbi->common.mb_rows)
    {
        int mb_col = -1;

        pbi->mb.mb_to_left_edge =  0;
        pbi->mb.mb_to_right_edge = mb_to_right_edge_start;

        while (++mb_col < pbi->common.mb_cols)
        {
#if CONFIG_ERROR_CONCEALMENT
            int mb_num = mb_row * pbi->common.mb_cols + mb_col;
#endif

            decode_mb_mode_mvs(pbi, mi, &mi->mbmi);

#if CONFIG_ERROR_CONCEALMENT
            

            if (vp8dx_bool_error(&pbi->mbc[8]) && mb_num <
                (int)pbi->mvs_corrupt_from_mb)
            {
                pbi->mvs_corrupt_from_mb = mb_num;
                


                return;
            }
#endif

            pbi->mb.mb_to_left_edge -= (16 << 3);
            pbi->mb.mb_to_right_edge -= (16 << 3);
            mi++;       
        }
        pbi->mb.mb_to_top_edge -= (16 << 3);
        pbi->mb.mb_to_bottom_edge -= (16 << 3);

        mi++;           
    }
}
