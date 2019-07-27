

























#include "gxvalid.h"
#include "gxvcommn.h"

#include FT_SFNT_NAMES_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvjust

  












  typedef struct  GXV_just_DataRec_
  {
    FT_UShort  wdc_offset_max;
    FT_UShort  wdc_offset_min;
    FT_UShort  pc_offset_max;
    FT_UShort  pc_offset_min;

  } GXV_just_DataRec, *GXV_just_Data;


#define  GXV_JUST_DATA( a )  GXV_TABLE_DATA( just, a )


  
  static void
  gxv_just_check_max_gid( FT_UShort         gid,
                          const FT_String*  msg_tag,
                          GXV_Validator     gxvalid )
  {
    if ( gid < gxvalid->face->num_glyphs )
      return;

    GXV_TRACE(( "just table includes too large %s"
                " GID=%d > %d (in maxp)\n",
                msg_tag, gid, gxvalid->face->num_glyphs ));
    GXV_SET_ERR_IF_PARANOID( FT_INVALID_GLYPH_ID );
  }


  static void
  gxv_just_wdp_entry_validate( FT_Bytes       table,
                               FT_Bytes       limit,
                               GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;
    FT_ULong   justClass;
#ifdef GXV_LOAD_UNUSED_VARS
    FT_Fixed   beforeGrowLimit;
    FT_Fixed   beforeShrinkGrowLimit;
    FT_Fixed   afterGrowLimit;
    FT_Fixed   afterShrinkGrowLimit;
    FT_UShort  growFlags;
    FT_UShort  shrinkFlags;
#endif


    GXV_LIMIT_CHECK( 4 + 4 + 4 + 4 + 4 + 2 + 2 );
    justClass             = FT_NEXT_ULONG( p );
#ifndef GXV_LOAD_UNUSED_VARS
    p += 4 + 4 + 4 + 4 + 2 + 2;
#else
    beforeGrowLimit       = FT_NEXT_ULONG( p );
    beforeShrinkGrowLimit = FT_NEXT_ULONG( p );
    afterGrowLimit        = FT_NEXT_ULONG( p );
    afterShrinkGrowLimit  = FT_NEXT_ULONG( p );
    growFlags             = FT_NEXT_USHORT( p );
    shrinkFlags           = FT_NEXT_USHORT( p );
#endif

    
    if ( ( justClass & 0xFFFFFF80UL ) != 0 )
    {
      GXV_TRACE(( "just table includes non-zero value"
                  " in unused justClass higher bits"
                  " of WidthDeltaPair" ));
      GXV_SET_ERR_IF_PARANOID( FT_INVALID_DATA );
    }

    gxvalid->subtable_length = p - table;
  }


  static void
  gxv_just_wdc_entry_validate( FT_Bytes       table,
                               FT_Bytes       limit,
                               GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;
    FT_ULong  count, i;


    GXV_LIMIT_CHECK( 4 );
    count = FT_NEXT_ULONG( p );
    for ( i = 0; i < count; i++ )
    {
      GXV_TRACE(( "validating wdc pair %d/%d\n", i + 1, count ));
      gxv_just_wdp_entry_validate( p, limit, gxvalid );
      p += gxvalid->subtable_length;
    }

    gxvalid->subtable_length = p - table;
  }


  static void
  gxv_just_widthDeltaClusters_validate( FT_Bytes       table,
                                        FT_Bytes       limit,
                                        GXV_Validator  gxvalid )
  {
    FT_Bytes  p         = table ;
    FT_Bytes  wdc_end   = table + GXV_JUST_DATA( wdc_offset_max );
    FT_UInt   i;


    GXV_NAME_ENTER( "just justDeltaClusters" );

    if ( limit <= wdc_end )
      FT_INVALID_OFFSET;

    for ( i = 0; p <= wdc_end; i++ )
    {
      gxv_just_wdc_entry_validate( p, limit, gxvalid );
      p += gxvalid->subtable_length;
    }

    gxvalid->subtable_length = p - table;

    GXV_EXIT;
  }


  static void
  gxv_just_actSubrecord_type0_validate( FT_Bytes       table,
                                        FT_Bytes       limit,
                                        GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;

    FT_Fixed   lowerLimit;
    FT_Fixed   upperLimit;
#ifdef GXV_LOAD_UNUSED_VARS
    FT_UShort  order;
#endif
    FT_UShort  decomposedCount;

    FT_UInt    i;


    GXV_LIMIT_CHECK( 4 + 4 + 2 + 2 );
    lowerLimit      = FT_NEXT_ULONG( p );
    upperLimit      = FT_NEXT_ULONG( p );
#ifdef GXV_LOAD_UNUSED_VARS
    order           = FT_NEXT_USHORT( p );
#else
    p += 2;
#endif
    decomposedCount = FT_NEXT_USHORT( p );

    if ( lowerLimit >= upperLimit )
    {
      GXV_TRACE(( "just table includes invalid range spec:"
                  " lowerLimit(%d) > upperLimit(%d)\n"     ));
      GXV_SET_ERR_IF_PARANOID( FT_INVALID_DATA );
    }

    for ( i = 0; i < decomposedCount; i++ )
    {
      FT_UShort glyphs;


      GXV_LIMIT_CHECK( 2 );
      glyphs = FT_NEXT_USHORT( p );
      gxv_just_check_max_gid( glyphs, "type0:glyphs", gxvalid );
    }

    gxvalid->subtable_length = p - table;
  }


  static void
  gxv_just_actSubrecord_type1_validate( FT_Bytes       table,
                                        FT_Bytes       limit,
                                        GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;
    FT_UShort  addGlyph;


    GXV_LIMIT_CHECK( 2 );
    addGlyph = FT_NEXT_USHORT( p );

    gxv_just_check_max_gid( addGlyph, "type1:addGlyph", gxvalid );

    gxvalid->subtable_length = p - table;
  }


  static void
  gxv_just_actSubrecord_type2_validate( FT_Bytes       table,
                                        FT_Bytes       limit,
                                        GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;
#ifdef GXV_LOAD_UNUSED_VARS
    FT_Fixed      substThreshhold; 
#endif
    FT_UShort  addGlyph;
    FT_UShort  substGlyph;


    GXV_LIMIT_CHECK( 4 + 2 + 2 );
#ifdef GXV_LOAD_UNUSED_VARS
    substThreshhold = FT_NEXT_ULONG( p );
#else
    p += 4;
#endif
    addGlyph        = FT_NEXT_USHORT( p );
    substGlyph      = FT_NEXT_USHORT( p );

    if ( addGlyph != 0xFFFF )
      gxv_just_check_max_gid( addGlyph, "type2:addGlyph", gxvalid );

    gxv_just_check_max_gid( substGlyph, "type2:substGlyph", gxvalid );

    gxvalid->subtable_length = p - table;
  }


  static void
  gxv_just_actSubrecord_type4_validate( FT_Bytes       table,
                                        FT_Bytes       limit,
                                        GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;
    FT_ULong  variantsAxis;
    FT_Fixed  minimumLimit;
    FT_Fixed  noStretchValue;
    FT_Fixed  maximumLimit;


    GXV_LIMIT_CHECK( 4 + 4 + 4 + 4 );
    variantsAxis   = FT_NEXT_ULONG( p );
    minimumLimit   = FT_NEXT_ULONG( p );
    noStretchValue = FT_NEXT_ULONG( p );
    maximumLimit   = FT_NEXT_ULONG( p );

    gxvalid->subtable_length = p - table;

    if ( variantsAxis != 0x64756374L ) 
      GXV_TRACE(( "variantsAxis 0x%08x is non default value",
                   variantsAxis ));

    if ( minimumLimit > noStretchValue )
      GXV_TRACE(( "type4:minimumLimit 0x%08x > noStretchValue 0x%08x\n",
                  minimumLimit, noStretchValue ));
    else if ( noStretchValue > maximumLimit )
      GXV_TRACE(( "type4:noStretchValue 0x%08x > maximumLimit 0x%08x\n",
                  noStretchValue, maximumLimit ));
    else if ( !IS_PARANOID_VALIDATION )
      return;

    FT_INVALID_DATA;
  }


  static void
  gxv_just_actSubrecord_type5_validate( FT_Bytes       table,
                                        FT_Bytes       limit,
                                        GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;
    FT_UShort  flags;
    FT_UShort  glyph;


    GXV_LIMIT_CHECK( 2 + 2 );
    flags = FT_NEXT_USHORT( p );
    glyph = FT_NEXT_USHORT( p );

    if ( flags )
      GXV_TRACE(( "type5: nonzero value 0x%04x in unused flags\n",
                   flags ));
    gxv_just_check_max_gid( glyph, "type5:glyph", gxvalid );

    gxvalid->subtable_length = p - table;
  }


  
  static void
  gxv_just_actSubrecord_validate( FT_Bytes       table,
                                  FT_Bytes       limit,
                                  GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;
    FT_UShort  actionClass;
    FT_UShort  actionType;
    FT_ULong   actionLength;


    GXV_NAME_ENTER( "just actSubrecord" );

    GXV_LIMIT_CHECK( 2 + 2 + 4 );
    actionClass  = FT_NEXT_USHORT( p );
    actionType   = FT_NEXT_USHORT( p );
    actionLength = FT_NEXT_ULONG( p );

    
    if ( ( actionClass & 0xFF80 ) != 0 )
      GXV_SET_ERR_IF_PARANOID( FT_INVALID_DATA );

    if ( actionType == 0 )
      gxv_just_actSubrecord_type0_validate( p, limit, gxvalid );
    else if ( actionType == 1 )
      gxv_just_actSubrecord_type1_validate( p, limit, gxvalid );
    else if ( actionType == 2 )
      gxv_just_actSubrecord_type2_validate( p, limit, gxvalid );
    else if ( actionType == 3 )
      ;                         
    else if ( actionType == 4 )
      gxv_just_actSubrecord_type4_validate( p, limit, gxvalid );
    else if ( actionType == 5 )
      gxv_just_actSubrecord_type5_validate( p, limit, gxvalid );
    else
      FT_INVALID_DATA;

    gxvalid->subtable_length = actionLength;

    GXV_EXIT;
  }


  static void
  gxv_just_pcActionRecord_validate( FT_Bytes       table,
                                    FT_Bytes       limit,
                                    GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;
    FT_ULong  actionCount;
    FT_ULong  i;


    GXV_LIMIT_CHECK( 4 );
    actionCount = FT_NEXT_ULONG( p );
    GXV_TRACE(( "actionCount = %d\n", actionCount ));

    for ( i = 0; i < actionCount; i++ )
    {
      gxv_just_actSubrecord_validate( p, limit, gxvalid );
      p += gxvalid->subtable_length;
    }

    gxvalid->subtable_length = p - table;

    GXV_EXIT;
  }


  static void
  gxv_just_pcTable_LookupValue_entry_validate( FT_UShort            glyph,
                                               GXV_LookupValueCPtr  value_p,
                                               GXV_Validator        gxvalid )
  {
    FT_UNUSED( glyph );

    if ( value_p->u > GXV_JUST_DATA( pc_offset_max ) )
      GXV_JUST_DATA( pc_offset_max ) = value_p->u;
    if ( value_p->u < GXV_JUST_DATA( pc_offset_max ) )
      GXV_JUST_DATA( pc_offset_min ) = value_p->u;
  }


  static void
  gxv_just_pcLookupTable_validate( FT_Bytes       table,
                                   FT_Bytes       limit,
                                   GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;


    GXV_NAME_ENTER( "just pcLookupTable" );
    GXV_JUST_DATA( pc_offset_max ) = 0x0000;
    GXV_JUST_DATA( pc_offset_min ) = 0xFFFFU;

    gxvalid->lookupval_sign = GXV_LOOKUPVALUE_UNSIGNED;
    gxvalid->lookupval_func = gxv_just_pcTable_LookupValue_entry_validate;

    gxv_LookupTable_validate( p, limit, gxvalid );

    

    GXV_EXIT;
  }


  static void
  gxv_just_postcompTable_validate( FT_Bytes       table,
                                   FT_Bytes       limit,
                                   GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;


    GXV_NAME_ENTER( "just postcompTable" );

    gxv_just_pcLookupTable_validate( p, limit, gxvalid );
    p += gxvalid->subtable_length;

    gxv_just_pcActionRecord_validate( p, limit, gxvalid );
    p += gxvalid->subtable_length;

    gxvalid->subtable_length = p - table;

    GXV_EXIT;
  }


  static void
  gxv_just_classTable_entry_validate(
    FT_Byte                         state,
    FT_UShort                       flags,
    GXV_StateTable_GlyphOffsetCPtr  glyphOffset_p,
    FT_Bytes                        table,
    FT_Bytes                        limit,
    GXV_Validator                   gxvalid )
  {
#ifdef GXV_LOAD_UNUSED_VARS
    
    FT_UShort  setMark;
    FT_UShort  dontAdvance;
    FT_UShort  markClass;
    FT_UShort  currentClass;
#endif

    FT_UNUSED( state );
    FT_UNUSED( glyphOffset_p );
    FT_UNUSED( table );
    FT_UNUSED( limit );
    FT_UNUSED( gxvalid );

#ifndef GXV_LOAD_UNUSED_VARS
    FT_UNUSED( flags );
#else
    setMark      = (FT_UShort)( ( flags >> 15 ) & 1    );
    dontAdvance  = (FT_UShort)( ( flags >> 14 ) & 1    );
    markClass    = (FT_UShort)( ( flags >> 7  ) & 0x7F );
    currentClass = (FT_UShort)(   flags         & 0x7F );
#endif
  }


  static void
  gxv_just_justClassTable_validate ( FT_Bytes       table,
                                     FT_Bytes       limit,
                                     GXV_Validator  gxvalid )
  {
    FT_Bytes   p = table;
    FT_UShort  length;
    FT_UShort  coverage;
    FT_ULong   subFeatureFlags;


    GXV_NAME_ENTER( "just justClassTable" );

    GXV_LIMIT_CHECK( 2 + 2 + 4 );
    length          = FT_NEXT_USHORT( p );
    coverage        = FT_NEXT_USHORT( p );
    subFeatureFlags = FT_NEXT_ULONG( p );

    GXV_TRACE(( "  justClassTable: coverage = 0x%04x (%s) ", coverage ));
    if ( ( coverage & 0x4000 ) == 0  )
      GXV_TRACE(( "ascending\n" ));
    else
      GXV_TRACE(( "descending\n" ));

    if ( subFeatureFlags )
      GXV_TRACE(( "  justClassTable: nonzero value (0x%08x)"
                  " in unused subFeatureFlags\n", subFeatureFlags ));

    gxvalid->statetable.optdata               = NULL;
    gxvalid->statetable.optdata_load_func     = NULL;
    gxvalid->statetable.subtable_setup_func   = NULL;
    gxvalid->statetable.entry_glyphoffset_fmt = GXV_GLYPHOFFSET_NONE;
    gxvalid->statetable.entry_validate_func   =
      gxv_just_classTable_entry_validate;

    gxv_StateTable_validate( p, table + length, gxvalid );

    

    GXV_EXIT;
  }


  static void
  gxv_just_wdcTable_LookupValue_validate( FT_UShort            glyph,
                                          GXV_LookupValueCPtr  value_p,
                                          GXV_Validator        gxvalid )
  {
    FT_UNUSED( glyph );

    if ( value_p->u > GXV_JUST_DATA( wdc_offset_max ) )
      GXV_JUST_DATA( wdc_offset_max ) = value_p->u;
    if ( value_p->u < GXV_JUST_DATA( wdc_offset_min ) )
      GXV_JUST_DATA( wdc_offset_min ) = value_p->u;
  }


  static void
  gxv_just_justData_lookuptable_validate( FT_Bytes       table,
                                          FT_Bytes       limit,
                                          GXV_Validator  gxvalid )
  {
    FT_Bytes  p = table;


    GXV_JUST_DATA( wdc_offset_max ) = 0x0000;
    GXV_JUST_DATA( wdc_offset_min ) = 0xFFFFU;

    gxvalid->lookupval_sign = GXV_LOOKUPVALUE_UNSIGNED;
    gxvalid->lookupval_func = gxv_just_wdcTable_LookupValue_validate;

    gxv_LookupTable_validate( p, limit, gxvalid );

    

    GXV_EXIT;
  }


  


  static void
  gxv_just_justData_validate( FT_Bytes       table,
                              FT_Bytes       limit,
                              GXV_Validator  gxvalid )
  {
    



    FT_UShort  justClassTableOffset;
    FT_UShort  wdcTableOffset;
    FT_UShort  pcTableOffset;
    FT_Bytes   p = table;

    GXV_ODTECT( 4, odtect );


    GXV_NAME_ENTER( "just justData" );

    GXV_ODTECT_INIT( odtect );
    GXV_LIMIT_CHECK( 2 + 2 + 2 );
    justClassTableOffset = FT_NEXT_USHORT( p );
    wdcTableOffset       = FT_NEXT_USHORT( p );
    pcTableOffset        = FT_NEXT_USHORT( p );

    GXV_TRACE(( " (justClassTableOffset = 0x%04x)\n", justClassTableOffset ));
    GXV_TRACE(( " (wdcTableOffset = 0x%04x)\n", wdcTableOffset ));
    GXV_TRACE(( " (pcTableOffset = 0x%04x)\n", pcTableOffset ));

    gxv_just_justData_lookuptable_validate( p, limit, gxvalid );
    gxv_odtect_add_range( p, gxvalid->subtable_length,
                          "just_LookupTable", odtect );

    if ( wdcTableOffset )
    {
      gxv_just_widthDeltaClusters_validate(
        gxvalid->root->base + wdcTableOffset, limit, gxvalid );
      gxv_odtect_add_range( gxvalid->root->base + wdcTableOffset,
                            gxvalid->subtable_length, "just_wdcTable", odtect );
    }

    if ( pcTableOffset )
    {
      gxv_just_postcompTable_validate( gxvalid->root->base + pcTableOffset,
                                       limit, gxvalid );
      gxv_odtect_add_range( gxvalid->root->base + pcTableOffset,
                            gxvalid->subtable_length, "just_pcTable", odtect );
    }

    if ( justClassTableOffset )
    {
      gxv_just_justClassTable_validate(
        gxvalid->root->base + justClassTableOffset, limit, gxvalid );
      gxv_odtect_add_range( gxvalid->root->base + justClassTableOffset,
                            gxvalid->subtable_length, "just_justClassTable",
                            odtect );
    }

    gxv_odtect_validate( odtect, gxvalid );

    GXV_EXIT;
  }


  FT_LOCAL_DEF( void )
  gxv_just_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  ftvalid )
  {
    FT_Bytes           p     = table;
    FT_Bytes           limit = 0;

    GXV_ValidatorRec   gxvalidrec;
    GXV_Validator      gxvalid = &gxvalidrec;
    GXV_just_DataRec   justrec;
    GXV_just_Data      just = &justrec;

    FT_ULong           version;
    FT_UShort          format;
    FT_UShort          horizOffset;
    FT_UShort          vertOffset;

    GXV_ODTECT( 3, odtect );


    GXV_ODTECT_INIT( odtect );

    gxvalid->root       = ftvalid;
    gxvalid->table_data = just;
    gxvalid->face       = face;

    FT_TRACE3(( "validating `just' table\n" ));
    GXV_INIT;

    limit      = gxvalid->root->limit;

    GXV_LIMIT_CHECK( 4 + 2 + 2 + 2 );
    version     = FT_NEXT_ULONG( p );
    format      = FT_NEXT_USHORT( p );
    horizOffset = FT_NEXT_USHORT( p );
    vertOffset  = FT_NEXT_USHORT( p );
    gxv_odtect_add_range( table, p - table, "just header", odtect );


    
    GXV_TRACE(( " (version = 0x%08x)\n", version ));
    if ( version != 0x00010000UL )
      FT_INVALID_FORMAT;

    
    GXV_TRACE(( " (format = 0x%04x)\n", format ));
    if ( format != 0x0000 )
        FT_INVALID_FORMAT;

    GXV_TRACE(( " (horizOffset = %d)\n", horizOffset  ));
    GXV_TRACE(( " (vertOffset = %d)\n", vertOffset  ));


    
    if ( 0 < horizOffset )
    {
      gxv_just_justData_validate( table + horizOffset, limit, gxvalid );
      gxv_odtect_add_range( table + horizOffset, gxvalid->subtable_length,
                            "horizJustData", odtect );
    }

    if ( 0 < vertOffset )
    {
      gxv_just_justData_validate( table + vertOffset, limit, gxvalid );
      gxv_odtect_add_range( table + vertOffset, gxvalid->subtable_length,
                            "vertJustData", odtect );
    }

    gxv_odtect_validate( odtect, gxvalid );

    FT_TRACE4(( "\n" ));
  }



