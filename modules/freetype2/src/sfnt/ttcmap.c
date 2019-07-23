

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H

#include "sferrors.h"           

#include FT_INTERNAL_VALIDATE_H
#include FT_INTERNAL_STREAM_H
#include "ttload.h"
#include "ttcmap.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttcmap


#define TT_PEEK_SHORT   FT_PEEK_SHORT
#define TT_PEEK_USHORT  FT_PEEK_USHORT
#define TT_PEEK_UINT24  FT_PEEK_UOFF3
#define TT_PEEK_LONG    FT_PEEK_LONG
#define TT_PEEK_ULONG   FT_PEEK_ULONG

#define TT_NEXT_SHORT   FT_NEXT_SHORT
#define TT_NEXT_USHORT  FT_NEXT_USHORT
#define TT_NEXT_UINT24  FT_NEXT_UOFF3
#define TT_NEXT_LONG    FT_NEXT_LONG
#define TT_NEXT_ULONG   FT_NEXT_ULONG


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap_init( TT_CMap   cmap,
                FT_Byte*  table )
  {
    cmap->data = table;
    return SFNT_Err_Ok;
  }


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_0

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap0_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p      = table + 2;
    FT_UInt   length = TT_NEXT_USHORT( p );


    if ( table + length > valid->limit || length < 262 )
      FT_INVALID_TOO_SHORT;

    
    if ( valid->level >= FT_VALIDATE_TIGHT )
    {
      FT_UInt  n, idx;


      p = table + 6;
      for ( n = 0; n < 256; n++ )
      {
        idx = *p++;
        if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
          FT_INVALID_GLYPH_ID;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap0_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*  table = cmap->data;


    return char_code < 256 ? table[6 + char_code] : 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap0_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_Byte*   table    = cmap->data;
    FT_UInt32  charcode = *pchar_code;
    FT_UInt32  result   = 0;
    FT_UInt    gindex   = 0;


    table += 6;  
    while ( ++charcode < 256 )
    {
      gindex = table[charcode];
      if ( gindex != 0 )
      {
        result = charcode;
        break;
      }
    }

    *pchar_code = result;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap0_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->format = 0;
    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap0_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap0_char_index,
      (FT_CMap_CharNextFunc) tt_cmap0_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    0,
    (TT_CMap_ValidateFunc)   tt_cmap0_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap0_get_info
  };

#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_2

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap2_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p      = table + 2;           
    FT_UInt   length = TT_PEEK_USHORT( p );
    FT_UInt   n, max_subs;
    FT_Byte*  keys;                         
    FT_Byte*  subs;                         
    FT_Byte*  glyph_ids;                    


    if ( table + length > valid->limit || length < 6 + 512 )
      FT_INVALID_TOO_SHORT;

    keys = table + 6;

    
    p        = keys;
    max_subs = 0;
    for ( n = 0; n < 256; n++ )
    {
      FT_UInt  idx = TT_NEXT_USHORT( p );


      
      if ( valid->level >= FT_VALIDATE_PARANOID && ( idx & 7 ) != 0 )
        FT_INVALID_DATA;

      idx >>= 3;

      if ( idx > max_subs )
        max_subs = idx;
    }

    FT_ASSERT( p == table + 518 );

    subs      = p;
    glyph_ids = subs + (max_subs + 1) * 8;
    if ( glyph_ids > valid->limit )
      FT_INVALID_TOO_SHORT;

    
    for ( n = 0; n <= max_subs; n++ )
    {
      FT_UInt   first_code, code_count, offset;
      FT_Int    delta;
      FT_Byte*  ids;


      first_code = TT_NEXT_USHORT( p );
      code_count = TT_NEXT_USHORT( p );
      delta      = TT_NEXT_SHORT( p );
      offset     = TT_NEXT_USHORT( p );

      
      if ( valid->level >= FT_VALIDATE_PARANOID )
      {
        if ( first_code >= 256 || first_code + code_count > 256 )
          FT_INVALID_DATA;
      }

      
      if ( offset != 0 )
      {
        ids = p - 2 + offset;
        if ( ids < glyph_ids || ids + code_count*2 > table + length )
          FT_INVALID_OFFSET;

        
        if ( valid->level >= FT_VALIDATE_TIGHT )
        {
          FT_Byte*  limit = p + code_count * 2;
          FT_UInt   idx;


          for ( ; p < limit; )
          {
            idx = TT_NEXT_USHORT( p );
            if ( idx != 0 )
            {
              idx = ( idx + delta ) & 0xFFFFU;
              if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
                FT_INVALID_GLYPH_ID;
            }
          }
        }
      }
    }

    return SFNT_Err_Ok;
  }


  
  
  static FT_Byte*
  tt_cmap2_get_subheader( FT_Byte*   table,
                          FT_UInt32  char_code )
  {
    FT_Byte*  result = NULL;


    if ( char_code < 0x10000UL )
    {
      FT_UInt   char_lo = (FT_UInt)( char_code & 0xFF );
      FT_UInt   char_hi = (FT_UInt)( char_code >> 8 );
      FT_Byte*  p       = table + 6;    
      FT_Byte*  subs    = table + 518;  
      FT_Byte*  sub;


      if ( char_hi == 0 )
      {
        
        
        
        sub = subs;  

        
        
        
        
        p += char_lo * 2;
        if ( TT_PEEK_USHORT( p ) != 0 )
          goto Exit;
      }
      else
      {
        

        
        p  += char_hi * 2;
        
        sub = subs + ( FT_PAD_FLOOR( TT_PEEK_USHORT( p ), 8 ) );

        
        if ( sub == subs )
          goto Exit;
      }
      result = sub;
    }
  Exit:
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap2_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*  table   = cmap->data;
    FT_UInt   result  = 0;
    FT_Byte*  subheader;


    subheader = tt_cmap2_get_subheader( table, char_code );
    if ( subheader )
    {
      FT_Byte*  p   = subheader;
      FT_UInt   idx = (FT_UInt)(char_code & 0xFF);
      FT_UInt   start, count;
      FT_Int    delta;
      FT_UInt   offset;


      start  = TT_NEXT_USHORT( p );
      count  = TT_NEXT_USHORT( p );
      delta  = TT_NEXT_SHORT ( p );
      offset = TT_PEEK_USHORT( p );

      idx -= start;
      if ( idx < count && offset != 0 )
      {
        p  += offset + 2 * idx;
        idx = TT_PEEK_USHORT( p );

        if ( idx != 0 )
          result = (FT_UInt)( idx + delta ) & 0xFFFFU;
      }
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap2_char_next( TT_CMap     cmap,
                      FT_UInt32  *pcharcode )
  {
    FT_Byte*   table    = cmap->data;
    FT_UInt    gindex   = 0;
    FT_UInt32  result   = 0;
    FT_UInt32  charcode = *pcharcode + 1;
    FT_Byte*   subheader;


    while ( charcode < 0x10000UL )
    {
      subheader = tt_cmap2_get_subheader( table, charcode );
      if ( subheader )
      {
        FT_Byte*  p       = subheader;
        FT_UInt   start   = TT_NEXT_USHORT( p );
        FT_UInt   count   = TT_NEXT_USHORT( p );
        FT_Int    delta   = TT_NEXT_SHORT ( p );
        FT_UInt   offset  = TT_PEEK_USHORT( p );
        FT_UInt   char_lo = (FT_UInt)( charcode & 0xFF );
        FT_UInt   pos, idx;


        if ( offset == 0 )
          goto Next_SubHeader;

        if ( char_lo < start )
        {
          char_lo = start;
          pos     = 0;
        }
        else
          pos = (FT_UInt)( char_lo - start );

        p       += offset + pos * 2;
        charcode = FT_PAD_FLOOR( charcode, 256 ) + char_lo;

        for ( ; pos < count; pos++, charcode++ )
        {
          idx = TT_NEXT_USHORT( p );

          if ( idx != 0 )
          {
            gindex = ( idx + delta ) & 0xFFFFU;
            if ( gindex != 0 )
            {
              result = charcode;
              goto Exit;
            }
          }
        }
      }

      
    Next_SubHeader:
      charcode = FT_PAD_FLOOR( charcode, 256 ) + 256;
    }

  Exit:
    *pcharcode = result;

    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap2_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->format = 2;
    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap2_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap2_char_index,
      (FT_CMap_CharNextFunc) tt_cmap2_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    2,
    (TT_CMap_ValidateFunc)   tt_cmap2_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap2_get_info
  };

#endif 


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_4

  typedef struct  TT_CMap4Rec_
  {
    TT_CMapRec  cmap;
    FT_UInt32   cur_charcode;   
    FT_UInt     cur_gindex;     

    FT_UInt     num_ranges;
    FT_UInt     cur_range;
    FT_UInt     cur_start;
    FT_UInt     cur_end;
    FT_Int      cur_delta;
    FT_Byte*    cur_values;

  } TT_CMap4Rec, *TT_CMap4;


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap4_init( TT_CMap4  cmap,
                 FT_Byte*  table )
  {
    FT_Byte*  p;


    cmap->cmap.data    = table;

    p                  = table + 6;
    cmap->num_ranges   = FT_PEEK_USHORT( p ) >> 1;
    cmap->cur_charcode = 0xFFFFFFFFUL;
    cmap->cur_gindex   = 0;

    return SFNT_Err_Ok;
  }


  static FT_Int
  tt_cmap4_set_range( TT_CMap4  cmap,
                      FT_UInt   range_index )
  {
    FT_Byte*  table = cmap->cmap.data;
    FT_Byte*  p;
    FT_UInt   num_ranges = cmap->num_ranges;


    while ( range_index < num_ranges )
    {
      FT_UInt  offset;


      p             = table + 14 + range_index * 2;
      cmap->cur_end = FT_PEEK_USHORT( p );

      p              += 2 + num_ranges * 2;
      cmap->cur_start = FT_PEEK_USHORT( p );

      p              += num_ranges * 2;
      cmap->cur_delta = FT_PEEK_SHORT( p );

      p     += num_ranges * 2;
      offset = FT_PEEK_USHORT( p );

      if ( offset != 0xFFFFU )
      {
        cmap->cur_values = offset ? p + offset : NULL;
        cmap->cur_range  = range_index;
        return 0;
      }

      
      range_index++;
    }

    return -1;
  }


  
  
  
  
  static void
  tt_cmap4_next( TT_CMap4  cmap )
  {
    FT_UInt  charcode;


    if ( cmap->cur_charcode >= 0xFFFFUL )
      goto Fail;

    charcode = cmap->cur_charcode + 1;

    if ( charcode < cmap->cur_start )
      charcode = cmap->cur_start;

    for ( ;; )
    {
      FT_Byte*  values = cmap->cur_values;
      FT_UInt   end    = cmap->cur_end;
      FT_Int    delta  = cmap->cur_delta;


      if ( charcode <= end )
      {
        if ( values )
        {
          FT_Byte*  p = values + 2 * ( charcode - cmap->cur_start );


          do
          {
            FT_UInt  gindex = FT_NEXT_USHORT( p );


            if ( gindex != 0 )
            {
              gindex = (FT_UInt)( ( gindex + delta ) & 0xFFFFU );
              if ( gindex != 0 )
              {
                cmap->cur_charcode = charcode;
                cmap->cur_gindex   = gindex;
                return;
              }
            }
          } while ( ++charcode <= end );
        }
        else
        {
          do
          {
            FT_UInt  gindex = (FT_UInt)( ( charcode + delta ) & 0xFFFFU );


            if ( gindex != 0 )
            {
              cmap->cur_charcode = charcode;
              cmap->cur_gindex   = gindex;
              return;
            }
          } while ( ++charcode <= end );
        }
      }

      
      if ( tt_cmap4_set_range( cmap, cmap->cur_range + 1 ) < 0 )
        break;

      if ( charcode < cmap->cur_start )
        charcode = cmap->cur_start;
    }

  Fail:
    cmap->cur_charcode = 0xFFFFFFFFUL;
    cmap->cur_gindex   = 0;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap4_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p      = table + 2;               
    FT_UInt   length = TT_NEXT_USHORT( p );
    FT_Byte   *ends, *starts, *offsets, *deltas, *glyph_ids;
    FT_UInt   num_segs;
    FT_Error  error = SFNT_Err_Ok;


    if ( length < 16 )
      FT_INVALID_TOO_SHORT;

    
    
    if ( table + length > valid->limit )
    {
      if ( valid->level >= FT_VALIDATE_TIGHT )
        FT_INVALID_TOO_SHORT;

      length = (FT_UInt)( valid->limit - table );
    }

    p        = table + 6;
    num_segs = TT_NEXT_USHORT( p );   

    if ( valid->level >= FT_VALIDATE_PARANOID )
    {
      
      if ( num_segs & 1 )
        FT_INVALID_DATA;
    }

    num_segs /= 2;

    if ( length < 16 + num_segs * 2 * 4 )
      FT_INVALID_TOO_SHORT;

    
    
    if ( valid->level >= FT_VALIDATE_PARANOID )
    {
      
      FT_UInt  search_range   = TT_NEXT_USHORT( p );
      FT_UInt  entry_selector = TT_NEXT_USHORT( p );
      FT_UInt  range_shift    = TT_NEXT_USHORT( p );


      if ( ( search_range | range_shift ) & 1 )  
        FT_INVALID_DATA;

      search_range /= 2;
      range_shift  /= 2;

      

      if ( search_range                > num_segs                 ||
           search_range * 2            < num_segs                 ||
           search_range + range_shift != num_segs                 ||
           search_range               != ( 1U << entry_selector ) )
        FT_INVALID_DATA;
    }

    ends      = table   + 14;
    starts    = table   + 16 + num_segs * 2;
    deltas    = starts  + num_segs * 2;
    offsets   = deltas  + num_segs * 2;
    glyph_ids = offsets + num_segs * 2;

    
    if ( valid->level >= FT_VALIDATE_PARANOID )
    {
      p = ends + ( num_segs - 1 ) * 2;
      if ( TT_PEEK_USHORT( p ) != 0xFFFFU )
        FT_INVALID_DATA;
    }

    {
      FT_UInt  start, end, offset, n;
      FT_UInt  last_start = 0, last_end = 0;
      FT_Int   delta;
      FT_Byte*  p_start   = starts;
      FT_Byte*  p_end     = ends;
      FT_Byte*  p_delta   = deltas;
      FT_Byte*  p_offset  = offsets;


      for ( n = 0; n < num_segs; n++ )
      {
        p      = p_offset;
        start  = TT_NEXT_USHORT( p_start );
        end    = TT_NEXT_USHORT( p_end );
        delta  = TT_NEXT_SHORT( p_delta );
        offset = TT_NEXT_USHORT( p_offset );

        if ( start > end )
          FT_INVALID_DATA;

        
        
        
        
        if ( start <= last_end && n > 0 )
        {
          if ( valid->level >= FT_VALIDATE_TIGHT )
            FT_INVALID_DATA;
          else
          {
            
            
            
            if ( last_start > start || last_end > end )
              error |= TT_CMAP_FLAG_UNSORTED;
            else
              error |= TT_CMAP_FLAG_OVERLAPPING;
          }
        }

        if ( offset && offset != 0xFFFFU )
        {
          p += offset;  

          
          if ( valid->level >= FT_VALIDATE_TIGHT )
          {
            if ( p < glyph_ids                                ||
                 p + ( end - start + 1 ) * 2 > table + length )
              FT_INVALID_DATA;
          }
          else
          {
            if ( p < glyph_ids                              ||
                 p + ( end - start + 1 ) * 2 > valid->limit )
              FT_INVALID_DATA;
          }

          
          if ( valid->level >= FT_VALIDATE_TIGHT )
          {
            FT_UInt  i, idx;


            for ( i = start; i < end; i++ )
            {
              idx = FT_NEXT_USHORT( p );
              if ( idx != 0 )
              {
                idx = (FT_UInt)( idx + delta ) & 0xFFFFU;

                if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
                  FT_INVALID_GLYPH_ID;
              }
            }
          }
        }
        else if ( offset == 0xFFFFU )
        {
          
          
          
          if ( valid->level >= FT_VALIDATE_PARANOID                     ||
               n != num_segs - 1                                        ||
               !( start == 0xFFFFU && end == 0xFFFFU && delta == 0x1U ) )
            FT_INVALID_DATA;
        }

        last_start = start;
        last_end   = end;
      }
    }

    return error;
  }


  static FT_UInt
  tt_cmap4_char_map_linear( TT_CMap   cmap,
                            FT_UInt*  pcharcode,
                            FT_Bool   next )
  {
    FT_UInt    num_segs2, start, end, offset;
    FT_Int     delta;
    FT_UInt    i, num_segs;
    FT_UInt32  charcode = *pcharcode;
    FT_UInt    gindex   = 0;
    FT_Byte*   p;


    p = cmap->data + 6;
    num_segs2 = FT_PAD_FLOOR( TT_PEEK_USHORT( p ), 2 );

    num_segs = num_segs2 >> 1;

    if ( !num_segs )
      return 0;

    if ( next )
      charcode++;

    
    for ( ; charcode <= 0xFFFFU; charcode++ )
    {
      FT_Byte*  q;


      p = cmap->data + 14;               
      q = cmap->data + 16 + num_segs2;   

      for ( i = 0; i < num_segs; i++ )
      {
        end   = TT_NEXT_USHORT( p );
        start = TT_NEXT_USHORT( q );

        if ( charcode >= start && charcode <= end )
        {
          p       = q - 2 + num_segs2;
          delta   = TT_PEEK_SHORT( p );
          p      += num_segs2;
          offset  = TT_PEEK_USHORT( p );

          if ( offset == 0xFFFFU )
            continue;

          if ( offset )
          {
            p += offset + ( charcode - start ) * 2;
            gindex = TT_PEEK_USHORT( p );
            if ( gindex != 0 )
              gindex = (FT_UInt)( gindex + delta ) & 0xFFFFU;
          }
          else
            gindex = (FT_UInt)( charcode + delta ) & 0xFFFFU;

          break;
        }
      }

      if ( !next || gindex )
        break;
    }

    if ( next && gindex )
      *pcharcode = charcode;

    return gindex;
  }


  static FT_UInt
  tt_cmap4_char_map_binary( TT_CMap   cmap,
                            FT_UInt*  pcharcode,
                            FT_Bool   next )
  {
    FT_UInt   num_segs2, start, end, offset;
    FT_Int    delta;
    FT_UInt   max, min, mid, num_segs;
    FT_UInt   charcode = *pcharcode;
    FT_UInt   gindex   = 0;
    FT_Byte*  p;


    p = cmap->data + 6;
    num_segs2 = FT_PAD_FLOOR( TT_PEEK_USHORT( p ), 2 );

    if ( !num_segs2 )
      return 0;

    num_segs = num_segs2 >> 1;

    
    mid = num_segs;
    end = 0xFFFFU;

    if ( next )
      charcode++;

    min = 0;
    max = num_segs;

    
    while ( min < max )
    {
      mid    = ( min + max ) >> 1;
      p      = cmap->data + 14 + mid * 2;
      end    = TT_PEEK_USHORT( p );
      p     += 2 + num_segs2;
      start  = TT_PEEK_USHORT( p );

      if ( charcode < start )
        max = mid;
      else if ( charcode > end )
        min = mid + 1;
      else
      {
        p     += num_segs2;
        delta  = TT_PEEK_SHORT( p );
        p     += num_segs2;
        offset = TT_PEEK_USHORT( p );

        
        if ( cmap->flags & TT_CMAP_FLAG_OVERLAPPING )
        {
          FT_UInt  i;


          
          max = mid;

          if ( offset == 0xFFFFU )
            mid = max + 1;

          
          for ( i = max ; i > 0; i-- )
          {
            FT_UInt   prev_end;
            FT_Byte*  old_p;


            old_p    = p;
            p        = cmap->data + 14 + ( i - 1 ) * 2;
            prev_end = TT_PEEK_USHORT( p );

            if ( charcode > prev_end )
            {
              p = old_p;
              break;
            }

            end    = prev_end;
            p     += 2 + num_segs2;
            start  = TT_PEEK_USHORT( p );
            p     += num_segs2;
            delta  = TT_PEEK_SHORT( p );
            p     += num_segs2;
            offset = TT_PEEK_USHORT( p );

            if ( offset != 0xFFFFU )
              mid = i - 1;
          }

          
          if ( mid == max + 1 )
          {
            if ( i != max )
            {
              p      = cmap->data + 14 + max * 2;
              end    = TT_PEEK_USHORT( p );
              p     += 2 + num_segs2;
              start  = TT_PEEK_USHORT( p );
              p     += num_segs2;
              delta  = TT_PEEK_SHORT( p );
              p     += num_segs2;
              offset = TT_PEEK_USHORT( p );
            }

            mid = max;

            
            for ( i = max + 1; i < num_segs; i++ )
            {
              FT_UInt  next_end, next_start;


              p          = cmap->data + 14 + i * 2;
              next_end   = TT_PEEK_USHORT( p );
              p         += 2 + num_segs2;
              next_start = TT_PEEK_USHORT( p );

              if ( charcode < next_start )
                break;

              end    = next_end;
              start  = next_start;
              p     += num_segs2;
              delta  = TT_PEEK_SHORT( p );
              p     += num_segs2;
              offset = TT_PEEK_USHORT( p );

              if ( offset != 0xFFFFU )
                mid = i;
            }
            i--;

            
            if ( mid == max )
            {
              mid = i;

              break;
            }
          }

          
          if ( mid != i )
          {
            p      = cmap->data + 14 + mid * 2;
            end    = TT_PEEK_USHORT( p );
            p     += 2 + num_segs2;
            start  = TT_PEEK_USHORT( p );
            p     += num_segs2;
            delta  = TT_PEEK_SHORT( p );
            p     += num_segs2;
            offset = TT_PEEK_USHORT( p );
          }
        }
        else
        {
          if ( offset == 0xFFFFU )
            break;
        }

        if ( offset )
        {
          p += offset + ( charcode - start ) * 2;
          gindex = TT_PEEK_USHORT( p );
          if ( gindex != 0 )
            gindex = (FT_UInt)( gindex + delta ) & 0xFFFFU;
        }
        else
          gindex = (FT_UInt)( charcode + delta ) & 0xFFFFU;

        break;
      }
    }

    if ( next )
    {
      TT_CMap4  cmap4 = (TT_CMap4)cmap;


      
      
      

      if ( charcode > end )
      {
        mid++;
        if ( mid == num_segs )
          return 0;
      }

      if ( tt_cmap4_set_range( cmap4, mid ) )
      {
        if ( gindex )
          *pcharcode = charcode;
      }
      else
      {
        cmap4->cur_charcode = charcode;

        if ( gindex )
          cmap4->cur_gindex = gindex;
        else
        {
          cmap4->cur_charcode = charcode;
          tt_cmap4_next( cmap4 );
          gindex = cmap4->cur_gindex;
        }

        if ( gindex )
          *pcharcode = cmap4->cur_charcode;
      }
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap4_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    if ( char_code >= 0x10000UL )
      return 0;

    if ( cmap->flags & TT_CMAP_FLAG_UNSORTED )
      return tt_cmap4_char_map_linear( cmap, &char_code, 0 );
    else
      return tt_cmap4_char_map_binary( cmap, &char_code, 0 );
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap4_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_UInt  gindex;


    if ( *pchar_code >= 0xFFFFU )
      return 0;

    if ( cmap->flags & TT_CMAP_FLAG_UNSORTED )
      gindex = tt_cmap4_char_map_linear( cmap, pchar_code, 1 );
    else
    {
      TT_CMap4  cmap4 = (TT_CMap4)cmap;


      
      if ( *pchar_code == cmap4->cur_charcode )
      {
        tt_cmap4_next( cmap4 );
        gindex = cmap4->cur_gindex;
        if ( gindex )
          *pchar_code = cmap4->cur_charcode;
      }
      else
        gindex = tt_cmap4_char_map_binary( cmap, pchar_code, 1 );
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap4_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->format = 4;
    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap4_class_rec =
  {
    {
      sizeof ( TT_CMap4Rec ),
      (FT_CMap_InitFunc)     tt_cmap4_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap4_char_index,
      (FT_CMap_CharNextFunc) tt_cmap4_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    4,
    (TT_CMap_ValidateFunc)   tt_cmap4_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap4_get_info
  };

#endif 


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_6

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap6_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*  p;
    FT_UInt   length, count;


    if ( table + 10 > valid->limit )
      FT_INVALID_TOO_SHORT;

    p      = table + 2;
    length = TT_NEXT_USHORT( p );

    p      = table + 8;             
    count  = TT_NEXT_USHORT( p );

    if ( table + length > valid->limit || length < 10 + count * 2 )
      FT_INVALID_TOO_SHORT;

    
    if ( valid->level >= FT_VALIDATE_TIGHT )
    {
      FT_UInt  gindex;


      for ( ; count > 0; count-- )
      {
        gindex = TT_NEXT_USHORT( p );
        if ( gindex >= TT_VALID_GLYPH_COUNT( valid ) )
          FT_INVALID_GLYPH_ID;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap6_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*  table  = cmap->data;
    FT_UInt   result = 0;
    FT_Byte*  p      = table + 6;
    FT_UInt   start  = TT_NEXT_USHORT( p );
    FT_UInt   count  = TT_NEXT_USHORT( p );
    FT_UInt   idx    = (FT_UInt)( char_code - start );


    if ( idx < count )
    {
      p += 2 * idx;
      result = TT_PEEK_USHORT( p );
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap6_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_Byte*   table     = cmap->data;
    FT_UInt32  result    = 0;
    FT_UInt32  char_code = *pchar_code + 1;
    FT_UInt    gindex    = 0;

    FT_Byte*   p         = table + 6;
    FT_UInt    start     = TT_NEXT_USHORT( p );
    FT_UInt    count     = TT_NEXT_USHORT( p );
    FT_UInt    idx;


    if ( char_code >= 0x10000UL )
      goto Exit;

    if ( char_code < start )
      char_code = start;

    idx = (FT_UInt)( char_code - start );
    p  += 2 * idx;

    for ( ; idx < count; idx++ )
    {
      gindex = TT_NEXT_USHORT( p );
      if ( gindex != 0 )
      {
        result = char_code;
        break;
      }
      char_code++;
    }

  Exit:
    *pchar_code = result;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap6_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 4;


    cmap_info->format = 6;
    cmap_info->language = (FT_ULong)TT_PEEK_USHORT( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap6_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap6_char_index,
      (FT_CMap_CharNextFunc) tt_cmap6_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    6,
    (TT_CMap_ValidateFunc)   tt_cmap6_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap6_get_info
  };

#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_8

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap8_validate( FT_Byte*      table,
                     FT_Validator  valid )
  {
    FT_Byte*   p = table + 4;
    FT_Byte*   is32;
    FT_UInt32  length;
    FT_UInt32  num_groups;


    if ( table + 16 + 8192 > valid->limit )
      FT_INVALID_TOO_SHORT;

    length = TT_NEXT_ULONG( p );
    if ( table + length > valid->limit || length < 8208 )
      FT_INVALID_TOO_SHORT;

    is32       = table + 12;
    p          = is32  + 8192;          
    num_groups = TT_NEXT_ULONG( p );

    if ( p + num_groups * 12 > valid->limit )
      FT_INVALID_TOO_SHORT;

    
    {
      FT_UInt32  n, start, end, start_id, count, last = 0;


      for ( n = 0; n < num_groups; n++ )
      {
        FT_UInt   hi, lo;


        start    = TT_NEXT_ULONG( p );
        end      = TT_NEXT_ULONG( p );
        start_id = TT_NEXT_ULONG( p );

        if ( start > end )
          FT_INVALID_DATA;

        if ( n > 0 && start <= last )
          FT_INVALID_DATA;

        if ( valid->level >= FT_VALIDATE_TIGHT )
        {
          if ( start_id + end - start >= TT_VALID_GLYPH_COUNT( valid ) )
            FT_INVALID_GLYPH_ID;

          count = (FT_UInt32)( end - start + 1 );

          if ( start & ~0xFFFFU )
          {
            
            
            for ( ; count > 0; count--, start++ )
            {
              hi = (FT_UInt)( start >> 16 );
              lo = (FT_UInt)( start & 0xFFFFU );

              if ( (is32[hi >> 3] & ( 0x80 >> ( hi & 7 ) ) ) == 0 )
                FT_INVALID_DATA;

              if ( (is32[lo >> 3] & ( 0x80 >> ( lo & 7 ) ) ) == 0 )
                FT_INVALID_DATA;
            }
          }
          else
          {
            
            

            
            if ( end & ~0xFFFFU )
              FT_INVALID_DATA;

            for ( ; count > 0; count--, start++ )
            {
              lo = (FT_UInt)( start & 0xFFFFU );

              if ( (is32[lo >> 3] & ( 0x80 >> ( lo & 7 ) ) ) != 0 )
                FT_INVALID_DATA;
            }
          }
        }

        last = end;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap8_char_index( TT_CMap    cmap,
                       FT_UInt32  char_code )
  {
    FT_Byte*   table      = cmap->data;
    FT_UInt    result     = 0;
    FT_Byte*   p          = table + 8204;
    FT_UInt32  num_groups = TT_NEXT_ULONG( p );
    FT_UInt32  start, end, start_id;


    for ( ; num_groups > 0; num_groups-- )
    {
      start    = TT_NEXT_ULONG( p );
      end      = TT_NEXT_ULONG( p );
      start_id = TT_NEXT_ULONG( p );

      if ( char_code < start )
        break;

      if ( char_code <= end )
      {
        result = (FT_UInt)( start_id + char_code - start );
        break;
      }
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap8_char_next( TT_CMap     cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_UInt32  result     = 0;
    FT_UInt32  char_code  = *pchar_code + 1;
    FT_UInt    gindex     = 0;
    FT_Byte*   table      = cmap->data;
    FT_Byte*   p          = table + 8204;
    FT_UInt32  num_groups = TT_NEXT_ULONG( p );
    FT_UInt32  start, end, start_id;


    p = table + 8208;

    for ( ; num_groups > 0; num_groups-- )
    {
      start    = TT_NEXT_ULONG( p );
      end      = TT_NEXT_ULONG( p );
      start_id = TT_NEXT_ULONG( p );

      if ( char_code < start )
        char_code = start;

      if ( char_code <= end )
      {
        gindex = (FT_UInt)( char_code - start + start_id );
        if ( gindex != 0 )
        {
          result = char_code;
          goto Exit;
        }
      }
    }

  Exit:
    *pchar_code = result;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap8_get_info( TT_CMap       cmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 8;


    cmap_info->format = 8;
    cmap_info->language = (FT_ULong)TT_PEEK_ULONG( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap8_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap8_char_index,
      (FT_CMap_CharNextFunc) tt_cmap8_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    8,
    (TT_CMap_ValidateFunc)   tt_cmap8_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap8_get_info
  };

#endif 


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_10

  FT_CALLBACK_DEF( FT_Error )
  tt_cmap10_validate( FT_Byte*      table,
                      FT_Validator  valid )
  {
    FT_Byte*  p = table + 4;
    FT_ULong  length, count;


    if ( table + 20 > valid->limit )
      FT_INVALID_TOO_SHORT;

    length = TT_NEXT_ULONG( p );
    p      = table + 16;
    count  = TT_NEXT_ULONG( p );

    if ( table + length > valid->limit || length < 20 + count * 2 )
      FT_INVALID_TOO_SHORT;

    
    if ( valid->level >= FT_VALIDATE_TIGHT )
    {
      FT_UInt  gindex;


      for ( ; count > 0; count-- )
      {
        gindex = TT_NEXT_USHORT( p );
        if ( gindex >= TT_VALID_GLYPH_COUNT( valid ) )
          FT_INVALID_GLYPH_ID;
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap10_char_index( TT_CMap    cmap,
                        FT_UInt32  char_code )
  {
    FT_Byte*   table  = cmap->data;
    FT_UInt    result = 0;
    FT_Byte*   p      = table + 12;
    FT_UInt32  start  = TT_NEXT_ULONG( p );
    FT_UInt32  count  = TT_NEXT_ULONG( p );
    FT_UInt32  idx    = (FT_ULong)( char_code - start );


    if ( idx < count )
    {
      p     += 2 * idx;
      result = TT_PEEK_USHORT( p );
    }
    return result;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap10_char_next( TT_CMap     cmap,
                       FT_UInt32  *pchar_code )
  {
    FT_Byte*   table     = cmap->data;
    FT_UInt32  char_code = *pchar_code + 1;
    FT_UInt    gindex    = 0;
    FT_Byte*   p         = table + 12;
    FT_UInt32  start     = TT_NEXT_ULONG( p );
    FT_UInt32  count     = TT_NEXT_ULONG( p );
    FT_UInt32  idx;


    if ( char_code < start )
      char_code = start;

    idx = (FT_UInt32)( char_code - start );
    p  += 2 * idx;

    for ( ; idx < count; idx++ )
    {
      gindex = TT_NEXT_USHORT( p );
      if ( gindex != 0 )
        break;
      char_code++;
    }

    *pchar_code = char_code;
    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap10_get_info( TT_CMap       cmap,
                      TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 8;


    cmap_info->format = 10;
    cmap_info->language = (FT_ULong)TT_PEEK_ULONG( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap10_class_rec =
  {
    {
      sizeof ( TT_CMapRec ),

      (FT_CMap_InitFunc)     tt_cmap_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap10_char_index,
      (FT_CMap_CharNextFunc) tt_cmap10_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    10,
    (TT_CMap_ValidateFunc)   tt_cmap10_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap10_get_info
  };

#endif 


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_12

  typedef struct  TT_CMap12Rec_
  {
    TT_CMapRec  cmap;
    FT_Bool     valid;
    FT_ULong    cur_charcode;
    FT_UInt     cur_gindex;
    FT_ULong    cur_group;
    FT_ULong    num_groups;

  } TT_CMap12Rec, *TT_CMap12;


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap12_init( TT_CMap12  cmap,
                  FT_Byte*   table )
  {
    cmap->cmap.data  = table;

    table           += 12;
    cmap->num_groups = FT_PEEK_ULONG( table );

    cmap->valid      = 0;

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap12_validate( FT_Byte*      table,
                      FT_Validator  valid )
  {
    FT_Byte*   p;
    FT_ULong   length;
    FT_ULong   num_groups;


    if ( table + 16 > valid->limit )
      FT_INVALID_TOO_SHORT;

    p      = table + 4;
    length = TT_NEXT_ULONG( p );

    p          = table + 12;
    num_groups = TT_NEXT_ULONG( p );

    if ( table + length > valid->limit || length < 16 + 12 * num_groups )
      FT_INVALID_TOO_SHORT;

    
    {
      FT_ULong  n, start, end, start_id, last = 0;


      for ( n = 0; n < num_groups; n++ )
      {
        start    = TT_NEXT_ULONG( p );
        end      = TT_NEXT_ULONG( p );
        start_id = TT_NEXT_ULONG( p );

        if ( start > end )
          FT_INVALID_DATA;

        if ( n > 0 && start <= last )
          FT_INVALID_DATA;

        if ( valid->level >= FT_VALIDATE_TIGHT )
        {
          if ( start_id + end - start >= TT_VALID_GLYPH_COUNT( valid ) )
            FT_INVALID_GLYPH_ID;
        }

        last = end;
      }
    }

    return SFNT_Err_Ok;
  }


  
  
  
  static void
  tt_cmap12_next( TT_CMap12  cmap )
  {
    FT_Byte*  p;
    FT_ULong  start, end, start_id, char_code;
    FT_ULong  n;
    FT_UInt   gindex;


    if ( cmap->cur_charcode >= 0xFFFFFFFFUL )
      goto Fail;

    char_code = cmap->cur_charcode + 1;

    n = cmap->cur_group;

    for ( n = cmap->cur_group; n < cmap->num_groups; n++ )
    {
      p        = cmap->cmap.data + 16 + 12 * n;
      start    = TT_NEXT_ULONG( p );
      end      = TT_NEXT_ULONG( p );
      start_id = TT_PEEK_ULONG( p );

      if ( char_code < start )
        char_code = start;

      for ( ; char_code <= end; char_code++ )
      {
        gindex = (FT_UInt)( start_id + char_code - start );

        if ( gindex )
        {
          cmap->cur_charcode = char_code;;
          cmap->cur_gindex   = gindex;
          cmap->cur_group    = n;

          return;
        }
      }
    }

  Fail:
    cmap->valid = 0;
  }


  static FT_UInt
  tt_cmap12_char_map_binary( TT_CMap     cmap,
                             FT_UInt32*  pchar_code,
                             FT_Bool     next )
  {
    FT_UInt    gindex     = 0;
    FT_Byte*   p          = cmap->data + 12;
    FT_UInt32  num_groups = TT_PEEK_ULONG( p );
    FT_UInt32  char_code  = *pchar_code;
    FT_UInt32  start, end, start_id;
    FT_UInt32  max, min, mid;


    if ( !num_groups )
      return 0;

    
    mid = num_groups;
    end = 0xFFFFFFFFUL;

    if ( next )
      char_code++;

    min = 0;
    max = num_groups;

    
    while ( min < max )
    {
      mid = ( min + max ) >> 1;
      p   = cmap->data + 16 + 12 * mid;

      start = TT_NEXT_ULONG( p );
      end   = TT_NEXT_ULONG( p );

      if ( char_code < start )
        max = mid;
      else if ( char_code > end )
        min = mid + 1;
      else
      {
        start_id = TT_PEEK_ULONG( p );
        gindex = (FT_UInt)( start_id + char_code - start );

        break;
      }
    }

    if ( next )
    {
      TT_CMap12  cmap12 = (TT_CMap12)cmap;


      
      
      

      if ( char_code > end )
      {
        mid++;
        if ( mid == num_groups )
          return 0;
      }

      cmap12->valid        = 1;
      cmap12->cur_charcode = char_code;
      cmap12->cur_group    = mid;

      if ( !gindex )
      {
        tt_cmap12_next( cmap12 );

        if ( cmap12->valid )
          gindex = cmap12->cur_gindex;
      }
      else
        cmap12->cur_gindex = gindex;

      if ( gindex )
        *pchar_code = cmap12->cur_charcode;
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap12_char_index( TT_CMap    cmap,
                        FT_UInt32  char_code )
  {
    return tt_cmap12_char_map_binary( cmap, &char_code, 0 );
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap12_char_next( TT_CMap     cmap,
                       FT_UInt32  *pchar_code )
  {
    TT_CMap12  cmap12 = (TT_CMap12)cmap;
    FT_ULong   gindex;


    if ( cmap12->cur_charcode >= 0xFFFFFFFFUL )
      return 0;

    
    if ( cmap12->valid && cmap12->cur_charcode == *pchar_code )
    {
      tt_cmap12_next( cmap12 );
      if ( cmap12->valid )
      {
        gindex = cmap12->cur_gindex;
        if ( gindex )
          *pchar_code = cmap12->cur_charcode;
      }
      else
        gindex = 0;
    }
    else
      gindex = tt_cmap12_char_map_binary( cmap, pchar_code, 1 );

    return gindex;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap12_get_info( TT_CMap       cmap,
                      TT_CMapInfo  *cmap_info )
  {
    FT_Byte*  p = cmap->data + 8;


    cmap_info->format = 12;
    cmap_info->language = (FT_ULong)TT_PEEK_ULONG( p );

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap12_class_rec =
  {
    {
      sizeof ( TT_CMap12Rec ),

      (FT_CMap_InitFunc)     tt_cmap12_init,
      (FT_CMap_DoneFunc)     NULL,
      (FT_CMap_CharIndexFunc)tt_cmap12_char_index,
      (FT_CMap_CharNextFunc) tt_cmap12_char_next,

      NULL, NULL, NULL, NULL, NULL
    },
    12,
    (TT_CMap_ValidateFunc)   tt_cmap12_validate,
    (TT_CMap_Info_GetFunc)   tt_cmap12_get_info
  };

#endif 


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#ifdef TT_CONFIG_CMAP_FORMAT_14

  typedef struct  TT_CMap14Rec_
  {
    TT_CMapRec  cmap;
    FT_ULong    num_selectors;

    



    FT_UInt     max_results;
    FT_UInt32*  results;
    FT_Memory   memory;

  } TT_CMap14Rec, *TT_CMap14;


  FT_CALLBACK_DEF( void )
  tt_cmap14_done( TT_CMap14  cmap )
  {
    FT_Memory  memory = cmap->memory;


    cmap->max_results = 0;
    if ( memory != NULL && cmap->results != NULL )
      FT_FREE( cmap->results );
  }


  static FT_Error
  tt_cmap14_ensure( TT_CMap14  cmap,
                    FT_UInt    num_results,
                    FT_Memory  memory )
  {
    FT_UInt   old_max = cmap->max_results;
    FT_Error  error   = 0;


    if ( num_results > cmap->max_results )
    {
       cmap->memory = memory;

       if ( FT_QRENEW_ARRAY( cmap->results, old_max, num_results ) )
         return error;

       cmap->max_results = num_results;
    }

    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap14_init( TT_CMap14  cmap,
                  FT_Byte*   table )
  {
    cmap->cmap.data = table;

    table               += 6;
    cmap->num_selectors = FT_PEEK_ULONG( table );
    cmap->max_results   = 0;
    cmap->results       = NULL;

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap14_validate( FT_Byte*      table,
                      FT_Validator  valid )
  {
    FT_Byte*  p             = table + 2;
    FT_ULong  length        = TT_NEXT_ULONG( p );
    FT_ULong  num_selectors = TT_NEXT_ULONG( p );


    if ( table + length > valid->limit || length < 10 + 11 * num_selectors )
      FT_INVALID_TOO_SHORT;

    
    {
      


      FT_ULong  n, lastVarSel = 1;


      for ( n = 0; n < num_selectors; n++ )
      {
        FT_ULong  varSel    = TT_NEXT_UINT24( p );
        FT_ULong  defOff    = TT_NEXT_ULONG( p );
        FT_ULong  nondefOff = TT_NEXT_ULONG( p );


        if ( defOff >= length || nondefOff >= length )
          FT_INVALID_TOO_SHORT;

        if ( varSel < lastVarSel )
          FT_INVALID_DATA;

        lastVarSel = varSel + 1;

        
        
        if ( defOff != 0 )
        {
          FT_Byte*  defp      = table + defOff;
          FT_ULong  numRanges = TT_NEXT_ULONG( defp );
          FT_ULong  i;
          FT_ULong  lastBase  = 0;


          if ( defp + numRanges * 4 > valid->limit )
            FT_INVALID_TOO_SHORT;

          for ( i = 0; i < numRanges; ++i )
          {
            FT_ULong  base = TT_NEXT_UINT24( defp );
            FT_ULong  cnt  = FT_NEXT_BYTE( defp );


            if ( base + cnt >= 0x110000UL )              
              FT_INVALID_DATA;

            if ( base < lastBase )
              FT_INVALID_DATA;

            lastBase = base + cnt + 1U;
          }
        }

        
        if ( nondefOff != 0 ) {
          FT_Byte*  ndp         = table + nondefOff;
          FT_ULong  numMappings = TT_NEXT_ULONG( ndp );
          FT_ULong  i, lastUni = 0;


          if ( ndp + numMappings * 4 > valid->limit )
            FT_INVALID_TOO_SHORT;

          for ( i = 0; i < numMappings; ++i )
          {
            FT_ULong  uni = TT_NEXT_UINT24( ndp );
            FT_ULong  gid = TT_NEXT_USHORT( ndp );


            if ( uni >= 0x110000UL )                     
              FT_INVALID_DATA;

            if ( uni < lastUni )
              FT_INVALID_DATA;

            lastUni = uni + 1U;

            if ( valid->level >= FT_VALIDATE_TIGHT    &&
                 gid >= TT_VALID_GLYPH_COUNT( valid ) )
              FT_INVALID_GLYPH_ID;
          }
        }
      }
    }

    return SFNT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap14_char_index( TT_CMap    cmap,
                        FT_UInt32  char_code )
  {
    FT_UNUSED( cmap );
    FT_UNUSED( char_code );

    
    return 0;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap14_char_next( TT_CMap     cmap,
                       FT_UInt32  *pchar_code )
  {
    FT_UNUSED( cmap );

    
    *pchar_code = 0;
    return 0;
  }


  FT_CALLBACK_DEF( FT_Error )
  tt_cmap14_get_info( TT_CMap       cmap,
                      TT_CMapInfo  *cmap_info )
  {
    FT_UNUSED( cmap );

    cmap_info->format = 14;
    
    cmap_info->language = 0xFFFFFFFFUL;

    return SFNT_Err_Ok;
  }


  static FT_UInt
  tt_cmap14_char_map_def_binary( FT_Byte    *base,
                                 FT_UInt32   char_code )
  {
    FT_UInt32  numRanges = TT_PEEK_ULONG( base );
    FT_UInt32  max, min;


    min = 0;
    max = numRanges;

    base += 4;

    
    while ( min < max )
    {
      FT_UInt32  mid   = ( min + max ) >> 1;
      FT_Byte*   p     = base + 4 * mid;
      FT_ULong   start = TT_NEXT_UINT24( p );
      FT_UInt    cnt   = FT_NEXT_BYTE( p );


      if ( char_code < start )
        max = mid;
      else if ( char_code > start+cnt )
        min = mid + 1;
      else
        return TRUE;
    }

    return FALSE;
  }


  static FT_UInt
  tt_cmap14_char_map_nondef_binary( FT_Byte    *base,
                                    FT_UInt32   char_code )
  {
    FT_UInt32  numMappings = TT_PEEK_ULONG( base );
    FT_UInt32  max, min;


    min = 0;
    max = numMappings;

    base += 4;

    
    while ( min < max )
    {
      FT_UInt32  mid = ( min + max ) >> 1;
      FT_Byte*   p   = base + 5 * mid;
      FT_UInt32  uni = TT_NEXT_UINT24( p );


      if ( char_code < uni )
        max = mid;
      else if ( char_code > uni )
        min = mid + 1;
      else
        return TT_PEEK_USHORT( p );
    }

    return 0;
  }


  static FT_Byte*
  tt_cmap14_find_variant( FT_Byte    *base,
                          FT_UInt32   variantCode )
  {
    FT_UInt32  numVar = TT_PEEK_ULONG( base );
    FT_UInt32  max, min;


    min = 0;
    max = numVar;

    base += 4;

    
    while ( min < max )
    {
      FT_UInt32  mid    = ( min + max ) >> 1;
      FT_Byte*   p      = base + 11 * mid;
      FT_ULong   varSel = TT_NEXT_UINT24( p );


      if ( variantCode < varSel )
        max = mid;
      else if ( variantCode > varSel )
        min = mid + 1;
      else
        return p;
    }

    return NULL;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap14_char_var_index( TT_CMap   cmap,
                            TT_CMap   ucmap,
                            FT_ULong  charcode,
                            FT_ULong  variantSelector)
  {
    FT_Byte*  p = tt_cmap14_find_variant( cmap->data + 6, variantSelector );
    FT_ULong  defOff;
    FT_ULong  nondefOff;


    if ( !p )
      return 0;

    defOff    = TT_NEXT_ULONG( p );
    nondefOff = TT_PEEK_ULONG( p );

    if ( defOff != 0                                                    &&
         tt_cmap14_char_map_def_binary( cmap->data + defOff, charcode ) )
    {
      
      
      return ucmap->cmap.clazz->char_index( &ucmap->cmap, charcode );
    }

    if ( nondefOff != 0 )
      return tt_cmap14_char_map_nondef_binary( cmap->data + nondefOff,
                                               charcode );

    return 0;
  }


  FT_CALLBACK_DEF( FT_Int )
  tt_cmap14_char_var_isdefault( TT_CMap   cmap,
                                FT_ULong  charcode,
                                FT_ULong  variantSelector )
  {
    FT_Byte*  p = tt_cmap14_find_variant( cmap->data + 6, variantSelector );
    FT_ULong  defOff;
    FT_ULong  nondefOff;


    if ( !p )
      return -1;

    defOff    = TT_NEXT_ULONG( p );
    nondefOff = TT_NEXT_ULONG( p );

    if ( defOff != 0                                                    &&
         tt_cmap14_char_map_def_binary( cmap->data + defOff, charcode ) )
      return 1;

    if ( nondefOff != 0                                            &&
         tt_cmap14_char_map_nondef_binary( cmap->data + nondefOff,
                                           charcode ) != 0         )
      return 0;

    return -1;
  }


  FT_CALLBACK_DEF( FT_UInt32* )
  tt_cmap14_variants( TT_CMap    cmap,
                      FT_Memory  memory )
  {
    TT_CMap14   cmap14 = (TT_CMap14)cmap;
    FT_UInt     count  = cmap14->num_selectors;
    FT_Byte*    p      = cmap->data + 10;
    FT_UInt32*  result;
    FT_UInt     i;


    if ( tt_cmap14_ensure( cmap14, ( count + 1 ), memory ) )
      return NULL;

    result = cmap14->results;
    for ( i = 0; i < count; ++i )
    {
      result[i] = TT_NEXT_UINT24( p );
      p        += 8;
    }
    result[i] = 0;

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt32 * )
  tt_cmap14_char_variants( TT_CMap    cmap,
                           FT_Memory  memory,
                           FT_ULong   charCode )
  {
    TT_CMap14   cmap14 = (TT_CMap14)  cmap;
    FT_UInt     count  = cmap14->num_selectors;
    FT_Byte*    p      = cmap->data + 10;
    FT_UInt32*  q;


    if ( tt_cmap14_ensure( cmap14, ( count + 1 ), memory ) )
      return NULL;

    for ( q = cmap14->results; count > 0; --count )
    {
      FT_UInt32  varSel    = TT_NEXT_UINT24( p );
      FT_ULong   defOff    = TT_NEXT_ULONG( p );
      FT_ULong   nondefOff = TT_NEXT_ULONG( p );


      if ( ( defOff != 0                                               &&
             tt_cmap14_char_map_def_binary( cmap->data + defOff,
                                            charCode )                 ) ||
           ( nondefOff != 0                                            &&
             tt_cmap14_char_map_nondef_binary( cmap->data + nondefOff,
                                               charCode ) != 0         ) )
      {
        q[0] = varSel;
        q++;
      }
    }
    q[0] = 0;

    return cmap14->results;
  }


  static FT_UInt
  tt_cmap14_def_char_count( FT_Byte  *p )
  {
    FT_UInt32  numRanges = TT_NEXT_ULONG( p );
    FT_UInt    tot       = 0;


    p += 3;  
    for ( ; numRanges > 0; numRanges-- )
    {
      tot += 1 + p[0];
      p   += 4;
    }

    return tot;
  }


  static FT_UInt32*
  tt_cmap14_get_def_chars( TT_CMap     cmap,
                           FT_Byte*    p,
                           FT_Memory   memory )
  {
    TT_CMap14   cmap14 = (TT_CMap14) cmap;
    FT_UInt32   numRanges;
    FT_UInt     cnt;
    FT_UInt32*  q;


    cnt       = tt_cmap14_def_char_count( p );
    numRanges = TT_NEXT_ULONG( p );

    if ( tt_cmap14_ensure( cmap14, ( cnt + 1 ), memory ) )
      return NULL;

    for ( q = cmap14->results; numRanges > 0; --numRanges )
    {
      FT_UInt  uni = TT_NEXT_UINT24( p );


      cnt = FT_NEXT_BYTE( p ) + 1;
      do
      {
        q[0]  = uni;
        uni  += 1;
        q    += 1;
      } while ( --cnt != 0 );
    }
    q[0] = 0;

    return cmap14->results;
  }


  static FT_UInt*
  tt_cmap14_get_nondef_chars( TT_CMap     cmap,
                              FT_Byte    *p,
                              FT_Memory   memory )
  {
    TT_CMap14   cmap14 = (TT_CMap14) cmap;
    FT_UInt32   numMappings;
    FT_UInt     i;
    FT_UInt32  *ret;


    numMappings = TT_NEXT_ULONG( p );

    if ( tt_cmap14_ensure( cmap14, ( numMappings + 1 ), memory ) )
      return NULL;

    ret = cmap14->results;
    for ( i = 0; i < numMappings; ++i )
    {
      ret[i] = TT_NEXT_UINT24( p );
      p += 2;
    }
    ret[i] = 0;

    return ret;
  }


  FT_CALLBACK_DEF( FT_UInt32 * )
  tt_cmap14_variant_chars( TT_CMap    cmap,
                           FT_Memory  memory,
                           FT_ULong   variantSelector )
  {
    FT_Byte    *p  = tt_cmap14_find_variant( cmap->data + 6,
                                             variantSelector );
    FT_UInt32  *ret;
    FT_Int      i;
    FT_ULong    defOff;
    FT_ULong    nondefOff;


    if ( !p )
      return NULL;

    defOff    = TT_NEXT_ULONG( p );
    nondefOff = TT_NEXT_ULONG( p );

    if ( defOff == 0 && nondefOff == 0 )
      return NULL;

    if ( defOff == 0 )
      return tt_cmap14_get_nondef_chars( cmap, cmap->data + nondefOff,
                                         memory );
    else if ( nondefOff == 0 )
      return tt_cmap14_get_def_chars( cmap, cmap->data + defOff,
                                      memory );
    else
    {
      
      
      TT_CMap14  cmap14 = (TT_CMap14) cmap;
      FT_UInt32  numRanges;
      FT_UInt32  numMappings;
      FT_UInt32  duni;
      FT_UInt32  dcnt;
      FT_UInt32  nuni;
      FT_Byte*   dp;
      FT_UInt    di, ni, k;


      p  = cmap->data + nondefOff;
      dp = cmap->data + defOff;

      numMappings = TT_NEXT_ULONG( p );
      dcnt        = tt_cmap14_def_char_count( dp );
      numRanges   = TT_NEXT_ULONG( dp );

      if ( numMappings == 0 )
        return tt_cmap14_get_def_chars( cmap, cmap->data + defOff,
                                        memory );
      if ( dcnt == 0 )
        return tt_cmap14_get_nondef_chars( cmap, cmap->data + nondefOff,
                                           memory );

      if ( tt_cmap14_ensure( cmap14, ( dcnt + numMappings + 1 ), memory ) )
        return NULL;

      ret  = cmap14->results;
      duni = TT_NEXT_UINT24( dp );
      dcnt = FT_NEXT_BYTE( dp );
      di   = 1;
      nuni = TT_NEXT_UINT24( p );
      p   += 2;
      ni   = 1;
      i    = 0;

      for ( ;; )
      {
        if ( nuni > duni + dcnt )
        {
          for ( k = 0; k <= dcnt; ++k )
            ret[i++] = duni + k;

          ++di;

          if ( di > numRanges )
            break;

          duni = TT_NEXT_UINT24( dp );
          dcnt = FT_NEXT_BYTE( dp );
        }
        else
        {
          if ( nuni < duni )
            ret[i++] = nuni;
          
          
          ++ni;
          if ( ni > numMappings )
            break;

          nuni = TT_NEXT_UINT24( p );
          p += 2;
        }
      }

      if ( ni <= numMappings )
      {
        
        
        
        ret[i++] = nuni;
        while ( ni < numMappings )
        {
          ret[i++] = TT_NEXT_UINT24( p );
          p += 2;
          ++ni;
        }
      }
      else if ( di <= numRanges )
      {
        
        
        
        for ( k = 0; k <= dcnt; ++k )
          ret[i++] = duni + k;

        while ( di < numRanges )
        {
          duni = TT_NEXT_UINT24( dp );
          dcnt = FT_NEXT_BYTE( dp );

          for ( k = 0; k <= dcnt; ++k )
            ret[i++] = duni + k;
          ++di;
        }
      }

      ret[i] = 0;

      return ret;
    }
  }


  FT_CALLBACK_TABLE_DEF
  const TT_CMap_ClassRec  tt_cmap14_class_rec =
  {
    {
      sizeof ( TT_CMap14Rec ),

      (FT_CMap_InitFunc)     tt_cmap14_init,
      (FT_CMap_DoneFunc)     tt_cmap14_done,
      (FT_CMap_CharIndexFunc)tt_cmap14_char_index,
      (FT_CMap_CharNextFunc) tt_cmap14_char_next,

      
      (FT_CMap_CharVarIndexFunc)    tt_cmap14_char_var_index,
      (FT_CMap_CharVarIsDefaultFunc)tt_cmap14_char_var_isdefault,
      (FT_CMap_VariantListFunc)     tt_cmap14_variants,
      (FT_CMap_CharVariantListFunc) tt_cmap14_char_variants,
      (FT_CMap_VariantCharListFunc) tt_cmap14_variant_chars
    },
    14,
    (TT_CMap_ValidateFunc)tt_cmap14_validate,
    (TT_CMap_Info_GetFunc)tt_cmap14_get_info
  };

#endif 


  static const TT_CMap_Class  tt_cmap_classes[] =
  {
#ifdef TT_CONFIG_CMAP_FORMAT_0
    &tt_cmap0_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_2
    &tt_cmap2_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_4
    &tt_cmap4_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_6
    &tt_cmap6_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_8
    &tt_cmap8_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_10
    &tt_cmap10_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_12
    &tt_cmap12_class_rec,
#endif

#ifdef TT_CONFIG_CMAP_FORMAT_14
    &tt_cmap14_class_rec,
#endif

    NULL,
  };


  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_face_build_cmaps( TT_Face  face )
  {
    FT_Byte*           table = face->cmap_table;
    FT_Byte*           limit = table + face->cmap_size;
    FT_UInt volatile   num_cmaps;
    FT_Byte* volatile  p     = table;


    if ( p + 4 > limit )
      return SFNT_Err_Invalid_Table;

    
    if ( TT_NEXT_USHORT( p ) != 0 )
    {
      p -= 2;
      FT_ERROR(( "tt_face_build_cmaps: unsupported `cmap' table format = %d\n",
                 TT_PEEK_USHORT( p ) ));
      return SFNT_Err_Invalid_Table;
    }

    num_cmaps = TT_NEXT_USHORT( p );

    for ( ; num_cmaps > 0 && p + 8 <= limit; num_cmaps-- )
    {
      FT_CharMapRec  charmap;
      FT_UInt32      offset;


      charmap.platform_id = TT_NEXT_USHORT( p );
      charmap.encoding_id = TT_NEXT_USHORT( p );
      charmap.face        = FT_FACE( face );
      charmap.encoding    = FT_ENCODING_NONE;  
      offset              = TT_NEXT_ULONG( p );

      if ( offset && offset <= face->cmap_size - 2 )
      {
        FT_Byte* volatile              cmap   = table + offset;
        volatile FT_UInt               format = TT_PEEK_USHORT( cmap );
        const TT_CMap_Class* volatile  pclazz = tt_cmap_classes;
        TT_CMap_Class volatile         clazz;


        for ( ; *pclazz; pclazz++ )
        {
          clazz = *pclazz;
          if ( clazz->format == format )
          {
            volatile TT_ValidatorRec  valid;
            volatile FT_Error         error = SFNT_Err_Ok;


            ft_validator_init( FT_VALIDATOR( &valid ), cmap, limit,
                               FT_VALIDATE_DEFAULT );

            valid.num_glyphs = (FT_UInt)face->max_profile.numGlyphs;

            if ( ft_setjmp(
              *((ft_jmp_buf*)&FT_VALIDATOR( &valid )->jump_buffer) ) == 0 )
            {
              
              error = clazz->validate( cmap, FT_VALIDATOR( &valid ) );
            }

            if ( valid.validator.error == 0 )
            {
              FT_CMap  ttcmap;


              
              
              

              if ( !FT_CMap_New( (FT_CMap_Class)clazz,
                                 cmap, &charmap, &ttcmap ) )
              {
                
                
                ((TT_CMap)ttcmap)->flags = (FT_Int)error;
              }
            }
            else
            {
              FT_ERROR(( "tt_face_build_cmaps:" ));
              FT_ERROR(( " broken cmap sub-table ignored!\n" ));
            }
            break;
          }
        }

        if ( *pclazz == NULL )
        {
          FT_ERROR(( "tt_face_build_cmaps:" ));
          FT_ERROR(( " unsupported cmap sub-table ignored!\n" ));
        }
      }
    }

    return SFNT_Err_Ok;
  }


  FT_LOCAL( FT_Error )
  tt_get_cmap_info( FT_CharMap    charmap,
                    TT_CMapInfo  *cmap_info )
  {
    FT_CMap        cmap  = (FT_CMap)charmap;
    TT_CMap_Class  clazz = (TT_CMap_Class)cmap->clazz;


    return clazz->get_cmap_info( charmap, cmap_info );
  }



