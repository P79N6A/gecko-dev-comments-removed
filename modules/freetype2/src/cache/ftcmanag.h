

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTCMANAG_H__
#define __FTCMANAG_H__


#include <ft2build.h>
#include FT_CACHE_H
#include "ftcmru.h"
#include "ftccache.h"


FT_BEGIN_HEADER


  
  
  
  
  
  


#define FTC_MAX_FACES_DEFAULT  2
#define FTC_MAX_SIZES_DEFAULT  4
#define FTC_MAX_BYTES_DEFAULT  200000L  /* ~200kByte by default */

  
#define FTC_MAX_CACHES         16


  typedef struct  FTC_ManagerRec_
  {
    FT_Library          library;
    FT_Memory           memory;

    FTC_Node            nodes_list;
    FT_ULong            max_weight;
    FT_ULong            cur_weight;
    FT_UInt             num_nodes;

    FTC_Cache           caches[FTC_MAX_CACHES];
    FT_UInt             num_caches;

    FTC_MruListRec      faces;
    FTC_MruListRec      sizes;

    FT_Pointer          request_data;
    FTC_Face_Requester  request_face;

  } FTC_ManagerRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL( void )
  FTC_Manager_Compress( FTC_Manager  manager );


  


  FT_LOCAL( FT_UInt )
  FTC_Manager_FlushN( FTC_Manager  manager,
                      FT_UInt      count );


  
  FT_LOCAL( FT_Error )
  FTC_Manager_RegisterCache( FTC_Manager      manager,
                             FTC_CacheClass   clazz,
                             FTC_Cache       *acache );

 

#define FTC_SCALER_COMPARE( a, b )                \
    ( (a)->face_id      == (b)->face_id      &&   \
      (a)->width        == (b)->width        &&   \
      (a)->height       == (b)->height       &&   \
      ((a)->pixel != 0) == ((b)->pixel != 0) &&   \
      ( (a)->pixel ||                             \
        ( (a)->x_res == (b)->x_res &&             \
          (a)->y_res == (b)->y_res ) ) )

#define FTC_SCALER_HASH( q )                                 \
    ( FTC_FACE_ID_HASH( (q)->face_id ) +                     \
      (q)->width + (q)->height*7 +                           \
      ( (q)->pixel ? 0 : ( (q)->x_res*33 ^ (q)->y_res*61 ) ) )

 

FT_END_HEADER

#endif 



