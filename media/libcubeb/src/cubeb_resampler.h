





#ifndef CUBEB_RESAMPLER_H
#define CUBEB_RESAMPLER_H

#include "cubeb/cubeb.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct cubeb_resampler cubeb_resampler;

typedef enum {
  CUBEB_RESAMPLER_QUALITY_VOIP,
  CUBEB_RESAMPLER_QUALITY_DEFAULT,
  CUBEB_RESAMPLER_QUALITY_DESKTOP
} cubeb_resampler_quality;















cubeb_resampler * cubeb_resampler_create(cubeb_stream * stream,
                                         cubeb_stream_params params,
                                         unsigned int out_rate,
                                         cubeb_data_callback callback,
                                         long buffer_frame_count,
                                         void * user_ptr,
                                         cubeb_resampler_quality quality);










long cubeb_resampler_fill(cubeb_resampler * resampler,
                          void * buffer, long frames_needed);





void cubeb_resampler_destroy(cubeb_resampler * resampler);

#if defined(__cplusplus)
}
#endif

#endif
