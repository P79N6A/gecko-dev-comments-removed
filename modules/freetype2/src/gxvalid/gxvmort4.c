


























#include "gxvmort.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvmort


  static void
  gxv_mort_subtable_type4_lookupval_validate( FT_UShort            glyph,
                                              GXV_LookupValueCPtr  value_p,
                                              GXV_Validator        gxvalid )
  {
    FT_UNUSED( glyph );

    gxv_glyphid_validate( value_p->u, gxvalid );
  }

  


























  static GXV_LookupValueDesc
  gxv_mort_subtable_type4_lookupfmt4_transit(
    FT_UShort            relative_gindex,
    GXV_LookupValueCPtr  base_value_p,
    FT_Bytes             lookuptbl_limit,
    GXV_Validator        gxvalid )
  {
    FT_Bytes             p;
    FT_Bytes             limit;
    FT_UShort            offset;
    GXV_LookupValueDesc  value;

    
    offset = (FT_UShort)( base_value_p->u +
                          relative_gindex * sizeof ( FT_UShort ) );

    p     = gxvalid->lookuptbl_head + offset;
    limit = lookuptbl_limit;

    GXV_LIMIT_CHECK( 2 );
    value.u = FT_NEXT_USHORT( p );

    return value;
  }


  FT_LOCAL_DEF( void )
  gxv_mort_subtable_type4_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;


    GXV_NAME_ENTER( "mort chain subtable type4 "
                    "(Non-Contextual Glyph Substitution)" );

    gxvalid->lookupval_sign   = GXV_LOOKUPVALUE_UNSIGNED;
    gxvalid->lookupval_func   = gxv_mort_subtable_type4_lookupval_validate;
    gxvalid->lookupfmt4_trans = gxv_mort_subtable_type4_lookupfmt4_transit;

    gxv_LookupTable_validate( p, limit, gxvalid );

    GXV_EXIT;
  }



