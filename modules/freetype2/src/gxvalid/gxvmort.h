

























#ifndef __GXVMORT_H__
#define __GXVMORT_H__

#include "gxvalid.h"
#include "gxvcommn.h"

#include FT_SFNT_NAMES_H


  typedef struct  GXV_mort_featureRec_
  {
    FT_UShort  featureType;
    FT_UShort  featureSetting;
    FT_ULong   enableFlags;
    FT_ULong   disableFlags;

  } GXV_mort_featureRec, *GXV_mort_feature;

#define GXV_MORT_FEATURE_OFF  {0, 1, 0x00000000UL, 0x00000000UL}

#define IS_GXV_MORT_FEATURE_OFF( f )              \
          ( (f).featureType    == 0            || \
            (f).featureSetting == 1            || \
            (f).enableFlags    == 0x00000000UL || \
            (f).disableFlags   == 0x00000000UL )


  FT_LOCAL( void )
  gxv_mort_featurearray_validate( FT_Bytes       table,
                                  FT_Bytes       limit,
                                  FT_UInt        nFeatureFlags,
                                  GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_coverage_validate( FT_UShort      coverage,
                              GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_subtable_type0_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_subtable_type1_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_subtable_type2_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_subtable_type4_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );

  FT_LOCAL( void )
  gxv_mort_subtable_type5_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid );


#endif 



