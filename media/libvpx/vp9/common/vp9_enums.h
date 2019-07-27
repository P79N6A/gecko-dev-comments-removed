









#ifndef VP9_COMMON_VP9_ENUMS_H_
#define VP9_COMMON_VP9_ENUMS_H_

#include "./vpx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MI_SIZE_LOG2 3
#define MI_BLOCK_SIZE_LOG2 (6 - MI_SIZE_LOG2)  // 64 = 2^6

#define MI_SIZE (1 << MI_SIZE_LOG2)  // pixels per mi-unit
#define MI_BLOCK_SIZE (1 << MI_BLOCK_SIZE_LOG2)  // mi-units per max block

#define MI_MASK (MI_BLOCK_SIZE - 1)








typedef enum BITSTREAM_PROFILE {
  PROFILE_0,
  PROFILE_1,
  PROFILE_2,
  PROFILE_3,
  MAX_PROFILES
} BITSTREAM_PROFILE;

typedef enum BLOCK_SIZE {
  BLOCK_4X4,
  BLOCK_4X8,
  BLOCK_8X4,
  BLOCK_8X8,
  BLOCK_8X16,
  BLOCK_16X8,
  BLOCK_16X16,
  BLOCK_16X32,
  BLOCK_32X16,
  BLOCK_32X32,
  BLOCK_32X64,
  BLOCK_64X32,
  BLOCK_64X64,
  BLOCK_SIZES,
  BLOCK_INVALID = BLOCK_SIZES
} BLOCK_SIZE;

typedef enum PARTITION_TYPE {
  PARTITION_NONE,
  PARTITION_HORZ,
  PARTITION_VERT,
  PARTITION_SPLIT,
  PARTITION_TYPES,
  PARTITION_INVALID = PARTITION_TYPES
} PARTITION_TYPE;

typedef char PARTITION_CONTEXT;
#define PARTITION_PLOFFSET   4  // number of probability models per block size
#define PARTITION_CONTEXTS (4 * PARTITION_PLOFFSET)


typedef enum {
  TX_4X4 = 0,                      
  TX_8X8 = 1,                      
  TX_16X16 = 2,                    
  TX_32X32 = 3,                    
  TX_SIZES
} TX_SIZE;


typedef enum {
  ONLY_4X4            = 0,        
  ALLOW_8X8           = 1,        
  ALLOW_16X16         = 2,        
  ALLOW_32X32         = 3,        
  TX_MODE_SELECT      = 4,        
  TX_MODES            = 5,
} TX_MODE;

typedef enum {
  DCT_DCT   = 0,                      
  ADST_DCT  = 1,                      
  DCT_ADST  = 2,                      
  ADST_ADST = 3,                      
  TX_TYPES = 4
} TX_TYPE;

typedef enum {
  VP9_LAST_FLAG = 1 << 0,
  VP9_GOLD_FLAG = 1 << 1,
  VP9_ALT_FLAG = 1 << 2,
} VP9_REFFRAME;

#ifdef __cplusplus
}  
#endif

#endif
