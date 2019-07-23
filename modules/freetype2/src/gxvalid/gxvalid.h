

























#ifndef __GXVALID_H__
#define __GXVALID_H__

#include <ft2build.h>
#include FT_FREETYPE_H

#include "gxverror.h"          

#include FT_INTERNAL_VALIDATE_H
#include FT_INTERNAL_STREAM_H


FT_BEGIN_HEADER


  FT_LOCAL( void )
  gxv_feat_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );


  FT_LOCAL( void )
  gxv_bsln_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );


  FT_LOCAL( void )
  gxv_trak_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_just_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_morx_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_kern_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_kern_validate_classic( FT_Bytes      table,
                             FT_Face       face,
                             FT_Int        dialect_flags,
                             FT_Validator  valid );

  FT_LOCAL( void )
  gxv_opbd_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_prop_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );

  FT_LOCAL( void )
  gxv_lcar_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  valid );


FT_END_HEADER


#endif 



