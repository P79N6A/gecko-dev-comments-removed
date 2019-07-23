


























#include "gxvmorx.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvmorx


  static void
  gxv_morx_subtable_type0_entry_validate(
    FT_UShort                        state,
    FT_UShort                        flags,
    GXV_XStateTable_GlyphOffsetDesc  glyphOffset,
    FT_Bytes                         table,
    FT_Bytes                         limit,
    GXV_Validator                    valid )
  {
    FT_UShort  markFirst;
    FT_UShort  dontAdvance;
    FT_UShort  markLast;
    FT_UShort  reserved;
    FT_UShort  verb;

    FT_UNUSED( state );
    FT_UNUSED( glyphOffset );
    FT_UNUSED( table );
    FT_UNUSED( limit );


    markFirst   = (FT_UShort)( ( flags >> 15 ) & 1 );
    dontAdvance = (FT_UShort)( ( flags >> 14 ) & 1 );
    markLast    = (FT_UShort)( ( flags >> 13 ) & 1 );

    reserved = (FT_UShort)( flags & 0x1FF0 );
    verb     = (FT_UShort)( flags & 0x000F );

    if ( 0 < reserved )
    {
      GXV_TRACE(( " non-zero bits found in reserved range\n" ));
      FT_INVALID_DATA;
    }
  }


  FT_LOCAL_DEF( void )
  gxv_morx_subtable_type0_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  valid )
  {
    FT_Bytes  p = table;


    GXV_NAME_ENTER(
      "morx chain subtable type0 (Indic-Script Rearrangement)" );

    GXV_LIMIT_CHECK( GXV_STATETABLE_HEADER_SIZE );

    valid->xstatetable.optdata               = NULL;
    valid->xstatetable.optdata_load_func     = NULL;
    valid->xstatetable.subtable_setup_func   = NULL;
    valid->xstatetable.entry_glyphoffset_fmt = GXV_GLYPHOFFSET_NONE;
    valid->xstatetable.entry_validate_func =
      gxv_morx_subtable_type0_entry_validate;

    gxv_XStateTable_validate( p, limit, valid );

    GXV_EXIT;
  }



