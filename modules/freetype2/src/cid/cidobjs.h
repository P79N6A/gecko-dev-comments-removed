

















#ifndef __CIDOBJS_H__
#define __CIDOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  
  typedef struct CID_Size_Hints_   CID_Size_Hints;
  typedef struct CID_Glyph_Hints_  CID_Glyph_Hints;


  
  
  
  
  
  
  
  
  typedef struct CID_DriverRec_*  CID_Driver;


  
  
  
  
  
  
  
  
  typedef struct CID_SizeRec_*  CID_Size;


  
  
  
  
  
  
  
  
  typedef struct CID_GlyphSlotRec_*  CID_GlyphSlot;


  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct CID_CharMapRec_*  CID_CharMap;


  
  
  
  
  


  typedef struct  CID_SizeRec_
  {
    FT_SizeRec  root;
    FT_Bool     valid;

  } CID_SizeRec;


  typedef struct  CID_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } CID_GlyphSlotRec;


  FT_LOCAL( void )
  cid_slot_done( FT_GlyphSlot  slot );

  FT_LOCAL( FT_Error )
  cid_slot_init( FT_GlyphSlot  slot );


  FT_LOCAL( void )
  cid_size_done( FT_Size  size );       

  FT_LOCAL( FT_Error )
  cid_size_init( FT_Size  size );       

  FT_LOCAL( FT_Error )
  cid_size_request( FT_Size          size,      
                    FT_Size_Request  req );

  FT_LOCAL( FT_Error )
  cid_face_init( FT_Stream      stream,
                 FT_Face        face,           
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params );

  FT_LOCAL( void )
  cid_face_done( FT_Face  face );               


  FT_LOCAL( FT_Error )
  cid_driver_init( FT_Module  driver );

  FT_LOCAL( void )
  cid_driver_done( FT_Module  driver );


FT_END_HEADER

#endif 



