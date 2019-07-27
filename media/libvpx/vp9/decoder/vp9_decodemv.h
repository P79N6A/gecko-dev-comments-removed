









#ifndef VP9_DECODER_VP9_DECODEMV_H_
#define VP9_DECODER_VP9_DECODEMV_H_

#include "vp9/decoder/vp9_onyxd_int.h"
#include "vp9/decoder/vp9_reader.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TileInfo;

void vp9_read_mode_info(VP9_COMMON *cm, MACROBLOCKD *xd,
                        const struct TileInfo *const tile,
                        int mi_row, int mi_col, vp9_reader *r);

#ifdef __cplusplus
}  
#endif

#endif
