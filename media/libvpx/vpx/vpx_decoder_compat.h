
























#ifdef __cplusplus
extern "C" {
#endif

#ifndef VPX_DECODER_COMPAT_H
#define VPX_DECODER_COMPAT_H


    typedef enum {
        
        VPX_DEC_OK = VPX_CODEC_OK,

        
        VPX_DEC_ERROR = VPX_CODEC_ERROR,

        
        VPX_DEC_MEM_ERROR = VPX_CODEC_MEM_ERROR,

        
        VPX_DEC_ABI_MISMATCH = VPX_CODEC_ABI_MISMATCH,

        




        VPX_DEC_UNSUP_BITSTREAM = VPX_CODEC_UNSUP_BITSTREAM,

        






        VPX_DEC_UNSUP_FEATURE = VPX_CODEC_UNSUP_FEATURE,

        







        VPX_DEC_CORRUPT_FRAME = VPX_CODEC_CORRUPT_FRAME,

        


        VPX_DEC_INVALID_PARAM = VPX_CODEC_INVALID_PARAM,

        


        VPX_DEC_LIST_END = VPX_CODEC_LIST_END

    }
    vpx_dec_err_t;

    







    typedef int vpx_dec_caps_t;
#define VPX_DEC_CAP_PUT_SLICE  0x0001 /**< Will issue put_slice callbacks */
#define VPX_DEC_CAP_PUT_FRAME  0x0002 /**< Will issue put_frame callbacks */
#define VPX_DEC_CAP_XMA        0x0004 /**< Supports eXternal Memory Allocation */

    





#if 1
    typedef vpx_codec_stream_info_t vpx_dec_stream_info_t;
#else
    typedef struct
    {
        unsigned int sz;     
        unsigned int w;      
        unsigned int h;      
        unsigned int is_kf;  
    } vpx_dec_stream_info_t;
#endif


    




    typedef const struct vpx_codec_iface vpx_dec_iface_t;
    typedef       struct vpx_codec_priv  vpx_dec_priv_t;

    



    typedef vpx_codec_iter_t vpx_dec_iter_t;

    







#if 1
    typedef vpx_codec_ctx_t vpx_dec_ctx_t;
#else
    typedef struct
    {
        const char            *name;        
        vpx_dec_iface_t       *iface;       
        vpx_dec_err_t          err;         
        vpx_dec_priv_t        *priv;        
    } vpx_dec_ctx_t;
#endif


    





    const char *vpx_dec_build_config(void) DEPRECATED;

    






    const char *vpx_dec_iface_name(vpx_dec_iface_t *iface) DEPRECATED;


    









    const char *vpx_dec_err_to_string(vpx_dec_err_t  err) DEPRECATED;


    









    const char *vpx_dec_error(vpx_dec_ctx_t  *ctx) DEPRECATED;


    









    const char *vpx_dec_error_detail(vpx_dec_ctx_t  *ctx) DEPRECATED;


    






    















    vpx_dec_err_t vpx_dec_init_ver(vpx_dec_ctx_t    *ctx,
                                   vpx_dec_iface_t  *iface,
                                   int               ver) DEPRECATED;
#define vpx_dec_init(ctx, iface) \
    vpx_dec_init_ver(ctx, iface, VPX_DECODER_ABI_VERSION)


    










    vpx_dec_err_t vpx_dec_destroy(vpx_dec_ctx_t *ctx) DEPRECATED;


    






    vpx_dec_caps_t vpx_dec_get_caps(vpx_dec_iface_t *iface) DEPRECATED;


    
















    vpx_dec_err_t vpx_dec_peek_stream_info(vpx_dec_iface_t       *iface,
                                           const uint8_t         *data,
                                           unsigned int           data_sz,
                                           vpx_dec_stream_info_t *si) DEPRECATED;


    












    vpx_dec_err_t vpx_dec_get_stream_info(vpx_dec_ctx_t         *ctx,
                                          vpx_dec_stream_info_t *si) DEPRECATED;


    





















    vpx_dec_err_t vpx_dec_control(vpx_dec_ctx_t  *ctx,
                                  int             ctrl_id,
                                  void           *data) DEPRECATED;

    






















    vpx_dec_err_t vpx_dec_decode(vpx_dec_ctx_t  *ctx,
                                 uint8_t        *data,
                                 unsigned int    data_sz,
                                 void       *user_priv,
                                 int         rel_pts) DEPRECATED;


    














    vpx_image_t *vpx_dec_get_frame(vpx_dec_ctx_t  *ctx,
                                   vpx_dec_iter_t *iter) DEPRECATED;


    








    




    typedef void (*vpx_dec_put_frame_cb_fn_t)(void          *user_priv,
            const vpx_image_t *img);


    














    vpx_dec_err_t vpx_dec_register_put_frame_cb(vpx_dec_ctx_t             *ctx,
            vpx_dec_put_frame_cb_fn_t  cb,
            void                      *user_priv) DEPRECATED;


    

    








    




    typedef void (*vpx_dec_put_slice_cb_fn_t)(void           *user_priv,
            const vpx_image_t      *img,
            const vpx_image_rect_t *valid,
            const vpx_image_rect_t *update);


    














    vpx_dec_err_t vpx_dec_register_put_slice_cb(vpx_dec_ctx_t             *ctx,
            vpx_dec_put_slice_cb_fn_t  cb,
            void                      *user_priv) DEPRECATED;


    

    








    





#if 1
#define VPX_DEC_MEM_ZERO     0x1  /**< Segment must be zeroed by allocation */
#define VPX_DEC_MEM_WRONLY   0x2  /**< Segment need not be readable */
#define VPX_DEC_MEM_FAST     0x4  /**< Place in fast memory, if available */
    typedef struct vpx_codec_mmap vpx_dec_mmap_t;
#else
    typedef struct vpx_dec_mmap
    {
        


        unsigned int   id;     
        unsigned long  sz;     
        unsigned int   align;  
        unsigned int   flags;  
#define VPX_DEC_MEM_ZERO     0x1  /**< Segment must be zeroed by allocation */
#define VPX_DEC_MEM_WRONLY   0x2  /**< Segment need not be readable */
#define VPX_DEC_MEM_FAST     0x4  /**< Place in fast memory, if available */

        
        void          *base;   
        void (*dtor)(struct vpx_dec_mmap *map);         
        void          *priv;   
    } vpx_dec_mmap_t;
#endif

    















    vpx_dec_err_t vpx_dec_xma_init_ver(vpx_dec_ctx_t    *ctx,
                                       vpx_dec_iface_t  *iface,
                                       int               ver) DEPRECATED;
#define vpx_dec_xma_init(ctx, iface) \
    vpx_dec_xma_init_ver(ctx, iface, VPX_DECODER_ABI_VERSION)


    























    vpx_dec_err_t vpx_dec_get_mem_map(vpx_dec_ctx_t                *ctx,
                                      vpx_dec_mmap_t               *mmap,
                                      const vpx_dec_stream_info_t  *si,
                                      vpx_dec_iter_t               *iter) DEPRECATED;


    




















    vpx_dec_err_t  vpx_dec_set_mem_map(vpx_dec_ctx_t   *ctx,
                                       vpx_dec_mmap_t  *mmaps,
                                       unsigned int     num_maps) DEPRECATED;

    
    


#endif
#ifdef __cplusplus
}
#endif
