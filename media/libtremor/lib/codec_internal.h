
















#ifndef _V_CODECI_H_
#define _V_CODECI_H_

#include "codebook.h"

typedef void vorbis_look_mapping;
typedef void vorbis_look_floor;
typedef void vorbis_look_residue;
typedef void vorbis_look_transform;


typedef struct {
  int blockflag;
  int windowtype;
  int transformtype;
  int mapping;
} vorbis_info_mode;

typedef void vorbis_info_floor;
typedef void vorbis_info_residue;
typedef void vorbis_info_mapping;

typedef struct private_state {
  
  const void             *window[2];

  
  int                     modebits;
  vorbis_look_mapping   **mode;

  ogg_int64_t sample_count;

} private_state;







typedef struct codec_setup_info {

  


  long blocksizes[2];

  




  int        modes;
  int        maps;
  int        times;
  int        floors;
  int        residues;
  int        books;

  vorbis_info_mode       *mode_param[64];
  int                     map_type[64];
  vorbis_info_mapping    *map_param[64];
  int                     time_type[64];
  int                     floor_type[64];
  vorbis_info_floor      *floor_param[64];
  int                     residue_type[64];
  vorbis_info_residue    *residue_param[64];
  static_codebook        *book_param[256];
  codebook               *fullbooks;

  int    passlimit[32];     
  int    coupling_passes;
} codec_setup_info;

#endif
