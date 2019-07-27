









#define USE_PREBUILT_TABLES

#include "entropymode.h"
#include "entropy.h"
#include "vpx_mem/vpx_mem.h"

#include "vp8_entropymodedata.h"

int vp8_mv_cont(const int_mv *l, const int_mv *a)
{
    int lez = (l->as_int == 0);
    int aez = (a->as_int == 0);
    int lea = (l->as_int == a->as_int);

    if (lea && lez)
        return SUBMVREF_LEFT_ABOVE_ZED;

    if (lea)
        return SUBMVREF_LEFT_ABOVE_SAME;

    if (aez)
        return SUBMVREF_ABOVE_ZED;

    if (lez)
        return SUBMVREF_LEFT_ZED;

    return SUBMVREF_NORMAL;
}

static const vp8_prob sub_mv_ref_prob [VP8_SUBMVREFS-1] = { 180, 162, 25};

const vp8_prob vp8_sub_mv_ref_prob2 [SUBMVREF_COUNT][VP8_SUBMVREFS-1] =
{
    { 147, 136, 18 },
    { 106, 145, 1  },
    { 179, 121, 1  },
    { 223, 1  , 34 },
    { 208, 1  , 1  }
};



const vp8_mbsplit vp8_mbsplits [VP8_NUMMBSPLITS] =
{
    {
        0,  0,  0,  0,
        0,  0,  0,  0,
        1,  1,  1,  1,
        1,  1,  1,  1,
    },
    {
        0,  0,  1,  1,
        0,  0,  1,  1,
        0,  0,  1,  1,
        0,  0,  1,  1,
    },
    {
        0,  0,  1,  1,
        0,  0,  1,  1,
        2,  2,  3,  3,
        2,  2,  3,  3,
    },
    {
        0,  1,  2,  3,
        4,  5,  6,  7,
        8,  9,  10, 11,
        12, 13, 14, 15,
    }
};

const int vp8_mbsplit_count [VP8_NUMMBSPLITS] = { 2, 2, 4, 16};

const vp8_prob vp8_mbsplit_probs [VP8_NUMMBSPLITS-1] = { 110, 111, 150};




const vp8_tree_index vp8_bmode_tree[18] =     
{
    -B_DC_PRED, 2,                             
    -B_TM_PRED, 4,                            
    -B_VE_PRED, 6,                           
    8, 12,                                  
    -B_HE_PRED, 10,                        
    -B_RD_PRED, -B_VR_PRED,               
    -B_LD_PRED, 14,                        
    -B_VL_PRED, 16,                      
    -B_HD_PRED, -B_HU_PRED             
};




const vp8_tree_index vp8_ymode_tree[8] =
{
    -DC_PRED, 2,
    4, 6,
    -V_PRED, -H_PRED,
    -TM_PRED, -B_PRED
};

const vp8_tree_index vp8_kf_ymode_tree[8] =
{
    -B_PRED, 2,
    4, 6,
    -DC_PRED, -V_PRED,
    -H_PRED, -TM_PRED
};

const vp8_tree_index vp8_uv_mode_tree[6] =
{
    -DC_PRED, 2,
    -V_PRED, 4,
    -H_PRED, -TM_PRED
};

const vp8_tree_index vp8_mbsplit_tree[6] =
{
    -3, 2,
    -2, 4,
    -0, -1
};

const vp8_tree_index vp8_mv_ref_tree[8] =
{
    -ZEROMV, 2,
    -NEARESTMV, 4,
    -NEARMV, 6,
    -NEWMV, -SPLITMV
};

const vp8_tree_index vp8_sub_mv_ref_tree[6] =
{
    -LEFT4X4, 2,
    -ABOVE4X4, 4,
    -ZERO4X4, -NEW4X4
};

const vp8_tree_index vp8_small_mvtree [14] =
{
    2, 8,
    4, 6,
    -0, -1,
    -2, -3,
    10, 12,
    -4, -5,
    -6, -7
};

void vp8_init_mbmode_probs(VP8_COMMON *x)
{
    memcpy(x->fc.ymode_prob, vp8_ymode_prob, sizeof(vp8_ymode_prob));
    memcpy(x->fc.uv_mode_prob, vp8_uv_mode_prob, sizeof(vp8_uv_mode_prob));
    memcpy(x->fc.sub_mv_ref_prob, sub_mv_ref_prob, sizeof(sub_mv_ref_prob));
}

void vp8_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES-1])
{
    memcpy(p, vp8_bmode_prob, sizeof(vp8_bmode_prob));
}

