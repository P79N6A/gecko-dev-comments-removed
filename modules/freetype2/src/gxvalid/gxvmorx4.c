


























#include "gxvmorx.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvmorx


  FT_LOCAL_DEF( void )
  gxv_morx_subtable_type4_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid )
  {
    GXV_NAME_ENTER( "morx chain subtable type4 "
                    "(Non-Contextual Glyph Substitution)" );

    gxv_mort_subtable_type4_validate( table, limit, valid );

    GXV_EXIT;
  }



