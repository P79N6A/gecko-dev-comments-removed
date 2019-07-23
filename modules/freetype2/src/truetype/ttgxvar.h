

















#ifndef __TTGXVAR_H__
#define __TTGXVAR_H__


#include <ft2build.h>
#include "ttobjs.h"


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  typedef struct  GX_AVarCorrespondenceRec_
  {
    FT_Fixed  fromCoord;
    FT_Fixed  toCoord;

  } GX_AVarCorrespondenceRec_, *GX_AVarCorrespondence;


  
  
  
  
  
  
  
  
  
  typedef struct  GX_AVarSegmentRec_
  {
    FT_UShort              pairCount;
    GX_AVarCorrespondence  correspondence; 

  } GX_AVarSegmentRec, *GX_AVarSegment;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  GX_BlendRec_
  {
    FT_UInt         num_axis;
    FT_Fixed*       normalizedcoords;

    FT_MM_Var*      mmvar;
    FT_Int          mmvar_len;

    FT_Bool         avar_checked;
    GX_AVarSegment  avar_segment;

    FT_UInt         tuplecount;      
    FT_Fixed*       tuplecoords;     

    FT_UInt         gv_glyphcnt;
    FT_ULong*       glyphoffsets;

  } GX_BlendRec;


  
  
  
  
  
  
  
  
  typedef enum  GX_TupleCountFlags_
  {
    GX_TC_TUPLES_SHARE_POINT_NUMBERS = 0x8000,
    GX_TC_RESERVED_TUPLE_FLAGS       = 0x7000,
    GX_TC_TUPLE_COUNT_MASK           = 0x0FFF

  } GX_TupleCountFlags;


  
  
  
  
  
  
  
  
  
  typedef enum  GX_TupleIndexFlags_
  {
    GX_TI_EMBEDDED_TUPLE_COORD  = 0x8000,
    GX_TI_INTERMEDIATE_TUPLE    = 0x4000,
    GX_TI_PRIVATE_POINT_NUMBERS = 0x2000,
    GX_TI_RESERVED_TUPLE_FLAG   = 0x1000,
    GX_TI_TUPLE_INDEX_MASK      = 0x0FFF

  } GX_TupleIndexFlags;


#define TTAG_wght  FT_MAKE_TAG( 'w', 'g', 'h', 't' )
#define TTAG_wdth  FT_MAKE_TAG( 'w', 'd', 't', 'h' )
#define TTAG_opsz  FT_MAKE_TAG( 'o', 'p', 's', 'z' )
#define TTAG_slnt  FT_MAKE_TAG( 's', 'l', 'n', 't' )


  FT_LOCAL( FT_Error )
  TT_Set_MM_Blend( TT_Face    face,
                   FT_UInt    num_coords,
                   FT_Fixed*  coords );

  FT_LOCAL( FT_Error )
  TT_Set_Var_Design( TT_Face    face,
                     FT_UInt    num_coords,
                     FT_Fixed*  coords );

  FT_LOCAL( FT_Error )
  TT_Get_MM_Var( TT_Face      face,
                 FT_MM_Var*  *master );


  FT_LOCAL( FT_Error )
  tt_face_vary_cvt( TT_Face    face,
                    FT_Stream  stream );


  FT_LOCAL( FT_Error )
  TT_Vary_Get_Glyph_Deltas( TT_Face      face,
                            FT_UInt      glyph_index,
                            FT_Vector*  *deltas,
                            FT_UInt      n_points );


  FT_LOCAL( void )
  tt_done_blend( FT_Memory  memory,
                 GX_Blend   blend );


FT_END_HEADER


#endif 



