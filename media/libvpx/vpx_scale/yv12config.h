










#ifndef YV12_CONFIG_H
#define YV12_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#define VP7BORDERINPIXELS       48
#define VP8BORDERINPIXELS       32

    










    typedef enum
    {
        REG_YUV = 0,    
        INT_YUV = 1     
              }
              YUV_TYPE;

    typedef struct
    {
        int   y_width;
        int   y_height;
        int   y_stride;


        int   uv_width;
        int   uv_height;
        int   uv_stride;


        unsigned char *y_buffer;
        unsigned char *u_buffer;
        unsigned char *v_buffer;

        unsigned char *buffer_alloc;
        int border;
        int frame_size;
        YUV_TYPE clrtype;

        int corrupted;
        int flags;
    } YV12_BUFFER_CONFIG;

    int vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf, int width, int height, int border);
    int vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf);

#ifdef __cplusplus
}
#endif


#endif
