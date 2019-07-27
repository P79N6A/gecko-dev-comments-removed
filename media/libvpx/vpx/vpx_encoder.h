








#ifndef VPX_VPX_ENCODER_H_
#define VPX_VPX_ENCODER_H_
















#ifdef __cplusplus
extern "C" {
#endif

#include "./vpx_codec.h"

  


#define VPX_TS_MAX_PERIODICITY 16

  
#define VPX_TS_MAX_LAYERS       5

  
#define MAX_PERIODICITY VPX_TS_MAX_PERIODICITY

  
#define MAX_LAYERS      VPX_TS_MAX_LAYERS


#define VPX_SS_MAX_LAYERS       5


#define VPX_SS_DEFAULT_LAYERS       3

  







#define VPX_ENCODER_ABI_VERSION (3 + VPX_CODEC_ABI_VERSION) /**<\hideinitializer*/


  








#define VPX_CODEC_CAP_PSNR  0x10000 /**< Can issue PSNR packets */

  




#define VPX_CODEC_CAP_OUTPUT_PARTITION  0x20000


  






#define VPX_CODEC_USE_PSNR  0x10000 /**< Calculate PSNR on each frame */
#define VPX_CODEC_USE_OUTPUT_PARTITION  0x20000 /**< Make the encoder output one
  partition at a time. */


  



  typedef struct vpx_fixed_buf {
    void          *buf; 
    size_t         sz;  
  } vpx_fixed_buf_t; 


  




  typedef int64_t vpx_codec_pts_t;


  






  typedef uint32_t vpx_codec_frame_flags_t;
#define VPX_FRAME_IS_KEY       0x1 /**< frame is the start of a GOP */
#define VPX_FRAME_IS_DROPPABLE 0x2 /**< frame can be dropped without affecting
  the stream (no future frame depends on
              this one) */
#define VPX_FRAME_IS_INVISIBLE 0x4 /**< frame should be decoded but will not
  be shown */
#define VPX_FRAME_IS_FRAGMENT  0x8 /**< this is a fragment of the encoded
  frame */

  





  typedef uint32_t vpx_codec_er_flags_t;
#define VPX_ERROR_RESILIENT_DEFAULT     0x1 /**< Improve resiliency against
  losses of whole frames */
#define VPX_ERROR_RESILIENT_PARTITIONS  0x2 /**< The frame partitions are
  independently decodable by the
  bool decoder, meaning that
  partitions can be decoded even
  though earlier partitions have
  been lost. Note that intra
  predicition is still done over
  the partition boundary. */

  





  enum vpx_codec_cx_pkt_kind {
    VPX_CODEC_CX_FRAME_PKT,    
    VPX_CODEC_STATS_PKT,       
    VPX_CODEC_PSNR_PKT,        
    VPX_CODEC_CUSTOM_PKT = 256 
  };


  




  typedef struct vpx_codec_cx_pkt {
    enum vpx_codec_cx_pkt_kind  kind; 
    union {
      struct {
        void                    *buf;      
        size_t                   sz;       
        vpx_codec_pts_t          pts;      

        unsigned long            duration; 

        vpx_codec_frame_flags_t  flags;    
        int                      partition_id; 






      } frame;  
      struct vpx_fixed_buf twopass_stats;  
      struct vpx_psnr_pkt {
        unsigned int samples[4];  
        uint64_t     sse[4];      
        double       psnr[4];     
      } psnr;                       
      struct vpx_fixed_buf raw;     

      




      char pad[128 - sizeof(enum vpx_codec_cx_pkt_kind)]; 
    } data; 
  } vpx_codec_cx_pkt_t; 


  



  typedef struct vpx_rational {
    int num; 
    int den; 
  } vpx_rational_t; 


  
  enum vpx_enc_pass {
    VPX_RC_ONE_PASS,   
    VPX_RC_FIRST_PASS, 
    VPX_RC_LAST_PASS   
  };


  
  enum vpx_rc_mode {
    VPX_VBR,  
    VPX_CBR,  
    VPX_CQ,   
    VPX_Q,    
  };


  







  enum vpx_kf_mode {
    VPX_KF_FIXED, 
    VPX_KF_AUTO,  
    VPX_KF_DISABLED = 0 
  };


  






  typedef long vpx_enc_frame_flags_t;
#define VPX_EFLAG_FORCE_KF (1<<0)  /**< Force this frame to be a keyframe */


  





  typedef struct vpx_codec_enc_cfg {
    



    






    unsigned int           g_usage;


    





    unsigned int           g_threads;


    







    unsigned int           g_profile;  



    






    unsigned int           g_w;


    






    unsigned int           g_h;


    











    struct vpx_rational    g_timebase;


    





    vpx_codec_er_flags_t   g_error_resilient;


    




    enum vpx_enc_pass      g_pass;


    











    unsigned int           g_lag_in_frames;


    



    















    unsigned int           rc_dropframe_thresh;


    






    unsigned int           rc_resize_allowed;


    





    unsigned int           rc_resize_up_thresh;


    





    unsigned int           rc_resize_down_thresh;


    







    enum vpx_rc_mode       rc_end_usage;


    




    struct vpx_fixed_buf   rc_twopass_stats_in;


    



    unsigned int           rc_target_bitrate;


    




    







    unsigned int           rc_min_quantizer;


    







    unsigned int           rc_max_quantizer;


    




    









    unsigned int           rc_undershoot_pct;


    









    unsigned int           rc_overshoot_pct;


    




    








    unsigned int           rc_buf_sz;


    






    unsigned int           rc_buf_initial_sz;


    






    unsigned int           rc_buf_optimal_sz;


    




    







    unsigned int           rc_2pass_vbr_bias_pct;       


    




    unsigned int           rc_2pass_vbr_minsection_pct;


    




    unsigned int           rc_2pass_vbr_maxsection_pct;


    



    





    enum vpx_kf_mode       kf_mode;


    






    unsigned int           kf_min_dist;


    






    unsigned int           kf_max_dist;

    



    



    unsigned int           ss_number_layers;

    



    unsigned int           ts_number_layers;

    




    unsigned int           ts_target_bitrate[VPX_TS_MAX_LAYERS];

    




    unsigned int           ts_rate_decimator[VPX_TS_MAX_LAYERS];

    






    unsigned int           ts_periodicity;

    






    unsigned int           ts_layer_id[VPX_TS_MAX_PERIODICITY];
  } vpx_codec_enc_cfg_t; 


  

























  vpx_codec_err_t vpx_codec_enc_init_ver(vpx_codec_ctx_t      *ctx,
                                         vpx_codec_iface_t    *iface,
                                         vpx_codec_enc_cfg_t  *cfg,
                                         vpx_codec_flags_t     flags,
                                         int                   ver);


  



#define vpx_codec_enc_init(ctx, iface, cfg, flags) \
  vpx_codec_enc_init_ver(ctx, iface, cfg, flags, VPX_ENCODER_ABI_VERSION)


  























  vpx_codec_err_t vpx_codec_enc_init_multi_ver(vpx_codec_ctx_t      *ctx,
                                               vpx_codec_iface_t    *iface,
                                               vpx_codec_enc_cfg_t  *cfg,
                                               int                   num_enc,
                                               vpx_codec_flags_t     flags,
                                               vpx_rational_t       *dsf,
                                               int                   ver);


  



#define vpx_codec_enc_init_multi(ctx, iface, cfg, num_enc, flags, dsf) \
  vpx_codec_enc_init_multi_ver(ctx, iface, cfg, num_enc, flags, dsf, \
                               VPX_ENCODER_ABI_VERSION)


  


















  vpx_codec_err_t  vpx_codec_enc_config_default(vpx_codec_iface_t    *iface,
                                                vpx_codec_enc_cfg_t  *cfg,
                                                unsigned int          usage);


  













  vpx_codec_err_t  vpx_codec_enc_config_set(vpx_codec_ctx_t            *ctx,
                                            const vpx_codec_enc_cfg_t  *cfg);


  










  vpx_fixed_buf_t *vpx_codec_get_global_headers(vpx_codec_ctx_t   *ctx);


#define VPX_DL_REALTIME     (1)        /**< deadline parameter analogous to
  *   VPx REALTIME mode. */
#define VPX_DL_GOOD_QUALITY (1000000)  /**< deadline parameter analogous to
  *   VPx GOOD QUALITY mode. */
#define VPX_DL_BEST_QUALITY (0)        

  



































  vpx_codec_err_t  vpx_codec_encode(vpx_codec_ctx_t            *ctx,
                                    const vpx_image_t          *img,
                                    vpx_codec_pts_t             pts,
                                    unsigned long               duration,
                                    vpx_enc_frame_flags_t       flags,
                                    unsigned long               deadline);

  










































  vpx_codec_err_t vpx_codec_set_cx_data_buf(vpx_codec_ctx_t       *ctx,
                                            const vpx_fixed_buf_t *buf,
                                            unsigned int           pad_before,
                                            unsigned int           pad_after);


  






















  const vpx_codec_cx_pkt_t *vpx_codec_get_cx_data(vpx_codec_ctx_t   *ctx,
                                                  vpx_codec_iter_t  *iter);


  











  const vpx_image_t *vpx_codec_get_preview_frame(vpx_codec_ctx_t   *ctx);


  
#ifdef __cplusplus
}
#endif
#endif

