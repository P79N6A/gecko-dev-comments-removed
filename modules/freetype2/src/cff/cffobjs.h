

















#ifndef __CFFOBJS_H__
#define __CFFOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include "cfftypes.h"
#include FT_INTERNAL_TRUETYPE_TYPES_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  typedef struct CFF_DriverRec_*  CFF_Driver;

  typedef TT_Face  CFF_Face;


  
  
  
  
  
  
  
  
  typedef struct  CFF_SizeRec_
  {
    FT_SizeRec  root;
    FT_ULong    strike_index;    

  } CFF_SizeRec, *CFF_Size;


  
  
  
  
  
  
  
  
  typedef struct  CFF_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } CFF_GlyphSlotRec, *CFF_GlyphSlot;


  
  
  
  
  
  
  
  
  typedef struct  CFF_InternalRec_
  {
    PSH_Globals  topfont;
    PSH_Globals  subfonts[CFF_MAX_CID_FONTS];

  } CFF_InternalRec, *CFF_Internal;


  
  
  
  
  typedef struct  CFF_Transform_
  {
    FT_Fixed    xx, xy;     
    FT_Fixed    yx, yy;
    FT_F26Dot6  ox, oy;     

  } CFF_Transform;


  
  
  
  
  typedef struct  CFF_DriverRec_
  {
    FT_DriverRec  root;

    FT_UInt  hinting_engine;
    FT_Bool  no_stem_darkening;

  } CFF_DriverRec;


  FT_LOCAL( FT_Error )
  cff_size_init( FT_Size  size );           

  FT_LOCAL( void )
  cff_size_done( FT_Size  size );           

  FT_LOCAL( FT_Error )
  cff_size_request( FT_Size          size,
                    FT_Size_Request  req );

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  FT_LOCAL( FT_Error )
  cff_size_select( FT_Size   size,
                   FT_ULong  strike_index );

#endif

  FT_LOCAL( void )
  cff_slot_done( FT_GlyphSlot  slot );

  FT_LOCAL( FT_Error )
  cff_slot_init( FT_GlyphSlot  slot );


  
  
  
  
  FT_LOCAL( FT_Error )
  cff_face_init( FT_Stream      stream,
                 FT_Face        face,           
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params );

  FT_LOCAL( void )
  cff_face_done( FT_Face  face );               


  
  
  
  
  FT_LOCAL( FT_Error )
  cff_driver_init( FT_Module  module );         

  FT_LOCAL( void )
  cff_driver_done( FT_Module  module );         


FT_END_HEADER

#endif 



