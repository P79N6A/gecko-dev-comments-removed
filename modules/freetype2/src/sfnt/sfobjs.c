

















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
#include FT_SFNT_NAMES_H
#include FT_GZIP_H
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
    FT_String*  string = NULL;
    FT_UInt     len, code, n;
    FT_Byte*    read   = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength / 2;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = FT_NEXT_USHORT( read );

      if ( code == 0 )
        break;

      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[n] = 0;

    return string;
  }


  
  static FT_String*
  tt_name_entry_ascii_from_other( TT_NameEntry  entry,
                                  FT_Memory     memory )
  {
    FT_String*  string = NULL;
    FT_UInt     len, code, n;
    FT_Byte*    read   = (FT_Byte*)entry->string;
    FT_Error    error;


    len = (FT_UInt)entry->stringLength;

    if ( FT_NEW_ARRAY( string, len + 1 ) )
      return NULL;

    for ( n = 0; n < len; n++ )
    {
      code = *read++;

      if ( code == 0 )
        break;

      if ( code < 32 || code > 127 )
        code = '?';

      string[n] = (char)code;
    }

    string[n] = 0;

    return string;
  }


  typedef FT_String*  (*TT_NameEntry_ConvertFunc)( TT_NameEntry  entry,
                                                   FT_Memory     memory );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Error
  tt_face_get_name( TT_Face      face,
                    FT_UShort    nameid,
                    FT_String**  name )
  {
    FT_Memory         memory = face->root.memory;
    FT_Error          error  = FT_Err_Ok;
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


    FT_ASSERT( name );

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
        FT_Stream  stream = face->name_table.stream;


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
    *name = result;
    return error;
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


#define WRITE_USHORT( p, v )                \
          do                                \
          {                                 \
            *(p)++ = (FT_Byte)( (v) >> 8 ); \
            *(p)++ = (FT_Byte)( (v) >> 0 ); \
                                            \
          } while ( 0 )

#define WRITE_ULONG( p, v )                  \
          do                                 \
          {                                  \
            *(p)++ = (FT_Byte)( (v) >> 24 ); \
            *(p)++ = (FT_Byte)( (v) >> 16 ); \
            *(p)++ = (FT_Byte)( (v) >>  8 ); \
            *(p)++ = (FT_Byte)( (v) >>  0 ); \
                                             \
          } while ( 0 )


  static void
  sfnt_stream_close( FT_Stream  stream )
  {
    FT_Memory  memory = stream->memory;


    FT_FREE( stream->base );

    stream->size  = 0;
    stream->base  = 0;
    stream->close = 0;
  }


  FT_CALLBACK_DEF( int )
  compare_offsets( const void*  a,
                   const void*  b )
  {
    WOFF_Table  table1 = *(WOFF_Table*)a;
    WOFF_Table  table2 = *(WOFF_Table*)b;

    FT_ULong  offset1 = table1->Offset;
    FT_ULong  offset2 = table2->Offset;


    if ( offset1 > offset2 )
      return 1;
    else if ( offset1 < offset2 )
      return -1;
    else
      return 0;
  }


  
  

  static FT_Error
  woff_open_font( FT_Stream  stream,
                  TT_Face    face )
  {
    FT_Memory       memory = stream->memory;
    FT_Error        error  = FT_Err_Ok;

    WOFF_HeaderRec  woff;
    WOFF_Table      tables  = NULL;
    WOFF_Table*     indices = NULL;

    FT_ULong        woff_offset;

    FT_Byte*        sfnt        = NULL;
    FT_Stream       sfnt_stream = NULL;

    FT_Byte*        sfnt_header;
    FT_ULong        sfnt_offset;

    FT_Int          nn;
    FT_ULong        old_tag = 0;

    static const FT_Frame_Field  woff_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  WOFF_HeaderRec

      FT_FRAME_START( 44 ),
        FT_FRAME_ULONG ( signature ),
        FT_FRAME_ULONG ( flavor ),
        FT_FRAME_ULONG ( length ),
        FT_FRAME_USHORT( num_tables ),
        FT_FRAME_USHORT( reserved ),
        FT_FRAME_ULONG ( totalSfntSize ),
        FT_FRAME_USHORT( majorVersion ),
        FT_FRAME_USHORT( minorVersion ),
        FT_FRAME_ULONG ( metaOffset ),
        FT_FRAME_ULONG ( metaLength ),
        FT_FRAME_ULONG ( metaOrigLength ),
        FT_FRAME_ULONG ( privOffset ),
        FT_FRAME_ULONG ( privLength ),
      FT_FRAME_END
    };


    FT_ASSERT( stream == face->root.stream );
    FT_ASSERT( FT_STREAM_POS() == 0 );

    if ( FT_STREAM_READ_FIELDS( woff_header_fields, &woff ) )
      return error;

    
    if ( woff.flavor == TTAG_wOFF || woff.flavor == TTAG_ttcf )
      return FT_THROW( Invalid_Table );

    
    if ( woff.length != stream->size                              ||
         woff.num_tables == 0                                     ||
         44 + woff.num_tables * 20UL >= woff.length               ||
         12 + woff.num_tables * 16UL >= woff.totalSfntSize        ||
         ( woff.totalSfntSize & 3 ) != 0                          ||
         ( woff.metaOffset == 0 && ( woff.metaLength != 0     ||
                                     woff.metaOrigLength != 0 ) ) ||
         ( woff.metaLength != 0 && woff.metaOrigLength == 0 )     ||
         ( woff.privOffset == 0 && woff.privLength != 0 )         )
      return FT_THROW( Invalid_Table );

    if ( FT_ALLOC( sfnt, woff.totalSfntSize ) ||
         FT_NEW( sfnt_stream )                )
      goto Exit;

    sfnt_header = sfnt;

    
    {
      FT_UInt  searchRange, entrySelector, rangeShift, x;


      x             = woff.num_tables;
      entrySelector = 0;
      while ( x )
      {
        x            >>= 1;
        entrySelector += 1;
      }
      entrySelector--;

      searchRange = ( 1 << entrySelector ) * 16;
      rangeShift  = woff.num_tables * 16 - searchRange;

      WRITE_ULONG ( sfnt_header, woff.flavor );
      WRITE_USHORT( sfnt_header, woff.num_tables );
      WRITE_USHORT( sfnt_header, searchRange );
      WRITE_USHORT( sfnt_header, entrySelector );
      WRITE_USHORT( sfnt_header, rangeShift );
    }

    
    
    

    if ( FT_NEW_ARRAY( tables, woff.num_tables )  ||
         FT_NEW_ARRAY( indices, woff.num_tables ) )
      goto Exit;

    FT_TRACE2(( "\n"
                "  tag    offset    compLen  origLen  checksum\n"
                "  -------------------------------------------\n" ));

    if ( FT_FRAME_ENTER( 20L * woff.num_tables ) )
      goto Exit;

    for ( nn = 0; nn < woff.num_tables; nn++ )
    {
      WOFF_Table  table = tables + nn;

      table->Tag        = FT_GET_TAG4();
      table->Offset     = FT_GET_ULONG();
      table->CompLength = FT_GET_ULONG();
      table->OrigLength = FT_GET_ULONG();
      table->CheckSum   = FT_GET_ULONG();

      FT_TRACE2(( "  %c%c%c%c  %08lx  %08lx  %08lx  %08lx\n",
                  (FT_Char)( table->Tag >> 24 ),
                  (FT_Char)( table->Tag >> 16 ),
                  (FT_Char)( table->Tag >> 8  ),
                  (FT_Char)( table->Tag       ),
                  table->Offset,
                  table->CompLength,
                  table->OrigLength,
                  table->CheckSum ));

      if ( table->Tag <= old_tag )
      {
        FT_FRAME_EXIT();
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      old_tag     = table->Tag;
      indices[nn] = table;
    }

    FT_FRAME_EXIT();

    

    ft_qsort( indices,
              woff.num_tables,
              sizeof ( WOFF_Table ),
              compare_offsets );

    

    woff_offset = 44 + woff.num_tables * 20L;
    sfnt_offset = 12 + woff.num_tables * 16L;

    for ( nn = 0; nn < woff.num_tables; nn++ )
    {
      WOFF_Table  table = indices[nn];


      if ( table->Offset != woff_offset                         ||
           table->CompLength > woff.length                      ||
           table->Offset > woff.length - table->CompLength      ||
           table->OrigLength > woff.totalSfntSize               ||
           sfnt_offset > woff.totalSfntSize - table->OrigLength ||
           table->CompLength > table->OrigLength                )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      table->OrigOffset = sfnt_offset;

      
      woff_offset += ( table->CompLength + 3 ) & ~3;
      sfnt_offset += ( table->OrigLength + 3 ) & ~3;
    }

    








    if ( woff.metaOffset )
    {
      if ( woff.metaOffset != woff_offset                  ||
           woff.metaOffset + woff.metaLength > woff.length )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      
      woff_offset += woff.metaLength;
    }

    if ( woff.privOffset )
    {
      
      woff_offset = ( woff_offset + 3 ) & ~3;

      if ( woff.privOffset != woff_offset                  ||
           woff.privOffset + woff.privLength > woff.length )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      
      woff_offset += woff.privLength;
    }

    if ( sfnt_offset != woff.totalSfntSize ||
         woff_offset != woff.length        )
    {
      error = FT_THROW( Invalid_Table );
      goto Exit;
    }

    

    for ( nn = 0; nn < woff.num_tables; nn++ )
    {
      WOFF_Table  table = tables + nn;


      
      WRITE_ULONG( sfnt_header, table->Tag );
      WRITE_ULONG( sfnt_header, table->CheckSum );
      WRITE_ULONG( sfnt_header, table->OrigOffset );
      WRITE_ULONG( sfnt_header, table->OrigLength );

      
      if ( FT_STREAM_SEEK( table->Offset )     ||
           FT_FRAME_ENTER( table->CompLength ) )
        goto Exit;

      if ( table->CompLength == table->OrigLength )
      {
        
        ft_memcpy( sfnt + table->OrigOffset,
                   stream->cursor,
                   table->OrigLength );
      }
      else
      {
#ifdef FT_CONFIG_OPTION_USE_ZLIB

        
        FT_ULong  output_len = table->OrigLength;


        error = FT_Gzip_Uncompress( memory,
                                    sfnt + table->OrigOffset, &output_len,
                                    stream->cursor, table->CompLength );
        if ( error )
          goto Exit;
        if ( output_len != table->OrigLength )
        {
          error = FT_THROW( Invalid_Table );
          goto Exit;
        }

#else 

        error = FT_THROW( Unimplemented_Feature );
        goto Exit;

#endif 
      }

      FT_FRAME_EXIT();

      
      
      sfnt_offset = table->OrigOffset + table->OrigLength;
      while ( sfnt_offset & 3 )
      {
        sfnt[sfnt_offset] = '\0';
        sfnt_offset++;
      }
    }

    
    FT_Stream_OpenMemory( sfnt_stream, sfnt, woff.totalSfntSize );
    sfnt_stream->memory = stream->memory;
    sfnt_stream->close  = sfnt_stream_close;

    FT_Stream_Free(
      face->root.stream,
      ( face->root.face_flags & FT_FACE_FLAG_EXTERNAL_STREAM ) != 0 );

    face->root.stream = sfnt_stream;

    face->root.face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;

  Exit:
    FT_FREE( tables );
    FT_FREE( indices );

    if ( error )
    {
      FT_FREE( sfnt );
      FT_Stream_Close( sfnt_stream );
      FT_FREE( sfnt_stream );
    }

    return error;
  }


#undef WRITE_USHORT
#undef WRITE_ULONG


  
  
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

  retry:
    offset = FT_STREAM_POS();

    if ( FT_READ_ULONG( tag ) )
      return error;

    if ( tag == TTAG_wOFF )
    {
      FT_TRACE2(( "sfnt_open_font: file is a WOFF; synthesizing SFNT\n" ));

      if ( FT_STREAM_SEEK( offset ) )
        return error;

      error = woff_open_font( stream, face );
      if ( error )
        return error;

      
      stream = face->root.stream;
      goto retry;
    }

    if ( tag != 0x00010000UL &&
         tag != TTAG_ttcf    &&
         tag != TTAG_OTTO    &&
         tag != TTAG_true    &&
         tag != TTAG_typ1    &&
         tag != 0x00020000UL )
    {
      FT_TRACE2(( "  not a font using the SFNT container format\n" ));
      return FT_THROW( Unknown_File_Format );
    }

    face->ttc_header.tag = TTAG_ttcf;

    if ( tag == TTAG_ttcf )
    {
      FT_Int  n;


      FT_TRACE3(( "sfnt_open_font: file is a collection\n" ));

      if ( FT_STREAM_READ_FIELDS( ttc_header_fields, &face->ttc_header ) )
        return error;

      if ( face->ttc_header.count == 0 )
        return FT_THROW( Invalid_Table );

      
      
      
      
      
      if ( (FT_ULong)face->ttc_header.count > stream->size / ( 28 + 4 ) )
        return FT_THROW( Array_Too_Large );

      
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

      if ( FT_NEW( face->ttc_header.offsets ) )
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
      {
        FT_ERROR(( "sfnt_init_face: cannot access `sfnt' module\n" ));
        return FT_THROW( Missing_Module );
      }

      face->sfnt       = sfnt;
      face->goto_table = sfnt->goto_table;
    }

    FT_FACE_FIND_GLOBAL_SERVICE( face, face->psnames, POSTSCRIPT_CMAPS );

    FT_TRACE2(( "SFNT driver\n" ));

    error = sfnt_open_font( stream, face );
    if ( error )
      return error;

    
    stream = face->root.stream;

    FT_TRACE2(( "sfnt_init_face: %08p, %ld\n", face, face_index ));

    if ( face_index < 0 )
      face_index = 0;

    if ( face_index >= face->ttc_header.count )
      return FT_THROW( Invalid_Argument );

    if ( FT_STREAM_SEEK( face->ttc_header.offsets[face_index] ) )
      return error;

    
    error = sfnt->load_font_dir( face, stream );
    if ( error )
      return error;

    face->root.num_faces  = face->ttc_header.count;
    face->root.face_index = face_index;

    return error;
  }


#define LOAD_( x )                                          \
  do                                                        \
  {                                                         \
    FT_TRACE2(( "`" #x "' " ));                             \
    FT_TRACE3(( "-->\n" ));                                 \
                                                            \
    error = sfnt->load_ ## x( face, stream );               \
                                                            \
    FT_TRACE2(( "%s\n", ( !error )                          \
                        ? "loaded"                          \
                        : FT_ERR_EQ( error, Table_Missing ) \
                          ? "missing"                       \
                          : "failed to load" ));            \
    FT_TRACE3(( "\n" ));                                    \
  } while ( 0 )

#define LOADM_( x, vertical )                               \
  do                                                        \
  {                                                         \
    FT_TRACE2(( "`%s" #x "' ",                              \
                vertical ? "vertical " : "" ));             \
    FT_TRACE3(( "-->\n" ));                                 \
                                                            \
    error = sfnt->load_ ## x( face, stream, vertical );     \
                                                            \
    FT_TRACE2(( "%s\n", ( !error )                          \
                        ? "loaded"                          \
                        : FT_ERR_EQ( error, Table_Missing ) \
                          ? "missing"                       \
                          : "failed to load" ));            \
    FT_TRACE3(( "\n" ));                                    \
  } while ( 0 )

#define GET_NAME( id, field )                                   \
  do                                                            \
  {                                                             \
    error = tt_face_get_name( face, TT_NAME_ID_ ## id, field ); \
    if ( error )                                                \
      goto Exit;                                                \
  } while ( 0 )


  FT_LOCAL_DEF( FT_Error )
  sfnt_load_face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error      error;
#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
    FT_Error      psnames_error;
#endif
    FT_Bool       has_outline;
    FT_Bool       is_apple_sbit;
    FT_Bool       is_apple_sbix;
    FT_Bool       ignore_preferred_family    = FALSE;
    FT_Bool       ignore_preferred_subfamily = FALSE;

    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;

    FT_UNUSED( face_index );


    

    {
      FT_Int  i;


      for ( i = 0; i < num_params; i++ )
      {
        if ( params[i].tag == FT_PARAM_TAG_IGNORE_PREFERRED_FAMILY )
          ignore_preferred_family = TRUE;
        else if ( params[i].tag == FT_PARAM_TAG_IGNORE_PREFERRED_SUBFAMILY )
          ignore_preferred_subfamily = TRUE;
      }
    }

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    FT_TRACE2(( "sfnt_load_face: %08p\n\n", face ));

    
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    has_outline = FT_BOOL( face->root.internal->incremental_interface != 0 ||
                           tt_face_lookup_table( face, TTAG_glyf )    != 0 ||
                           tt_face_lookup_table( face, TTAG_CFF )     != 0 );
#else
    has_outline = FT_BOOL( tt_face_lookup_table( face, TTAG_glyf ) != 0 ||
                           tt_face_lookup_table( face, TTAG_CFF )  != 0 );
#endif

    is_apple_sbit = 0;
    is_apple_sbix = !face->goto_table( face, TTAG_sbix, stream, 0 );

    


    if ( is_apple_sbix )
      has_outline = FALSE;

    
    
    if ( !has_outline && sfnt->load_bhed )
    {
      LOAD_( bhed );
      is_apple_sbit = FT_BOOL( !error );
    }

    
    
    if ( !is_apple_sbit || is_apple_sbix )
    {
      LOAD_( head );
      if ( error )
        goto Exit;
    }

    if ( face->header.Units_Per_EM == 0 )
    {
      error = FT_THROW( Invalid_Table );

      goto Exit;
    }

    
    
    LOAD_( maxp );
    LOAD_( cmap );

    
    
    LOAD_( name );
    LOAD_( post );

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
    psnames_error = error;
#endif

    
    
    if ( !is_apple_sbit )
    {
      
      LOADM_( hhea, 0 );
      if ( !error )
      {
        LOADM_( hmtx, 0 );
        if ( FT_ERR_EQ( error, Table_Missing ) )
        {
          error = FT_THROW( Hmtx_Table_Missing );

#ifdef FT_CONFIG_OPTION_INCREMENTAL
          
          
          if ( face->root.internal->incremental_interface          &&
               face->root.internal->incremental_interface->funcs->
                 get_glyph_metrics                                 )
          {
            face->horizontal.number_Of_HMetrics = 0;
            error                               = FT_Err_Ok;
          }
#endif
        }
      }
      else if ( FT_ERR_EQ( error, Table_Missing ) )
      {
        
        if ( face->format_tag == TTAG_true )
        {
          FT_TRACE2(( "This is an SFNT Mac font.\n" ));

          has_outline = 0;
          error       = FT_Err_Ok;
        }
        else
        {
          error = FT_THROW( Horiz_Header_Missing );

#ifdef FT_CONFIG_OPTION_INCREMENTAL
          
          
          if ( face->root.internal->incremental_interface          &&
               face->root.internal->incremental_interface->funcs->
                 get_glyph_metrics                                 )
          {
            face->horizontal.number_Of_HMetrics = 0;
            error                               = FT_Err_Ok;
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

      if ( error && FT_ERR_NEQ( error, Table_Missing ) )
        goto Exit;

      LOAD_( os2 );
      if ( error )
      {
        
        face->os2.version = 0xFFFFU;
      }
    }

    

    
    if ( sfnt->load_eblc )
    {
      LOAD_( eblc );
      if ( error )
      {
        
        
        
        if ( FT_ERR_EQ( error, Table_Missing ) )
          error = FT_Err_Ok;
        else
          goto Exit;
      }
    }

    LOAD_( pclt );
    if ( error )
    {
      if ( FT_ERR_NEQ( error, Table_Missing ) )
        goto Exit;

      face->pclt.Version = 0;
    }

    
    LOAD_( gasp );
    LOAD_( kern );

    face->root.num_glyphs = face->max_profile.numGlyphs;

    
    
    
    
    

    face->root.family_name = NULL;
    face->root.style_name  = NULL;
    if ( face->os2.version != 0xFFFFU && face->os2.fsSelection & 256 )
    {
      if ( !ignore_preferred_family )
        GET_NAME( PREFERRED_FAMILY, &face->root.family_name );
      if ( !face->root.family_name )
        GET_NAME( FONT_FAMILY, &face->root.family_name );

      if ( !ignore_preferred_subfamily )
        GET_NAME( PREFERRED_SUBFAMILY, &face->root.style_name );
      if ( !face->root.style_name )
        GET_NAME( FONT_SUBFAMILY, &face->root.style_name );
    }
    else
    {
      GET_NAME( WWS_FAMILY, &face->root.family_name );
      if ( !face->root.family_name && !ignore_preferred_family )
        GET_NAME( PREFERRED_FAMILY, &face->root.family_name );
      if ( !face->root.family_name )
        GET_NAME( FONT_FAMILY, &face->root.family_name );

      GET_NAME( WWS_SUBFAMILY, &face->root.style_name );
      if ( !face->root.style_name && !ignore_preferred_subfamily )
        GET_NAME( PREFERRED_SUBFAMILY, &face->root.style_name );
      if ( !face->root.style_name )
        GET_NAME( FONT_SUBFAMILY, &face->root.style_name );
    }

    
    {
      FT_Face  root  = &face->root;
      FT_Long  flags = root->face_flags;


      
      
      
      
      if ( face->sbit_table_type == TT_SBIT_TABLE_TYPE_CBLC ||
           face->sbit_table_type == TT_SBIT_TABLE_TYPE_SBIX )
        flags |= FT_FACE_FLAG_COLOR;      

      if ( has_outline == TRUE )
        flags |= FT_FACE_FLAG_SCALABLE;   

      
      
      flags |= FT_FACE_FLAG_SFNT       |  
               FT_FACE_FLAG_HORIZONTAL;   

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
      if ( !psnames_error                             &&
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

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

      




      {
        FT_UInt  i, count;


        count = face->sbit_num_strikes;

        if ( count > 0 )
        {
          FT_Memory        memory   = face->root.stream->memory;
          FT_UShort        em_size  = face->header.Units_Per_EM;
          FT_Short         avgwidth = face->os2.xAvgCharWidth;
          FT_Size_Metrics  metrics;


          if ( em_size == 0 || face->os2.version == 0xFFFFU )
          {
            avgwidth = 1;
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

      
      
      if ( !FT_HAS_FIXED_SIZES( root ) && !FT_IS_SCALABLE( root ) )
        root->face_flags |= FT_FACE_FLAG_SCALABLE;


      
      
      
      
      if ( FT_IS_SCALABLE( root ) )
      {
        
        
        root->bbox.xMin    = face->header.xMin;
        root->bbox.yMin    = face->header.yMin;
        root->bbox.xMax    = face->header.xMax;
        root->bbox.yMax    = face->header.yMax;
        root->units_per_EM = face->header.Units_Per_EM;


        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        

        root->ascender  = face->horizontal.Ascender;
        root->descender = face->horizontal.Descender;

        root->height = (FT_Short)( root->ascender - root->descender +
                                   face->horizontal.Line_Gap );

        if ( !( root->ascender || root->descender ) )
        {
          if ( face->os2.version != 0xFFFFU )
          {
            if ( face->os2.sTypoAscender || face->os2.sTypoDescender )
            {
              root->ascender  = face->os2.sTypoAscender;
              root->descender = face->os2.sTypoDescender;

              root->height = (FT_Short)( root->ascender - root->descender +
                                         face->os2.sTypoLineGap );
            }
            else
            {
              root->ascender  =  (FT_Short)face->os2.usWinAscent;
              root->descender = -(FT_Short)face->os2.usWinDescent;

              root->height = (FT_UShort)( root->ascender - root->descender );
            }
          }
        }

        root->max_advance_width  = face->horizontal.advance_Width_Max;
        root->max_advance_height = (FT_Short)( face->vertical_info
                                     ? face->vertical.advance_Height_Max
                                     : root->height );

        
        
        
        root->underline_position  = face->postscript.underlinePosition -
                                    face->postscript.underlineThickness / 2;
        root->underline_thickness = face->postscript.underlineThickness;
      }

    }

  Exit:
    FT_TRACE2(( "sfnt_load_face: done\n" ));

    return error;
  }


#undef LOAD_
#undef LOADM_
#undef GET_NAME


  FT_LOCAL_DEF( void )
  sfnt_done_face( TT_Face  face )
  {
    FT_Memory     memory;
    SFNT_Service  sfnt;


    if ( !face )
      return;

    memory = face->root.memory;
    sfnt   = (SFNT_Service)face->sfnt;

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

    
    {
      FT_Stream  stream = FT_FACE_STREAM( face );


      FT_FRAME_RELEASE( face->horz_metrics );
      FT_FRAME_RELEASE( face->vert_metrics );
      face->horz_metrics_size = 0;
      face->vert_metrics_size = 0;
    }

    
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



