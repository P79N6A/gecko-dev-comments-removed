









#include "./vpx_config.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_reconinter.h"
#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_seg_common.h"


















static const uint64_t left_64x64_txform_mask[TX_SIZES]= {
    0xffffffffffffffff,  
    0xffffffffffffffff,  
    0x5555555555555555,  
    0x1111111111111111,  
};


















static const uint64_t above_64x64_txform_mask[TX_SIZES]= {
    0xffffffffffffffff,  
    0xffffffffffffffff,  
    0x00ff00ff00ff00ff,  
    0x000000ff000000ff,  
};
















static const uint64_t left_prediction_mask[BLOCK_SIZES] = {
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000101,  
    0x0000000000000001,  
    0x0000000000000101,  
    0x0000000001010101,  
    0x0000000000000101,  
    0x0000000001010101,  
    0x0101010101010101,  
    0x0000000001010101,  
    0x0101010101010101,  
};


static const uint64_t above_prediction_mask[BLOCK_SIZES] = {
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000003,  
    0x0000000000000003,  
    0x0000000000000003,  
    0x000000000000000f,  
    0x000000000000000f,  
    0x000000000000000f,  
    0x00000000000000ff,  
    0x00000000000000ff,  
};



static const uint64_t size_mask[BLOCK_SIZES] = {
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000001,  
    0x0000000000000101,  
    0x0000000000000003,  
    0x0000000000000303,  
    0x0000000003030303,  
    0x0000000000000f0f,  
    0x000000000f0f0f0f,  
    0x0f0f0f0f0f0f0f0f,  
    0x00000000ffffffff,  
    0xffffffffffffffff,  
};


static const uint64_t left_border =  0x1111111111111111;
static const uint64_t above_border = 0x000000ff000000ff;


static const uint16_t left_64x64_txform_mask_uv[TX_SIZES]= {
    0xffff,  
    0xffff,  
    0x5555,  
    0x1111,  
};

static const uint16_t above_64x64_txform_mask_uv[TX_SIZES]= {
    0xffff,  
    0xffff,  
    0x0f0f,  
    0x000f,  
};


static const uint16_t left_prediction_mask_uv[BLOCK_SIZES] = {
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0011,  
    0x0001,  
    0x0011,  
    0x1111,  
    0x0011,  
    0x1111,  
};

static const uint16_t above_prediction_mask_uv[BLOCK_SIZES] = {
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0003,  
    0x0003,  
    0x0003,  
    0x000f,  
    0x000f,  
};


static const uint16_t size_mask_uv[BLOCK_SIZES] = {
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0001,  
    0x0011,  
    0x0003,  
    0x0033,  
    0x3333,  
    0x00ff,  
    0xffff,  
};
static const uint16_t left_border_uv =  0x1111;
static const uint16_t above_border_uv = 0x000f;

static const int mode_lf_lut[MB_MODE_COUNT] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
  1, 1, 0, 1                     
};

static void update_sharpness(loop_filter_info_n *lfi, int sharpness_lvl) {
  int lvl;

  
  for (lvl = 0; lvl <= MAX_LOOP_FILTER; lvl++) {
    
    int block_inside_limit = lvl >> ((sharpness_lvl > 0) + (sharpness_lvl > 4));

    if (sharpness_lvl > 0) {
      if (block_inside_limit > (9 - sharpness_lvl))
        block_inside_limit = (9 - sharpness_lvl);
    }

    if (block_inside_limit < 1)
      block_inside_limit = 1;

    vpx_memset(lfi->lfthr[lvl].lim, block_inside_limit, SIMD_WIDTH);
    vpx_memset(lfi->lfthr[lvl].mblim, (2 * (lvl + 2) + block_inside_limit),
               SIMD_WIDTH);
  }
}

void vp9_loop_filter_init(VP9_COMMON *cm) {
  loop_filter_info_n *lfi = &cm->lf_info;
  struct loopfilter *lf = &cm->lf;
  int lvl;

  
  update_sharpness(lfi, lf->sharpness_level);
  lf->last_sharpness_level = lf->sharpness_level;

  
  for (lvl = 0; lvl <= MAX_LOOP_FILTER; lvl++)
    vpx_memset(lfi->lfthr[lvl].hev_thr, (lvl >> 4), SIMD_WIDTH);
}

void vp9_loop_filter_frame_init(VP9_COMMON *cm, int default_filt_lvl) {
  int seg_id;
  
  
  
  const int scale = 1 << (default_filt_lvl >> 5);
  loop_filter_info_n *const lfi = &cm->lf_info;
  struct loopfilter *const lf = &cm->lf;
  const struct segmentation *const seg = &cm->seg;

  
  if (lf->last_sharpness_level != lf->sharpness_level) {
    update_sharpness(lfi, lf->sharpness_level);
    lf->last_sharpness_level = lf->sharpness_level;
  }

  for (seg_id = 0; seg_id < MAX_SEGMENTS; seg_id++) {
    int lvl_seg = default_filt_lvl;
    if (vp9_segfeature_active(seg, seg_id, SEG_LVL_ALT_LF)) {
      const int data = vp9_get_segdata(seg, seg_id, SEG_LVL_ALT_LF);
      lvl_seg = seg->abs_delta == SEGMENT_ABSDATA
                  ? data
                  : clamp(default_filt_lvl + data, 0, MAX_LOOP_FILTER);
    }

    if (!lf->mode_ref_delta_enabled) {
      
      
      vpx_memset(lfi->lvl[seg_id], lvl_seg, sizeof(lfi->lvl[seg_id]));
    } else {
      int ref, mode;
      const int intra_lvl = lvl_seg + lf->ref_deltas[INTRA_FRAME] * scale;
      lfi->lvl[seg_id][INTRA_FRAME][0] = clamp(intra_lvl, 0, MAX_LOOP_FILTER);

      for (ref = LAST_FRAME; ref < MAX_REF_FRAMES; ++ref) {
        for (mode = 0; mode < MAX_MODE_LF_DELTAS; ++mode) {
          const int inter_lvl = lvl_seg + lf->ref_deltas[ref] * scale
                                        + lf->mode_deltas[mode] * scale;
          lfi->lvl[seg_id][ref][mode] = clamp(inter_lvl, 0, MAX_LOOP_FILTER);
        }
      }
    }
  }
}

static void filter_selectively_vert_row2(PLANE_TYPE plane_type,
                                         uint8_t *s, int pitch,
                                         unsigned int mask_16x16_l,
                                         unsigned int mask_8x8_l,
                                         unsigned int mask_4x4_l,
                                         unsigned int mask_4x4_int_l,
                                         const loop_filter_info_n *lfi_n,
                                         const uint8_t *lfl) {
  const int mask_shift = plane_type ? 4 : 8;
  const int mask_cutoff = plane_type ? 0xf : 0xff;
  const int lfl_forward = plane_type ? 4 : 8;

  unsigned int mask_16x16_0 = mask_16x16_l & mask_cutoff;
  unsigned int mask_8x8_0 = mask_8x8_l & mask_cutoff;
  unsigned int mask_4x4_0 = mask_4x4_l & mask_cutoff;
  unsigned int mask_4x4_int_0 = mask_4x4_int_l & mask_cutoff;
  unsigned int mask_16x16_1 = (mask_16x16_l >> mask_shift) & mask_cutoff;
  unsigned int mask_8x8_1 = (mask_8x8_l >> mask_shift) & mask_cutoff;
  unsigned int mask_4x4_1 = (mask_4x4_l >> mask_shift) & mask_cutoff;
  unsigned int mask_4x4_int_1 = (mask_4x4_int_l >> mask_shift) & mask_cutoff;
  unsigned int mask;

  for (mask = mask_16x16_0 | mask_8x8_0 | mask_4x4_0 | mask_4x4_int_0 |
      mask_16x16_1 | mask_8x8_1 | mask_4x4_1 | mask_4x4_int_1;
      mask; mask >>= 1) {
    const loop_filter_thresh *lfi0 = lfi_n->lfthr + *lfl;
    const loop_filter_thresh *lfi1 = lfi_n->lfthr + *(lfl + lfl_forward);

    
    if (mask & 1) {
      if ((mask_16x16_0 | mask_16x16_1) & 1) {
        if ((mask_16x16_0 & mask_16x16_1) & 1) {
          vp9_lpf_vertical_16_dual(s, pitch, lfi0->mblim, lfi0->lim,
                                   lfi0->hev_thr);
        } else if (mask_16x16_0 & 1) {
          vp9_lpf_vertical_16(s, pitch, lfi0->mblim, lfi0->lim,
                              lfi0->hev_thr);
        } else {
          vp9_lpf_vertical_16(s + 8 *pitch, pitch, lfi1->mblim,
                              lfi1->lim, lfi1->hev_thr);
        }
      }

      if ((mask_8x8_0 | mask_8x8_1) & 1) {
        if ((mask_8x8_0 & mask_8x8_1) & 1) {
          vp9_lpf_vertical_8_dual(s, pitch, lfi0->mblim, lfi0->lim,
                                  lfi0->hev_thr, lfi1->mblim, lfi1->lim,
                                  lfi1->hev_thr);
        } else if (mask_8x8_0 & 1) {
          vp9_lpf_vertical_8(s, pitch, lfi0->mblim, lfi0->lim, lfi0->hev_thr,
                             1);
        } else {
          vp9_lpf_vertical_8(s + 8 * pitch, pitch, lfi1->mblim, lfi1->lim,
                             lfi1->hev_thr, 1);
        }
      }

      if ((mask_4x4_0 | mask_4x4_1) & 1) {
        if ((mask_4x4_0 & mask_4x4_1) & 1) {
          vp9_lpf_vertical_4_dual(s, pitch, lfi0->mblim, lfi0->lim,
                                  lfi0->hev_thr, lfi1->mblim, lfi1->lim,
                                  lfi1->hev_thr);
        } else if (mask_4x4_0 & 1) {
          vp9_lpf_vertical_4(s, pitch, lfi0->mblim, lfi0->lim, lfi0->hev_thr,
                             1);
        } else {
          vp9_lpf_vertical_4(s + 8 * pitch, pitch, lfi1->mblim, lfi1->lim,
                             lfi1->hev_thr, 1);
        }
      }

      if ((mask_4x4_int_0 | mask_4x4_int_1) & 1) {
        if ((mask_4x4_int_0 & mask_4x4_int_1) & 1) {
          vp9_lpf_vertical_4_dual(s + 4, pitch, lfi0->mblim, lfi0->lim,
                                  lfi0->hev_thr, lfi1->mblim, lfi1->lim,
                                  lfi1->hev_thr);
        } else if (mask_4x4_int_0 & 1) {
          vp9_lpf_vertical_4(s + 4, pitch, lfi0->mblim, lfi0->lim,
                             lfi0->hev_thr, 1);
        } else {
          vp9_lpf_vertical_4(s + 8 * pitch + 4, pitch, lfi1->mblim, lfi1->lim,
                             lfi1->hev_thr, 1);
        }
      }
    }

    s += 8;
    lfl += 1;
    mask_16x16_0 >>= 1;
    mask_8x8_0 >>= 1;
    mask_4x4_0 >>= 1;
    mask_4x4_int_0 >>= 1;
    mask_16x16_1 >>= 1;
    mask_8x8_1 >>= 1;
    mask_4x4_1 >>= 1;
    mask_4x4_int_1 >>= 1;
  }
}

static void filter_selectively_horiz(uint8_t *s, int pitch,
                                     unsigned int mask_16x16,
                                     unsigned int mask_8x8,
                                     unsigned int mask_4x4,
                                     unsigned int mask_4x4_int,
                                     const loop_filter_info_n *lfi_n,
                                     const uint8_t *lfl) {
  unsigned int mask;
  int count;

  for (mask = mask_16x16 | mask_8x8 | mask_4x4 | mask_4x4_int;
       mask; mask >>= count) {
    const loop_filter_thresh *lfi = lfi_n->lfthr + *lfl;

    count = 1;
    if (mask & 1) {
      if (mask_16x16 & 1) {
        if ((mask_16x16 & 3) == 3) {
          vp9_lpf_horizontal_16(s, pitch, lfi->mblim, lfi->lim,
                                lfi->hev_thr, 2);
          count = 2;
        } else {
          vp9_lpf_horizontal_16(s, pitch, lfi->mblim, lfi->lim,
                                lfi->hev_thr, 1);
        }
      } else if (mask_8x8 & 1) {
        if ((mask_8x8 & 3) == 3) {
          
          const loop_filter_thresh *lfin = lfi_n->lfthr + *(lfl + 1);

          vp9_lpf_horizontal_8_dual(s, pitch, lfi->mblim, lfi->lim,
                                    lfi->hev_thr, lfin->mblim, lfin->lim,
                                    lfin->hev_thr);

          if ((mask_4x4_int & 3) == 3) {
            vp9_lpf_horizontal_4_dual(s + 4 * pitch, pitch, lfi->mblim,
                                      lfi->lim, lfi->hev_thr, lfin->mblim,
                                      lfin->lim, lfin->hev_thr);
          } else {
            if (mask_4x4_int & 1)
              vp9_lpf_horizontal_4(s + 4 * pitch, pitch, lfi->mblim, lfi->lim,
                                   lfi->hev_thr, 1);
            else if (mask_4x4_int & 2)
              vp9_lpf_horizontal_4(s + 8 + 4 * pitch, pitch, lfin->mblim,
                                   lfin->lim, lfin->hev_thr, 1);
          }
          count = 2;
        } else {
          vp9_lpf_horizontal_8(s, pitch, lfi->mblim, lfi->lim, lfi->hev_thr, 1);

          if (mask_4x4_int & 1)
            vp9_lpf_horizontal_4(s + 4 * pitch, pitch, lfi->mblim, lfi->lim,
                                 lfi->hev_thr, 1);
        }
      } else if (mask_4x4 & 1) {
        if ((mask_4x4 & 3) == 3) {
          
          const loop_filter_thresh *lfin = lfi_n->lfthr + *(lfl + 1);

          vp9_lpf_horizontal_4_dual(s, pitch, lfi->mblim, lfi->lim,
                                    lfi->hev_thr, lfin->mblim, lfin->lim,
                                    lfin->hev_thr);
          if ((mask_4x4_int & 3) == 3) {
            vp9_lpf_horizontal_4_dual(s + 4 * pitch, pitch, lfi->mblim,
                                      lfi->lim, lfi->hev_thr, lfin->mblim,
                                      lfin->lim, lfin->hev_thr);
          } else {
            if (mask_4x4_int & 1)
              vp9_lpf_horizontal_4(s + 4 * pitch, pitch, lfi->mblim, lfi->lim,
                                   lfi->hev_thr, 1);
            else if (mask_4x4_int & 2)
              vp9_lpf_horizontal_4(s + 8 + 4 * pitch, pitch, lfin->mblim,
                                   lfin->lim, lfin->hev_thr, 1);
          }
          count = 2;
        } else {
          vp9_lpf_horizontal_4(s, pitch, lfi->mblim, lfi->lim, lfi->hev_thr, 1);

          if (mask_4x4_int & 1)
            vp9_lpf_horizontal_4(s + 4 * pitch, pitch, lfi->mblim, lfi->lim,
                                 lfi->hev_thr, 1);
        }
      } else if (mask_4x4_int & 1) {
        vp9_lpf_horizontal_4(s + 4 * pitch, pitch, lfi->mblim, lfi->lim,
                             lfi->hev_thr, 1);
      }
    }
    s += 8 * count;
    lfl += count;
    mask_16x16 >>= count;
    mask_8x8 >>= count;
    mask_4x4 >>= count;
    mask_4x4_int >>= count;
  }
}








static void build_masks(const loop_filter_info_n *const lfi_n,
                        const MODE_INFO *mi, const int shift_y,
                        const int shift_uv,
                        LOOP_FILTER_MASK *lfm) {
  const BLOCK_SIZE block_size = mi->mbmi.sb_type;
  const TX_SIZE tx_size_y = mi->mbmi.tx_size;
  const TX_SIZE tx_size_uv = get_uv_tx_size(&mi->mbmi);
  const int skip = mi->mbmi.skip;
  const int seg = mi->mbmi.segment_id;
  const int ref = mi->mbmi.ref_frame[0];
  const int filter_level = lfi_n->lvl[seg][ref][mode_lf_lut[mi->mbmi.mode]];
  uint64_t *left_y = &lfm->left_y[tx_size_y];
  uint64_t *above_y = &lfm->above_y[tx_size_y];
  uint64_t *int_4x4_y = &lfm->int_4x4_y;
  uint16_t *left_uv = &lfm->left_uv[tx_size_uv];
  uint16_t *above_uv = &lfm->above_uv[tx_size_uv];
  uint16_t *int_4x4_uv = &lfm->int_4x4_uv;
  int i;
  int w = num_8x8_blocks_wide_lookup[block_size];
  int h = num_8x8_blocks_high_lookup[block_size];

  
  if (!filter_level) {
    return;
  } else {
    int index = shift_y;
    for (i = 0; i < h; i++) {
      vpx_memset(&lfm->lfl_y[index], filter_level, w);
      index += 8;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  *above_y |= above_prediction_mask[block_size] << shift_y;
  *above_uv |= above_prediction_mask_uv[block_size] << shift_uv;
  *left_y |= left_prediction_mask[block_size] << shift_y;
  *left_uv |= left_prediction_mask_uv[block_size] << shift_uv;

  
  
  if (skip && ref > INTRA_FRAME)
    return;

  
  
  
  
  *above_y |= (size_mask[block_size] &
               above_64x64_txform_mask[tx_size_y]) << shift_y;
  *above_uv |= (size_mask_uv[block_size] &
                above_64x64_txform_mask_uv[tx_size_uv]) << shift_uv;

  *left_y |= (size_mask[block_size] &
              left_64x64_txform_mask[tx_size_y]) << shift_y;
  *left_uv |= (size_mask_uv[block_size] &
               left_64x64_txform_mask_uv[tx_size_uv]) << shift_uv;

  
  
  
  
  if (tx_size_y == TX_4X4) {
    *int_4x4_y |= (size_mask[block_size] & 0xffffffffffffffff) << shift_y;
  }
  if (tx_size_uv == TX_4X4) {
    *int_4x4_uv |= (size_mask_uv[block_size] & 0xffff) << shift_uv;
  }
}




static void build_y_mask(const loop_filter_info_n *const lfi_n,
                         const MODE_INFO *mi, const int shift_y,
                         LOOP_FILTER_MASK *lfm) {
  const BLOCK_SIZE block_size = mi->mbmi.sb_type;
  const TX_SIZE tx_size_y = mi->mbmi.tx_size;
  const int skip = mi->mbmi.skip;
  const int seg = mi->mbmi.segment_id;
  const int ref = mi->mbmi.ref_frame[0];
  const int filter_level = lfi_n->lvl[seg][ref][mode_lf_lut[mi->mbmi.mode]];
  uint64_t *left_y = &lfm->left_y[tx_size_y];
  uint64_t *above_y = &lfm->above_y[tx_size_y];
  uint64_t *int_4x4_y = &lfm->int_4x4_y;
  int i;
  int w = num_8x8_blocks_wide_lookup[block_size];
  int h = num_8x8_blocks_high_lookup[block_size];

  if (!filter_level) {
    return;
  } else {
    int index = shift_y;
    for (i = 0; i < h; i++) {
      vpx_memset(&lfm->lfl_y[index], filter_level, w);
      index += 8;
    }
  }

  *above_y |= above_prediction_mask[block_size] << shift_y;
  *left_y |= left_prediction_mask[block_size] << shift_y;

  if (skip && ref > INTRA_FRAME)
    return;

  *above_y |= (size_mask[block_size] &
               above_64x64_txform_mask[tx_size_y]) << shift_y;

  *left_y |= (size_mask[block_size] &
              left_64x64_txform_mask[tx_size_y]) << shift_y;

  if (tx_size_y == TX_4X4) {
    *int_4x4_y |= (size_mask[block_size] & 0xffffffffffffffff) << shift_y;
  }
}




void vp9_setup_mask(VP9_COMMON *const cm, const int mi_row, const int mi_col,
                    MODE_INFO **mi_8x8, const int mode_info_stride,
                    LOOP_FILTER_MASK *lfm) {
  int idx_32, idx_16, idx_8;
  const loop_filter_info_n *const lfi_n = &cm->lf_info;
  MODE_INFO **mip = mi_8x8;
  MODE_INFO **mip2 = mi_8x8;

  
  
  
  
  const int offset_32[] = {4, (mode_info_stride << 2) - 4, 4,
                           -(mode_info_stride << 2) - 4};
  const int offset_16[] = {2, (mode_info_stride << 1) - 2, 2,
                           -(mode_info_stride << 1) - 2};
  const int offset[] = {1, mode_info_stride - 1, 1, -mode_info_stride - 1};

  
  
  
  
  const int shift_32_y[] = {0, 4, 32, 36};
  const int shift_16_y[] = {0, 2, 16, 18};
  const int shift_8_y[] = {0, 1, 8, 9};
  const int shift_32_uv[] = {0, 2, 8, 10};
  const int shift_16_uv[] = {0, 1, 4, 5};
  int i;
  const int max_rows = (mi_row + MI_BLOCK_SIZE > cm->mi_rows ?
                        cm->mi_rows - mi_row : MI_BLOCK_SIZE);
  const int max_cols = (mi_col + MI_BLOCK_SIZE > cm->mi_cols ?
                        cm->mi_cols - mi_col : MI_BLOCK_SIZE);

  vp9_zero(*lfm);

  
  
  
  switch (mip[0]->mbmi.sb_type) {
    case BLOCK_64X64:
      build_masks(lfi_n, mip[0] , 0, 0, lfm);
      break;
    case BLOCK_64X32:
      build_masks(lfi_n, mip[0], 0, 0, lfm);
      mip2 = mip + mode_info_stride * 4;
      if (4 >= max_rows)
        break;
      build_masks(lfi_n, mip2[0], 32, 8, lfm);
      break;
    case BLOCK_32X64:
      build_masks(lfi_n, mip[0], 0, 0, lfm);
      mip2 = mip + 4;
      if (4 >= max_cols)
        break;
      build_masks(lfi_n, mip2[0], 4, 2, lfm);
      break;
    default:
      for (idx_32 = 0; idx_32 < 4; mip += offset_32[idx_32], ++idx_32) {
        const int shift_y = shift_32_y[idx_32];
        const int shift_uv = shift_32_uv[idx_32];
        const int mi_32_col_offset = ((idx_32 & 1) << 2);
        const int mi_32_row_offset = ((idx_32 >> 1) << 2);
        if (mi_32_col_offset >= max_cols || mi_32_row_offset >= max_rows)
          continue;
        switch (mip[0]->mbmi.sb_type) {
          case BLOCK_32X32:
            build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
            break;
          case BLOCK_32X16:
            build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
            if (mi_32_row_offset + 2 >= max_rows)
              continue;
            mip2 = mip + mode_info_stride * 2;
            build_masks(lfi_n, mip2[0], shift_y + 16, shift_uv + 4, lfm);
            break;
          case BLOCK_16X32:
            build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
            if (mi_32_col_offset + 2 >= max_cols)
              continue;
            mip2 = mip + 2;
            build_masks(lfi_n, mip2[0], shift_y + 2, shift_uv + 1, lfm);
            break;
          default:
            for (idx_16 = 0; idx_16 < 4; mip += offset_16[idx_16], ++idx_16) {
              const int shift_y = shift_32_y[idx_32] + shift_16_y[idx_16];
              const int shift_uv = shift_32_uv[idx_32] + shift_16_uv[idx_16];
              const int mi_16_col_offset = mi_32_col_offset +
                  ((idx_16 & 1) << 1);
              const int mi_16_row_offset = mi_32_row_offset +
                  ((idx_16 >> 1) << 1);

              if (mi_16_col_offset >= max_cols || mi_16_row_offset >= max_rows)
                continue;

              switch (mip[0]->mbmi.sb_type) {
                case BLOCK_16X16:
                  build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
                  break;
                case BLOCK_16X8:
                  build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
                  if (mi_16_row_offset + 1 >= max_rows)
                    continue;
                  mip2 = mip + mode_info_stride;
                  build_y_mask(lfi_n, mip2[0], shift_y+8, lfm);
                  break;
                case BLOCK_8X16:
                  build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
                  if (mi_16_col_offset +1 >= max_cols)
                    continue;
                  mip2 = mip + 1;
                  build_y_mask(lfi_n, mip2[0], shift_y+1, lfm);
                  break;
                default: {
                  const int shift_y = shift_32_y[idx_32] +
                                      shift_16_y[idx_16] +
                                      shift_8_y[0];
                  build_masks(lfi_n, mip[0], shift_y, shift_uv, lfm);
                  mip += offset[0];
                  for (idx_8 = 1; idx_8 < 4; mip += offset[idx_8], ++idx_8) {
                    const int shift_y = shift_32_y[idx_32] +
                                        shift_16_y[idx_16] +
                                        shift_8_y[idx_8];
                    const int mi_8_col_offset = mi_16_col_offset +
                        ((idx_8 & 1));
                    const int mi_8_row_offset = mi_16_row_offset +
                        ((idx_8 >> 1));

                    if (mi_8_col_offset >= max_cols ||
                        mi_8_row_offset >= max_rows)
                      continue;
                    build_y_mask(lfi_n, mip[0], shift_y, lfm);
                  }
                  break;
                }
              }
            }
            break;
        }
      }
      break;
  }
  
  
  lfm->left_y[TX_16X16] |= lfm->left_y[TX_32X32];
  lfm->above_y[TX_16X16] |= lfm->above_y[TX_32X32];
  lfm->left_uv[TX_16X16] |= lfm->left_uv[TX_32X32];
  lfm->above_uv[TX_16X16] |= lfm->above_uv[TX_32X32];

  
  
  
  lfm->left_y[TX_8X8] |= lfm->left_y[TX_4X4] & left_border;
  lfm->left_y[TX_4X4] &= ~left_border;
  lfm->above_y[TX_8X8] |= lfm->above_y[TX_4X4] & above_border;
  lfm->above_y[TX_4X4] &= ~above_border;
  lfm->left_uv[TX_8X8] |= lfm->left_uv[TX_4X4] & left_border_uv;
  lfm->left_uv[TX_4X4] &= ~left_border_uv;
  lfm->above_uv[TX_8X8] |= lfm->above_uv[TX_4X4] & above_border_uv;
  lfm->above_uv[TX_4X4] &= ~above_border_uv;

  
  if (mi_row + MI_BLOCK_SIZE > cm->mi_rows) {
    const uint64_t rows = cm->mi_rows - mi_row;

    
    const uint64_t mask_y = (((uint64_t) 1 << (rows << 3)) - 1);
    const uint16_t mask_uv = (((uint16_t) 1 << (((rows + 1) >> 1) << 2)) - 1);

    
    for (i = 0; i < TX_32X32; i++) {
      lfm->left_y[i] &= mask_y;
      lfm->above_y[i] &= mask_y;
      lfm->left_uv[i] &= mask_uv;
      lfm->above_uv[i] &= mask_uv;
    }
    lfm->int_4x4_y &= mask_y;
    lfm->int_4x4_uv &= mask_uv;

    
    
    if (rows == 1) {
      lfm->above_uv[TX_8X8] |= lfm->above_uv[TX_16X16];
      lfm->above_uv[TX_16X16] = 0;
    }
    if (rows == 5) {
      lfm->above_uv[TX_8X8] |= lfm->above_uv[TX_16X16] & 0xff00;
      lfm->above_uv[TX_16X16] &= ~(lfm->above_uv[TX_16X16] & 0xff00);
    }
  }

  if (mi_col + MI_BLOCK_SIZE > cm->mi_cols) {
    const uint64_t columns = cm->mi_cols - mi_col;

    
    
    const uint64_t mask_y  = (((1 << columns) - 1)) * 0x0101010101010101;
    const uint16_t mask_uv = ((1 << ((columns + 1) >> 1)) - 1) * 0x1111;

    
    
    const uint16_t mask_uv_int = ((1 << (columns >> 1)) - 1) * 0x1111;

    
    for (i = 0; i < TX_32X32; i++) {
      lfm->left_y[i] &= mask_y;
      lfm->above_y[i] &= mask_y;
      lfm->left_uv[i] &= mask_uv;
      lfm->above_uv[i] &= mask_uv;
    }
    lfm->int_4x4_y &= mask_y;
    lfm->int_4x4_uv &= mask_uv_int;

    
    
    if (columns == 1) {
      lfm->left_uv[TX_8X8] |= lfm->left_uv[TX_16X16];
      lfm->left_uv[TX_16X16] = 0;
    }
    if (columns == 5) {
      lfm->left_uv[TX_8X8] |= (lfm->left_uv[TX_16X16] & 0xcccc);
      lfm->left_uv[TX_16X16] &= ~(lfm->left_uv[TX_16X16] & 0xcccc);
    }
  }
  
  if (mi_col == 0) {
    for (i = 0; i < TX_32X32; i++) {
      lfm->left_y[i] &= 0xfefefefefefefefe;
      lfm->left_uv[i] &= 0xeeee;
    }
  }

  
  assert(!(lfm->left_y[TX_16X16] & lfm->left_y[TX_8X8]));
  assert(!(lfm->left_y[TX_16X16] & lfm->left_y[TX_4X4]));
  assert(!(lfm->left_y[TX_8X8] & lfm->left_y[TX_4X4]));
  assert(!(lfm->int_4x4_y & lfm->left_y[TX_16X16]));
  assert(!(lfm->left_uv[TX_16X16]&lfm->left_uv[TX_8X8]));
  assert(!(lfm->left_uv[TX_16X16] & lfm->left_uv[TX_4X4]));
  assert(!(lfm->left_uv[TX_8X8] & lfm->left_uv[TX_4X4]));
  assert(!(lfm->int_4x4_uv & lfm->left_uv[TX_16X16]));
  assert(!(lfm->above_y[TX_16X16] & lfm->above_y[TX_8X8]));
  assert(!(lfm->above_y[TX_16X16] & lfm->above_y[TX_4X4]));
  assert(!(lfm->above_y[TX_8X8] & lfm->above_y[TX_4X4]));
  assert(!(lfm->int_4x4_y & lfm->above_y[TX_16X16]));
  assert(!(lfm->above_uv[TX_16X16] & lfm->above_uv[TX_8X8]));
  assert(!(lfm->above_uv[TX_16X16] & lfm->above_uv[TX_4X4]));
  assert(!(lfm->above_uv[TX_8X8] & lfm->above_uv[TX_4X4]));
  assert(!(lfm->int_4x4_uv & lfm->above_uv[TX_16X16]));
}

#if CONFIG_NON420
static uint8_t build_lfi(const loop_filter_info_n *lfi_n,
                     const MB_MODE_INFO *mbmi) {
  const int seg = mbmi->segment_id;
  const int ref = mbmi->ref_frame[0];
  return lfi_n->lvl[seg][ref][mode_lf_lut[mbmi->mode]];
}

static void filter_selectively_vert(uint8_t *s, int pitch,
                                    unsigned int mask_16x16,
                                    unsigned int mask_8x8,
                                    unsigned int mask_4x4,
                                    unsigned int mask_4x4_int,
                                    const loop_filter_info_n *lfi_n,
                                    const uint8_t *lfl) {
  unsigned int mask;

  for (mask = mask_16x16 | mask_8x8 | mask_4x4 | mask_4x4_int;
       mask; mask >>= 1) {
    const loop_filter_thresh *lfi = lfi_n->lfthr + *lfl;

    if (mask & 1) {
      if (mask_16x16 & 1) {
        vp9_lpf_vertical_16(s, pitch, lfi->mblim, lfi->lim, lfi->hev_thr);
      } else if (mask_8x8 & 1) {
        vp9_lpf_vertical_8(s, pitch, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
      } else if (mask_4x4 & 1) {
        vp9_lpf_vertical_4(s, pitch, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
      }
    }
    if (mask_4x4_int & 1)
      vp9_lpf_vertical_4(s + 4, pitch, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
    s += 8;
    lfl += 1;
    mask_16x16 >>= 1;
    mask_8x8 >>= 1;
    mask_4x4 >>= 1;
    mask_4x4_int >>= 1;
  }
}

static void filter_block_plane_non420(VP9_COMMON *cm,
                                      struct macroblockd_plane *plane,
                                      MODE_INFO **mi_8x8,
                                      int mi_row, int mi_col) {
  const int ss_x = plane->subsampling_x;
  const int ss_y = plane->subsampling_y;
  const int row_step = 1 << ss_x;
  const int col_step = 1 << ss_y;
  const int row_step_stride = cm->mode_info_stride * row_step;
  struct buf_2d *const dst = &plane->dst;
  uint8_t* const dst0 = dst->buf;
  unsigned int mask_16x16[MI_BLOCK_SIZE] = {0};
  unsigned int mask_8x8[MI_BLOCK_SIZE] = {0};
  unsigned int mask_4x4[MI_BLOCK_SIZE] = {0};
  unsigned int mask_4x4_int[MI_BLOCK_SIZE] = {0};
  uint8_t lfl[MI_BLOCK_SIZE * MI_BLOCK_SIZE];
  int r, c;

  for (r = 0; r < MI_BLOCK_SIZE && mi_row + r < cm->mi_rows; r += row_step) {
    unsigned int mask_16x16_c = 0;
    unsigned int mask_8x8_c = 0;
    unsigned int mask_4x4_c = 0;
    unsigned int border_mask;

    
    for (c = 0; c < MI_BLOCK_SIZE && mi_col + c < cm->mi_cols; c += col_step) {
      const MODE_INFO *mi = mi_8x8[c];
      const BLOCK_SIZE sb_type = mi[0].mbmi.sb_type;
      const int skip_this = mi[0].mbmi.skip && is_inter_block(&mi[0].mbmi);
      
      const int block_edge_left = (num_4x4_blocks_wide_lookup[sb_type] > 1) ?
          !(c & (num_8x8_blocks_wide_lookup[sb_type] - 1)) : 1;
      const int skip_this_c = skip_this && !block_edge_left;
      
      const int block_edge_above = (num_4x4_blocks_high_lookup[sb_type] > 1) ?
          !(r & (num_8x8_blocks_high_lookup[sb_type] - 1)) : 1;
      const int skip_this_r = skip_this && !block_edge_above;
      const TX_SIZE tx_size = (plane->plane_type == PLANE_TYPE_UV)
                            ? get_uv_tx_size(&mi[0].mbmi)
                            : mi[0].mbmi.tx_size;
      const int skip_border_4x4_c = ss_x && mi_col + c == cm->mi_cols - 1;
      const int skip_border_4x4_r = ss_y && mi_row + r == cm->mi_rows - 1;

      
      if (!(lfl[(r << 3) + (c >> ss_x)] =
          build_lfi(&cm->lf_info, &mi[0].mbmi)))
        continue;

      
      if (tx_size == TX_32X32) {
        if (!skip_this_c && ((c >> ss_x) & 3) == 0) {
          if (!skip_border_4x4_c)
            mask_16x16_c |= 1 << (c >> ss_x);
          else
            mask_8x8_c |= 1 << (c >> ss_x);
        }
        if (!skip_this_r && ((r >> ss_y) & 3) == 0) {
          if (!skip_border_4x4_r)
            mask_16x16[r] |= 1 << (c >> ss_x);
          else
            mask_8x8[r] |= 1 << (c >> ss_x);
        }
      } else if (tx_size == TX_16X16) {
        if (!skip_this_c && ((c >> ss_x) & 1) == 0) {
          if (!skip_border_4x4_c)
            mask_16x16_c |= 1 << (c >> ss_x);
          else
            mask_8x8_c |= 1 << (c >> ss_x);
        }
        if (!skip_this_r && ((r >> ss_y) & 1) == 0) {
          if (!skip_border_4x4_r)
            mask_16x16[r] |= 1 << (c >> ss_x);
          else
            mask_8x8[r] |= 1 << (c >> ss_x);
        }
      } else {
        
        if (!skip_this_c) {
          if (tx_size == TX_8X8 || ((c >> ss_x) & 3) == 0)
            mask_8x8_c |= 1 << (c >> ss_x);
          else
            mask_4x4_c |= 1 << (c >> ss_x);
        }

        if (!skip_this_r) {
          if (tx_size == TX_8X8 || ((r >> ss_y) & 3) == 0)
            mask_8x8[r] |= 1 << (c >> ss_x);
          else
            mask_4x4[r] |= 1 << (c >> ss_x);
        }

        if (!skip_this && tx_size < TX_8X8 && !skip_border_4x4_c)
          mask_4x4_int[r] |= 1 << (c >> ss_x);
      }
    }

    
    border_mask = ~(mi_col == 0);
    filter_selectively_vert(dst->buf, dst->stride,
                            mask_16x16_c & border_mask,
                            mask_8x8_c & border_mask,
                            mask_4x4_c & border_mask,
                            mask_4x4_int[r],
                            &cm->lf_info, &lfl[r << 3]);
    dst->buf += 8 * dst->stride;
    mi_8x8 += row_step_stride;
  }

  
  dst->buf = dst0;
  for (r = 0; r < MI_BLOCK_SIZE && mi_row + r < cm->mi_rows; r += row_step) {
    const int skip_border_4x4_r = ss_y && mi_row + r == cm->mi_rows - 1;
    const unsigned int mask_4x4_int_r = skip_border_4x4_r ? 0 : mask_4x4_int[r];

    unsigned int mask_16x16_r;
    unsigned int mask_8x8_r;
    unsigned int mask_4x4_r;

    if (mi_row + r == 0) {
      mask_16x16_r = 0;
      mask_8x8_r = 0;
      mask_4x4_r = 0;
    } else {
      mask_16x16_r = mask_16x16[r];
      mask_8x8_r = mask_8x8[r];
      mask_4x4_r = mask_4x4[r];
    }

    filter_selectively_horiz(dst->buf, dst->stride,
                             mask_16x16_r,
                             mask_8x8_r,
                             mask_4x4_r,
                             mask_4x4_int_r,
                             &cm->lf_info, &lfl[r << 3]);
    dst->buf += 8 * dst->stride;
  }
}
#endif

void vp9_filter_block_plane(VP9_COMMON *const cm,
                            struct macroblockd_plane *const plane,
                            int mi_row,
                            LOOP_FILTER_MASK *lfm) {
  struct buf_2d *const dst = &plane->dst;
  uint8_t* const dst0 = dst->buf;
  int r, c;

  if (!plane->plane_type) {
    uint64_t mask_16x16 = lfm->left_y[TX_16X16];
    uint64_t mask_8x8 = lfm->left_y[TX_8X8];
    uint64_t mask_4x4 = lfm->left_y[TX_4X4];
    uint64_t mask_4x4_int = lfm->int_4x4_y;

    
    for (r = 0; r < MI_BLOCK_SIZE && mi_row + r < cm->mi_rows; r += 2) {
      unsigned int mask_16x16_l = mask_16x16 & 0xffff;
      unsigned int mask_8x8_l = mask_8x8 & 0xffff;
      unsigned int mask_4x4_l = mask_4x4 & 0xffff;
      unsigned int mask_4x4_int_l = mask_4x4_int & 0xffff;

      
      filter_selectively_vert_row2(plane->plane_type,
                                   dst->buf, dst->stride,
                                   mask_16x16_l,
                                   mask_8x8_l,
                                   mask_4x4_l,
                                   mask_4x4_int_l,
                                   &cm->lf_info, &lfm->lfl_y[r << 3]);

      dst->buf += 16 * dst->stride;
      mask_16x16 >>= 16;
      mask_8x8 >>= 16;
      mask_4x4 >>= 16;
      mask_4x4_int >>= 16;
    }

    
    dst->buf = dst0;
    mask_16x16 = lfm->above_y[TX_16X16];
    mask_8x8 = lfm->above_y[TX_8X8];
    mask_4x4 = lfm->above_y[TX_4X4];
    mask_4x4_int = lfm->int_4x4_y;

    for (r = 0; r < MI_BLOCK_SIZE && mi_row + r < cm->mi_rows; r++) {
      unsigned int mask_16x16_r;
      unsigned int mask_8x8_r;
      unsigned int mask_4x4_r;

      if (mi_row + r == 0) {
        mask_16x16_r = 0;
        mask_8x8_r = 0;
        mask_4x4_r = 0;
      } else {
        mask_16x16_r = mask_16x16 & 0xff;
        mask_8x8_r = mask_8x8 & 0xff;
        mask_4x4_r = mask_4x4 & 0xff;
      }

      filter_selectively_horiz(dst->buf, dst->stride,
                               mask_16x16_r,
                               mask_8x8_r,
                               mask_4x4_r,
                               mask_4x4_int & 0xff,
                               &cm->lf_info, &lfm->lfl_y[r << 3]);

      dst->buf += 8 * dst->stride;
      mask_16x16 >>= 8;
      mask_8x8 >>= 8;
      mask_4x4 >>= 8;
      mask_4x4_int >>= 8;
    }
  } else {
    uint16_t mask_16x16 = lfm->left_uv[TX_16X16];
    uint16_t mask_8x8 = lfm->left_uv[TX_8X8];
    uint16_t mask_4x4 = lfm->left_uv[TX_4X4];
    uint16_t mask_4x4_int = lfm->int_4x4_uv;

    
    for (r = 0; r < MI_BLOCK_SIZE && mi_row + r < cm->mi_rows; r += 4) {
      if (plane->plane_type == 1) {
        for (c = 0; c < (MI_BLOCK_SIZE >> 1); c++) {
          lfm->lfl_uv[(r << 1) + c] = lfm->lfl_y[(r << 3) + (c << 1)];
          lfm->lfl_uv[((r + 2) << 1) + c] = lfm->lfl_y[((r + 2) << 3) +
                                                       (c << 1)];
        }
      }

      {
        unsigned int mask_16x16_l = mask_16x16 & 0xff;
        unsigned int mask_8x8_l = mask_8x8 & 0xff;
        unsigned int mask_4x4_l = mask_4x4 & 0xff;
        unsigned int mask_4x4_int_l = mask_4x4_int & 0xff;

        
        filter_selectively_vert_row2(plane->plane_type,
                                     dst->buf, dst->stride,
                                     mask_16x16_l,
                                     mask_8x8_l,
                                     mask_4x4_l,
                                     mask_4x4_int_l,
                                     &cm->lf_info, &lfm->lfl_uv[r << 1]);

        dst->buf += 16 * dst->stride;
        mask_16x16 >>= 8;
        mask_8x8 >>= 8;
        mask_4x4 >>= 8;
        mask_4x4_int >>= 8;
      }
    }

    
    dst->buf = dst0;
    mask_16x16 = lfm->above_uv[TX_16X16];
    mask_8x8 = lfm->above_uv[TX_8X8];
    mask_4x4 = lfm->above_uv[TX_4X4];
    mask_4x4_int = lfm->int_4x4_uv;

    for (r = 0; r < MI_BLOCK_SIZE && mi_row + r < cm->mi_rows; r += 2) {
      const int skip_border_4x4_r = mi_row + r == cm->mi_rows - 1;
      const unsigned int mask_4x4_int_r = skip_border_4x4_r ?
          0 : (mask_4x4_int & 0xf);
      unsigned int mask_16x16_r;
      unsigned int mask_8x8_r;
      unsigned int mask_4x4_r;

      if (mi_row + r == 0) {
        mask_16x16_r = 0;
        mask_8x8_r = 0;
        mask_4x4_r = 0;
      } else {
        mask_16x16_r = mask_16x16 & 0xf;
        mask_8x8_r = mask_8x8 & 0xf;
        mask_4x4_r = mask_4x4 & 0xf;
      }

      filter_selectively_horiz(dst->buf, dst->stride,
                               mask_16x16_r,
                               mask_8x8_r,
                               mask_4x4_r,
                               mask_4x4_int_r,
                               &cm->lf_info, &lfm->lfl_uv[r << 1]);

      dst->buf += 8 * dst->stride;
      mask_16x16 >>= 4;
      mask_8x8 >>= 4;
      mask_4x4 >>= 4;
      mask_4x4_int >>= 4;
    }
  }
}

void vp9_loop_filter_rows(const YV12_BUFFER_CONFIG *frame_buffer,
                          VP9_COMMON *cm, MACROBLOCKD *xd,
                          int start, int stop, int y_only) {
  const int num_planes = y_only ? 1 : MAX_MB_PLANE;
  int mi_row, mi_col;
  LOOP_FILTER_MASK lfm;
#if CONFIG_NON420
  int use_420 = y_only || (xd->plane[1].subsampling_y == 1 &&
      xd->plane[1].subsampling_x == 1);
#endif

  for (mi_row = start; mi_row < stop; mi_row += MI_BLOCK_SIZE) {
    MODE_INFO **mi_8x8 = cm->mi_grid_visible + mi_row * cm->mode_info_stride;

    for (mi_col = 0; mi_col < cm->mi_cols; mi_col += MI_BLOCK_SIZE) {
      int plane;

      setup_dst_planes(xd, frame_buffer, mi_row, mi_col);

      
#if CONFIG_NON420
      if (use_420)
#endif
        vp9_setup_mask(cm, mi_row, mi_col, mi_8x8 + mi_col,
                       cm->mode_info_stride, &lfm);

      for (plane = 0; plane < num_planes; ++plane) {
#if CONFIG_NON420
        if (use_420)
#endif
          vp9_filter_block_plane(cm, &xd->plane[plane], mi_row, &lfm);
#if CONFIG_NON420
        else
          filter_block_plane_non420(cm, &xd->plane[plane], mi_8x8 + mi_col,
                                    mi_row, mi_col);
#endif
      }
    }
  }
}

void vp9_loop_filter_frame(VP9_COMMON *cm, MACROBLOCKD *xd,
                           int frame_filter_level,
                           int y_only, int partial_frame) {
  int start_mi_row, end_mi_row, mi_rows_to_filter;
  if (!frame_filter_level) return;
  start_mi_row = 0;
  mi_rows_to_filter = cm->mi_rows;
  if (partial_frame && cm->mi_rows > 8) {
    start_mi_row = cm->mi_rows >> 1;
    start_mi_row &= 0xfffffff8;
    mi_rows_to_filter = MAX(cm->mi_rows / 8, 8);
  }
  end_mi_row = start_mi_row + mi_rows_to_filter;
  vp9_loop_filter_frame_init(cm, frame_filter_level);
  vp9_loop_filter_rows(cm->frame_to_show, cm, xd,
                       start_mi_row, end_mi_row,
                       y_only);
}

int vp9_loop_filter_worker(void *arg1, void *arg2) {
  LFWorkerData *const lf_data = (LFWorkerData*)arg1;
  (void)arg2;
  vp9_loop_filter_rows(lf_data->frame_buffer, lf_data->cm, &lf_data->xd,
                       lf_data->start, lf_data->stop, lf_data->y_only);
  return 1;
}
