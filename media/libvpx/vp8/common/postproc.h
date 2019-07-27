










#ifndef VP8_COMMON_POSTPROC_H_
#define VP8_COMMON_POSTPROC_H_

#include "vpx_ports/mem.h"
struct postproc_state
{
    int           last_q;
    int           last_noise;
    char          noise[3072];
    int           last_base_qindex;
    int           last_frame_valid;
    DECLARE_ALIGNED(16, char, blackclamp[16]);
    DECLARE_ALIGNED(16, char, whiteclamp[16]);
    DECLARE_ALIGNED(16, char, bothclamp[16]);
};
#include "onyxc_int.h"
#include "ppflags.h"

#ifdef __cplusplus
extern "C" {
#endif
int vp8_post_proc_frame(struct VP8Common *oci, YV12_BUFFER_CONFIG *dest,
                        vp8_ppflags_t *flags);


void vp8_de_noise(struct VP8Common           *oci,
                  YV12_BUFFER_CONFIG         *source,
                  YV12_BUFFER_CONFIG         *post,
                  int                         q,
                  int                         low_var_thresh,
                  int                         flag);

void vp8_deblock(struct VP8Common           *oci,
                 YV12_BUFFER_CONFIG         *source,
                 YV12_BUFFER_CONFIG         *post,
                 int                         q,
                 int                         low_var_thresh,
                 int                         flag);

#define MFQE_PRECISION 4

void vp8_multiframe_quality_enhance(struct VP8Common *cm);
#ifdef __cplusplus
}  
#endif

#endif
