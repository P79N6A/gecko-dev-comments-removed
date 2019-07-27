









#ifndef VP9_ENCODER_VP9_FIRSTPASS_H_
#define VP9_ENCODER_VP9_FIRSTPASS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  double frame;
  double intra_error;
  double coded_error;
  double sr_coded_error;
  double ssim_weighted_pred_err;
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
} FIRSTPASS_STATS;

struct twopass_rc {
  unsigned int section_intra_rating;
  unsigned int next_iiratio;
  unsigned int this_iiratio;
  FIRSTPASS_STATS total_stats;
  FIRSTPASS_STATS this_frame_stats;
  FIRSTPASS_STATS *stats_in, *stats_in_end, *stats_in_start;
  FIRSTPASS_STATS total_left_stats;
  int first_pass_done;
  int64_t bits_left;
  int64_t clip_bits_total;
  double avg_iiratio;
  double modified_error_min;
  double modified_error_max;
  double modified_error_total;
  double modified_error_left;
  double kf_intra_err_min;
  double gf_intra_err_min;
  int static_scene_max_gf_interval;
  int kf_bits;
  
  int64_t gf_group_error_left;

  
  int64_t kf_group_bits;

  
  int64_t kf_group_error_left;

  
  int64_t gf_group_bits;
  
  int gf_bits;
  int alt_extra_bits;

  int sr_update_lag;

  int kf_zeromotion_pct;
  int gf_zeromotion_pct;

  int active_worst_quality;
};

struct VP9_COMP;

void vp9_init_first_pass(struct VP9_COMP *cpi);
void vp9_rc_get_first_pass_params(struct VP9_COMP *cpi);
void vp9_first_pass(struct VP9_COMP *cpi);
void vp9_end_first_pass(struct VP9_COMP *cpi);

void vp9_init_second_pass(struct VP9_COMP *cpi);
void vp9_rc_get_second_pass_params(struct VP9_COMP *cpi);
void vp9_end_second_pass(struct VP9_COMP *cpi);
int vp9_twopass_worst_quality(struct VP9_COMP *cpi, FIRSTPASS_STATS *fpstats,
                              int section_target_bandwitdh);


void vp9_twopass_postencode_update(struct VP9_COMP *cpi,
                                   uint64_t bytes_used);
#ifdef __cplusplus
}  
#endif

#endif
