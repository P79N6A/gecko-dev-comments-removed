

























#include "gxvalid.h"
#include "gxvcommn.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvtrak


  
  
  
  
  
  
  

    























  typedef struct  GXV_trak_DataRec_
  {
    FT_UShort  trackValueOffset_min;
    FT_UShort  trackValueOffset_max;

  } GXV_trak_DataRec, *GXV_trak_Data;


#define GXV_TRAK_DATA( FIELD )  GXV_TABLE_DATA( trak, FIELD )


  
  
  
  
  
  
  

  static void
  gxv_trak_trackTable_validate( FT_Bytes       table,
                                FT_Bytes       limit,
                                FT_UShort      nTracks,
                                GXV_Validator  valid )
  {
    FT_Bytes   p = table;

    FT_Fixed   track;
    FT_UShort  nameIndex;
    FT_UShort  offset;
    FT_UShort  i;


    GXV_NAME_ENTER( "trackTable" );

    GXV_TRAK_DATA( trackValueOffset_min ) = 0xFFFFU;
    GXV_TRAK_DATA( trackValueOffset_max ) = 0x0000;

    for ( i = 0; i < nTracks; i ++ )
    {
      GXV_LIMIT_CHECK( 4 + 2 + 2 );
      track     = FT_NEXT_LONG( p );
      nameIndex = FT_NEXT_USHORT( p );
      offset    = FT_NEXT_USHORT( p );

      if ( offset < GXV_TRAK_DATA( trackValueOffset_min ) )
        GXV_TRAK_DATA( trackValueOffset_min ) = offset;
      if ( offset > GXV_TRAK_DATA( trackValueOffset_max ) )
        GXV_TRAK_DATA( trackValueOffset_max ) = offset;

      gxv_sfntName_validate( nameIndex, 256, 32767, valid );
    }

    valid->subtable_length = p - table;
    GXV_EXIT;
  }


  static void
  gxv_trak_trackData_validate( FT_Bytes       table,
                               FT_Bytes       limit,
                               GXV_Validator  valid )
  {
    FT_Bytes   p = table;
    FT_UShort  nTracks;
    FT_UShort  nSizes;
    FT_ULong   sizeTableOffset;

    GXV_ODTECT( 4, odtect );


    GXV_ODTECT_INIT( odtect );
    GXV_NAME_ENTER( "trackData" );

    
    GXV_LIMIT_CHECK( 2 + 2 + 4 );
    nTracks         = FT_NEXT_USHORT( p );
    nSizes          = FT_NEXT_USHORT( p );
    sizeTableOffset = FT_NEXT_ULONG( p );

    gxv_odtect_add_range( table, p - table, "trackData header", odtect );

    
    gxv_trak_trackTable_validate( p, limit, nTracks, valid );
    gxv_odtect_add_range( p, valid->subtable_length,
                          "trackTable", odtect );

    
    p = valid->root->base + sizeTableOffset;
    GXV_LIMIT_CHECK( nSizes * 4 );
    gxv_odtect_add_range( p, nSizes * 4, "sizeTable", odtect );

    
    p = valid->root->base + GXV_TRAK_DATA( trackValueOffset_min );
    if ( limit - p < nTracks * nSizes * 2 )
      GXV_TRACE(( "too short trackValue array\n" ));

    p = valid->root->base + GXV_TRAK_DATA( trackValueOffset_max );
    GXV_LIMIT_CHECK( nSizes * 2 );

    gxv_odtect_add_range( valid->root->base
                            + GXV_TRAK_DATA( trackValueOffset_min ),
                          GXV_TRAK_DATA( trackValueOffset_max )
                            - GXV_TRAK_DATA( trackValueOffset_min )
                            + nSizes * 2,
                          "trackValue array", odtect );

    gxv_odtect_validate( odtect, valid );

    GXV_EXIT;
  }


  
  
  
  
  
  
  

  FT_LOCAL_DEF( void )
  gxv_trak_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  ftvalid )
  {
    FT_Bytes          p = table;
    FT_Bytes          limit = 0;
    FT_UInt           table_size;

    GXV_ValidatorRec  validrec;
    GXV_Validator     valid = &validrec;
    GXV_trak_DataRec  trakrec;
    GXV_trak_Data     trak = &trakrec;

    FT_ULong   version;
    FT_UShort  format;
    FT_UShort  horizOffset;
    FT_UShort  vertOffset;
    FT_UShort  reserved;


    GXV_ODTECT( 3, odtect );

    GXV_ODTECT_INIT( odtect );
    valid->root       = ftvalid;
    valid->table_data = trak;
    valid->face       = face;

    limit      = valid->root->limit;
    table_size = limit - table;

    FT_TRACE3(( "validating `trak' table\n" ));
    GXV_INIT;

    GXV_LIMIT_CHECK( 4 + 2 + 2 + 2 + 2 );
    version     = FT_NEXT_ULONG( p );
    format      = FT_NEXT_USHORT( p );
    horizOffset = FT_NEXT_USHORT( p );
    vertOffset  = FT_NEXT_USHORT( p );
    reserved    = FT_NEXT_USHORT( p );

    GXV_TRACE(( " (version = 0x%08x)\n", version ));
    GXV_TRACE(( " (format = 0x%04x)\n", format ));
    GXV_TRACE(( " (horizOffset = 0x%04x)\n", horizOffset ));
    GXV_TRACE(( " (vertOffset = 0x%04x)\n", vertOffset ));
    GXV_TRACE(( " (reserved = 0x%04x)\n", reserved ));

    
    if ( version != 0x00010000UL )
      FT_INVALID_FORMAT;

    
    if ( format != 0x0000 )
      FT_INVALID_FORMAT;

    GXV_32BIT_ALIGNMENT_VALIDATE( horizOffset );
    GXV_32BIT_ALIGNMENT_VALIDATE( vertOffset );

    
    if ( reserved != 0x0000 )
      FT_INVALID_DATA;

    
    if ( 0 < horizOffset )
    {
      gxv_trak_trackData_validate( table + horizOffset, limit, valid );
      gxv_odtect_add_range( table + horizOffset, valid->subtable_length,
                            "horizJustData", odtect );
    }

    if ( 0 < vertOffset )
    {
      gxv_trak_trackData_validate( table + vertOffset, limit, valid );
      gxv_odtect_add_range( table + vertOffset, valid->subtable_length,
                            "vertJustData", odtect );
    }

    gxv_odtect_validate( odtect, valid );

    FT_TRACE4(( "\n" ));
  }



