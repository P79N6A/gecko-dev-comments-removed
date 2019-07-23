

















#ifndef __SVOTVAL_H__
#define __SVOTVAL_H__

#include FT_OPENTYPE_VALIDATE_H
#include FT_INTERNAL_VALIDATE_H

FT_BEGIN_HEADER


#define FT_SERVICE_ID_OPENTYPE_VALIDATE  "opentype-validate"


  typedef FT_Error
  (*otv_validate_func)( FT_Face volatile  face,
                        FT_UInt           ot_flags,
                        FT_Bytes         *base,
                        FT_Bytes         *gdef,
                        FT_Bytes         *gpos,
                        FT_Bytes         *gsub,
                        FT_Bytes         *jstf );


  FT_DEFINE_SERVICE( OTvalidate )
  {
    otv_validate_func  validate;
  };

  


FT_END_HEADER


#endif 



