
















#ifndef _V_REG_H_
#define _V_REG_H_

#define VI_TRANSFORMB 1
#define VI_WINDOWB 1
#define VI_TIMEB 1
#define VI_FLOORB 2
#define VI_RESB 3
#define VI_MAPB 1

#include "backends.h"

#if defined(_WIN32) && defined(VORBISDLL_IMPORT)
# define EXTERN __declspec(dllimport) extern
#else
# define EXTERN extern
#endif

EXTERN vorbis_func_floor     *_floor_P[];
EXTERN vorbis_func_residue   *_residue_P[];
EXTERN vorbis_func_mapping   *_mapping_P[];

#endif
