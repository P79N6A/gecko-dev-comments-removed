









#ifndef VP9_ENCODER_VP9_FIRSTPASS_H_
#define VP9_ENCODER_VP9_FIRSTPASS_H_

#include "vp9/encoder/vp9_lookahead.h"
#include "vp9/encoder/vp9_ratectrl.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_FP_MB_STATS

#define FPMB_DCINTRA_MASK 0x01

#define FPMB_MOTION_ZERO_MASK 0x02
#define FPMB_MOTION_LEFT_MASK 0x04
#define FPMB_MOTION_RIGHT_MASK 0x08
#define FPMB_MOTION_UP_MASK 0x10
#define FPMB_MOTION_DOWN_MASK 0x20

#define FPMB_ERROR_SMALL_MASK 0x40
#define FPMB_ERROR_LARGE_MASK 0x80
#define FPMB_ERROR_SMALL_TH 2000
#define FPMB_ERROR_LARGE_TH 48000

typedef struct {
  uint8_t *mb_stats_start;
  uint8_t *mb_stats_end;
} FIRSTPASS_MB_STATS;
#endif

typedef struct {
  double frame;
  double intra_error;
  double coded_error;
  double sr_coded_error;
  double pcnt_inter;
  double pcnt_motion;
  double pcnt_second_ref;
  double pcnt_neutral;
  double MVr;
  double mvr_abs;
  double MVc;
  double mvc_abs;
  double MVrv;
  double MVcv;
  double mv_in_out_count;
  double new_mv_count;
  double duration;
  double count;
  int64_t spatial_layer_id;
} FIRSTPASS_STATS;

typedef enum {
  KF_UPDATE = 0,
  LF_UPDATE = 1,
  GF_UPDATE = 2,
  ARF_UPDATE = 3,
  OVERLAY_UPDATE = 4,
  FRAME_UPDATE_TYPES = 5
} FRAME_UPDATE_TYPE;

typedef struct {
  unsigned char index;
  RATE_FACTOR_LEVEL rf_level[(MAX_LAG_BUFFERS * 2) + 1];
  FRAME_UPDATE_TYPE update_type[(MAX_LAG_BUFFERS * 2) + 1];
  unsigned char arf_src_offset[(MAX_LAG_BUFFERS * 2) + 1];
  unsigned char arf_update_idx[(MAX_LAG_BUFFERS * 2) + 1];
  unsigned char arf_ref_idx[(MAX_LAG_BUFFERS * 2) + 1];
  int bit_allocation[(MAX_LAG_BUFFERS * 2) + 1];
} GF_GROUP;

typedef struct {
  unsigned int section_intra_rating;
  FIRSTPASS_STATS total_stats;
  FIRSTPASS_STATS this_frame_stats;
  const FIRSTPASS_STATS *stats_in;
  const FIRSTPASS_STATS *stats_in_start;
  const FIRSTPASS_STATS *stats_in_end;
  FIRSTPASS_STATS total_left_stats;
  int first_pass_done;
  int64_t bits_left;
  double modified_error_min;
  double modified_error_max;
  double modified_error_left;
  double kf_intra_err_min;
  double gf_intra_err_min;

#if CONFIG_FP_MB_STATS
  uint8_t *frame_mb_stats_buf;
  uint8_t *this_frame_mb_stats;
  FIRSTPASS_MB_STATS firstpass_mb_stats;
#endif

  
  int64_t kf_group_bits;

  
  int64_t kf_group_error_left;
  int sr_update_lag;

  int kf_zeromotion_pct;
  int gf_zeromotion_pct;

  int active_worst_quality;

  GF_GROUP gf_group;
} TWO_PASS;

struct VP9_COMP;

void vp9_init_first_pass(struct VP9_COMP *cpi);
void vp9_rc_get_first_pass_params(struct VP9_COMP *cpi);
void vp9_first_pass(struct VP9_COMP *cpi, const struct lookahead_entry *source);
void vp9_end_first_pass(struct VP9_COMP *cpi);

void vp9_init_second_pass(struct VP9_COMP *cpi);
void vp9_rc_get_second_pass_params(struct VP9_COMP *cpi);


void vp9_twopass_postencode_update(struct VP9_COMP *cpi);
#ifdef __cplusplus
}  
#endif

#endif
