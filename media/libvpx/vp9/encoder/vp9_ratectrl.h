










#ifndef VP9_ENCODER_VP9_RATECTRL_H_
#define VP9_ENCODER_VP9_RATECTRL_H_

#include "vpx/vpx_codec.h"
#include "vpx/vpx_integer.h"

#include "vp9/common/vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif


#define BPER_MB_NORMBITS    9

typedef enum {
  INTER_NORMAL = 0,
  INTER_HIGH = 1,
  GF_ARF_LOW = 2,
  GF_ARF_STD = 3,
  KF_STD = 4,
  RATE_FACTOR_LEVELS = 5
} RATE_FACTOR_LEVEL;


typedef enum {
  UNSCALED = 0,     
  SCALE_STEP1 = 1,  
  FRAME_SCALE_STEPS
} FRAME_SCALE_LEVEL;






static const int frame_scale_factor[FRAME_SCALE_STEPS] = {16, 24};


static const double rate_thresh_mult[FRAME_SCALE_STEPS] = {1.0, 2.0};



static const double rcf_mult[FRAME_SCALE_STEPS] = {1.0, 2.0};

typedef struct {
  
  int base_frame_target;           
                                   
  int this_frame_target;           
  int projected_frame_size;
  int sb64_target_rate;
  int last_q[FRAME_TYPES];         
  int last_boosted_qindex;         
  int last_kf_qindex;              

  int gfu_boost;
  int last_boost;
  int kf_boost;

  double rate_correction_factors[RATE_FACTOR_LEVELS];

  int frames_since_golden;
  int frames_till_gf_update_due;
  int min_gf_interval;
  int max_gf_interval;
  int static_scene_max_gf_interval;
  int baseline_gf_interval;
  int constrained_gf_group;
  int frames_to_key;
  int frames_since_key;
  int this_key_frame_forced;
  int next_key_frame_forced;
  int source_alt_ref_pending;
  int source_alt_ref_active;
  int is_src_frame_alt_ref;

  int avg_frame_bandwidth;  
  int min_frame_bandwidth;  
  int max_frame_bandwidth;  

  int ni_av_qi;
  int ni_tot_qi;
  int ni_frames;
  int avg_frame_qindex[FRAME_TYPES];
  double tot_q;
  double avg_q;

  int64_t buffer_level;
  int64_t bits_off_target;
  int64_t vbr_bits_off_target;
  int64_t vbr_bits_off_target_fast;

  int decimation_factor;
  int decimation_count;

  int rolling_target_bits;
  int rolling_actual_bits;

  int long_rolling_target_bits;
  int long_rolling_actual_bits;

  int rate_error_estimate;

  int64_t total_actual_bits;
  int64_t total_target_bits;
  int64_t total_target_vs_actual;

  int worst_quality;
  int best_quality;

  int64_t starting_buffer_level;
  int64_t optimal_buffer_level;
  int64_t maximum_buffer_size;

  
  
  
  
  int rc_1_frame;
  int rc_2_frame;
  int q_1_frame;
  int q_2_frame;

  
  FRAME_SCALE_LEVEL frame_size_selector;
  FRAME_SCALE_LEVEL next_frame_size_selector;
  int frame_width[FRAME_SCALE_STEPS];
  int frame_height[FRAME_SCALE_STEPS];
  int rf_level_maxq[RATE_FACTOR_LEVELS];
} RATE_CONTROL;

struct VP9_COMP;
struct VP9EncoderConfig;

void vp9_rc_init(const struct VP9EncoderConfig *oxcf, int pass,
                 RATE_CONTROL *rc);

int vp9_estimate_bits_at_q(FRAME_TYPE frame_kind, int q, int mbs,
                           double correction_factor,
                           vpx_bit_depth_t bit_depth);

double vp9_convert_qindex_to_q(int qindex, vpx_bit_depth_t bit_depth);

void vp9_rc_init_minq_luts(void);

























void vp9_rc_get_one_pass_vbr_params(struct VP9_COMP *cpi);
void vp9_rc_get_one_pass_cbr_params(struct VP9_COMP *cpi);
void vp9_rc_get_svc_params(struct VP9_COMP *cpi);



void vp9_rc_postencode_update(struct VP9_COMP *cpi, uint64_t bytes_used);

void vp9_rc_postencode_update_drop_frame(struct VP9_COMP *cpi);



void vp9_rc_update_rate_correction_factors(struct VP9_COMP *cpi);



int vp9_rc_drop_frame(struct VP9_COMP *cpi);


void vp9_rc_compute_frame_size_bounds(const struct VP9_COMP *cpi,
                                      int this_frame_target,
                                      int *frame_under_shoot_limit,
                                      int *frame_over_shoot_limit);


int vp9_rc_pick_q_and_bounds(const struct VP9_COMP *cpi,
                             int *bottom_index,
                             int *top_index);


int vp9_rc_regulate_q(const struct VP9_COMP *cpi, int target_bits_per_frame,
                      int active_best_quality, int active_worst_quality);


int vp9_rc_bits_per_mb(FRAME_TYPE frame_type, int qindex,
                       double correction_factor, vpx_bit_depth_t bit_depth);


int vp9_rc_clamp_iframe_target_size(const struct VP9_COMP *const cpi,
                                    int target);
int vp9_rc_clamp_pframe_target_size(const struct VP9_COMP *const cpi,
                                    int target);


void vp9_rc_set_frame_target(struct VP9_COMP *cpi, int target);



int vp9_compute_qdelta(const RATE_CONTROL *rc, double qstart, double qtarget,
                       vpx_bit_depth_t bit_depth);



int vp9_compute_qdelta_by_rate(const RATE_CONTROL *rc, FRAME_TYPE frame_type,
                               int qindex, double rate_target_ratio,
                               vpx_bit_depth_t bit_depth);

int vp9_frame_type_qdelta(const struct VP9_COMP *cpi, int rf_level, int q);

void vp9_rc_update_framerate(struct VP9_COMP *cpi);

void vp9_rc_set_gf_interval_range(const struct VP9_COMP *const cpi,
                                  RATE_CONTROL *const rc);

void vp9_set_target_rate(struct VP9_COMP *cpi);

#ifdef __cplusplus
}  
#endif

#endif
