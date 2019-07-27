














#ifndef VPX_VPX_IMAGE_H_
#define VPX_VPX_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif









#define VPX_IMAGE_ABI_VERSION (2) /**<\hideinitializer*/


#define VPX_IMG_FMT_PLANAR     0x100  /**< Image is a planar format */
#define VPX_IMG_FMT_UV_FLIP    0x200  /**< V plane precedes U plane in memory */
#define VPX_IMG_FMT_HAS_ALPHA  0x400  /**< Image has an alpha channel component */


  
  typedef enum vpx_img_fmt {
    VPX_IMG_FMT_NONE,
    VPX_IMG_FMT_RGB24,   
    VPX_IMG_FMT_RGB32,   
    VPX_IMG_FMT_RGB565,  
    VPX_IMG_FMT_RGB555,  
    VPX_IMG_FMT_UYVY,    
    VPX_IMG_FMT_YUY2,    
    VPX_IMG_FMT_YVYU,    
    VPX_IMG_FMT_BGR24,   
    VPX_IMG_FMT_RGB32_LE, 
    VPX_IMG_FMT_ARGB,     
    VPX_IMG_FMT_ARGB_LE,  
    VPX_IMG_FMT_RGB565_LE,  
    VPX_IMG_FMT_RGB555_LE,  
    VPX_IMG_FMT_YV12    = VPX_IMG_FMT_PLANAR | VPX_IMG_FMT_UV_FLIP | 1, 
    VPX_IMG_FMT_I420    = VPX_IMG_FMT_PLANAR | 2,
    VPX_IMG_FMT_VPXYV12 = VPX_IMG_FMT_PLANAR | VPX_IMG_FMT_UV_FLIP | 3, 
    VPX_IMG_FMT_VPXI420 = VPX_IMG_FMT_PLANAR | 4,
    VPX_IMG_FMT_I422    = VPX_IMG_FMT_PLANAR | 5,
    VPX_IMG_FMT_I444    = VPX_IMG_FMT_PLANAR | 6,
    VPX_IMG_FMT_444A    = VPX_IMG_FMT_PLANAR | VPX_IMG_FMT_HAS_ALPHA | 7
  } vpx_img_fmt_t; 

#if !defined(VPX_CODEC_DISABLE_COMPAT) || !VPX_CODEC_DISABLE_COMPAT
#define IMG_FMT_PLANAR         VPX_IMG_FMT_PLANAR     /**< \deprecated Use #VPX_IMG_FMT_PLANAR */
#define IMG_FMT_UV_FLIP        VPX_IMG_FMT_UV_FLIP    /**< \deprecated Use #VPX_IMG_FMT_UV_FLIP */
#define IMG_FMT_HAS_ALPHA      VPX_IMG_FMT_HAS_ALPHA  /**< \deprecated Use #VPX_IMG_FMT_HAS_ALPHA */

  


#define img_fmt   vpx_img_fmt
  


#define img_fmt_t vpx_img_fmt_t

#define IMG_FMT_NONE       VPX_IMG_FMT_NONE       /**< \deprecated Use #VPX_IMG_FMT_NONE */
#define IMG_FMT_RGB24      VPX_IMG_FMT_RGB24      /**< \deprecated Use #VPX_IMG_FMT_RGB24 */
#define IMG_FMT_RGB32      VPX_IMG_FMT_RGB32      /**< \deprecated Use #VPX_IMG_FMT_RGB32 */
#define IMG_FMT_RGB565     VPX_IMG_FMT_RGB565     /**< \deprecated Use #VPX_IMG_FMT_RGB565 */
#define IMG_FMT_RGB555     VPX_IMG_FMT_RGB555     /**< \deprecated Use #VPX_IMG_FMT_RGB555 */
#define IMG_FMT_UYVY       VPX_IMG_FMT_UYVY       /**< \deprecated Use #VPX_IMG_FMT_UYVY */
#define IMG_FMT_YUY2       VPX_IMG_FMT_YUY2       /**< \deprecated Use #VPX_IMG_FMT_YUY2 */
#define IMG_FMT_YVYU       VPX_IMG_FMT_YVYU       /**< \deprecated Use #VPX_IMG_FMT_YVYU */
#define IMG_FMT_BGR24      VPX_IMG_FMT_BGR24      /**< \deprecated Use #VPX_IMG_FMT_BGR24 */
#define IMG_FMT_RGB32_LE   VPX_IMG_FMT_RGB32_LE   /**< \deprecated Use #VPX_IMG_FMT_RGB32_LE */
#define IMG_FMT_ARGB       VPX_IMG_FMT_ARGB       /**< \deprecated Use #VPX_IMG_FMT_ARGB */
#define IMG_FMT_ARGB_LE    VPX_IMG_FMT_ARGB_LE    /**< \deprecated Use #VPX_IMG_FMT_ARGB_LE */
#define IMG_FMT_RGB565_LE  VPX_IMG_FMT_RGB565_LE  /**< \deprecated Use #VPX_IMG_FMT_RGB565_LE */
#define IMG_FMT_RGB555_LE  VPX_IMG_FMT_RGB555_LE  /**< \deprecated Use #VPX_IMG_FMT_RGB555_LE */
#define IMG_FMT_YV12       VPX_IMG_FMT_YV12       /**< \deprecated Use #VPX_IMG_FMT_YV12 */
#define IMG_FMT_I420       VPX_IMG_FMT_I420       /**< \deprecated Use #VPX_IMG_FMT_I420 */
#define IMG_FMT_VPXYV12    VPX_IMG_FMT_VPXYV12    /**< \deprecated Use #VPX_IMG_FMT_VPXYV12 */
#define IMG_FMT_VPXI420    VPX_IMG_FMT_VPXI420    /**< \deprecated Use #VPX_IMG_FMT_VPXI420 */
#endif 

  
  typedef struct vpx_image {
    vpx_img_fmt_t fmt; 

    
    unsigned int  w;   
    unsigned int  h;   

    
    unsigned int  d_w;   
    unsigned int  d_h;   

    
    unsigned int  x_chroma_shift;   
    unsigned int  y_chroma_shift;   

    
#define VPX_PLANE_PACKED 0   /**< To be used for all packed formats */
#define VPX_PLANE_Y      0   /**< Y (Luminance) plane */
#define VPX_PLANE_U      1   /**< U (Chroma) plane */
#define VPX_PLANE_V      2   /**< V (Chroma) plane */
#define VPX_PLANE_ALPHA  3   /**< A (Transparency) plane */
#if !defined(VPX_CODEC_DISABLE_COMPAT) || !VPX_CODEC_DISABLE_COMPAT
#define PLANE_PACKED     VPX_PLANE_PACKED
#define PLANE_Y          VPX_PLANE_Y
#define PLANE_U          VPX_PLANE_U
#define PLANE_V          VPX_PLANE_V
#define PLANE_ALPHA      VPX_PLANE_ALPHA
#endif
    unsigned char *planes[4];  
    int      stride[4];  

    int     bps; 

    


    void    *user_priv; 


    
    unsigned char *img_data;       
    int      img_data_owner; 
    int      self_allocd;    

    void    *fb_priv; 
  } vpx_image_t; 

  
  typedef struct vpx_image_rect {
    unsigned int x; 
    unsigned int y; 
    unsigned int w; 
    unsigned int h; 
  } vpx_image_rect_t; 

  

















  vpx_image_t *vpx_img_alloc(vpx_image_t  *img,
                             vpx_img_fmt_t fmt,
                             unsigned int d_w,
                             unsigned int d_h,
                             unsigned int align);

  


















  vpx_image_t *vpx_img_wrap(vpx_image_t  *img,
                            vpx_img_fmt_t fmt,
                            unsigned int d_w,
                            unsigned int d_h,
                            unsigned int align,
                            unsigned char      *img_data);


  












  int vpx_img_set_rect(vpx_image_t  *img,
                       unsigned int  x,
                       unsigned int  y,
                       unsigned int  w,
                       unsigned int  h);


  






  void vpx_img_flip(vpx_image_t *img);

  





  void vpx_img_free(vpx_image_t *img);

#ifdef __cplusplus
}  
#endif

#endif
