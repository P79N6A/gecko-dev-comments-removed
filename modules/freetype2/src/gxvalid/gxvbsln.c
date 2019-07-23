

























#include "gxvalid.h"
#include "gxvcommn.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvbsln


  
  
  
  
  
  
  

#define GXV_BSLN_VALUE_COUNT  32
#define GXV_BSLN_VALUE_EMPTY  0xFFFFU


  typedef struct  GXV_bsln_DataRec_
  {
    FT_Bytes   ctlPoints_p;
    FT_UShort  defaultBaseline;

  } GXV_bsln_DataRec, *GXV_bsln_Data;


#define GXV_BSLN_DATA( field )  GXV_TABLE_DATA( bsln, field )


  
  
  
  
  
  
  

  static void
  gxv_bsln_LookupValue_validate( FT_UShort            glyph,
                                 GXV_LookupValueDesc  value,
                                 GXV_Validator        valid )
  {
    FT_UShort   v = value.u;
    FT_UShort*  ctlPoints;

    FT_UNUSED( glyph );


    GXV_NAME_ENTER( "lookup value" );

    if ( v >= GXV_BSLN_VALUE_COUNT )
      FT_INVALID_DATA;

    ctlPoints = (FT_UShort*)GXV_BSLN_DATA( ctlPoints_p );
    if ( ctlPoints && ctlPoints[v] == GXV_BSLN_VALUE_EMPTY )
      FT_INVALID_DATA;

    GXV_EXIT;
  }


  


























  static GXV_LookupValueDesc
  gxv_bsln_LookupFmt4_transit( FT_UShort            relative_gindex,
                               GXV_LookupValueDesc  base_value,
                               FT_Bytes             lookuptbl_limit,
                               GXV_Validator        valid )
  {
    FT_Bytes             p;
    FT_Bytes             limit;
    FT_UShort            offset;
    GXV_LookupValueDesc  value;

    
    offset = (FT_UShort)( base_value.u +
                          ( relative_gindex * sizeof ( FT_UShort ) ) );

    p     = valid->lookuptbl_head + offset;
    limit = lookuptbl_limit;
    GXV_LIMIT_CHECK( 2 );

    value.u = FT_NEXT_USHORT( p );

    return value;
  }


  static void
  gxv_bsln_parts_fmt0_validate( FT_Bytes       tables,
                                FT_Bytes       limit,
                                GXV_Validator  valid )
  {
    FT_Bytes  p = tables;


    GXV_NAME_ENTER( "parts format 0" );

    
    GXV_LIMIT_CHECK( 2 * GXV_BSLN_VALUE_COUNT );

    valid->table_data = NULL;      

    GXV_EXIT;
  }


  static void
  gxv_bsln_parts_fmt1_validate( FT_Bytes       tables,
                                FT_Bytes       limit,
                                GXV_Validator  valid )
  {
    FT_Bytes  p = tables;


    GXV_NAME_ENTER( "parts format 1" );

    
    gxv_bsln_parts_fmt0_validate( p, limit, valid );

    
    valid->lookupval_sign   = GXV_LOOKUPVALUE_UNSIGNED;
    valid->lookupval_func   = gxv_bsln_LookupValue_validate;
    valid->lookupfmt4_trans = gxv_bsln_LookupFmt4_transit;
    gxv_LookupTable_validate( p + 2 * GXV_BSLN_VALUE_COUNT,
                              limit,
                              valid );

    GXV_EXIT;
  }


  static void
  gxv_bsln_parts_fmt2_validate( FT_Bytes       tables,
                                FT_Bytes       limit,
                                GXV_Validator  valid )
  {
    FT_Bytes   p = tables;

    FT_UShort  stdGlyph;
    FT_UShort  ctlPoint;
    FT_Int     i;

    FT_UShort  defaultBaseline = GXV_BSLN_DATA( defaultBaseline );


    GXV_NAME_ENTER( "parts format 2" );

    GXV_LIMIT_CHECK( 2 + ( 2 * GXV_BSLN_VALUE_COUNT ) );

    
    stdGlyph = FT_NEXT_USHORT( p );
    GXV_TRACE(( " (stdGlyph = %u)\n", stdGlyph ));

    gxv_glyphid_validate( stdGlyph, valid );

    
    GXV_BSLN_DATA( ctlPoints_p ) = p;

    
    for ( i = 0; i < GXV_BSLN_VALUE_COUNT; i++ )
    {
      ctlPoint = FT_NEXT_USHORT( p );
      if ( ctlPoint == GXV_BSLN_VALUE_EMPTY )
      {
        if ( i == defaultBaseline )
          FT_INVALID_DATA;
      }
      else
        gxv_ctlPoint_validate( stdGlyph, (FT_Short)ctlPoint, valid );
    }

    GXV_EXIT;
  }


  static void
  gxv_bsln_parts_fmt3_validate( FT_Bytes       tables,
                                FT_Bytes       limit,
                                GXV_Validator  valid)
  {
    FT_Bytes  p = tables;


    GXV_NAME_ENTER( "parts format 3" );

    
    gxv_bsln_parts_fmt2_validate( p, limit, valid );

    
    valid->lookupval_sign   = GXV_LOOKUPVALUE_UNSIGNED;
    valid->lookupval_func   = gxv_bsln_LookupValue_validate;
    valid->lookupfmt4_trans = gxv_bsln_LookupFmt4_transit;
    gxv_LookupTable_validate( p + ( 2 + 2 * GXV_BSLN_VALUE_COUNT ),
                              limit,
                              valid );

    GXV_EXIT;
  }


  
  
  
  
  
  
  

  FT_LOCAL_DEF( void )
  gxv_bsln_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  ftvalid )
  {
    GXV_ValidatorRec  validrec;
    GXV_Validator     valid = &validrec;

    GXV_bsln_DataRec  bslnrec;
    GXV_bsln_Data     bsln = &bslnrec;

    FT_Bytes  p     = table;
    FT_Bytes  limit = 0;

    FT_ULong   version;
    FT_UShort  format;
    FT_UShort  defaultBaseline;

    GXV_Validate_Func  fmt_funcs_table [] =
    {
      gxv_bsln_parts_fmt0_validate,
      gxv_bsln_parts_fmt1_validate,
      gxv_bsln_parts_fmt2_validate,
      gxv_bsln_parts_fmt3_validate,
    };


    valid->root       = ftvalid;
    valid->table_data = bsln;
    valid->face       = face;

    FT_TRACE3(( "validating `bsln' table\n" ));
    GXV_INIT;


    GXV_LIMIT_CHECK( 4 + 2 + 2 );
    version         = FT_NEXT_ULONG( p );
    format          = FT_NEXT_USHORT( p );
    defaultBaseline = FT_NEXT_USHORT( p );

    
    if ( version != 0x00010000UL )
      FT_INVALID_FORMAT;

    
    GXV_TRACE(( " (format = %d)\n", format ));
    if ( format > 3 )
      FT_INVALID_FORMAT;

    if ( defaultBaseline > 31 )
      FT_INVALID_FORMAT;

    bsln->defaultBaseline = defaultBaseline;

    fmt_funcs_table[format]( p, limit, valid );

    FT_TRACE4(( "\n" ));
  }







