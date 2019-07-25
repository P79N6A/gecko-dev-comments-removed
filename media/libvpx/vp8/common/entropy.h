










#ifndef __INC_ENTROPY_H
#define __INC_ENTROPY_H

#include "treecoder.h"
#include "blockd.h"



#define ZERO_TOKEN              0       /* 0         Extra Bits 0+0 */
#define ONE_TOKEN               1       /* 1         Extra Bits 0+1 */
#define TWO_TOKEN               2       /* 2         Extra Bits 0+1 */
#define THREE_TOKEN             3       /* 3         Extra Bits 0+1 */
#define FOUR_TOKEN              4       /* 4         Extra Bits 0+1 */
#define DCT_VAL_CATEGORY1       5       /* 5-6       Extra Bits 1+1 */
#define DCT_VAL_CATEGORY2       6       /* 7-10      Extra Bits 2+1 */
#define DCT_VAL_CATEGORY3       7       /* 11-18     Extra Bits 3+1 */
#define DCT_VAL_CATEGORY4       8       /* 19-34     Extra Bits 4+1 */
#define DCT_VAL_CATEGORY5       9       /* 35-66     Extra Bits 5+1 */
#define DCT_VAL_CATEGORY6       10      /* 67+       Extra Bits 11+1 */
#define DCT_EOB_TOKEN           11      /* EOB       Extra Bits 0+0 */

#define MAX_ENTROPY_TOKENS 12
#define ENTROPY_NODES 11

extern const vp8_tree_index vp8_coef_tree[];

extern struct vp8_token_struct vp8_coef_encodings[MAX_ENTROPY_TOKENS];

typedef struct
{
    vp8_tree_p tree;
    const vp8_prob *prob;
    int Len;
    int base_val;
} vp8_extra_bit_struct;

extern vp8_extra_bit_struct vp8_extra_bits[12];    

#define PROB_UPDATE_BASELINE_COST   7

#define MAX_PROB                255
#define DCT_MAX_VALUE           2048






#define BLOCK_TYPES 4




#define COEF_BANDS 8
extern DECLARE_ALIGNED(16, const unsigned char, vp8_coef_bands[16]);

















 
#   define PREV_COEF_CONTEXTS       3

extern DECLARE_ALIGNED(16, const unsigned char, vp8_prev_token_class[MAX_ENTROPY_TOKENS]);

extern const vp8_prob vp8_coef_update_probs [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [ENTROPY_NODES];


struct VP8Common;
void vp8_default_coef_probs(struct VP8Common *);

extern DECLARE_ALIGNED(16, const int, vp8_default_zig_zag1d[16]);
extern DECLARE_ALIGNED(16, const short, vp8_default_inv_zig_zag[16]);
extern short vp8_default_zig_zag_mask[16];
extern const int vp8_mb_feature_data_bits[MB_LVL_MAX];

void vp8_coef_tree_initialize(void);
#endif
