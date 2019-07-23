


























#ifndef __SVGXVAL_H__
#define __SVGXVAL_H__

#include FT_GX_VALIDATE_H
#include FT_INTERNAL_VALIDATE_H

FT_BEGIN_HEADER


#define FT_SERVICE_ID_GX_VALIDATE           "truetypegx-validate"
#define FT_SERVICE_ID_CLASSICKERN_VALIDATE  "classickern-validate"

  typedef FT_Error
  (*gxv_validate_func)( FT_Face   face,
                        FT_UInt   gx_flags,
                        FT_Bytes  tables[FT_VALIDATE_GX_LENGTH],
                        FT_UInt   table_length );


  typedef FT_Error
  (*ckern_validate_func)( FT_Face   face,
                          FT_UInt   ckern_flags,
                          FT_Bytes  *ckern_table );


  FT_DEFINE_SERVICE( GXvalidate )
  {
    gxv_validate_func  validate;
  };

  FT_DEFINE_SERVICE( CKERNvalidate )
  {
    ckern_validate_func  validate;
  };

  


FT_END_HEADER


#endif 



