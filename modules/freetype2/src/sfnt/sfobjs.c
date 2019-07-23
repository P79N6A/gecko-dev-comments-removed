

















#include <ft2build.h>
#include "sfobjs.h"
#include "ttload.h"
#include "ttcmap.h"
#include "ttkern.h"
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_DEBUG_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include "sferrors.h"

#ifdef TT_CONFIG_OPTION_BDF
#include "ttbdf.h"
#endif


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_sfobjs



  
  static FT_String*
  tt_name_entry_ascii_from_utf16( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength / 2;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = FT_NEXT_USHORT( read );
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  
  static FT_String*
  tt_name_entry_ascii_from_other( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string;
    FT_UInt     len, code, n;
    FT_Byte*    read = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = *read++;
      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[len] = 0;

    return string;
  }


  typedef FT_String*  (*TT_NameEntry_ConvertFunc)( TT_NameEntry  entry,
                                                   FT_Memory     memory );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_String*
  tt_face_get_name( TT_Face    face,
                    FT_UShort  nameid )
  {
    FT_Memory         memory = face->root.memory;
    FT_String*        result = NULL;
    FT_UShort         n;
    TT_NameEntryRec*  rec;
    FT_Int            found_apple         = -1;
    FT_Int            found_apple_roman   = -1;
    FT_Int            found_apple_english = -1;
    FT_Int            found_win           = -1;
    FT_Int            found_unicode       = -1;

    FT_Bool           is_english = 0;

    TT_NameEntry_ConvertFunc  convert;


    rec = face->name_table.names;
    for ( n = 0; n < face->num_names; n++, rec++ )
    {
      
      
      
      
      
      
      
      
      
      if ( rec->nameID == nameid && rec->stringLength > 0 )
      {
        switch ( rec->platformID )
        {
        case TT_PLATFORM_APPLE_UNICODE:
        case TT_PLATFORM_ISO:
          
          
          
          
          found_unicode = n;
          break;

        case TT_PLATFORM_MACINTOSH:
          
          
          
          
          if ( rec->languageID == TT_MAC_LANGID_ENGLISH )
            found_apple_english = n;
          else if ( rec->encodingID == TT_MAC_ID_ROMAN )
            found_apple_roman = n;
          break;

        case TT_PLATFORM_MICROSOFT:
          
          
          
          if ( found_win == -1 || ( rec->languageID & 0x3FF ) == 0x009 )
          {
            switch ( rec->encodingID )
            {
            case TT_MS_ID_SYMBOL_CS:
            case TT_MS_ID_UNICODE_CS:
            case TT_MS_ID_UCS_4:
              is_english = FT_BOOL( ( rec->languageID & 0x3FF ) == 0x009 );
              found_win  = n;
              break;

            default:
              ;
            }
          }
          break;

        default:
          ;
        }
      }
    }

    found_apple = found_apple_roman;
    if ( found_apple_english >= 0 )
      found_apple = found_apple_english;

    
    
    
    
    convert = NULL;
    if ( found_win >= 0 && !( found_apple >= 0 && !is_english ) )
    {
      rec = face->name_table.names + found_win;
      switch ( rec->encodingID )
      {
        
      case TT_MS_ID_UNICODE_CS:
      case TT_MS_ID_SYMBOL_CS:
        convert = tt_name_entry_ascii_from_utf16;
        break;

      case TT_MS_ID_UCS_4:
        
        
        
        
        
        convert = tt_name_entry_ascii_from_utf16;
        break;

      default:
        ;
      }
    }
    else if ( found_apple >= 0 )
    {
      rec     = face->name_table.names + found_apple;
      convert = tt_name_entry_ascii_from_other;
    }
    else if ( found_unicode >= 0 )
    {
      rec     = face->name_table.names + found_unicode;
      convert = tt_name_entry_ascii_from_utf16;
    }

    if ( rec && convert )
    {
      if ( rec->string == NULL )
      {
        FT_Error   error  = SFNT_Err_Ok;
        FT_Stream  stream = face->name_table.stream;

        FT_UNUSED( error );


        if ( FT_QNEW_ARRAY ( rec->string, rec->stringLength ) ||
             FT_STREAM_SEEK( rec->stringOffset )              ||
             FT_STREAM_READ( rec->string, rec->stringLength ) )
        {
          FT_FREE( rec->string );
          rec->stringLength = 0;
          result            = NULL;
          goto Exit;
        }
      }

      result = convert( rec, memory );
    }

  Exit:
    return result;
  }


  static FT_Encoding
  sfnt_find_encoding( int  platform_id,
                      int  encoding_id )
  {
    typedef struct  TEncoding_
    {
      int          platform_id;
      int          encoding_id;
      FT_Encoding  encoding;

    } TEncoding;

    static
    const TEncoding  tt_encodings[] =
    {
      { TT_PLATFORM_ISO,           -1,                  FT_ENCODING_UNICODE },

      { TT_PLATFORM_APPLE_UNICODE, -1,                  FT_ENCODING_UNICODE },

      { TT_PLATFORM_MACINTOSH,     TT_MAC_ID_ROMAN,     FT_ENCODING_APPLE_ROMAN },

      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SYMBOL_CS,  FT_ENCODING_MS_SYMBOL },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UCS_4,      FT_ENCODING_UNICODE },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UNICODE_CS, FT_ENCODING_UNICODE },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SJIS,       FT_ENCODING_SJIS },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_GB2312,     FT_ENCODING_GB2312 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_BIG_5,      FT_ENCODING_BIG5 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_WANSUNG,    FT_ENCODING_WANSUNG },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_JOHAB,      FT_ENCODING_JOHAB }
    };

    const TEncoding  *cur, *limit;


    cur   = tt_encodings;
    limit = cur + sizeof ( tt_encodings ) / sizeof ( tt_encodings[0] );

    for ( ; cur < limit; cur++ )
    {
      if ( cur->platform_id == platform_id )
      {
        if ( cur->encoding_id == encoding_id ||
             cur->encoding_id == -1          )
          return cur->encoding;
      }
    }

    return FT_ENCODING_NONE;
  }


  
  
  static FT_Error
  sfnt_open_font( FT_Stream  stream,
                  TT_Face    face )
  {
    FT_Memory  memory = stream->memory;
    FT_Error   error;
    FT_ULong   tag, offset;

    static const FT_Frame_Field  ttc_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TTC_HeaderRec

      FT_FRAME_START( 8 ),
        FT_FRAME_LONG( version ),
        FT_FRAME_LONG( count   ),
      FT_FRAME_END
    };


    face->ttc_header.tag     = 0;
    face->ttc_header.version = 0;
    face->ttc_header.count   = 0;

    offset = FT_STREAM_POS();

    if ( FT_READ_ULONG( tag ) )
      return error;

    if ( tag != 0x00010000UL                      &&
         tag != TTAG_ttcf                         &&
         tag != FT_MAKE_TAG( 'O', 'T', 'T', 'O' ) &&
         tag != TTAG_true                         &&
         tag != 0x00020000UL                      )
      return SFNT_Err_Unknown_File_Format;

    face->ttc_header.tag = TTAG_ttcf;

    if ( tag == TTAG_ttcf )
    {
      FT_Int  n;


      FT_TRACE3(( "sfnt_open_font: file is a collection\n" ));

      if ( FT_STREAM_READ_FIELDS( ttc_header_fields, &face->ttc_header ) )
        return error;

      
      if ( FT_NEW_ARRAY( face->ttc_header.offsets, face->ttc_header.count ) )
        return error;

      if ( FT_FRAME_ENTER( face->ttc_header.count * 4L ) )
        return error;

      for ( n = 0; n < face->ttc_header.count; n++ )
        face->ttc_header.offsets[n] = FT_GET_ULONG();

      FT_FRAME_EXIT();
    }
    else
    {
      FT_TRACE3(( "sfnt_open_font: synthesize TTC\n" ));

      face->ttc_header.version = 1 << 16;
      face->ttc_header.count   = 1;

      if ( FT_NEW( face->ttc_header.offsets) )
        return error;

      face->ttc_header.offsets[0] = offset;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  sfnt_init_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error        error;
    FT_Library      library = face->root.driver->root.library;
    SFNT_Service    sfnt;


    
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    sfnt = (SFNT_Service)face->sfnt;
    if ( !sfnt )
    {
      sfnt = (SFNT_Service)FT_Get_Module_Interface( library, "sfnt" );
      if ( !sfnt )
        return SFNT_Err_Invalid_File_Format;

      face->sfnt       = sfnt;
      face->goto_table = sfnt->goto_table;
    }

    FT_FACE_FIND_GLOBAL_SERVICE( face, face->psnames, POSTSCRIPT_CMAPS );

    error = sfnt_open_font( stream, face );
    if ( error )
      return error;

    FT_TRACE2(( "sfnt_init_face: %08p, %ld\n", face, face_index ));

    if ( face_index < 0 )
      face_index = 0;

    if ( face_index >= face->ttc_header.count )
        return SFNT_Err_Bad_Argument;

    if ( FT_STREAM_SEEK( face->ttc_header.offsets[face_index] ) )
      return error;

    
    error = sfnt->load_font_dir( face, stream );
    if ( error )
      return error;

    face->root.num_faces = face->ttc_header.count;

    return error;
  }


#define LOAD_( x )                                            \
  do {                                                        \
    FT_TRACE2(( "`" #x "' " ));                               \
    FT_TRACE3(( "-->\n" ));                                   \
                                                              \
    error = sfnt->load_##x( face, stream );                   \
                                                              \
    FT_TRACE2(( "%s\n", ( !error )                            \
                        ? "loaded"                            \
                        : ( error == SFNT_Err_Table_Missing ) \
                          ? "missing"                         \
                          : "failed to load" ));              \
    FT_TRACE3(( "\n" ));                                      \
  } while ( 0 )

#define LOADM_( x, vertical )                                 \
  do {                                                        \
    FT_TRACE2(( "`%s" #x "' ",                                \
                vertical ? "vertical " : "" ));               \
    FT_TRACE3(( "-->\n" ));                                   \
                                                              \
    error = sfnt->load_##x( face, stream, vertical );         \
                                                              \
    FT_TRACE2(( "%s\n", ( !error )                            \
                        ? "loaded"                            \
                        : ( error == SFNT_Err_Table_Missing ) \
                          ? "missing"                         \
                          : "failed to load" ));              \
    FT_TRACE3(( "\n" ));                                      \
  } while ( 0 )


  FT_LOCAL_DEF( FT_Error )
  sfnt_load_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error      error, psnames_error;
    FT_Bool       has_outline;
    FT_Bool       is_apple_sbit;

    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;

    FT_UNUSED( face_index );
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    FT_TRACE2(( "sfnt_load_face: %08p\n\n", face ));

    
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    has_outline   = FT_BOOL( face->root.internal->incremental_interface != 0 ||
                             tt_face_lookup_table( face, TTAG_glyf )    != 0 ||
                             tt_face_lookup_table( face, TTAG_CFF )     != 0 );
#else
    has_outline   = FT_BOOL( tt_face_lookup_table( face, TTAG_glyf ) != 0 ||
                             tt_face_lookup_table( face, TTAG_CFF )  != 0 );
#endif

    is_apple_sbit = 0;

    
    
    if ( !has_outline && sfnt->load_bhed )
    {
      LOAD_( bhed );
      is_apple_sbit = FT_BOOL( !error );
    }

    
    
    if ( !is_apple_sbit )
    {
      LOAD_( head );
      if ( error )
        goto Exit;
    }

    if ( face->header.Units_Per_EM == 0 )
    {
      error = SFNT_Err_Invalid_Table;

      goto Exit;
    }

    
    
    LOAD_( maxp );
    LOAD_( cmap );

    
    
    LOAD_( name );
    LOAD_( post );
    psnames_error = error;

    
    
    if ( !is_apple_sbit )
    {
      
      LOADM_( hhea, 0 );
      if ( !error )
      {
        LOADM_( hmtx, 0 );
        if ( error == SFNT_Err_Table_Missing )
        {
          error = SFNT_Err_Hmtx_Table_Missing;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
          
          
          if ( face->root.internal->incremental_interface          &&
               face->root.internal->incremental_interface->funcs->
                 get_glyph_metrics                                 )
          {
            face->horizontal.number_Of_HMetrics = 0;
            error = SFNT_Err_Ok;
          }
#endif
        }
      }
      else if ( error == SFNT_Err_Table_Missing )
      {
        
        if ( face->format_tag == TTAG_true )
        {
          FT_TRACE2(( "This is an SFNT Mac font.\n" ));
          has_outline = 0;
          error = SFNT_Err_Ok;
        }
        else
        {
          error = SFNT_Err_Horiz_Header_Missing;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
          
          
          if ( face->root.internal->incremental_interface          &&
               face->root.internal->incremental_interface->funcs->
                 get_glyph_metrics                                 )
          {
            face->horizontal.number_Of_HMetrics = 0;
            error = SFNT_Err_Ok;
          }
#endif

        }
      }

      if ( error )
        goto Exit;

      
      LOADM_( hhea, 1 );
      if ( !error )
      {
        LOADM_( hmtx, 1 );
        if ( !error )
          face->vertical_info = 1;
      }

      if ( error && error != SFNT_Err_Table_Missing )
        goto Exit;

      LOAD_( os2 );
      if ( error )
      {
        if ( error != SFNT_Err_Table_Missing )
          goto Exit;

        face->os2.version = 0xFFFFU;
      }

    }

    

    
    if ( sfnt->load_eblc )
    {
      LOAD_( eblc );
      if ( error )
      {
        
        if ( error == SFNT_Err_Table_Missing && has_outline )
          error = SFNT_Err_Ok;
        else
          goto Exit;
      }
    }

    LOAD_( pclt );
    if ( error )
    {
      if ( error != SFNT_Err_Table_Missing )
        goto Exit;

      face->pclt.Version = 0;
    }

    
    LOAD_( gasp );
    LOAD_( kern );

    error = SFNT_Err_Ok;

    face->root.num_glyphs = face->max_profile.numGlyphs;

#if 0
    
    
    
    
    

    if ( face->os2.version != 0xFFFFU && face->os2.fsSelection & 256 )
#endif
    {
      face->root.family_name =
        tt_face_get_name( face, TT_NAME_ID_PREFERRED_FAMILY );
      if ( !face->root.family_name )
        face->root.family_name =
          tt_face_get_name( face, TT_NAME_ID_FONT_FAMILY );

      face->root.style_name =
        tt_face_get_name( face, TT_NAME_ID_PREFERRED_SUBFAMILY );
      if ( !face->root.style_name )
        face->root.style_name =
          tt_face_get_name( face, TT_NAME_ID_FONT_SUBFAMILY );
    }
#if 0
    else
    {
      
      
      

      face->root.family_name =
        tt_face_get_name( face, TT_NAME_ID_WWS_FAMILY );
      if ( !face->root.family_name )
        face->root.family_name =
          tt_face_get_name( face, TT_NAME_ID_PREFERRED_FAMILY );
      if ( !face->root.family_name )
        face->root.family_name =
          tt_face_get_name( face, TT_NAME_ID_FONT_FAMILY );

      face->root.style_name =
        tt_face_get_name( face, TT_NAME_ID_WWS_SUBFAMILY );
      if ( !face->root.style_name )
        face->root.style_name =
          tt_face_get_name( face, TT_NAME_ID_PREFERRED_SUBFAMILY );
      if ( !face->root.style_name )
        face->root.style_name =
          tt_face_get_name( face, TT_NAME_ID_FONT_SUBFAMILY );
    }
#endif


    
    {
      FT_Face   root  = &face->root;
      FT_Int32  flags = root->face_flags;


      
      
      
      
      if ( has_outline == TRUE )
        flags |= FT_FACE_FLAG_SCALABLE;   

      
      
      flags |= FT_FACE_FLAG_SFNT       |  
               FT_FACE_FLAG_HORIZONTAL;   

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
      if ( psnames_error == SFNT_Err_Ok               &&
           face->postscript.FormatType != 0x00030000L )
        flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

      
      if ( face->postscript.isFixedPitch )
        flags |= FT_FACE_FLAG_FIXED_WIDTH;

      
      if ( face->vertical_info )
        flags |= FT_FACE_FLAG_VERTICAL;

      
      if ( TT_FACE_HAS_KERNING( face ) )
        flags |= FT_FACE_FLAG_KERNING;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
      
      
      if ( tt_face_lookup_table( face, TTAG_glyf ) != 0 &&
           tt_face_lookup_table( face, TTAG_fvar ) != 0 &&
           tt_face_lookup_table( face, TTAG_gvar ) != 0 )
        flags |= FT_FACE_FLAG_MULTIPLE_MASTERS;
#endif

      root->face_flags = flags;

      
      
      
      

      flags = 0;
      if ( has_outline == TRUE && face->os2.version != 0xFFFFU )
      {
        
        
        
        

        if ( face->os2.fsSelection & 512 )       
          flags |= FT_STYLE_FLAG_ITALIC;
        else if ( face->os2.fsSelection & 1 )    
          flags |= FT_STYLE_FLAG_ITALIC;

        if ( face->os2.fsSelection & 32 )        
          flags |= FT_STYLE_FLAG_BOLD;
      }
      else
      {
        
        if ( face->header.Mac_Style & 1 )
          flags |= FT_STYLE_FLAG_BOLD;

        if ( face->header.Mac_Style & 2 )
          flags |= FT_STYLE_FLAG_ITALIC;
      }

      root->style_flags = flags;

      
      
      
      
      
      
      

      tt_face_build_cmaps( face );  


      
      {
        FT_Int  m;


        for ( m = 0; m < root->num_charmaps; m++ )
        {
          FT_CharMap  charmap = root->charmaps[m];


          charmap->encoding = sfnt_find_encoding( charmap->platform_id,
                                                  charmap->encoding_id );

#if 0
          if ( root->charmap     == NULL &&
               charmap->encoding == FT_ENCODING_UNICODE )
          {
            
            root->charmap = charmap;
          }
#endif
        }
      }


      
      
      
      
      if ( has_outline == TRUE )
      {
        
        
        root->bbox.xMin    = face->header.xMin;
        root->bbox.yMin    = face->header.yMin;
        root->bbox.xMax    = face->header.xMax;
        root->bbox.yMax    = face->header.yMax;
        root->units_per_EM = face->header.Units_Per_EM;


        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        
        
        

        root->ascender  = face->horizontal.Ascender;
        root->descender = face->horizontal.Descender;

        root->height    = (FT_Short)( root->ascender - root->descender +
                                      face->horizontal.Line_Gap );

#if 0
        
        
        if ( face->horizontal.Line_Gap == 0 )
          root->height = (FT_Short)( ( root->height * 115 + 50 ) / 100 );
#endif

#if 0

        
        
        if ( face->os2.version != 0xFFFFU && root->ascender )
        {
          FT_Int  height;


          root->ascender  =  face->os2.sTypoAscender;
          root->descender = -face->os2.sTypoDescender;

          height = root->ascender + root->descender + face->os2.sTypoLineGap;
          if ( height > root->height )
            root->height = height;
        }

#endif 

        root->max_advance_width   = face->horizontal.advance_Width_Max;

        root->max_advance_height  = (FT_Short)( face->vertical_info
                                      ? face->vertical.advance_Height_Max
                                      : root->height );

        root->underline_position  = face->postscript.underlinePosition;
        root->underline_thickness = face->postscript.underlineThickness;
      }

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

      




      {
        FT_UInt  i, count;


#if !defined FT_CONFIG_OPTION_OLD_INTERNALS
        count = face->sbit_num_strikes;
#else
        count = (FT_UInt)face->num_sbit_strikes;
#endif

        if ( count > 0 )
        {
          FT_Memory        memory   = face->root.stream->memory;
          FT_UShort        em_size  = face->header.Units_Per_EM;
          FT_Short         avgwidth = face->os2.xAvgCharWidth;
          FT_Size_Metrics  metrics;


          if ( em_size == 0 || face->os2.version == 0xFFFFU )
          {
            avgwidth = 0;
            em_size = 1;
          }

          if ( FT_NEW_ARRAY( root->available_sizes, count ) )
            goto Exit;

          for ( i = 0; i < count; i++ )
          {
            FT_Bitmap_Size*  bsize = root->available_sizes + i;


            error = sfnt->load_strike_metrics( face, i, &metrics );
            if ( error )
              goto Exit;

            bsize->height = (FT_Short)( metrics.height >> 6 );
            bsize->width = (FT_Short)(
                ( avgwidth * metrics.x_ppem + em_size / 2 ) / em_size );

            bsize->x_ppem = metrics.x_ppem << 6;
            bsize->y_ppem = metrics.y_ppem << 6;

            
            bsize->size   = metrics.y_ppem << 6;
          }

          root->face_flags     |= FT_FACE_FLAG_FIXED_SIZES;
          root->num_fixed_sizes = (FT_Int)count;
        }
      }

#endif 

    }

  Exit:
    FT_TRACE2(( "sfnt_load_face: done\n" ));

    return error;
  }


#undef LOAD_
#undef LOADM_


  FT_LOCAL_DEF( void )
  sfnt_done_face( TT_Face  face )
  {
    FT_Memory     memory = face->root.memory;
    SFNT_Service  sfnt   = (SFNT_Service)face->sfnt;


    if ( sfnt )
    {
      
      if ( sfnt->free_psnames )
        sfnt->free_psnames( face );

      
      if ( sfnt->free_eblc )
        sfnt->free_eblc( face );
    }

#ifdef TT_CONFIG_OPTION_BDF
    
    tt_face_free_bdf_props( face );
#endif

    
    tt_face_done_kern( face );

    
    FT_FREE( face->ttc_header.offsets );
    face->ttc_header.count = 0;

    
    FT_FREE( face->dir_tables );
    face->num_tables = 0;

    {
      FT_Stream  stream = FT_FACE_STREAM( face );


      
      FT_FRAME_RELEASE( face->cmap_table );
      face->cmap_size = 0;
    }

    
#if !defined FT_CONFIG_OPTION_OLD_INTERNALS
    {
      FT_Stream  stream = FT_FACE_STREAM( face );


      FT_FRAME_RELEASE( face->horz_metrics );
      FT_FRAME_RELEASE( face->vert_metrics );
      face->horz_metrics_size = 0;
      face->vert_metrics_size = 0;
    }
#else
    FT_FREE( face->horizontal.long_metrics );
    FT_FREE( face->horizontal.short_metrics );
#endif

    
    if ( face->vertical_info )
    {
      FT_FREE( face->vertical.long_metrics  );
      FT_FREE( face->vertical.short_metrics );
      face->vertical_info = 0;
    }

    
    FT_FREE( face->gasp.gaspRanges );
    face->gasp.numRanges = 0;

    
    if ( sfnt )
      sfnt->free_name( face );

    
    FT_FREE( face->root.family_name );
    FT_FREE( face->root.style_name );

    
    FT_FREE( face->root.available_sizes );
    face->root.num_fixed_sizes = 0;

    FT_FREE( face->postscript_name );

    face->sfnt = 0;
  }



