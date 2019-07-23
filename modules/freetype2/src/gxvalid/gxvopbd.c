

























#include "gxvalid.h"
#include "gxvcommn.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvopbd


  
  
  
  
  
  
  

  typedef struct  GXV_opbd_DataRec_
  {
    FT_UShort  format;
    FT_UShort  valueOffset_min;

  } GXV_opbd_DataRec, *GXV_opbd_Data;


#define GXV_OPBD_DATA( FIELD )  GXV_TABLE_DATA( opbd, FIELD )


  
  
  
  
  
  
  

  static void
  gxv_opbd_LookupValue_validate( FT_UShort            glyph,
                                 GXV_LookupValueDesc  value,
                                 GXV_Validator        valid )
  {
    
    FT_Bytes   p     = valid->root->base + value.u;
    FT_Bytes   limit = valid->root->limit;
    FT_Short   delta_value;
    int        i;


    if ( value.u < GXV_OPBD_DATA( valueOffset_min ) )
      GXV_OPBD_DATA( valueOffset_min ) = value.u;

    for ( i = 0; i < 4; i++ )
    {
      GXV_LIMIT_CHECK( 2 );
      delta_value = FT_NEXT_SHORT( p );

      if ( GXV_OPBD_DATA( format ) )    
      {
        if ( delta_value == -1 )
          continue;

        gxv_ctlPoint_validate( glyph, delta_value, valid );
      }
      else                              
        continue;
    }
  }


  































  static GXV_LookupValueDesc
  gxv_opbd_LookupFmt4_transit( FT_UShort            relative_gindex,
                               GXV_LookupValueDesc  base_value,
                               FT_Bytes             lookuptbl_limit,
                               GXV_Validator        valid )
  {
    GXV_LookupValueDesc  value;

    FT_UNUSED( lookuptbl_limit );
    FT_UNUSED( valid );

    
    value.u = (FT_UShort)( base_value.u +
                           relative_gindex * 4 * sizeof ( FT_Short ) );

    return value;
  }


  
  
  
  
  
  
  

  FT_LOCAL_DEF( void )
  gxv_opbd_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  ftvalid )
  {
    GXV_ValidatorRec  validrec;
    GXV_Validator     valid = &validrec;
    GXV_opbd_DataRec  opbdrec;
    GXV_opbd_Data     opbd  = &opbdrec;
    FT_Bytes          p     = table;
    FT_Bytes          limit = 0;

    FT_ULong  version;


    valid->root       = ftvalid;
    valid->table_data = opbd;
    valid->face       = face;

    FT_TRACE3(( "validating `opbd' table\n" ));
    GXV_INIT;
    GXV_OPBD_DATA( valueOffset_min ) = 0xFFFFU;


    GXV_LIMIT_CHECK( 4 + 2 );
    version                 = FT_NEXT_ULONG( p );
    GXV_OPBD_DATA( format ) = FT_NEXT_USHORT( p );


    
    GXV_TRACE(( "(version=0x%08x)\n", version ));
    if ( 0x00010000UL != version )
      FT_INVALID_FORMAT;

    
    GXV_TRACE(( "(format=0x%04x)\n", GXV_OPBD_DATA( format ) ));
    if ( 0x0001 < GXV_OPBD_DATA( format ) )
      FT_INVALID_FORMAT;

    valid->lookupval_sign   = GXV_LOOKUPVALUE_UNSIGNED;
    valid->lookupval_func   = gxv_opbd_LookupValue_validate;
    valid->lookupfmt4_trans = gxv_opbd_LookupFmt4_transit;

    gxv_LookupTable_validate( p, limit, valid );
    p += valid->subtable_length;

    if ( p > table + GXV_OPBD_DATA( valueOffset_min ) )
    {
      GXV_TRACE((
        "found overlap between LookupTable and opbd_value array\n" ));
      FT_INVALID_OFFSET;
    }

    FT_TRACE4(( "\n" ));
  }



