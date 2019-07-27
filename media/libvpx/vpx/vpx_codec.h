





































#ifndef VPX_VPX_CODEC_H_
#define VPX_VPX_CODEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "./vpx_integer.h"
#include "./vpx_image.h"

  
#ifndef DEPRECATED
#if defined(__GNUC__) && __GNUC__
#define DEPRECATED          __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED
#else
#define DEPRECATED
#endif
#endif  

#ifndef DECLSPEC_DEPRECATED
#if defined(__GNUC__) && __GNUC__
#define DECLSPEC_DEPRECATED
#elif defined(_MSC_VER)
#define DECLSPEC_DEPRECATED __declspec(deprecated) /**< \copydoc #DEPRECATED */
#else
#define DECLSPEC_DEPRECATED
#endif
#endif  

  
#ifdef UNUSED
#elif __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

  







#define VPX_CODEC_ABI_VERSION (2 + VPX_IMAGE_ABI_VERSION) /**<\hideinitializer*/

  
  typedef enum {
    
    VPX_CODEC_OK,

    
    VPX_CODEC_ERROR,

    
    VPX_CODEC_MEM_ERROR,

    
    VPX_CODEC_ABI_MISMATCH,

    
    VPX_CODEC_INCAPABLE,

    




    VPX_CODEC_UNSUP_BITSTREAM,

    






    VPX_CODEC_UNSUP_FEATURE,

    







    VPX_CODEC_CORRUPT_FRAME,

    


    VPX_CODEC_INVALID_PARAM,

    


    VPX_CODEC_LIST_END

  }
  vpx_codec_err_t;


  







  typedef long vpx_codec_caps_t;
#define VPX_CODEC_CAP_DECODER 0x1 /**< Is a decoder */
#define VPX_CODEC_CAP_ENCODER 0x2 /**< Is an encoder */
#define VPX_CODEC_CAP_XMA     0x4 /**< Supports eXternal Memory Allocation */


  






  typedef long vpx_codec_flags_t;
#define VPX_CODEC_USE_XMA 0x00000001    /**< Use eXternal Memory Allocation mode */


  




  typedef const struct vpx_codec_iface vpx_codec_iface_t;


  




  typedef       struct vpx_codec_priv  vpx_codec_priv_t;


  



  typedef const void *vpx_codec_iter_t;


  







  typedef struct vpx_codec_ctx {
    const char              *name;        
    vpx_codec_iface_t       *iface;       
    vpx_codec_err_t          err;         
    const char              *err_detail;  
    vpx_codec_flags_t        init_flags;  
    union {
      struct vpx_codec_dec_cfg  *dec;   
      struct vpx_codec_enc_cfg  *enc;   
      void                      *raw;
    }                        config;      
    vpx_codec_priv_t        *priv;        
  } vpx_codec_ctx_t;


  








  







  int vpx_codec_version(void);
#define VPX_VERSION_MAJOR(v) ((v>>16)&0xff) /**< extract major from packed version */
#define VPX_VERSION_MINOR(v) ((v>>8)&0xff)  /**< extract minor from packed version */
#define VPX_VERSION_PATCH(v) ((v>>0)&0xff)  /**< extract patch from packed version */

  
#define vpx_codec_version_major() ((vpx_codec_version()>>16)&0xff)

  
#define vpx_codec_version_minor() ((vpx_codec_version()>>8)&0xff)

  
#define vpx_codec_version_patch() ((vpx_codec_version()>>0)&0xff)


  






  const char *vpx_codec_version_str(void);


  





  const char *vpx_codec_version_extra_str(void);


  





  const char *vpx_codec_build_config(void);


  






  const char *vpx_codec_iface_name(vpx_codec_iface_t *iface);


  









  const char *vpx_codec_err_to_string(vpx_codec_err_t  err);


  









  const char *vpx_codec_error(vpx_codec_ctx_t  *ctx);


  









  const char *vpx_codec_error_detail(vpx_codec_ctx_t  *ctx);


  





  










  vpx_codec_err_t vpx_codec_destroy(vpx_codec_ctx_t *ctx);


  






  vpx_codec_caps_t vpx_codec_get_caps(vpx_codec_iface_t *iface);


  























  vpx_codec_err_t vpx_codec_control_(vpx_codec_ctx_t  *ctx,
                                     int               ctrl_id,
                                     ...);
#if defined(VPX_DISABLE_CTRL_TYPECHECKS) && VPX_DISABLE_CTRL_TYPECHECKS
#    define vpx_codec_control(ctx,id,data) vpx_codec_control_(ctx,id,data)
#    define VPX_CTRL_USE_TYPE(id, typ)
#    define VPX_CTRL_USE_TYPE_DEPRECATED(id, typ)
#    define VPX_CTRL_VOID(id, typ)

#else
  








#    define vpx_codec_control(ctx,id,data) vpx_codec_control_##id(ctx,id,data)\
  /**<\hideinitializer*/


  










#    define VPX_CTRL_USE_TYPE(id, typ) \
  static vpx_codec_err_t \
  vpx_codec_control_##id(vpx_codec_ctx_t*, int, typ) UNUSED;\
  \
  static vpx_codec_err_t \
  vpx_codec_control_##id(vpx_codec_ctx_t  *ctx, int ctrl_id, typ data) {\
    return vpx_codec_control_(ctx, ctrl_id, data);\
  } /**<\hideinitializer*/


  









#    define VPX_CTRL_USE_TYPE_DEPRECATED(id, typ) \
  DECLSPEC_DEPRECATED static vpx_codec_err_t \
  vpx_codec_control_##id(vpx_codec_ctx_t*, int, typ) DEPRECATED UNUSED;\
  \
  DECLSPEC_DEPRECATED static vpx_codec_err_t \
  vpx_codec_control_##id(vpx_codec_ctx_t  *ctx, int ctrl_id, typ data) {\
    return vpx_codec_control_(ctx, ctrl_id, data);\
  } /**<\hideinitializer*/


  









#    define VPX_CTRL_VOID(id) \
  static vpx_codec_err_t \
  vpx_codec_control_##id(vpx_codec_ctx_t*, int) UNUSED;\
  \
  static vpx_codec_err_t \
  vpx_codec_control_##id(vpx_codec_ctx_t  *ctx, int ctrl_id) {\
    return vpx_codec_control_(ctx, ctrl_id);\
  } /**<\hideinitializer*/


#endif


  









  





  typedef struct vpx_codec_mmap {
    


    unsigned int   id;     
    unsigned long  sz;     
    unsigned int   align;  
    unsigned int   flags;  
#define VPX_CODEC_MEM_ZERO     0x1  /**< Segment must be zeroed by allocation */
#define VPX_CODEC_MEM_WRONLY   0x2  /**< Segment need not be readable */
#define VPX_CODEC_MEM_FAST     0x4  /**< Place in fast memory, if available */

    
    void          *base;   
    void (*dtor)(struct vpx_codec_mmap *map);         
    void          *priv;   
  } vpx_codec_mmap_t; 


  




















  vpx_codec_err_t vpx_codec_get_mem_map(vpx_codec_ctx_t                *ctx,
                                        vpx_codec_mmap_t               *mmap,
                                        vpx_codec_iter_t               *iter);


  




















  vpx_codec_err_t  vpx_codec_set_mem_map(vpx_codec_ctx_t   *ctx,
                                         vpx_codec_mmap_t  *mmaps,
                                         unsigned int       num_maps);

  
  
#ifdef __cplusplus
}
#endif
#endif

