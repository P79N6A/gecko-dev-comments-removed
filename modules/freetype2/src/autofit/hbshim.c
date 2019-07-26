

















#include <ft2build.h>
#include FT_FREETYPE_H
#include "afglobal.h"
#include "aftypes.h"
#include "hbshim.h"

#ifdef FT_CONFIG_OPTION_USE_HARFBUZZ


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_afharfbuzz


  



















  
#undef  COVERAGE
#define COVERAGE( name, NAME, description,             \
                  tag1, tag2, tag3, tag4 )             \
          static const hb_tag_t  name ## _coverage[] = \
          {                                            \
            HB_TAG( tag1, tag2, tag3, tag4 ),          \
            HB_TAG_NONE                                \
          };


#include "afcover.h"


  
#undef  COVERAGE
#define COVERAGE( name, NAME, description, \
                  tag1, tag2, tag3, tag4 ) \
          name ## _coverage,


  static const hb_tag_t*  coverages[] =
  {
#include "afcover.h"

    NULL 
  };


  
#undef  SCRIPT
#define SCRIPT( s, S, d, h, sc1, sc2, sc3 )  h,


  static const hb_script_t  scripts[] =
  {
#include "afscript.h"
  };


  FT_Error
  af_get_coverage( AF_FaceGlobals  globals,
                   AF_StyleClass   style_class,
                   FT_Byte*        gstyles )
  {
    hb_face_t*  face;

    hb_set_t*  gsub_lookups;  
    hb_set_t*  gsub_glyphs;   
    hb_set_t*  gpos_lookups;  
    hb_set_t*  gpos_glyphs;   

    hb_script_t      script;
    const hb_tag_t*  coverage_tags;
    hb_tag_t         script_tags[] = { HB_TAG_NONE,
                                       HB_TAG_NONE,
                                       HB_TAG_NONE,
                                       HB_TAG_NONE };

    hb_codepoint_t  idx;
#ifdef FT_DEBUG_LEVEL_TRACE
    int             count;
#endif


    if ( !globals || !style_class || !gstyles )
      return FT_THROW( Invalid_Argument );

    face = hb_font_get_face( globals->hb_font );

    gsub_lookups = hb_set_create();
    gsub_glyphs  = hb_set_create();
    gpos_lookups = hb_set_create();
    gpos_glyphs  = hb_set_create();

    coverage_tags = coverages[style_class->coverage];
    script        = scripts[style_class->script];

    
    
    
    hb_ot_tags_from_script( script,
                            &script_tags[0],
                            &script_tags[1] );

    
    
    
    if ( style_class->script == globals->module->default_script &&
         style_class->coverage == AF_COVERAGE_DEFAULT           )
    {
      if ( script_tags[0] == HB_TAG_NONE )
        script_tags[0] = HB_OT_TAG_DEFAULT_SCRIPT;
      else
      {
        if ( script_tags[1] == HB_TAG_NONE )
          script_tags[1] = HB_OT_TAG_DEFAULT_SCRIPT;
        else if ( script_tags[1] != HB_OT_TAG_DEFAULT_SCRIPT )
          script_tags[2] = HB_OT_TAG_DEFAULT_SCRIPT;
      }
    }
    else
    {
      if ( script_tags[1] == HB_OT_TAG_DEFAULT_SCRIPT )
        script_tags[1] = HB_TAG_NONE;
    }

    hb_ot_layout_collect_lookups( face,
                                  HB_OT_TAG_GSUB,
                                  script_tags,
                                  NULL,
                                  coverage_tags,
                                  gsub_lookups );

    if ( hb_set_is_empty( gsub_lookups ) )
      goto Exit; 

    hb_ot_layout_collect_lookups( face,
                                  HB_OT_TAG_GPOS,
                                  script_tags,
                                  NULL,
                                  coverage_tags,
                                  gpos_lookups );

    FT_TRACE4(( "GSUB lookups (style `%s'):\n"
                " ",
                af_style_names[style_class->style] ));

#ifdef FT_DEBUG_LEVEL_TRACE
    count = 0;
#endif

    for ( idx = -1; hb_set_next( gsub_lookups, &idx ); )
    {
#ifdef FT_DEBUG_LEVEL_TRACE
      FT_TRACE4(( " %d", idx ));
      count++;
#endif

      
      hb_ot_layout_lookup_collect_glyphs( face,
                                          HB_OT_TAG_GSUB,
                                          idx,
                                          NULL,
                                          NULL,
                                          NULL,
                                          gsub_glyphs );
    }

#ifdef FT_DEBUG_LEVEL_TRACE
    if ( !count )
      FT_TRACE4(( " (none)" ));
    FT_TRACE4(( "\n\n" ));
#endif

    FT_TRACE4(( "GPOS lookups (style `%s'):\n"
                " ",
                af_style_names[style_class->style] ));

#ifdef FT_DEBUG_LEVEL_TRACE
    count = 0;
#endif

    for ( idx = -1; hb_set_next( gpos_lookups, &idx ); )
    {
#ifdef FT_DEBUG_LEVEL_TRACE
      FT_TRACE4(( " %d", idx ));
      count++;
#endif

      
      hb_ot_layout_lookup_collect_glyphs( face,
                                          HB_OT_TAG_GPOS,
                                          idx,
                                          NULL,
                                          gpos_glyphs,
                                          NULL,
                                          NULL );
    }

#ifdef FT_DEBUG_LEVEL_TRACE
    if ( !count )
      FT_TRACE4(( " (none)" ));
    FT_TRACE4(( "\n\n" ));
#endif

    





    {
      AF_Blue_Stringset         bss = style_class->blue_stringset;
      const AF_Blue_StringRec*  bs  = &af_blue_stringsets[bss];

      FT_Bool  found = 0;


      for ( ; bs->string != AF_BLUE_STRING_MAX; bs++ )
      {
        const char*  p = &af_blue_strings[bs->string];


        while ( *p )
        {
          hb_codepoint_t  ch;


          GET_UTF8_CHAR( ch, p );

          for ( idx = -1; hb_set_next( gsub_lookups, &idx ); )
          {
            hb_codepoint_t  gidx = FT_Get_Char_Index( globals->face, ch );


            if ( hb_ot_layout_lookup_would_substitute( face, idx,
                                                       &gidx, 1, 1 ) )
            {
              found = 1;
              break;
            }
          }
        }
      }

      if ( !found )
      {
        FT_TRACE4(( "  no blue characters found; style skipped\n" ));
        goto Exit;
      }
    }

    








































    hb_set_subtract( gsub_glyphs, gpos_glyphs );

#ifdef FT_DEBUG_LEVEL_TRACE
    FT_TRACE4(( "  glyphs without GPOS data (`*' means already assigned)" ));
    count = 0;
#endif

    for ( idx = -1; hb_set_next( gsub_glyphs, &idx ); )
    {
#ifdef FT_DEBUG_LEVEL_TRACE
      if ( !( count % 10 ) )
        FT_TRACE4(( "\n"
                    "   " ));

      FT_TRACE4(( " %d", idx ));
      count++;
#endif

      if ( gstyles[idx] == AF_STYLE_UNASSIGNED )
        gstyles[idx] = (FT_Byte)style_class->style;
#ifdef FT_DEBUG_LEVEL_TRACE
      else
        FT_TRACE4(( "*" ));
#endif
    }

#ifdef FT_DEBUG_LEVEL_TRACE
    if ( !count )
      FT_TRACE4(( "\n"
                  "    (none)" ));
    FT_TRACE4(( "\n\n" ));
#endif

  Exit:
    hb_set_destroy( gsub_lookups );
    hb_set_destroy( gsub_glyphs  );
    hb_set_destroy( gpos_lookups );
    hb_set_destroy( gpos_glyphs  );

    return FT_Err_Ok;
  }


  
#undef  COVERAGE
#define COVERAGE( name, NAME, description,                \
                  tag1, tag2, tag3, tag4 )                \
          static const hb_feature_t  name ## _feature[] = \
          {                                               \
            {                                             \
              HB_TAG( tag1, tag2, tag3, tag4 ),           \
              1, 0, (unsigned int)-1                      \
            }                                             \
          };


#include "afcover.h"


  
#undef  COVERAGE
#define COVERAGE( name, NAME, description, \
                  tag1, tag2, tag3, tag4 ) \
          name ## _feature,


  static const hb_feature_t*  features[] =
  {
#include "afcover.h"

    NULL 
  };


  FT_Error
  af_get_char_index( AF_StyleMetrics  metrics,
                     FT_ULong         charcode,
                     FT_ULong        *codepoint,
                     FT_Long         *y_offset )
  {
    AF_StyleClass  style_class;

    const hb_feature_t*  feature;

    FT_ULong  in_idx, out_idx;


    if ( !metrics )
      return FT_THROW( Invalid_Argument );

    in_idx = FT_Get_Char_Index( metrics->globals->face, charcode );

    style_class = metrics->style_class;

    feature = features[style_class->coverage];

    if ( feature )
    {
      FT_UInt  upem = metrics->globals->face->units_per_EM;

      hb_font_t*    font = metrics->globals->hb_font;
      hb_buffer_t*  buf  = hb_buffer_create();

      uint32_t  c = (uint32_t)charcode;

      hb_glyph_info_t*      ginfo;
      hb_glyph_position_t*  gpos;
      unsigned int          gcount;


      
      hb_font_set_scale( font, upem, upem );

      
      hb_buffer_set_direction( buf, HB_DIRECTION_LTR );
      hb_buffer_set_script( buf, scripts[style_class->script] );

      
      hb_buffer_add_utf32( buf, &c, 1, 0, 1 );

      
      hb_shape( font, buf, feature, 1 );

      ginfo = hb_buffer_get_glyph_infos( buf, &gcount );
      gpos  = hb_buffer_get_glyph_positions( buf, &gcount );

      out_idx = ginfo[0].codepoint;

      
      
      if ( in_idx == out_idx )
      {
        *codepoint = 0;
        *y_offset  = 0;
      }
      else
      {
        *codepoint = out_idx;
        *y_offset  = gpos[0].y_offset;
      }

      hb_buffer_destroy( buf );

#ifdef FT_DEBUG_LEVEL_TRACE
      if ( gcount > 1 )
        FT_TRACE1(( "af_get_char_index:"
                    " input character mapped to multiple glyphs\n" ));
#endif
    }
    else
    {
      *codepoint = in_idx;
      *y_offset  = 0;
    }

    return FT_Err_Ok;
  }


#else 


  FT_Error
  af_get_coverage( AF_FaceGlobals  globals,
                   AF_StyleClass   style_class,
                   FT_Byte*        gstyles )
  {
    FT_UNUSED( globals );
    FT_UNUSED( style_class );
    FT_UNUSED( gstyles );

    return FT_Err_Ok;
  }


  FT_Error
  af_get_char_index( AF_StyleMetrics  metrics,
                     FT_ULong         charcode,
                     FT_ULong        *codepoint,
                     FT_Long         *y_offset )
  {
    FT_Face  face;


    if ( !metrics )
      return FT_THROW( Invalid_Argument );

    face = metrics->globals->face;

    *codepoint = FT_Get_Char_Index( face, charcode );
    *y_offset  = 0;

    return FT_Err_Ok;
  }


#endif 



