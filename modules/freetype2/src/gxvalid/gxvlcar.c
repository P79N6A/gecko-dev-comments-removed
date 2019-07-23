

























#include "gxvalid.h"
#include "gxvcommn.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvlcar


  
  
  
  
  
  
  

  typedef struct  GXV_lcar_DataRec_
  {
    FT_UShort  format;

  } GXV_lcar_DataRec, *GXV_lcar_Data;


#define GXV_LCAR_DATA( FIELD )  GXV_TABLE_DATA( lcar, FIELD )


  
  
  
  
  
  
  

  static void
  gxv_lcar_partial_validate( FT_UShort      partial,
                             FT_UShort      glyph,
                             GXV_Validator  valid )
  {
    GXV_NAME_ENTER( "partial" );

    if ( GXV_LCAR_DATA( format ) != 1 )
      goto Exit;

    gxv_ctlPoint_validate( glyph, partial, valid );

  Exit:
    GXV_EXIT;
  }


  static void
  gxv_lcar_LookupValue_validate( FT_UShort            glyph,
                                 GXV_LookupValueDesc  value,
                                 GXV_Validator        valid )
  {
    FT_Bytes   p     = valid->root->base + value.u;
    FT_Bytes   limit = valid->root->limit;
    FT_UShort  count;
    FT_Short   partial;
    FT_UShort  i;


    GXV_NAME_ENTER( "element in lookupTable" );

    GXV_LIMIT_CHECK( 2 );
    count = FT_NEXT_USHORT( p );

    GXV_LIMIT_CHECK( 2 * count );
    for ( i = 0; i < count; i++ )
    {
      partial = FT_NEXT_SHORT( p );
      gxv_lcar_partial_validate( partial, glyph, valid );
    }

    GXV_EXIT;
  }


  


































  static GXV_LookupValueDesc
  gxv_lcar_LookupFmt4_transit( FT_UShort            relative_gindex,
                               GXV_LookupValueDesc  base_value,
                               FT_Bytes             lookuptbl_limit,
                               GXV_Validator        valid )
  {
    FT_Bytes             p;
    FT_Bytes             limit;
    FT_UShort            offset;
    GXV_LookupValueDesc  value;

    FT_UNUSED( lookuptbl_limit );

    
    offset = (FT_UShort)( base_value.u +
                          relative_gindex * sizeof ( FT_UShort ) );
    p      = valid->root->base + offset;
    limit  = valid->root->limit;

    GXV_LIMIT_CHECK ( 2 );
    value.u = FT_NEXT_USHORT( p );

    return value;
  }


  
  
  
  
  
  
  

  FT_LOCAL_DEF( void )
  gxv_lcar_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  ftvalid )
  {
    FT_Bytes          p     = table;
    FT_Bytes          limit = 0;
    GXV_ValidatorRec  validrec;
    GXV_Validator     valid = &validrec;

    GXV_lcar_DataRec  lcarrec;
    GXV_lcar_Data     lcar = &lcarrec;

    FT_Fixed          version;


    valid->root       = ftvalid;
    valid->table_data = lcar;
    valid->face       = face;

    FT_TRACE3(( "validating `lcar' table\n" ));
    GXV_INIT;

    GXV_LIMIT_CHECK( 4 + 2 );
    version = FT_NEXT_ULONG( p );
    GXV_LCAR_DATA( format ) = FT_NEXT_USHORT( p );

    if ( version != 0x00010000UL)
      FT_INVALID_FORMAT;

    if ( GXV_LCAR_DATA( format ) > 1 )
      FT_INVALID_FORMAT;

    valid->lookupval_sign   = GXV_LOOKUPVALUE_UNSIGNED;
    valid->lookupval_func   = gxv_lcar_LookupValue_validate;
    valid->lookupfmt4_trans = gxv_lcar_LookupFmt4_transit;
    gxv_LookupTable_validate( p, limit, valid );

    FT_TRACE4(( "\n" ));
  }



