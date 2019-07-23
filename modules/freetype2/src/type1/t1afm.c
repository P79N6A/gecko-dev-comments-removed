

















#include <ft2build.h>
#include "t1afm.h"
#include "t1errors.h"
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1afm


  FT_LOCAL_DEF( void )
  T1_Done_Metrics( FT_Memory     memory,
                   AFM_FontInfo  fi )
  {
    FT_FREE( fi->KernPairs );
    fi->NumKernPair = 0;

    FT_FREE( fi->TrackKerns );
    fi->NumTrackKern = 0;

    FT_FREE( fi );
  }


  
  static FT_Int
  t1_get_index( const char*  name,
                FT_UInt      len,
                void*        user_data )
  {
    T1_Font  type1 = (T1_Font)user_data;
    FT_Int   n;


    for ( n = 0; n < type1->num_glyphs; n++ )
    {
      char*  gname = (char*)type1->glyph_names[n];


      if ( gname && gname[0] == name[0]        &&
           ft_strlen( gname ) == len           &&
           ft_strncmp( gname, name, len ) == 0 )
        return n;
    }

    return 0;
  }


#undef  KERN_INDEX
#define KERN_INDEX( g1, g2 )  ( ( (FT_ULong)(g1) << 16 ) | (g2) )


  
  FT_CALLBACK_DEF( int )
  compare_kern_pairs( const void*  a,
                      const void*  b )
  {
    AFM_KernPair  pair1 = (AFM_KernPair)a;
    AFM_KernPair  pair2 = (AFM_KernPair)b;

    FT_ULong  index1 = KERN_INDEX( pair1->index1, pair1->index2 );
    FT_ULong  index2 = KERN_INDEX( pair2->index1, pair2->index2 );


    return (int)( index1 - index2 );
  }


  
  static FT_Error
  T1_Read_PFM( FT_Face       t1_face,
               FT_Stream     stream,
               AFM_FontInfo  fi )
  {
    FT_Error      error = T1_Err_Ok;
    FT_Memory     memory = stream->memory;
    FT_Byte*      start;
    FT_Byte*      limit;
    FT_Byte*      p;
    AFM_KernPair  kp;
    FT_Int        width_table_length;
    FT_CharMap    oldcharmap;
    FT_CharMap    charmap;
    FT_Int        n;


    start = (FT_Byte*)stream->cursor;
    limit = (FT_Byte*)stream->limit;
    p     = start;

    
    
    p = start + 99;
    if ( p + 2 > limit )
    {
      error = T1_Err_Unknown_File_Format;
      goto Exit;
    }
    width_table_length = FT_PEEK_USHORT_LE( p );

    p += 18 + width_table_length;
    if ( p + 0x12 > limit || FT_PEEK_USHORT_LE( p ) < 0x12 )
      
      goto Exit;

    
    p += 14;
    p = start + FT_PEEK_ULONG_LE( p );

    if ( p == start )
      
      goto Exit;

    if ( p + 2 > limit )
    {
      error = T1_Err_Unknown_File_Format;
      goto Exit;
    }

    fi->NumKernPair = FT_PEEK_USHORT_LE( p );
    p += 2;
    if ( p + 4 * fi->NumKernPair > limit )
    {
      error = T1_Err_Unknown_File_Format;
      goto Exit;
    }

    
    if ( fi->NumKernPair == 0 )
      goto Exit;

    
    if ( FT_QNEW_ARRAY( fi->KernPairs, fi->NumKernPair ) )
      goto Exit;

    
    kp             = fi->KernPairs;
    limit          = p + 4 * fi->NumKernPair;

    
    
    
    
    oldcharmap = t1_face->charmap;
    charmap    = NULL;

    for ( n = 0; n < t1_face->num_charmaps; n++ )
    {
      charmap = t1_face->charmaps[n];
      
      if ( charmap->platform_id == 7 )
      {
        error = FT_Set_Charmap( t1_face, charmap );
        if ( error )
          goto Exit;
        break;
      }
    }

    
    
    
    
    
    for ( ; p < limit ; p += 4 )
    {
      kp->index1 = FT_Get_Char_Index( t1_face, p[0] );
      kp->index2 = FT_Get_Char_Index( t1_face, p[1] );

      kp->x = (FT_Int)FT_PEEK_SHORT_LE(p + 2);
      kp->y = 0;

      kp++;
    }

    if ( oldcharmap != NULL )
      error = FT_Set_Charmap( t1_face, oldcharmap );
    if ( error )
      goto Exit;

    
    ft_qsort( fi->KernPairs, fi->NumKernPair, sizeof ( AFM_KernPairRec ),
              compare_kern_pairs );

  Exit:
    if ( error )
    {
      FT_FREE( fi->KernPairs );
      fi->NumKernPair = 0;
    }

    return error;
  }


  
  
  FT_LOCAL_DEF( FT_Error )
  T1_Read_Metrics( FT_Face    t1_face,
                   FT_Stream  stream )
  {
    PSAux_Service  psaux;
    FT_Memory      memory = stream->memory;
    AFM_ParserRec  parser;
    AFM_FontInfo   fi;
    FT_Error       error = T1_Err_Unknown_File_Format;
    T1_Font        t1_font = &( (T1_Face)t1_face )->type1;


    if ( FT_NEW( fi )                   ||
         FT_FRAME_ENTER( stream->size ) )
      goto Exit;

    fi->FontBBox  = t1_font->font_bbox;
    fi->Ascender  = t1_font->font_bbox.yMax;
    fi->Descender = t1_font->font_bbox.yMin;

    psaux = (PSAux_Service)( (T1_Face)t1_face )->psaux;
    if ( psaux && psaux->afm_parser_funcs )
    {
      error = psaux->afm_parser_funcs->init( &parser,
                                             stream->memory,
                                             stream->cursor,
                                             stream->limit );

      if ( !error )
      {
        parser.FontInfo  = fi;
        parser.get_index = t1_get_index;
        parser.user_data = t1_font;

        error = psaux->afm_parser_funcs->parse( &parser );
        psaux->afm_parser_funcs->done( &parser );
      }
    }

    if ( error == T1_Err_Unknown_File_Format )
    {
      FT_Byte*  start = stream->cursor;


      
      if ( stream->size > 6                              &&
           start[1] < 4                                  &&
           FT_PEEK_ULONG_LE( start + 2 ) == stream->size )
        error = T1_Read_PFM( t1_face, stream, fi );
    }

    if ( !error )
    {
      t1_font->font_bbox = fi->FontBBox;

      t1_face->bbox.xMin =   fi->FontBBox.xMin             >> 16;
      t1_face->bbox.yMin =   fi->FontBBox.yMin             >> 16;
      t1_face->bbox.xMax = ( fi->FontBBox.xMax + 0xFFFFU ) >> 16;
      t1_face->bbox.yMax = ( fi->FontBBox.yMax + 0xFFFFU ) >> 16;

      t1_face->ascender  = (FT_Short)( ( fi->Ascender  + 0x8000U ) >> 16 );
      t1_face->descender = (FT_Short)( ( fi->Descender + 0x8000U ) >> 16 );

      if ( fi->NumKernPair )
      {
        t1_face->face_flags |= FT_FACE_FLAG_KERNING;
        ( (T1_Face)t1_face )->afm_data = fi;
        fi = NULL;
      }
    }

    FT_FRAME_EXIT();

  Exit:
    if ( fi != NULL )
      T1_Done_Metrics( memory, fi );

    return error;
  }


  
  FT_LOCAL_DEF( void )
  T1_Get_Kerning( AFM_FontInfo  fi,
                  FT_UInt       glyph1,
                  FT_UInt       glyph2,
                  FT_Vector*    kerning )
  {
    AFM_KernPair  min, mid, max;
    FT_ULong      idx = KERN_INDEX( glyph1, glyph2 );


    
    min = fi->KernPairs;
    max = min + fi->NumKernPair - 1;

    while ( min <= max )
    {
      FT_ULong  midi;


      mid  = min + ( max - min ) / 2;
      midi = KERN_INDEX( mid->index1, mid->index2 );

      if ( midi == idx )
      {
        kerning->x = mid->x;
        kerning->y = mid->y;

        return;
      }

      if ( midi < idx )
        min = mid + 1;
      else
        max = mid - 1;
    }

    kerning->x = 0;
    kerning->y = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Get_Track_Kerning( FT_Face    face,
                        FT_Fixed   ptsize,
                        FT_Int     degree,
                        FT_Fixed*  kerning )
  {
    AFM_FontInfo  fi = (AFM_FontInfo)( (T1_Face)face )->afm_data;
    FT_Int        i;


    if ( !fi )
      return T1_Err_Invalid_Argument;

    for ( i = 0; i < fi->NumTrackKern; i++ )
    {
      AFM_TrackKern  tk = fi->TrackKerns + i;


      if ( tk->degree != degree )
        continue;

      if ( ptsize < tk->min_ptsize )
        *kerning = tk->min_kern;
      else if ( ptsize > tk->max_ptsize )
        *kerning = tk->max_kern;
      else
      {
        *kerning = FT_MulDiv( ptsize - tk->min_ptsize,
                              tk->max_kern - tk->min_kern,
                              tk->max_ptsize - tk->min_ptsize ) +
                   tk->min_kern;
      }
    }

    return T1_Err_Ok;
  }



