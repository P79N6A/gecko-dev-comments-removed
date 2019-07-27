









#include "entropy.h"
#include "blockd.h"
#include "onyxc_int.h"
#include "vpx_mem/vpx_mem.h"

#include "coefupdateprobs.h"

DECLARE_ALIGNED(16, const unsigned char, vp8_norm[256]) =
{
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

DECLARE_ALIGNED(16, const unsigned char, vp8_coef_bands[16]) =
{ 0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7};

DECLARE_ALIGNED(16, const unsigned char,
                vp8_prev_token_class[MAX_ENTROPY_TOKENS]) =
{ 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0};

DECLARE_ALIGNED(16, const int, vp8_default_zig_zag1d[16]) =
{
    0,  1,  4,  8,
    5,  2,  3,  6,
    9, 12, 13, 10,
    7, 11, 14, 15,
};

DECLARE_ALIGNED(16, const short, vp8_default_inv_zig_zag[16]) =
{
    1,  2,  6,  7,
    3,  5,  8, 13,
    4,  9, 12, 14,
   10, 11, 15, 16
};














DECLARE_ALIGNED(16, const short, vp8_default_zig_zag_mask[16]) =
{
     1,    2,    32,     64,
     4,   16,   128,   4096,
     8,  256,  2048,   8192,
   512, 1024, 16384, -32768
};

const int vp8_mb_feature_data_bits[MB_LVL_MAX] = {7, 6};



const vp8_tree_index vp8_coef_tree[ 22] =     
{
    -DCT_EOB_TOKEN, 2,                             
    -ZERO_TOKEN, 4,                               
    -ONE_TOKEN, 6,                               
    8, 12,                                      
    -TWO_TOKEN, 10,                            
    -THREE_TOKEN, -FOUR_TOKEN,                
    14, 16,                                    
    -DCT_VAL_CATEGORY1, -DCT_VAL_CATEGORY2,   
    18, 20,                                   
    -DCT_VAL_CATEGORY3, -DCT_VAL_CATEGORY4,  
    -DCT_VAL_CATEGORY5, -DCT_VAL_CATEGORY6   
};




vp8_token vp8_coef_encodings[MAX_ENTROPY_TOKENS] =
{
    {2, 2},
    {6, 3},
    {28, 5},
    {58, 6},
    {59, 6},
    {60, 6},
    {61, 6},
    {124, 7},
    {125, 7},
    {126, 7},
    {127, 7},
    {0, 1}
};




static const vp8_prob Pcat1[] = { 159};
static const vp8_prob Pcat2[] = { 165, 145};
static const vp8_prob Pcat3[] = { 173, 148, 140};
static const vp8_prob Pcat4[] = { 176, 155, 140, 135};
static const vp8_prob Pcat5[] = { 180, 157, 141, 134, 130};
static const vp8_prob Pcat6[] =
{ 254, 254, 243, 230, 196, 177, 153, 140, 133, 130, 129};




























static const vp8_tree_index cat1[2] = { 0, 0 };
static const vp8_tree_index cat2[4] = { 2, 2, 0, 0 };
static const vp8_tree_index cat3[6] = { 2, 2, 4, 4, 0, 0 };
static const vp8_tree_index cat4[8] = { 2, 2, 4, 4, 6, 6, 0, 0 };
static const vp8_tree_index cat5[10] = { 2, 2, 4, 4, 6, 6, 8, 8, 0, 0 };
static const vp8_tree_index cat6[22] = { 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12,
                                        14, 14, 16, 16, 18, 18, 20, 20, 0, 0 };

const vp8_extra_bit_struct vp8_extra_bits[12] =
{
    { 0, 0, 0, 0},
    { 0, 0, 0, 1},
    { 0, 0, 0, 2},
    { 0, 0, 0, 3},
    { 0, 0, 0, 4},
    { cat1, Pcat1, 1, 5},
    { cat2, Pcat2, 2, 7},
    { cat3, Pcat3, 3, 11},
    { cat4, Pcat4, 4, 19},
    { cat5, Pcat5, 5, 35},
    { cat6, Pcat6, 11, 67},
    { 0, 0, 0, 0}
};

#include "default_coef_probs.h"

void vp8_default_coef_probs(VP8_COMMON *pc)
{
    memcpy(pc->fc.coef_probs, default_coef_probs, sizeof(default_coef_probs));
}

