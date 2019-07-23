

















#ifndef __T1OBJS_H__
#define __T1OBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  
  typedef struct T1_Size_Hints_   T1_Size_Hints;
  typedef struct T1_Glyph_Hints_  T1_Glyph_Hints;


  
  
  
  
  
  
  
  
  typedef struct T1_DriverRec_   *T1_Driver;


  
  
  
  
  
  
  
  
  typedef struct T1_SizeRec_*  T1_Size;


  
  
  
  
  
  
  
  
  typedef struct T1_GlyphSlotRec_*  T1_GlyphSlot;


  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct T1_CharMapRec_*   T1_CharMap;


  
  
  
  
  


  
  
  
  
  
  
  
  
  typedef struct  T1_SizeRec_
  {
    FT_SizeRec  root;

  } T1_SizeRec;


  FT_LOCAL( void )
  T1_Size_Done( T1_Size  size );

  FT_LOCAL( FT_Error )
  T1_Size_Request( T1_Size          size,
                   FT_Size_Request  req );

  FT_LOCAL( FT_Error )
  T1_Size_Init( T1_Size  size );


  
  
  
  
  
  
  
  
  typedef struct  T1_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Int           max_points;
    FT_Int           max_contours;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } T1_GlyphSlotRec;


  FT_LOCAL( FT_Error )
  T1_Face_Init( FT_Stream      stream,
                T1_Face        face,
                FT_Int         face_index,
                FT_Int         num_params,
                FT_Parameter*  params );

  FT_LOCAL( void )
  T1_Face_Done( T1_Face  face );

  FT_LOCAL( FT_Error )
  T1_GlyphSlot_Init( T1_GlyphSlot  slot );

  FT_LOCAL( void )
  T1_GlyphSlot_Done( T1_GlyphSlot  slot );

  FT_LOCAL( FT_Error )
  T1_Driver_Init( T1_Driver  driver );

  FT_LOCAL( void )
  T1_Driver_Done( T1_Driver  driver );


FT_END_HEADER

#endif 



