

























#ifndef __GXVMORX_H__
#define __GXVMORX_H__


#include "gxvalid.h"
#include "gxvcommn.h"
#include "gxvmort.h"

#include FT_SFNT_NAMES_H


  FT_LOCAL( void )
  gxv_morx_subtable_type0_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_morx_subtable_type1_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_morx_subtable_type2_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_morx_subtable_type4_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_morx_subtable_type5_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );


#endif 



