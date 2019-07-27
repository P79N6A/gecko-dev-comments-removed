









#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_seg_common.h"

const vp9_prob vp9_kf_y_mode_prob[INTRA_MODES][INTRA_MODES][INTRA_MODES - 1] = {
  {  
    { 137,  30,  42, 148, 151, 207,  70,  52,  91 },  
    {  92,  45, 102, 136, 116, 180,  74,  90, 100 },  
    {  73,  32,  19, 187, 222, 215,  46,  34, 100 },  
    {  91,  30,  32, 116, 121, 186,  93,  86,  94 },  
    {  72,  35,  36, 149,  68, 206,  68,  63, 105 },  
    {  73,  31,  28, 138,  57, 124,  55, 122, 151 },  
    {  67,  23,  21, 140, 126, 197,  40,  37, 171 },  
    {  86,  27,  28, 128, 154, 212,  45,  43,  53 },  
    {  74,  32,  27, 107,  86, 160,  63, 134, 102 },  
    {  59,  67,  44, 140, 161, 202,  78,  67, 119 }   
  }, {  
    {  63,  36, 126, 146, 123, 158,  60,  90,  96 },  
    {  43,  46, 168, 134, 107, 128,  69, 142,  92 },  
    {  44,  29,  68, 159, 201, 177,  50,  57,  77 },  
    {  58,  38,  76, 114,  97, 172,  78, 133,  92 },  
    {  46,  41,  76, 140,  63, 184,  69, 112,  57 },  
    {  38,  32,  85, 140,  46, 112,  54, 151, 133 },  
    {  39,  27,  61, 131, 110, 175,  44,  75, 136 },  
    {  52,  30,  74, 113, 130, 175,  51,  64,  58 },  
    {  47,  35,  80, 100,  74, 143,  64, 163,  74 },  
    {  36,  61, 116, 114, 128, 162,  80, 125,  82 }   
  }, {  
    {  82,  26,  26, 171, 208, 204,  44,  32, 105 },  
    {  55,  44,  68, 166, 179, 192,  57,  57, 108 },  
    {  42,  26,  11, 199, 241, 228,  23,  15,  85 },  
    {  68,  42,  19, 131, 160, 199,  55,  52,  83 },  
    {  58,  50,  25, 139, 115, 232,  39,  52, 118 },  
    {  50,  35,  33, 153, 104, 162,  64,  59, 131 },  
    {  44,  24,  16, 150, 177, 202,  33,  19, 156 },  
    {  55,  27,  12, 153, 203, 218,  26,  27,  49 },  
    {  53,  49,  21, 110, 116, 168,  59,  80,  76 },  
    {  38,  72,  19, 168, 203, 212,  50,  50, 107 }   
  }, {  
    { 103,  26,  36, 129, 132, 201,  83,  80,  93 },  
    {  59,  38,  83, 112, 103, 162,  98, 136,  90 },  
    {  62,  30,  23, 158, 200, 207,  59,  57,  50 },  
    {  67,  30,  29,  84,  86, 191, 102,  91,  59 },  
    {  60,  32,  33, 112,  71, 220,  64,  89, 104 },  
    {  53,  26,  34, 130,  56, 149,  84, 120, 103 },  
    {  53,  21,  23, 133, 109, 210,  56,  77, 172 },  
    {  77,  19,  29, 112, 142, 228,  55,  66,  36 },  
    {  61,  29,  29,  93,  97, 165,  83, 175, 162 },  
    {  47,  47,  43, 114, 137, 181, 100,  99,  95 }   
  }, {  
    {  69,  23,  29, 128,  83, 199,  46,  44, 101 },  
    {  53,  40,  55, 139,  69, 183,  61,  80, 110 },  
    {  40,  29,  19, 161, 180, 207,  43,  24,  91 },  
    {  60,  34,  19, 105,  61, 198,  53,  64,  89 },  
    {  52,  31,  22, 158,  40, 209,  58,  62,  89 },  
    {  44,  31,  29, 147,  46, 158,  56, 102, 198 },  
    {  35,  19,  12, 135,  87, 209,  41,  45, 167 },  
    {  55,  25,  21, 118,  95, 215,  38,  39,  66 },  
    {  51,  38,  25, 113,  58, 164,  70,  93,  97 },  
    {  47,  54,  34, 146, 108, 203,  72, 103, 151 }   
  }, {  
    {  64,  19,  37, 156,  66, 138,  49,  95, 133 },  
    {  46,  27,  80, 150,  55, 124,  55, 121, 135 },  
    {  36,  23,  27, 165, 149, 166,  54,  64, 118 },  
    {  53,  21,  36, 131,  63, 163,  60, 109,  81 },  
    {  40,  26,  35, 154,  40, 185,  51,  97, 123 },  
    {  35,  19,  34, 179,  19,  97,  48, 129, 124 },  
    {  36,  20,  26, 136,  62, 164,  33,  77, 154 },  
    {  45,  18,  32, 130,  90, 157,  40,  79,  91 },  
    {  45,  26,  28, 129,  45, 129,  49, 147, 123 },  
    {  38,  44,  51, 136,  74, 162,  57,  97, 121 }   
  }, {  
    {  75,  17,  22, 136, 138, 185,  32,  34, 166 },  
    {  56,  39,  58, 133, 117, 173,  48,  53, 187 },  
    {  35,  21,  12, 161, 212, 207,  20,  23, 145 },  
    {  56,  29,  19, 117, 109, 181,  55,  68, 112 },  
    {  47,  29,  17, 153,  64, 220,  59,  51, 114 },  
    {  46,  16,  24, 136,  76, 147,  41,  64, 172 },  
    {  34,  17,  11, 108, 152, 187,  13,  15, 209 },  
    {  51,  24,  14, 115, 133, 209,  32,  26, 104 },  
    {  55,  30,  18, 122,  79, 179,  44,  88, 116 },  
    {  37,  49,  25, 129, 168, 164,  41,  54, 148 }   
  }, {  
    {  82,  22,  32, 127, 143, 213,  39,  41,  70 },  
    {  62,  44,  61, 123, 105, 189,  48,  57,  64 },  
    {  47,  25,  17, 175, 222, 220,  24,  30,  86 },  
    {  68,  36,  17, 106, 102, 206,  59,  74,  74 },  
    {  57,  39,  23, 151,  68, 216,  55,  63,  58 },  
    {  49,  30,  35, 141,  70, 168,  82,  40, 115 },  
    {  51,  25,  15, 136, 129, 202,  38,  35, 139 },  
    {  68,  26,  16, 111, 141, 215,  29,  28,  28 },  
    {  59,  39,  19, 114,  75, 180,  77, 104,  42 },  
    {  40,  61,  26, 126, 152, 206,  61,  59,  93 }   
  }, {  
    {  78,  23,  39, 111, 117, 170,  74, 124,  94 },  
    {  48,  34,  86, 101,  92, 146,  78, 179, 134 },  
    {  47,  22,  24, 138, 187, 178,  68,  69,  59 },  
    {  56,  25,  33, 105, 112, 187,  95, 177, 129 },  
    {  48,  31,  27, 114,  63, 183,  82, 116,  56 },  
    {  43,  28,  37, 121,  63, 123,  61, 192, 169 },  
    {  42,  17,  24, 109,  97, 177,  56,  76, 122 },  
    {  58,  18,  28, 105, 139, 182,  70,  92,  63 },  
    {  46,  23,  32,  74,  86, 150,  67, 183,  88 },  
    {  36,  38,  48,  92, 122, 165,  88, 137,  91 }   
  }, {  
    {  65,  70,  60, 155, 159, 199,  61,  60,  81 },  
    {  44,  78, 115, 132, 119, 173,  71, 112,  93 },  
    {  39,  38,  21, 184, 227, 206,  42,  32,  64 },  
    {  58,  47,  36, 124, 137, 193,  80,  82,  78 },  
    {  49,  50,  35, 144,  95, 205,  63,  78,  59 },  
    {  41,  53,  52, 148,  71, 142,  65, 128,  51 },  
    {  40,  36,  28, 143, 143, 202,  40,  55, 137 },  
    {  52,  34,  29, 129, 183, 227,  42,  35,  43 },  
    {  42,  44,  44, 104, 105, 164,  64, 130,  80 },  
    {  43,  81,  53, 140, 169, 204,  68,  84,  72 }   
  }
};

const vp9_prob vp9_kf_uv_mode_prob[INTRA_MODES][INTRA_MODES - 1] = {
  { 144,  11,  54, 157, 195, 130,  46,  58, 108 },  
  { 118,  15, 123, 148, 131, 101,  44,  93, 131 },  
  { 113,  12,  23, 188, 226, 142,  26,  32, 125 },  
  { 120,  11,  50, 123, 163, 135,  64,  77, 103 },  
  { 113,   9,  36, 155, 111, 157,  32,  44, 161 },  
  { 116,   9,  55, 176,  76,  96,  37,  61, 149 },  
  { 115,   9,  28, 141, 161, 167,  21,  25, 193 },  
  { 120,  12,  32, 145, 195, 142,  32,  38,  86 },  
  { 116,  12,  64, 120, 140, 125,  49, 115, 121 },  
  { 102,  19,  66, 162, 182, 122,  35,  59, 128 }   
};

static const vp9_prob default_if_y_probs[BLOCK_SIZE_GROUPS][INTRA_MODES - 1] = {
  {  65,  32,  18, 144, 162, 194,  41,  51,  98 },  
  { 132,  68,  18, 165, 217, 196,  45,  40,  78 },  
  { 173,  80,  19, 176, 240, 193,  64,  35,  46 },  
  { 221, 135,  38, 194, 248, 121,  96,  85,  29 }   
};

static const vp9_prob default_if_uv_probs[INTRA_MODES][INTRA_MODES - 1] = {
  { 120,   7,  76, 176, 208, 126,  28,  54, 103 },  
  {  48,  12, 154, 155, 139,  90,  34, 117, 119 },  
  {  67,   6,  25, 204, 243, 158,  13,  21,  96 },  
  {  97,   5,  44, 131, 176, 139,  48,  68,  97 },  
  {  83,   5,  42, 156, 111, 152,  26,  49, 152 },  
  {  80,   5,  58, 178,  74,  83,  33,  62, 145 },  
  {  86,   5,  32, 154, 192, 168,  14,  22, 163 },  
  {  85,   5,  32, 156, 216, 148,  19,  29,  73 },  
  {  77,   7,  64, 116, 132, 122,  37, 126, 120 },  
  { 101,  21, 107, 181, 192, 103,  19,  67, 125 }   
};

const vp9_prob vp9_kf_partition_probs[PARTITION_CONTEXTS]
                                     [PARTITION_TYPES - 1] = {
  
  { 158,  97,  94 },  
  {  93,  24,  99 },  
  {  85, 119,  44 },  
  {  62,  59,  67 },  
  
  { 149,  53,  53 },  
  {  94,  20,  48 },  
  {  83,  53,  24 },  
  {  52,  18,  18 },  
  
  { 150,  40,  39 },  
  {  78,  12,  26 },  
  {  67,  33,  11 },  
  {  24,   7,   5 },  
  
  { 174,  35,  49 },  
  {  68,  11,  27 },  
  {  57,  15,   9 },  
  {  12,   3,   3 },  
};

static const vp9_prob default_partition_probs[PARTITION_CONTEXTS]
                                             [PARTITION_TYPES - 1] = {
  
  { 199, 122, 141 },  
  { 147,  63, 159 },  
  { 148, 133, 118 },  
  { 121, 104, 114 },  
  
  { 174,  73,  87 },  
  {  92,  41,  83 },  
  {  82,  99,  50 },  
  {  53,  39,  39 },  
  
  { 177,  58,  59 },  
  {  68,  26,  63 },  
  {  52,  79,  25 },  
  {  17,  14,  12 },  
  
  { 222,  34,  30 },  
  {  72,  16,  44 },  
  {  58,  32,  12 },  
  {  10,   7,   6 },  
};

static const vp9_prob default_inter_mode_probs[INTER_MODE_CONTEXTS]
                                              [INTER_MODES - 1] = {
  {2,       173,   34},  
  {7,       145,   85},  
  {7,       166,   63},  
  {7,       94,    66},  
  {8,       64,    46},  
  {17,      81,    31},  
  {25,      29,    30},  
};


const vp9_tree_index vp9_intra_mode_tree[TREE_SIZE(INTRA_MODES)] = {
  -DC_PRED, 2,                      
  -TM_PRED, 4,                      
  -V_PRED, 6,                       
  8, 12,                            
  -H_PRED, 10,                      
  -D135_PRED, -D117_PRED,           
  -D45_PRED, 14,                    
  -D63_PRED, 16,                    
  -D153_PRED, -D207_PRED             
};

const vp9_tree_index vp9_inter_mode_tree[TREE_SIZE(INTER_MODES)] = {
  -INTER_OFFSET(ZEROMV), 2,
  -INTER_OFFSET(NEARESTMV), 4,
  -INTER_OFFSET(NEARMV), -INTER_OFFSET(NEWMV)
};

const vp9_tree_index vp9_partition_tree[TREE_SIZE(PARTITION_TYPES)] = {
  -PARTITION_NONE, 2,
  -PARTITION_HORZ, 4,
  -PARTITION_VERT, -PARTITION_SPLIT
};

static const vp9_prob default_intra_inter_p[INTRA_INTER_CONTEXTS] = {
  9, 102, 187, 225
};

static const vp9_prob default_comp_inter_p[COMP_INTER_CONTEXTS] = {
  239, 183, 119,  96,  41
};

static const vp9_prob default_comp_ref_p[REF_CONTEXTS] = {
  50, 126, 123, 221, 226
};

static const vp9_prob default_single_ref_p[REF_CONTEXTS][2] = {
  {  33,  16 },
  {  77,  74 },
  { 142, 142 },
  { 172, 170 },
  { 238, 247 }
};

static const struct tx_probs default_tx_probs = {
  { { 3, 136, 37 },
    { 5, 52,  13 } },

  { { 20, 152 },
    { 15, 101 } },

  { { 100 },
    { 66  } }
};

void tx_counts_to_branch_counts_32x32(const unsigned int *tx_count_32x32p,
                                      unsigned int (*ct_32x32p)[2]) {
  ct_32x32p[0][0] = tx_count_32x32p[TX_4X4];
  ct_32x32p[0][1] = tx_count_32x32p[TX_8X8] +
                    tx_count_32x32p[TX_16X16] +
                    tx_count_32x32p[TX_32X32];
  ct_32x32p[1][0] = tx_count_32x32p[TX_8X8];
  ct_32x32p[1][1] = tx_count_32x32p[TX_16X16] +
                    tx_count_32x32p[TX_32X32];
  ct_32x32p[2][0] = tx_count_32x32p[TX_16X16];
  ct_32x32p[2][1] = tx_count_32x32p[TX_32X32];
}

void tx_counts_to_branch_counts_16x16(const unsigned int *tx_count_16x16p,
                                      unsigned int (*ct_16x16p)[2]) {
  ct_16x16p[0][0] = tx_count_16x16p[TX_4X4];
  ct_16x16p[0][1] = tx_count_16x16p[TX_8X8] + tx_count_16x16p[TX_16X16];
  ct_16x16p[1][0] = tx_count_16x16p[TX_8X8];
  ct_16x16p[1][1] = tx_count_16x16p[TX_16X16];
}

void tx_counts_to_branch_counts_8x8(const unsigned int *tx_count_8x8p,
                                    unsigned int (*ct_8x8p)[2]) {
  ct_8x8p[0][0] = tx_count_8x8p[TX_4X4];
  ct_8x8p[0][1] = tx_count_8x8p[TX_8X8];
}

static const vp9_prob default_skip_probs[SKIP_CONTEXTS] = {
  192, 128, 64
};

static const vp9_prob default_switchable_interp_prob[SWITCHABLE_FILTER_CONTEXTS]
                                                    [SWITCHABLE_FILTERS - 1] = {
  { 235, 162, },
  { 36, 255, },
  { 34, 3, },
  { 149, 144, },
};

void vp9_init_mbmode_probs(VP9_COMMON *cm) {
  vp9_copy(cm->fc.uv_mode_prob, default_if_uv_probs);
  vp9_copy(cm->fc.y_mode_prob, default_if_y_probs);
  vp9_copy(cm->fc.switchable_interp_prob, default_switchable_interp_prob);
  vp9_copy(cm->fc.partition_prob, default_partition_probs);
  vp9_copy(cm->fc.intra_inter_prob, default_intra_inter_p);
  vp9_copy(cm->fc.comp_inter_prob, default_comp_inter_p);
  vp9_copy(cm->fc.comp_ref_prob, default_comp_ref_p);
  vp9_copy(cm->fc.single_ref_prob, default_single_ref_p);
  cm->fc.tx_probs = default_tx_probs;
  vp9_copy(cm->fc.skip_probs, default_skip_probs);
  vp9_copy(cm->fc.inter_mode_probs, default_inter_mode_probs);
}

const vp9_tree_index vp9_switchable_interp_tree
                         [TREE_SIZE(SWITCHABLE_FILTERS)] = {
  -EIGHTTAP, 2,
  -EIGHTTAP_SMOOTH, -EIGHTTAP_SHARP
};

#define COUNT_SAT 20
#define MAX_UPDATE_FACTOR 128

static int adapt_prob(vp9_prob pre_prob, const unsigned int ct[2]) {
  return merge_probs(pre_prob, ct, COUNT_SAT, MAX_UPDATE_FACTOR);
}

static void adapt_probs(const vp9_tree_index *tree,
                        const vp9_prob *pre_probs, const unsigned int *counts,
                        vp9_prob *probs) {
  vp9_tree_merge_probs(tree, pre_probs, counts, COUNT_SAT, MAX_UPDATE_FACTOR,
                   probs);
}

void vp9_adapt_mode_probs(VP9_COMMON *cm) {
  int i, j;
  FRAME_CONTEXT *fc = &cm->fc;
  const FRAME_CONTEXT *pre_fc = &cm->frame_contexts[cm->frame_context_idx];
  const FRAME_COUNTS *counts = &cm->counts;

  for (i = 0; i < INTRA_INTER_CONTEXTS; i++)
    fc->intra_inter_prob[i] = adapt_prob(pre_fc->intra_inter_prob[i],
                                         counts->intra_inter[i]);
  for (i = 0; i < COMP_INTER_CONTEXTS; i++)
    fc->comp_inter_prob[i] = adapt_prob(pre_fc->comp_inter_prob[i],
                                        counts->comp_inter[i]);
  for (i = 0; i < REF_CONTEXTS; i++)
    fc->comp_ref_prob[i] = adapt_prob(pre_fc->comp_ref_prob[i],
                                      counts->comp_ref[i]);
  for (i = 0; i < REF_CONTEXTS; i++)
    for (j = 0; j < 2; j++)
      fc->single_ref_prob[i][j] = adapt_prob(pre_fc->single_ref_prob[i][j],
                                             counts->single_ref[i][j]);

  for (i = 0; i < INTER_MODE_CONTEXTS; i++)
    adapt_probs(vp9_inter_mode_tree, pre_fc->inter_mode_probs[i],
                counts->inter_mode[i], fc->inter_mode_probs[i]);

  for (i = 0; i < BLOCK_SIZE_GROUPS; i++)
    adapt_probs(vp9_intra_mode_tree, pre_fc->y_mode_prob[i],
                counts->y_mode[i], fc->y_mode_prob[i]);

  for (i = 0; i < INTRA_MODES; ++i)
    adapt_probs(vp9_intra_mode_tree, pre_fc->uv_mode_prob[i],
                counts->uv_mode[i], fc->uv_mode_prob[i]);

  for (i = 0; i < PARTITION_CONTEXTS; i++)
    adapt_probs(vp9_partition_tree, pre_fc->partition_prob[i],
                counts->partition[i], fc->partition_prob[i]);

  if (cm->interp_filter == SWITCHABLE) {
    for (i = 0; i < SWITCHABLE_FILTER_CONTEXTS; i++)
      adapt_probs(vp9_switchable_interp_tree, pre_fc->switchable_interp_prob[i],
                  counts->switchable_interp[i], fc->switchable_interp_prob[i]);
  }

  if (cm->tx_mode == TX_MODE_SELECT) {
    int j;
    unsigned int branch_ct_8x8p[TX_SIZES - 3][2];
    unsigned int branch_ct_16x16p[TX_SIZES - 2][2];
    unsigned int branch_ct_32x32p[TX_SIZES - 1][2];

    for (i = 0; i < TX_SIZE_CONTEXTS; ++i) {
      tx_counts_to_branch_counts_8x8(counts->tx.p8x8[i], branch_ct_8x8p);
      for (j = 0; j < TX_SIZES - 3; ++j)
        fc->tx_probs.p8x8[i][j] = adapt_prob(pre_fc->tx_probs.p8x8[i][j],
                                             branch_ct_8x8p[j]);

      tx_counts_to_branch_counts_16x16(counts->tx.p16x16[i], branch_ct_16x16p);
      for (j = 0; j < TX_SIZES - 2; ++j)
        fc->tx_probs.p16x16[i][j] = adapt_prob(pre_fc->tx_probs.p16x16[i][j],
                                               branch_ct_16x16p[j]);

      tx_counts_to_branch_counts_32x32(counts->tx.p32x32[i], branch_ct_32x32p);
      for (j = 0; j < TX_SIZES - 1; ++j)
        fc->tx_probs.p32x32[i][j] = adapt_prob(pre_fc->tx_probs.p32x32[i][j],
                                               branch_ct_32x32p[j]);
    }
  }

  for (i = 0; i < SKIP_CONTEXTS; ++i)
    fc->skip_probs[i] = adapt_prob(pre_fc->skip_probs[i], counts->skip[i]);
}

static void set_default_lf_deltas(struct loopfilter *lf) {
  lf->mode_ref_delta_enabled = 1;
  lf->mode_ref_delta_update = 1;

  lf->ref_deltas[INTRA_FRAME] = 1;
  lf->ref_deltas[LAST_FRAME] = 0;
  lf->ref_deltas[GOLDEN_FRAME] = -1;
  lf->ref_deltas[ALTREF_FRAME] = -1;

  lf->mode_deltas[0] = 0;
  lf->mode_deltas[1] = 0;
}

void vp9_setup_past_independence(VP9_COMMON *cm) {
  
  
  struct loopfilter *const lf = &cm->lf;

  int i;
  vp9_clearall_segfeatures(&cm->seg);
  cm->seg.abs_delta = SEGMENT_DELTADATA;
  if (cm->last_frame_seg_map)
    vpx_memset(cm->last_frame_seg_map, 0, (cm->mi_rows * cm->mi_cols));

  
  vp9_zero(lf->last_ref_deltas);
  vp9_zero(lf->last_mode_deltas);
  set_default_lf_deltas(lf);

  
  lf->last_sharpness_level = -1;

  vp9_default_coef_probs(cm);
  vp9_init_mbmode_probs(cm);
  vp9_init_mv_probs(cm);

  if (cm->frame_type == KEY_FRAME ||
      cm->error_resilient_mode || cm->reset_frame_context == 3) {
    
    for (i = 0; i < FRAME_CONTEXTS; ++i)
      cm->frame_contexts[i] = cm->fc;
  } else if (cm->reset_frame_context == 2) {
    
    cm->frame_contexts[cm->frame_context_idx] = cm->fc;
  }

  vpx_memset(cm->prev_mip, 0,
             cm->mode_info_stride * (cm->mi_rows + 1) * sizeof(MODE_INFO));
  vpx_memset(cm->mip, 0,
             cm->mode_info_stride * (cm->mi_rows + 1) * sizeof(MODE_INFO));

  vp9_zero(cm->ref_frame_sign_bias);

  cm->frame_context_idx = 0;
}
