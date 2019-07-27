






#if !defined(CUBEB_PANNER)
#define CUBEB_PANNER

#if defined(__cplusplus)
extern "C" {
#endif







void cubeb_pan_stereo_buffer_float(float * buf, uint32_t frames, float pan);
void cubeb_pan_stereo_buffer_int(short* buf, uint32_t frames, float pan);

#if defined(__cplusplus)
}
#endif

#endif
