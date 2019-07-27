









#ifndef VP9_COMMON_VP9_ONYX_H_
#define VP9_COMMON_VP9_ONYX_H_

#include "./vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "vpx/vp8cx.h"
#include "vpx_scale/yv12config.h"
#include "vp9/common/vp9_ppflags.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SEGMENTS 8

  typedef int *VP9_PTR;

  

  typedef enum {
    NORMAL      = 0,
    FOURFIVE    = 1,
    THREEFIVE   = 2,
    ONETWO      = 3
  } VPX_SCALING;

  typedef enum {
    VP9_LAST_FLAG = 1,
    VP9_GOLD_FLAG = 2,
    VP9_ALT_FLAG = 4
  } VP9_REFFRAME;


  typedef enum {
    USAGE_LOCAL_FILE_PLAYBACK   = 0x0,
    USAGE_STREAM_FROM_SERVER    = 0x1,
    USAGE_CONSTRAINED_QUALITY   = 0x2,
    USAGE_CONSTANT_QUALITY      = 0x3,
  } END_USAGE;


  typedef enum {
    MODE_GOODQUALITY    = 0x1,
    MODE_BESTQUALITY    = 0x2,
    MODE_FIRSTPASS      = 0x3,
    MODE_SECONDPASS     = 0x4,
    MODE_SECONDPASS_BEST = 0x5,
    MODE_REALTIME       = 0x6,
  } MODE;

  typedef enum {
    FRAMEFLAGS_KEY    = 1,
    FRAMEFLAGS_GOLDEN = 2,
    FRAMEFLAGS_ALTREF = 4,
  } FRAMETYPE_FLAGS;

  typedef enum {
    NO_AQ = 0,
    VARIANCE_AQ = 1,
    COMPLEXITY_AQ = 2,
    AQ_MODES_COUNT  
  } AQ_MODES;

  typedef struct {
    int version;  
                  
                  
    int width;  
    int height;  
    double framerate;  
    int64_t target_bandwidth;  

    int noise_sensitivity;  
    int sharpness;  
    int cpu_used;
    unsigned int rc_max_intra_bitrate_pct;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int mode;

    
    int auto_key;  
    int key_freq;  

    int allow_lag;  
    int lag_in_frames;  

    
    

    int end_usage;  

    
    int under_shoot_pct;
    int over_shoot_pct;

    
    int64_t starting_buffer_level;  
    int64_t optimal_buffer_level;
    int64_t maximum_buffer_size;

    
    int drop_frames_water_mark;

    
    int fixed_q;
    int worst_allowed_q;
    int best_allowed_q;
    int cq_level;
    int lossless;
    int aq_mode;  

    
    int two_pass_vbrbias;        
    int two_pass_vbrmin_section;
    int two_pass_vbrmax_section;
    
    

    
    int ss_number_layers;  
    int ts_number_layers;  
    
    int ts_target_bitrate[VPX_TS_MAX_LAYERS];
    int ts_rate_decimator[VPX_TS_MAX_LAYERS];

    
    int play_alternate;
    int alt_freq;

    int encode_breakout;  

    



    unsigned int error_resilient_mode;

    



    unsigned int frame_parallel_decoding_mode;

    int arnr_max_frames;
    int arnr_strength;
    int arnr_type;

    int tile_columns;
    int tile_rows;

    struct vpx_fixed_buf         two_pass_stats_in;
    struct vpx_codec_pkt_list  *output_pkt_list;

    vp8e_tuning tuning;
  } VP9_CONFIG;


  void vp9_initialize_enc();

  VP9_PTR vp9_create_compressor(VP9_CONFIG *oxcf);
  void vp9_remove_compressor(VP9_PTR *comp);

  void vp9_change_config(VP9_PTR onyx, VP9_CONFIG *oxcf);

  
  
  int vp9_receive_raw_frame(VP9_PTR comp, unsigned int frame_flags,
                            YV12_BUFFER_CONFIG *sd, int64_t time_stamp,
                            int64_t end_time_stamp);

  int vp9_get_compressed_data(VP9_PTR comp, unsigned int *frame_flags,
                              size_t *size, uint8_t *dest,
                              int64_t *time_stamp, int64_t *time_end,
                              int flush);

  int vp9_get_preview_raw_frame(VP9_PTR comp, YV12_BUFFER_CONFIG *dest,
                                vp9_ppflags_t *flags);

  int vp9_use_as_reference(VP9_PTR comp, int ref_frame_flags);

  int vp9_update_reference(VP9_PTR comp, int ref_frame_flags);

  int vp9_copy_reference_enc(VP9_PTR comp, VP9_REFFRAME ref_frame_flag,
                             YV12_BUFFER_CONFIG *sd);

  int vp9_get_reference_enc(VP9_PTR ptr, int index, YV12_BUFFER_CONFIG **fb);

  int vp9_set_reference_enc(VP9_PTR comp, VP9_REFFRAME ref_frame_flag,
                            YV12_BUFFER_CONFIG *sd);

  int vp9_update_entropy(VP9_PTR comp, int update);

  int vp9_set_roimap(VP9_PTR comp, unsigned char *map,
                     unsigned int rows, unsigned int cols,
                     int delta_q[MAX_SEGMENTS],
                     int delta_lf[MAX_SEGMENTS],
                     unsigned int threshold[MAX_SEGMENTS]);

  int vp9_set_active_map(VP9_PTR comp, unsigned char *map,
                         unsigned int rows, unsigned int cols);

  int vp9_set_internal_size(VP9_PTR comp,
                            VPX_SCALING horiz_mode, VPX_SCALING vert_mode);

  int vp9_set_size_literal(VP9_PTR comp, unsigned int width,
                           unsigned int height);

  void vp9_set_svc(VP9_PTR comp, int use_svc);

  int vp9_get_quantizer(VP9_PTR c);

#ifdef __cplusplus
}  
#endif

#endif
