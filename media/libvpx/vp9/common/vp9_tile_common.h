









#ifndef VP9_COMMON_VP9_TILE_COMMON_H_
#define VP9_COMMON_VP9_TILE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

struct VP9Common;

typedef struct TileInfo {
  int mi_row_start, mi_row_end;
  int mi_col_start, mi_col_end;
} TileInfo;



void vp9_tile_init(TileInfo *tile, const struct VP9Common *cm,
                   int row, int col);

void vp9_get_tile_n_bits(int mi_cols,
                         int *min_log2_tile_cols, int *max_log2_tile_cols);

#ifdef __cplusplus
}  
#endif

#endif
