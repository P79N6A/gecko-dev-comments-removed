



















#ifndef VPX_VP8DX_H_
#define VPX_VP8DX_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "./vp8.h"







extern vpx_codec_iface_t  vpx_codec_vp8_dx_algo;
extern vpx_codec_iface_t *vpx_codec_vp8_dx(void);


extern vpx_codec_iface_t  vpx_codec_vp9_dx_algo;
extern vpx_codec_iface_t *vpx_codec_vp9_dx(void);











enum vp8_dec_control_id {
  


  VP8D_GET_LAST_REF_UPDATES = VP8_DECODER_CTRL_ID_START,

  
  VP8D_GET_FRAME_CORRUPTED,

  


  VP8D_GET_LAST_REF_USED,

  



  VP8D_SET_DECRYPTOR,

  
  VP9D_GET_DISPLAY_SIZE,

  
  VP9_INVERT_TILE_DECODE_ORDER,

  VP8_DECODER_CTRL_ID_MAX
};





typedef struct vp8_decrypt_init {
    


    void (*decrypt_cb)(void *decrypt_state, const unsigned char *input,
                       unsigned char *output, int count);
    
    void *decrypt_state;
} vp8_decrypt_init;









VPX_CTRL_USE_TYPE(VP8D_GET_LAST_REF_UPDATES,   int *)
VPX_CTRL_USE_TYPE(VP8D_GET_FRAME_CORRUPTED,    int *)
VPX_CTRL_USE_TYPE(VP8D_GET_LAST_REF_USED,      int *)
VPX_CTRL_USE_TYPE(VP8D_SET_DECRYPTOR,          vp8_decrypt_init *)
VPX_CTRL_USE_TYPE(VP9D_GET_DISPLAY_SIZE,       int *)
VPX_CTRL_USE_TYPE(VP9_INVERT_TILE_DECODE_ORDER, int)



#ifdef __cplusplus
}  
#endif

#endif  
