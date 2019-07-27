










#ifndef VP9_ENCODER_VP9_RATECTRL_H_
#define VP9_ENCODER_VP9_RATECTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_OVERHEAD_BITS 200

typedef struct {
  
  int this_frame_target;
  int projected_frame_size;
  int sb64_target_rate;
  int last_q[3];                   
  int last_boosted_qindex;         

  int gfu_boost;
  int last_boost;
  int kf_boost;

  double rate_correction_factor;
  double key_frame_rate_correction_factor;
  double gf_rate_correction_factor;

  unsigned int frames_since_golden;
  unsigned int frames_till_gf_update_due;  
  unsigned int max_gf_interval;
  unsigned int baseline_gf_interval;
  unsigned int frames_to_key;
  unsigned int frames_since_key;
  unsigned int this_key_frame_forced;
  unsigned int next_key_frame_forced;
  unsigned int source_alt_ref_pending;
  unsigned int source_alt_ref_active;
  unsigned int is_src_frame_alt_ref;

  int av_per_frame_bandwidth;     
  int min_frame_bandwidth;        
  int max_frame_bandwidth;        

  int ni_av_qi;
  int ni_tot_qi;
  int ni_frames;
  int avg_frame_qindex[3];  
  double tot_q;
  double avg_q;

  int buffer_level;
  int bits_off_target;

  int decimation_factor;
  int decimation_count;

  int rolling_target_bits;
  int rolling_actual_bits;

  int long_rolling_target_bits;
  int long_rolling_actual_bits;

  int64_t total_actual_bits;
  int total_target_vs_actual;        

  int worst_quality;
  int best_quality;
  
} RATE_CONTROL;

struct VP9_COMP;

void vp9_save_coding_context(struct VP9_COMP *cpi);
void vp9_restore_coding_context(struct VP9_COMP *cpi);

void vp9_setup_key_frame(struct VP9_COMP *cpi);
void vp9_setup_inter_frame(struct VP9_COMP *cpi);

double vp9_convert_qindex_to_q(int qindex);


void vp9_rc_init_minq_luts(void);

























void vp9_rc_get_one_pass_vbr_params(struct VP9_COMP *cpi);
void vp9_rc_get_one_pass_cbr_params(struct VP9_COMP *cpi);
void vp9_rc_get_svc_params(struct VP9_COMP *cpi);



void vp9_rc_postencode_update(struct VP9_COMP *cpi,
                              uint64_t bytes_used);

void vp9_rc_postencode_update_drop_frame(struct VP9_COMP *cpi);



void vp9_rc_update_rate_correction_factors(struct VP9_COMP *cpi, int damp_var);



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
                       double correction_factor);


int vp9_rc_clamp_iframe_target_size(const struct VP9_COMP *const cpi,
                                    int target);
int vp9_rc_clamp_pframe_target_size(const struct VP9_COMP *const cpi,
                                    int target);


void vp9_rc_set_frame_target(struct VP9_COMP *cpi, int target);

#ifdef __cplusplus
}  
#endif

#endif
