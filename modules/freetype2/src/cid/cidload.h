

















#ifndef __CIDLOAD_H__
#define __CIDLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_STREAM_H
#include "cidparse.h"


FT_BEGIN_HEADER


  typedef struct  CID_Loader_
  {
    CID_Parser  parser;          
    FT_Int      num_chars;       

  } CID_Loader;


  FT_LOCAL( FT_Long )
  cid_get_offset( FT_Byte**  start,
                  FT_Byte    offsize );

  FT_LOCAL( FT_Error )
  cid_face_open( CID_Face  face,
                 FT_Int    face_index );


FT_END_HEADER

#endif 



