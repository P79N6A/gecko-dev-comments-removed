










#if JPEG_LIB_VERSION >= 70
#define _DCT_scaled_size DCT_h_scaled_size
#define _min_DCT_scaled_size min_DCT_h_scaled_size
#define _min_DCT_h_scaled_size min_DCT_h_scaled_size
#define _min_DCT_v_scaled_size min_DCT_v_scaled_size
#define _jpeg_width jpeg_width
#define _jpeg_height jpeg_height
#else
#define _DCT_scaled_size DCT_scaled_size
#define _min_DCT_scaled_size min_DCT_scaled_size
#define _min_DCT_h_scaled_size min_DCT_scaled_size
#define _min_DCT_v_scaled_size min_DCT_scaled_size
#define _jpeg_width image_width
#define _jpeg_height image_height
#endif
