










#ifndef __INC_VP8_H
#define __INC_VP8_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "vpx/internal/vpx_codec_internal.h"
#include "vpx/vp8cx.h"
#include "vpx_scale/yv12config.h"
#include "type_aliases.h"
#include "ppflags.h"
    typedef int *VP8_PTR;

    

    typedef enum
    {
        NORMAL      = 0,
        FOURFIVE    = 1,
        THREEFIVE   = 2,
        ONETWO      = 3

    } VPX_SCALING;

    typedef enum
    {
        VP8_LAST_FLAG = 1,
        VP8_GOLD_FLAG = 2,
        VP8_ALT_FLAG = 4
    } VP8_REFFRAME;


    typedef enum
    {
        USAGE_STREAM_FROM_SERVER    = 0x0,
        USAGE_LOCAL_FILE_PLAYBACK   = 0x1,
        USAGE_CONSTRAINED_QUALITY   = 0x2
    } END_USAGE;


    typedef enum
    {
        MODE_REALTIME       = 0x0,
        MODE_GOODQUALITY    = 0x1,
        MODE_BESTQUALITY    = 0x2,
        MODE_FIRSTPASS      = 0x3,
        MODE_SECONDPASS     = 0x4,
        MODE_SECONDPASS_BEST = 0x5,
    } MODE;

    typedef enum
    {
        FRAMEFLAGS_KEY    = 1,
        FRAMEFLAGS_GOLDEN = 2,
        FRAMEFLAGS_ALTREF = 4,
    } FRAMETYPE_FLAGS;


#include <assert.h>
    static __inline void Scale2Ratio(int mode, int *hr, int *hs)
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
        double frame_rate;       
        int target_bandwidth;    

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

        
        int starting_buffer_level;  
        int optimal_buffer_level;
        int maximum_buffer_size;

        
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
        int arnr_strength ;
        int arnr_type     ;

        struct vpx_fixed_buf         two_pass_stats_in;
        struct vpx_codec_pkt_list  *output_pkt_list;

        vp8e_tuning tuning;
    } VP8_CONFIG;


    void vp8_initialize();

    VP8_PTR vp8_create_compressor(VP8_CONFIG *oxcf);
    void vp8_remove_compressor(VP8_PTR *comp);

    void vp8_init_config(VP8_PTR onyx, VP8_CONFIG *oxcf);
    void vp8_change_config(VP8_PTR onyx, VP8_CONFIG *oxcf);



    int vp8_receive_raw_frame(VP8_PTR comp, unsigned int frame_flags, YV12_BUFFER_CONFIG *sd, int64_t time_stamp, int64_t end_time_stamp);
    int vp8_get_compressed_data(VP8_PTR comp, unsigned int *frame_flags, unsigned long *size, unsigned char *dest, int64_t *time_stamp, int64_t *time_end, int flush);
    int vp8_get_preview_raw_frame(VP8_PTR comp, YV12_BUFFER_CONFIG *dest, vp8_ppflags_t *flags);

    int vp8_use_as_reference(VP8_PTR comp, int ref_frame_flags);
    int vp8_update_reference(VP8_PTR comp, int ref_frame_flags);
    int vp8_get_reference(VP8_PTR comp, VP8_REFFRAME ref_frame_flag, YV12_BUFFER_CONFIG *sd);
    int vp8_set_reference(VP8_PTR comp, VP8_REFFRAME ref_frame_flag, YV12_BUFFER_CONFIG *sd);
    int vp8_update_entropy(VP8_PTR comp, int update);
    int vp8_set_roimap(VP8_PTR comp, unsigned char *map, unsigned int rows, unsigned int cols, int delta_q[4], int delta_lf[4], unsigned int threshold[4]);
    int vp8_set_active_map(VP8_PTR comp, unsigned char *map, unsigned int rows, unsigned int cols);
    int vp8_set_internal_size(VP8_PTR comp, VPX_SCALING horiz_mode, VPX_SCALING vert_mode);
    int vp8_get_quantizer(VP8_PTR c);

#ifdef __cplusplus
}
#endif

#endif
