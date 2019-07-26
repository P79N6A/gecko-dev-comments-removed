










#ifndef VP9_DECODER_VP9_TREEREADER_H_
#define VP9_DECODER_VP9_TREEREADER_H_

#include "vp9/common/vp9_treecoder.h"
#include "vp9/decoder/vp9_dboolhuff.h"


static int treed_read(vp9_reader *const r, 
                      vp9_tree t,
                      const vp9_prob *const p) {
  register vp9_tree_index i = 0;

  while ((i = t[ i + vp9_read(r, p[i >> 1])]) > 0)
    continue;

  return -i;
}

#endif  
