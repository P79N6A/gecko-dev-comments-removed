









#ifndef VP9_ENCODER_VP9_TOKENIZE_H_
#define VP9_ENCODER_VP9_TOKENIZE_H_

#include "vp9/common/vp9_entropy.h"
#include "vp9/encoder/vp9_block.h"

void vp9_tokenize_initialize();

typedef struct {
  int16_t token;
  int16_t extra;
} TOKENVALUE;

typedef struct {
  const vp9_prob *context_tree;
  int16_t         extra;
  uint8_t         token;
  uint8_t         skip_eob_node;
} TOKENEXTRA;

int vp9_sb_is_skippable(MACROBLOCKD *xd, BLOCK_SIZE bsize);
int vp9_is_skippable_in_plane(MACROBLOCKD *xd, BLOCK_SIZE bsize,
                              int plane);
struct VP9_COMP;

void vp9_tokenize_sb(struct VP9_COMP *cpi, TOKENEXTRA **t, int dry_run,
                     BLOCK_SIZE bsize);

extern const int *vp9_dct_value_cost_ptr;




extern const TOKENVALUE *vp9_dct_value_tokens_ptr;

#endif  
