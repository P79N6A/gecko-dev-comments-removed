









#include "vp9/encoder/vp9_context_tree.h"
#include "vp9/encoder/vp9_encoder.h"

static const BLOCK_SIZE square[] = {
  BLOCK_8X8,
  BLOCK_16X16,
  BLOCK_32X32,
  BLOCK_64X64,
};

static void alloc_mode_context(VP9_COMMON *cm, int num_4x4_blk,
                               PICK_MODE_CONTEXT *ctx) {
  const int num_blk = (num_4x4_blk < 4 ? 4 : num_4x4_blk);
  const int num_pix = num_blk << 4;
  int i, k;
  ctx->num_4x4_blk = num_blk;

  CHECK_MEM_ERROR(cm, ctx->zcoeff_blk,
                  vpx_calloc(num_4x4_blk, sizeof(uint8_t)));
  for (i = 0; i < MAX_MB_PLANE; ++i) {
    for (k = 0; k < 3; ++k) {
      CHECK_MEM_ERROR(cm, ctx->coeff[i][k],
                      vpx_memalign(16, num_pix * sizeof(*ctx->coeff[i][k])));
      CHECK_MEM_ERROR(cm, ctx->qcoeff[i][k],
                      vpx_memalign(16, num_pix * sizeof(*ctx->qcoeff[i][k])));
      CHECK_MEM_ERROR(cm, ctx->dqcoeff[i][k],
                      vpx_memalign(16, num_pix * sizeof(*ctx->dqcoeff[i][k])));
      CHECK_MEM_ERROR(cm, ctx->eobs[i][k],
                      vpx_memalign(16, num_pix * sizeof(*ctx->eobs[i][k])));
      ctx->coeff_pbuf[i][k]   = ctx->coeff[i][k];
      ctx->qcoeff_pbuf[i][k]  = ctx->qcoeff[i][k];
      ctx->dqcoeff_pbuf[i][k] = ctx->dqcoeff[i][k];
      ctx->eobs_pbuf[i][k]    = ctx->eobs[i][k];
    }
  }
}

static void free_mode_context(PICK_MODE_CONTEXT *ctx) {
  int i, k;
  vpx_free(ctx->zcoeff_blk);
  ctx->zcoeff_blk = 0;
  for (i = 0; i < MAX_MB_PLANE; ++i) {
    for (k = 0; k < 3; ++k) {
      vpx_free(ctx->coeff[i][k]);
      ctx->coeff[i][k] = 0;
      vpx_free(ctx->qcoeff[i][k]);
      ctx->qcoeff[i][k] = 0;
      vpx_free(ctx->dqcoeff[i][k]);
      ctx->dqcoeff[i][k] = 0;
      vpx_free(ctx->eobs[i][k]);
      ctx->eobs[i][k] = 0;
    }
  }
}

static void alloc_tree_contexts(VP9_COMMON *cm, PC_TREE *tree,
                                int num_4x4_blk) {
  alloc_mode_context(cm, num_4x4_blk, &tree->none);
  alloc_mode_context(cm, num_4x4_blk/2, &tree->horizontal[0]);
  alloc_mode_context(cm, num_4x4_blk/2, &tree->vertical[0]);

  

  alloc_mode_context(cm, num_4x4_blk/2, &tree->horizontal[1]);
  alloc_mode_context(cm, num_4x4_blk/2, &tree->vertical[1]);
}

static void free_tree_contexts(PC_TREE *tree) {
  free_mode_context(&tree->none);
  free_mode_context(&tree->horizontal[0]);
  free_mode_context(&tree->horizontal[1]);
  free_mode_context(&tree->vertical[0]);
  free_mode_context(&tree->vertical[1]);
}





void vp9_setup_pc_tree(VP9_COMMON *cm, ThreadData *td) {
  int i, j;
  const int leaf_nodes = 64;
  const int tree_nodes = 64 + 16 + 4 + 1;
  int pc_tree_index = 0;
  PC_TREE *this_pc;
  PICK_MODE_CONTEXT *this_leaf;
  int square_index = 1;
  int nodes;

  vpx_free(td->leaf_tree);
  CHECK_MEM_ERROR(cm, td->leaf_tree, vpx_calloc(leaf_nodes,
                                                sizeof(*td->leaf_tree)));
  vpx_free(td->pc_tree);
  CHECK_MEM_ERROR(cm, td->pc_tree, vpx_calloc(tree_nodes,
                                              sizeof(*td->pc_tree)));

  this_pc = &td->pc_tree[0];
  this_leaf = &td->leaf_tree[0];

  
  
  for (i = 0; i < leaf_nodes; ++i)
    alloc_mode_context(cm, 1, &td->leaf_tree[i]);

  
  for (pc_tree_index = 0; pc_tree_index < leaf_nodes; ++pc_tree_index) {
    PC_TREE *const tree = &td->pc_tree[pc_tree_index];
    tree->block_size = square[0];
    alloc_tree_contexts(cm, tree, 4);
    tree->leaf_split[0] = this_leaf++;
    for (j = 1; j < 4; j++)
      tree->leaf_split[j] = tree->leaf_split[0];
  }

  
  
  for (nodes = 16; nodes > 0; nodes >>= 2) {
    for (i = 0; i < nodes; ++i) {
      PC_TREE *const tree = &td->pc_tree[pc_tree_index];
      alloc_tree_contexts(cm, tree, 4 << (2 * square_index));
      tree->block_size = square[square_index];
      for (j = 0; j < 4; j++)
        tree->split[j] = this_pc++;
      ++pc_tree_index;
    }
    ++square_index;
  }
  td->pc_root = &td->pc_tree[tree_nodes - 1];
  td->pc_root[0].none.best_mode_index = 2;
}

void vp9_free_pc_tree(ThreadData *td) {
  const int tree_nodes = 64 + 16 + 4 + 1;
  int i;

  
  for (i = 0; i < 64; ++i)
    free_mode_context(&td->leaf_tree[i]);

  
  for (i = 0; i < tree_nodes; ++i)
    free_tree_contexts(&td->pc_tree[i]);

  vpx_free(td->pc_tree);
  td->pc_tree = NULL;
  vpx_free(td->leaf_tree);
  td->leaf_tree = NULL;
}
