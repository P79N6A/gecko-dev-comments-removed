

















#ifndef __OTVALID_H__
#define __OTVALID_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#include "otverror.h"           

#include FT_INTERNAL_VALIDATE_H
#include FT_INTERNAL_STREAM_H


FT_BEGIN_HEADER


  FT_LOCAL( void )
  otv_BASE_validate( FT_Bytes      table,
                     FT_Validator  valid );

  
  
  FT_LOCAL( void )
  otv_GDEF_validate( FT_Bytes      table,
                     FT_Bytes      gsub,
                     FT_Bytes      gpos,
                     FT_Validator  valid );

  FT_LOCAL( void )
  otv_GPOS_validate( FT_Bytes      table,
                     FT_UInt       glyph_count,
                     FT_Validator  valid );

  FT_LOCAL( void )
  otv_GSUB_validate( FT_Bytes      table,
                     FT_UInt       glyph_count,
                     FT_Validator  valid );

  
  
  FT_LOCAL( void )
  otv_JSTF_validate( FT_Bytes      table,
                     FT_Bytes      gsub,
                     FT_Bytes      gpos,
                     FT_UInt       glyph_count,
                     FT_Validator  valid );

  FT_LOCAL( void )
  otv_MATH_validate( FT_Bytes      table,
                     FT_UInt       glyph_count,
                     FT_Validator  ftvalid );


FT_END_HEADER

#endif 



