










#ifndef VP8_COMMON_ONYX_H_
#define VP8_COMMON_ONYX_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_encoder.h"
#include "vpx_scale/yv12config.h"
#include "ppflags.h"

    struct VP8_COMP;

    

    typedef enum
    {
        NORMAL      = 0,
        FOURFIVE    = 1,
        THREEFIVE   = 2,
        ONETWO      = 3

    } VPX_SCALING;

    typedef enum
    {
        USAGE_LOCAL_FILE_PLAYBACK   = 0x0,
        USAGE_STREAM_FROM_SERVER    = 0x1,
        USAGE_CONSTRAINED_QUALITY   = 0x2,
        USAGE_CONSTANT_QUALITY      = 0x3
    } END_USAGE;


    typedef enum
    {
        MODE_REALTIME       = 0x0,
        MODE_GOODQUALITY    = 0x1,
        MODE_BESTQUALITY    = 0x2,
        MODE_FIRSTPASS      = 0x3,
        MODE_SECONDPASS     = 0x4,
        MODE_SECONDPASS_BEST = 0x5
    } MODE;

    typedef enum
    {
        FRAMEFLAGS_KEY    = 1,
        FRAMEFLAGS_GOLDEN = 2,
        FRAMEFLAGS_ALTREF = 4
    } FRAMETYPE_FLAGS;


#include <assert.h>
    static void Scale2Ratio(int mode, int *hr, int *hs)
    {
        switch (mode)
        {
        case    NORMAL:
            *hr = 1;
            *hs = 1;
            break;
        case    FOURFIVE:
            *hr = 4;
            *hs = 5;
            break;
        case    THREEFIVE:
            *hr = 3;
            *hs = 5;
            break;
        case    ONETWO:
            *hr = 1;
            *hs = 2;
            break;
        default:
            *hr = 1;
            *hs = 1;
            assert(0);
            break;
        }
    }

    typedef struct
    {
        


        int Version;
        int Width;
        int Height;
        struct vpx_rational  timebase;
        unsigned int target_bandwidth;    

        
        int noise_sensitivity;

        
        int Sharpness;
        int cpu_used;
        unsigned int rc_max_intra_bitrate_pct;

        























        int Mode;

        
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

        int64_t starting_buffer_level_in_ms;
        int64_t optimal_buffer_level_in_ms;
        int64_t maximum_buffer_size_in_ms;

        
        int fixed_q;
        int worst_allowed_q;
        int best_allowed_q;
        int cq_level;

        
        int allow_spatial_resampling;
        int resample_down_water_mark;
        int resample_up_water_mark;

        
        int allow_df;
        int drop_frames_water_mark;

        
        int two_pass_vbrbias;
        int two_pass_vbrmin_section;
        int two_pass_vbrmax_section;

        



        
        int play_alternate;
        int alt_freq;
        int alt_q;
        int key_q;
        int gold_q;


        int multi_threaded;   
        int token_partitions; 

        
        int encode_breakout;

        



        unsigned int error_resilient_mode;

        int arnr_max_frames;
        int arnr_strength;
        int arnr_type;

        struct vpx_fixed_buf        two_pass_stats_in;
        struct vpx_codec_pkt_list  *output_pkt_list;

        vp8e_tuning tuning;

        
        unsigned int number_of_layers;
        unsigned int target_bitrate[VPX_TS_MAX_PERIODICITY];
        unsigned int rate_decimator[VPX_TS_MAX_PERIODICITY];
        unsigned int periodicity;
        unsigned int layer_id[VPX_TS_MAX_PERIODICITY];

#if CONFIG_MULTI_RES_ENCODING
        
        unsigned int mr_total_resolutions;

        
        unsigned int mr_encoder_id;

        
        vpx_rational_t mr_down_sampling_factor;

        
        void* mr_low_res_mode_info;
#endif
    } VP8_CONFIG;


    void vp8_initialize();

    struct VP8_COMP* vp8_create_compressor(VP8_CONFIG *oxcf);
    void vp8_remove_compressor(struct VP8_COMP* *comp);

    void vp8_init_config(struct VP8_COMP* onyx, VP8_CONFIG *oxcf);
    void vp8_change_config(struct VP8_COMP* onyx, VP8_CONFIG *oxcf);

    int vp8_receive_raw_frame(struct VP8_COMP* comp, unsigned int frame_flags, YV12_BUFFER_CONFIG *sd, int64_t time_stamp, int64_t end_time_stamp);
    int vp8_get_compressed_data(struct VP8_COMP* comp, unsigned int *frame_flags, unsigned long *size, unsigned char *dest, unsigned char *dest_end, int64_t *time_stamp, int64_t *time_end, int flush);
    int vp8_get_preview_raw_frame(struct VP8_COMP* comp, YV12_BUFFER_CONFIG *dest, vp8_ppflags_t *flags);

    int vp8_use_as_reference(struct VP8_COMP* comp, int ref_frame_flags);
    int vp8_update_reference(struct VP8_COMP* comp, int ref_frame_flags);
    int vp8_get_reference(struct VP8_COMP* comp, enum vpx_ref_frame_type ref_frame_flag, YV12_BUFFER_CONFIG *sd);
    int vp8_set_reference(struct VP8_COMP* comp, enum vpx_ref_frame_type ref_frame_flag, YV12_BUFFER_CONFIG *sd);
    int vp8_update_entropy(struct VP8_COMP* comp, int update);
    int vp8_set_roimap(struct VP8_COMP* comp, unsigned char *map, unsigned int rows, unsigned int cols, int delta_q[4], int delta_lf[4], unsigned int threshold[4]);
    int vp8_set_active_map(struct VP8_COMP* comp, unsigned char *map, unsigned int rows, unsigned int cols);
    int vp8_set_internal_size(struct VP8_COMP* comp, VPX_SCALING horiz_mode, VPX_SCALING vert_mode);
    int vp8_get_quantizer(struct VP8_COMP* c);

#ifdef __cplusplus
}
#endif

#endif
