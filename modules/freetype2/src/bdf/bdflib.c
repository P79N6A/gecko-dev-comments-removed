























  
  
  
  
  
  
  


#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "bdf.h"
#include "bdferror.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_bdflib


  
  
  
  
  


  static const bdf_options_t  _bdf_opts =
  {
    1,                
    1,                
    0,                
    BDF_PROPORTIONAL  
  };


  
  
  
  
  

  
  

  static const bdf_property_t  _bdf_properties[] =
  {
    { (char *)"ADD_STYLE_NAME",          BDF_ATOM,     1, { 0 } },
    { (char *)"AVERAGE_WIDTH",           BDF_INTEGER,  1, { 0 } },
    { (char *)"AVG_CAPITAL_WIDTH",       BDF_INTEGER,  1, { 0 } },
    { (char *)"AVG_LOWERCASE_WIDTH",     BDF_INTEGER,  1, { 0 } },
    { (char *)"CAP_HEIGHT",              BDF_INTEGER,  1, { 0 } },
    { (char *)"CHARSET_COLLECTIONS",     BDF_ATOM,     1, { 0 } },
    { (char *)"CHARSET_ENCODING",        BDF_ATOM,     1, { 0 } },
    { (char *)"CHARSET_REGISTRY",        BDF_ATOM,     1, { 0 } },
    { (char *)"COMMENT",                 BDF_ATOM,     1, { 0 } },
    { (char *)"COPYRIGHT",               BDF_ATOM,     1, { 0 } },
    { (char *)"DEFAULT_CHAR",            BDF_CARDINAL, 1, { 0 } },
    { (char *)"DESTINATION",             BDF_CARDINAL, 1, { 0 } },
    { (char *)"DEVICE_FONT_NAME",        BDF_ATOM,     1, { 0 } },
    { (char *)"END_SPACE",               BDF_INTEGER,  1, { 0 } },
    { (char *)"FACE_NAME",               BDF_ATOM,     1, { 0 } },
    { (char *)"FAMILY_NAME",             BDF_ATOM,     1, { 0 } },
    { (char *)"FIGURE_WIDTH",            BDF_INTEGER,  1, { 0 } },
    { (char *)"FONT",                    BDF_ATOM,     1, { 0 } },
    { (char *)"FONTNAME_REGISTRY",       BDF_ATOM,     1, { 0 } },
    { (char *)"FONT_ASCENT",             BDF_INTEGER,  1, { 0 } },
    { (char *)"FONT_DESCENT",            BDF_INTEGER,  1, { 0 } },
    { (char *)"FOUNDRY",                 BDF_ATOM,     1, { 0 } },
    { (char *)"FULL_NAME",               BDF_ATOM,     1, { 0 } },
    { (char *)"ITALIC_ANGLE",            BDF_INTEGER,  1, { 0 } },
    { (char *)"MAX_SPACE",               BDF_INTEGER,  1, { 0 } },
    { (char *)"MIN_SPACE",               BDF_INTEGER,  1, { 0 } },
    { (char *)"NORM_SPACE",              BDF_INTEGER,  1, { 0 } },
    { (char *)"NOTICE",                  BDF_ATOM,     1, { 0 } },
    { (char *)"PIXEL_SIZE",              BDF_INTEGER,  1, { 0 } },
    { (char *)"POINT_SIZE",              BDF_INTEGER,  1, { 0 } },
    { (char *)"QUAD_WIDTH",              BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_ASCENT",              BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_AVERAGE_WIDTH",       BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_AVG_CAPITAL_WIDTH",   BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_AVG_LOWERCASE_WIDTH", BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_CAP_HEIGHT",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_DESCENT",             BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_END_SPACE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_FIGURE_WIDTH",        BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_MAX_SPACE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_MIN_SPACE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_NORM_SPACE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_PIXEL_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_POINT_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_PIXELSIZE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_POINTSIZE",           BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_QUAD_WIDTH",          BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SMALL_CAP_SIZE",      BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_STRIKEOUT_ASCENT",    BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_STRIKEOUT_DESCENT",   BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUBSCRIPT_SIZE",      BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUBSCRIPT_X",         BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUBSCRIPT_Y",         BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUPERSCRIPT_SIZE",    BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUPERSCRIPT_X",       BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_SUPERSCRIPT_Y",       BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_UNDERLINE_POSITION",  BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_UNDERLINE_THICKNESS", BDF_INTEGER,  1, { 0 } },
    { (char *)"RAW_X_HEIGHT",            BDF_INTEGER,  1, { 0 } },
    { (char *)"RELATIVE_SETWIDTH",       BDF_CARDINAL, 1, { 0 } },
    { (char *)"RELATIVE_WEIGHT",         BDF_CARDINAL, 1, { 0 } },
    { (char *)"RESOLUTION",              BDF_INTEGER,  1, { 0 } },
    { (char *)"RESOLUTION_X",            BDF_CARDINAL, 1, { 0 } },
    { (char *)"RESOLUTION_Y",            BDF_CARDINAL, 1, { 0 } },
    { (char *)"SETWIDTH_NAME",           BDF_ATOM,     1, { 0 } },
    { (char *)"SLANT",                   BDF_ATOM,     1, { 0 } },
    { (char *)"SMALL_CAP_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"SPACING",                 BDF_ATOM,     1, { 0 } },
    { (char *)"STRIKEOUT_ASCENT",        BDF_INTEGER,  1, { 0 } },
    { (char *)"STRIKEOUT_DESCENT",       BDF_INTEGER,  1, { 0 } },
    { (char *)"SUBSCRIPT_SIZE",          BDF_INTEGER,  1, { 0 } },
    { (char *)"SUBSCRIPT_X",             BDF_INTEGER,  1, { 0 } },
    { (char *)"SUBSCRIPT_Y",             BDF_INTEGER,  1, { 0 } },
    { (char *)"SUPERSCRIPT_SIZE",        BDF_INTEGER,  1, { 0 } },
    { (char *)"SUPERSCRIPT_X",           BDF_INTEGER,  1, { 0 } },
    { (char *)"SUPERSCRIPT_Y",           BDF_INTEGER,  1, { 0 } },
    { (char *)"UNDERLINE_POSITION",      BDF_INTEGER,  1, { 0 } },
    { (char *)"UNDERLINE_THICKNESS",     BDF_INTEGER,  1, { 0 } },
    { (char *)"WEIGHT",                  BDF_CARDINAL, 1, { 0 } },
    { (char *)"WEIGHT_NAME",             BDF_ATOM,     1, { 0 } },
    { (char *)"X_HEIGHT",                BDF_INTEGER,  1, { 0 } },
    { (char *)"_MULE_BASELINE_OFFSET",   BDF_INTEGER,  1, { 0 } },
    { (char *)"_MULE_RELATIVE_COMPOSE",  BDF_INTEGER,  1, { 0 } },
  };

  static const unsigned long
  _num_bdf_properties = sizeof ( _bdf_properties ) /
                        sizeof ( _bdf_properties[0] );


  
#define ACMSG1   "FONT_ASCENT property missing.  " \
                 "Added `FONT_ASCENT %hd'.\n"
#define ACMSG2   "FONT_DESCENT property missing.  " \
                 "Added `FONT_DESCENT %hd'.\n"
#define ACMSG3   "Font width != actual width.  Old: %hd New: %hd.\n"
#define ACMSG4   "Font left bearing != actual left bearing.  " \
                 "Old: %hd New: %hd.\n"
#define ACMSG5   "Font ascent != actual ascent.  Old: %hd New: %hd.\n"
#define ACMSG6   "Font descent != actual descent.  Old: %hd New: %hd.\n"
#define ACMSG7   "Font height != actual height. Old: %hd New: %hd.\n"
#define ACMSG8   "Glyph scalable width (SWIDTH) adjustments made.\n"
#define ACMSG9   "SWIDTH field missing at line %ld.  Set automatically.\n"
#define ACMSG10  "DWIDTH field missing at line %ld.  Set to glyph width.\n"
#define ACMSG11  "SIZE bits per pixel field adjusted to %hd.\n"
#define ACMSG12  "Duplicate encoding %ld (%s) changed to unencoded.\n"
#define ACMSG13  "Glyph %ld extra rows removed.\n"
#define ACMSG14  "Glyph %ld extra columns removed.\n"
#define ACMSG15  "Incorrect glyph count: %ld indicated but %ld found.\n"
#define ACMSG16  "Glyph %ld missing columns padded with zero bits.\n"

  
#define ERRMSG1  "[line %ld] Missing `%s' line.\n"
#define ERRMSG2  "[line %ld] Font header corrupted or missing fields.\n"
#define ERRMSG3  "[line %ld] Font glyphs corrupted or missing fields.\n"
#define ERRMSG4  "[line %ld] BBX too big.\n"
#define ERRMSG5  "[line %ld] `%s' value too big.\n"
#define ERRMSG6  "[line %ld] Input line too long.\n"
#define ERRMSG7  "[line %ld] Font name too long.\n"
#define ERRMSG8  "[line %ld] Invalid `%s' value.\n"
#define ERRMSG9  "[line %ld] Invalid keyword.\n"

  
#define DBGMSG1  "  [%6ld] %s" /* no \n */
#define DBGMSG2  " (0x%lX)\n"


  
  
  
  
  

  


#define INITIAL_HT_SIZE  241

  typedef void
  (*hash_free_func)( hashnode  node );

  static hashnode*
  hash_bucket( const char*  key,
               hashtable*   ht )
  {
    const char*    kp  = key;
    unsigned long  res = 0;
    hashnode*      bp  = ht->table, *ndp;


    
    while ( *kp )
      res = ( res << 5 ) - res + *kp++;

    ndp = bp + ( res % ht->size );
    while ( *ndp )
    {
      kp = (*ndp)->key;
      if ( kp[0] == key[0] && ft_strcmp( kp, key ) == 0 )
        break;
      ndp--;
      if ( ndp < bp )
        ndp = bp + ( ht->size - 1 );
    }

    return ndp;
  }


  static FT_Error
  hash_rehash( hashtable*  ht,
               FT_Memory   memory )
  {
    hashnode*  obp = ht->table, *bp, *nbp;
    int        i, sz = ht->size;
    FT_Error   error = BDF_Err_Ok;


    ht->size <<= 1;
    ht->limit  = ht->size / 3;

    if ( FT_NEW_ARRAY( ht->table, ht->size ) )
      goto Exit;

    for ( i = 0, bp = obp; i < sz; i++, bp++ )
    {
      if ( *bp )
      {
        nbp = hash_bucket( (*bp)->key, ht );
        *nbp = *bp;
      }
    }
    FT_FREE( obp );

  Exit:
    return error;
  }


  static FT_Error
  hash_init( hashtable*  ht,
             FT_Memory   memory )
  {
    int       sz = INITIAL_HT_SIZE;
    FT_Error  error = BDF_Err_Ok;


    ht->size  = sz;
    ht->limit = sz / 3;
    ht->used  = 0;

    if ( FT_NEW_ARRAY( ht->table, sz ) )
      goto Exit;

  Exit:
    return error;
  }


  static void
  hash_free( hashtable*  ht,
             FT_Memory   memory )
  {
    if ( ht != 0 )
    {
      int        i, sz = ht->size;
      hashnode*  bp = ht->table;


      for ( i = 0; i < sz; i++, bp++ )
        FT_FREE( *bp );

      FT_FREE( ht->table );
    }
  }


  static FT_Error
  hash_insert( char*       key,
               size_t      data,
               hashtable*  ht,
               FT_Memory   memory )
  {
    hashnode  nn, *bp = hash_bucket( key, ht );
    FT_Error  error = BDF_Err_Ok;


    nn = *bp;
    if ( !nn )
    {
      if ( FT_NEW( nn ) )
        goto Exit;
      *bp = nn;

      nn->key  = key;
      nn->data = data;

      if ( ht->used >= ht->limit )
      {
        error = hash_rehash( ht, memory );
        if ( error )
          goto Exit;
      }
      ht->used++;
    }
    else
      nn->data = data;

  Exit:
    return error;
  }


  static hashnode
  hash_lookup( const char* key,
               hashtable*  ht )
  {
    hashnode *np = hash_bucket( key, ht );


    return *np;
  }


  
  
  
  
  


  

  typedef FT_Error
  (*_bdf_line_func_t)( char*          line,
                       unsigned long  linelen,
                       unsigned long  lineno,
                       void*          call_data,
                       void*          client_data );


  

  typedef struct  _bdf_list_t_
  {
    char**         field;
    unsigned long  size;
    unsigned long  used;
    FT_Memory      memory;

  } _bdf_list_t;


  

  typedef struct  _bdf_parse_t_
  {
    unsigned long   flags;
    unsigned long   cnt;
    unsigned long   row;

    short           minlb;
    short           maxlb;
    short           maxrb;
    short           maxas;
    short           maxds;

    short           rbearing;

    char*           glyph_name;
    long            glyph_enc;

    bdf_font_t*     font;
    bdf_options_t*  opts;

    unsigned long   have[34816]; 
                                 
    _bdf_list_t     list;

    FT_Memory       memory;

  } _bdf_parse_t;


#define setsbit( m, cc ) \
          ( m[(FT_Byte)(cc) >> 3] |= (FT_Byte)( 1 << ( (cc) & 7 ) ) )
#define sbitset( m, cc ) \
          ( m[(FT_Byte)(cc) >> 3]  & ( 1 << ( (cc) & 7 ) ) )


  static void
  _bdf_list_init( _bdf_list_t*  list,
                  FT_Memory     memory )
  {
    FT_ZERO( list );
    list->memory = memory;
  }


  static void
  _bdf_list_done( _bdf_list_t*  list )
  {
    FT_Memory  memory = list->memory;


    if ( memory )
    {
      FT_FREE( list->field );
      FT_ZERO( list );
    }
  }


  static FT_Error
  _bdf_list_ensure( _bdf_list_t*   list,
                    unsigned long  num_items ) 
  {
    FT_Error  error = BDF_Err_Ok;


    if ( num_items > list->size )
    {
      unsigned long  oldsize = list->size; 
      unsigned long  newsize = oldsize + ( oldsize >> 1 ) + 5;
      unsigned long  bigsize = (unsigned long)( FT_INT_MAX / sizeof ( char* ) );
      FT_Memory      memory  = list->memory;


      if ( oldsize == bigsize )
      {
        error = BDF_Err_Out_Of_Memory;
        goto Exit;
      }
      else if ( newsize < oldsize || newsize > bigsize )
        newsize = bigsize;

      if ( FT_RENEW_ARRAY( list->field, oldsize, newsize ) )
        goto Exit;

      list->size = newsize;
    }

  Exit:
    return error;
  }


  static void
  _bdf_list_shift( _bdf_list_t*   list,
                   unsigned long  n )
  {
    unsigned long  i, u;


    if ( list == 0 || list->used == 0 || n == 0 )
      return;

    if ( n >= list->used )
    {
      list->used = 0;
      return;
    }

    for ( u = n, i = 0; u < list->used; i++, u++ )
      list->field[i] = list->field[u];
    list->used -= n;
  }


  

  static const char  empty[1] = { 0 };      


  static char *
  _bdf_list_join( _bdf_list_t*    list,
                  int             c,
                  unsigned long  *alen )
  {
    unsigned long  i, j;
    char           *fp, *dp;


    *alen = 0;

    if ( list == 0 || list->used == 0 )
      return 0;

    dp = list->field[0];
    for ( i = j = 0; i < list->used; i++ )
    {
      fp = list->field[i];
      while ( *fp )
        dp[j++] = *fp++;

      if ( i + 1 < list->used )
        dp[j++] = (char)c;
    }
    if ( dp != empty )
      dp[j] = 0;

    *alen = j;
    return dp;
  }


  
  
  

  static FT_Error
  _bdf_list_split( _bdf_list_t*   list,
                   char*          separators,
                   char*          line,
                   unsigned long  linelen )
  {
    int       mult, final_empty;
    char      *sp, *ep, *end;
    char      seps[32];
    FT_Error  error = BDF_Err_Ok;


    
    list->used = 0;
    if ( list->size )
    {
      list->field[0] = (char*)empty;
      list->field[1] = (char*)empty;
      list->field[2] = (char*)empty;
      list->field[3] = (char*)empty;
    }

    
    if ( linelen == 0 || line[0] == 0 )
      goto Exit;

    
    
    
    if ( separators == 0 || *separators == 0 )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    
    FT_MEM_ZERO( seps, 32 );

    
    
    
    for ( mult = 0, sp = separators; sp && *sp; sp++ )
    {
      if ( *sp == '+' && *( sp + 1 ) == 0 )
        mult = 1;
      else
        setsbit( seps, *sp );
    }

    
    for ( final_empty = 0, sp = ep = line, end = sp + linelen;
          sp < end && *sp; )
    {
      
      for ( ; *ep && !sbitset( seps, *ep ); ep++ )
        ;

      
      if ( list->used == list->size )
      {
        error = _bdf_list_ensure( list, list->used + 1 );
        if ( error )
          goto Exit;
      }

      
      list->field[list->used++] = ( ep > sp ) ? sp : (char*)empty;

      sp = ep;

      if ( mult )
      {
        
        
        for ( ; *ep && sbitset( seps, *ep ); ep++ )
          *ep = 0;
      }
      else if ( *ep != 0 )
        
        
        *ep++ = 0;

      final_empty = ( ep > sp && *ep == 0 );
      sp = ep;
    }

    
    if ( list->used + final_empty >= list->size )
    {
      error = _bdf_list_ensure( list, list->used + final_empty + 1 );
      if ( error )
        goto Exit;
    }

    if ( final_empty )
      list->field[list->used++] = (char*)empty;

    list->field[list->used] = 0;

  Exit:
    return error;
  }


#define NO_SKIP  256  /* this value cannot be stored in a 'char' */


  static FT_Error
  _bdf_readstream( FT_Stream         stream,
                   _bdf_line_func_t  callback,
                   void*             client_data,
                   unsigned long    *lno )
  {
    _bdf_line_func_t  cb;
    unsigned long     lineno, buf_size;
    int               refill, hold, to_skip;
    ptrdiff_t         bytes, start, end, cursor, avail;
    char*             buf = 0;
    FT_Memory         memory = stream->memory;
    FT_Error          error = BDF_Err_Ok;


    if ( callback == 0 )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    
    buf_size = 1024;

    if ( FT_NEW_ARRAY( buf, buf_size ) )
      goto Exit;

    cb      = callback;
    lineno  = 1;
    buf[0]  = 0;
    start   = 0;
    end     = 0;
    avail   = 0;
    cursor  = 0;
    refill  = 1;
    to_skip = NO_SKIP;
    bytes   = 0;        

    for (;;)
    {
      if ( refill )
      {
        bytes  = (ptrdiff_t)FT_Stream_TryRead(
                   stream, (FT_Byte*)buf + cursor,
                   (FT_ULong)( buf_size - cursor ) );
        avail  = cursor + bytes;
        cursor = 0;
        refill = 0;
      }

      end = start;

      
      if ( start < avail && buf[start] == to_skip )
      {
        start  += 1;
        to_skip = NO_SKIP;
        continue;
      }

      
      while ( end < avail && buf[end] != '\n' && buf[end] != '\r' )
        end++;

      
      
      if ( end >= avail )
      {
        if ( bytes == 0 )  
          break;           

        if ( start == 0 )
        {
          
          
          FT_ULong  new_size;


          if ( buf_size >= 65536UL )  
          {
            FT_ERROR(( "_bdf_readstream: " ERRMSG6, lineno ));
            error = BDF_Err_Invalid_Argument;
            goto Exit;
          }

          new_size = buf_size * 2;
          if ( FT_RENEW_ARRAY( buf, buf_size, new_size ) )
            goto Exit;

          cursor   = buf_size;
          buf_size = new_size;
        }
        else
        {
          bytes = avail - start;

          FT_MEM_COPY( buf, buf + start, bytes );

          cursor = bytes;
          avail -= bytes;
          start  = 0;
        }
        refill = 1;
        continue;
      }

      
      hold     = buf[end];
      buf[end] = 0;

      
      if ( buf[start] != '#' && buf[start] != 0x1a && end > start )
      {
        error = (*cb)( buf + start, end - start, lineno,
                       (void*)&cb, client_data );
        
        if ( error == -1 )
          error = (*cb)( buf + start, end - start, lineno,
                         (void*)&cb, client_data );
        if ( error )
          break;
      }

      lineno  += 1;
      buf[end] = (char)hold;
      start    = end + 1;

      if ( hold == '\n' )
        to_skip = '\r';
      else if ( hold == '\r' )
        to_skip = '\n';
      else
        to_skip = NO_SKIP;
    }

    *lno = lineno;

  Exit:
    FT_FREE( buf );
    return error;
  }


  

  static const unsigned char  a2i[128] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  static const unsigned char  odigits[32] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  static const unsigned char  ddigits[32] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  static const unsigned char  hdigits[32] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03,
    0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };


#define isdigok( m, d )  (m[(d) >> 3] & ( 1 << ( (d) & 7 ) ) )


  
  static unsigned long
  _bdf_atoul( char*   s,
              char**  end,
              int     base )
  {
    unsigned long         v;
    const unsigned char*  dmap;


    if ( s == 0 || *s == 0 )
      return 0;

    
    switch ( base )
    {
    case 8:
      dmap = odigits;
      break;
    case 16:
      dmap = hdigits;
      break;
    default:
      base = 10;
      dmap = ddigits;
      break;
    }

    
    if ( *s == '0'                                  &&
         ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) )
    {
      base = 16;
      dmap = hdigits;
      s   += 2;
    }

    for ( v = 0; isdigok( dmap, *s ); s++ )
      v = v * base + a2i[(int)*s];

    if ( end != 0 )
      *end = s;

    return v;
  }


  
  static long
  _bdf_atol( char*   s,
             char**  end,
             int     base )
  {
    long                  v, neg;
    const unsigned char*  dmap;


    if ( s == 0 || *s == 0 )
      return 0;

    
    switch ( base )
    {
    case 8:
      dmap = odigits;
      break;
    case 16:
      dmap = hdigits;
      break;
    default:
      base = 10;
      dmap = ddigits;
      break;
    }

    
    neg = 0;
    if ( *s == '-' )
    {
      s++;
      neg = 1;
    }

    
    if ( *s == '0'                                  &&
         ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) )
    {
      base = 16;
      dmap = hdigits;
      s   += 2;
    }

    for ( v = 0; isdigok( dmap, *s ); s++ )
      v = v * base + a2i[(int)*s];

    if ( end != 0 )
      *end = s;

    return ( !neg ) ? v : -v;
  }


  
  static short
  _bdf_atos( char*   s,
             char**  end,
             int     base )
  {
    short                 v, neg;
    const unsigned char*  dmap;


    if ( s == 0 || *s == 0 )
      return 0;

    
    switch ( base )
    {
    case 8:
      dmap = odigits;
      break;
    case 16:
      dmap = hdigits;
      break;
    default:
      base = 10;
      dmap = ddigits;
      break;
    }

    
    neg = 0;
    if ( *s == '-' )
    {
      s++;
      neg = 1;
    }

    
    if ( *s == '0'                                  &&
         ( *( s + 1 ) == 'x' || *( s + 1 ) == 'X' ) )
    {
      base = 16;
      dmap = hdigits;
      s   += 2;
    }

    for ( v = 0; isdigok( dmap, *s ); s++ )
      v = (short)( v * base + a2i[(int)*s] );

    if ( end != 0 )
      *end = s;

    return (short)( ( !neg ) ? v : -v );
  }


  
  static int
  by_encoding( const void*  a,
               const void*  b )
  {
    bdf_glyph_t  *c1, *c2;


    c1 = (bdf_glyph_t *)a;
    c2 = (bdf_glyph_t *)b;

    if ( c1->encoding < c2->encoding )
      return -1;

    if ( c1->encoding > c2->encoding )
      return 1;

    return 0;
  }


  static FT_Error
  bdf_create_property( char*        name,
                       int          format,
                       bdf_font_t*  font )
  {
    size_t           n;
    bdf_property_t*  p;
    FT_Memory        memory = font->memory;
    FT_Error         error = BDF_Err_Ok;


    
    
    
    if ( hash_lookup( name, &(font->proptbl) ) )
      goto Exit;

    if ( FT_RENEW_ARRAY( font->user_props,
                         font->nuser_props,
                         font->nuser_props + 1 ) )
      goto Exit;

    p = font->user_props + font->nuser_props;
    FT_ZERO( p );

    n = ft_strlen( name ) + 1;
    if ( n > FT_ULONG_MAX )
      return BDF_Err_Invalid_Argument;

    if ( FT_NEW_ARRAY( p->name, n ) )
      goto Exit;

    FT_MEM_COPY( (char *)p->name, name, n );

    p->format  = format;
    p->builtin = 0;

    n = _num_bdf_properties + font->nuser_props;

    error = hash_insert( p->name, n, &(font->proptbl), memory );
    if ( error )
      goto Exit;

    font->nuser_props++;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( bdf_property_t * )
  bdf_get_property( char*        name,
                    bdf_font_t*  font )
  {
    hashnode  hn;
    size_t    propid;


    if ( name == 0 || *name == 0 )
      return 0;

    if ( ( hn = hash_lookup( name, &(font->proptbl) ) ) == 0 )
      return 0;

    propid = hn->data;
    if ( propid >= _num_bdf_properties )
      return font->user_props + ( propid - _num_bdf_properties );

    return (bdf_property_t*)_bdf_properties + propid;
  }


  
  
  
  
  


  

#define _BDF_START      0x0001
#define _BDF_FONT_NAME  0x0002
#define _BDF_SIZE       0x0004
#define _BDF_FONT_BBX   0x0008
#define _BDF_PROPS      0x0010
#define _BDF_GLYPHS     0x0020
#define _BDF_GLYPH      0x0040
#define _BDF_ENCODING   0x0080
#define _BDF_SWIDTH     0x0100
#define _BDF_DWIDTH     0x0200
#define _BDF_BBX        0x0400
#define _BDF_BITMAP     0x0800

#define _BDF_SWIDTH_ADJ  0x1000

#define _BDF_GLYPH_BITS ( _BDF_GLYPH    | \
                          _BDF_ENCODING | \
                          _BDF_SWIDTH   | \
                          _BDF_DWIDTH   | \
                          _BDF_BBX      | \
                          _BDF_BITMAP   )

#define _BDF_GLYPH_WIDTH_CHECK   0x40000000UL
#define _BDF_GLYPH_HEIGHT_CHECK  0x80000000UL


  static FT_Error
  _bdf_add_comment( bdf_font_t*    font,
                    char*          comment,
                    unsigned long  len )
  {
    char*      cp;
    FT_Memory  memory = font->memory;
    FT_Error   error = BDF_Err_Ok;


    if ( FT_RENEW_ARRAY( font->comments,
                         font->comments_len,
                         font->comments_len + len + 1 ) )
      goto Exit;

    cp = font->comments + font->comments_len;

    FT_MEM_COPY( cp, comment, len );
    cp[len] = '\n';

    font->comments_len += len + 1;

  Exit:
    return error;
  }


  
  
  static FT_Error
  _bdf_set_default_spacing( bdf_font_t*     font,
                            bdf_options_t*  opts,
                            unsigned long   lineno )
  {
    size_t       len;
    char         name[256];
    _bdf_list_t  list;
    FT_Memory    memory;
    FT_Error     error = BDF_Err_Ok;


    if ( font == 0 || font->name == 0 || font->name[0] == 0 )
    {
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    memory = font->memory;

    _bdf_list_init( &list, memory );

    font->spacing = opts->font_spacing;

    len = ft_strlen( font->name ) + 1;
    
    if ( len >= 256 )
    {
      FT_ERROR(( "_bdf_set_default_spacing: " ERRMSG7, lineno ));
      error = BDF_Err_Invalid_Argument;
      goto Exit;
    }

    FT_MEM_COPY( name, font->name, len );

    error = _bdf_list_split( &list, (char *)"-", name, len );
    if ( error )
      goto Fail;

    if ( list.used == 15 )
    {
      switch ( list.field[11][0] )
      {
      case 'C':
      case 'c':
        font->spacing = BDF_CHARCELL;
        break;
      case 'M':
      case 'm':
        font->spacing = BDF_MONOWIDTH;
        break;
      case 'P':
      case 'p':
        font->spacing = BDF_PROPORTIONAL;
        break;
      }
    }

  Fail:
    _bdf_list_done( &list );

  Exit:
    return error;
  }


  
  
  static int
  _bdf_is_atom( char*          line,
                unsigned long  linelen,
                char**         name,
                char**         value,
                bdf_font_t*    font )
  {
    int              hold;
    char             *sp, *ep;
    bdf_property_t*  p;


    *name = sp = ep = line;

    while ( *ep && *ep != ' ' && *ep != '\t' )
      ep++;

    hold = -1;
    if ( *ep )
    {
      hold = *ep;
      *ep  = 0;
    }

    p = bdf_get_property( sp, font );

    
    if ( hold != -1 )
      *ep = (char)hold;

    
    if ( p && p->format != BDF_ATOM )
      return 0;

    
    
    sp = ep;
    ep = line + linelen;

    
    if ( *sp )
      *sp++ = 0;
    while ( *sp                           &&
            ( *sp == ' ' || *sp == '\t' ) )
      sp++;

    
    if ( *sp == '"' )
      sp++;
    *value = sp;

    
    while ( ep > sp                                       &&
            ( *( ep - 1 ) == ' ' || *( ep - 1 ) == '\t' ) )
      *--ep = 0;

    
    if ( ep > sp && *( ep - 1 ) == '"' )
      *--ep = 0;

    return 1;
  }


  static FT_Error
  _bdf_add_property( bdf_font_t*    font,
                     char*          name,
                     char*          value,
                     unsigned long  lineno )
  {
    size_t          propid;
    hashnode        hn;
    bdf_property_t  *prop, *fp;
    FT_Memory       memory = font->memory;
    FT_Error        error = BDF_Err_Ok;


    
    if ( ( hn = hash_lookup( name, (hashtable *)font->internal ) ) != 0 )
    {
      
      
      fp = font->props + hn->data;

      switch ( fp->format )
      {
      case BDF_ATOM:
        
        FT_FREE( fp->value.atom );

        if ( value && value[0] != 0 )
        {
          if ( FT_STRDUP( fp->value.atom, value ) )
            goto Exit;
        }
        break;

      case BDF_INTEGER:
        fp->value.l = _bdf_atol( value, 0, 10 );
        break;

      case BDF_CARDINAL:
        fp->value.ul = _bdf_atoul( value, 0, 10 );
        break;

      default:
        ;
      }

      goto Exit;
    }

    
    
    hn = hash_lookup( name, &(font->proptbl) );
    if ( hn == 0 )
    {
      error = bdf_create_property( name, BDF_ATOM, font );
      if ( error )
        goto Exit;
      hn = hash_lookup( name, &(font->proptbl) );
    }

    
    if ( font->props_used == font->props_size )
    {
      if ( font->props_size == 0 )
      {
        if ( FT_NEW_ARRAY( font->props, 1 ) )
          goto Exit;
      }
      else
      {
        if ( FT_RENEW_ARRAY( font->props,
                             font->props_size,
                             font->props_size + 1 ) )
          goto Exit;
      }

      fp = font->props + font->props_size;
      FT_MEM_ZERO( fp, sizeof ( bdf_property_t ) );
      font->props_size++;
    }

    propid = hn->data;
    if ( propid >= _num_bdf_properties )
      prop = font->user_props + ( propid - _num_bdf_properties );
    else
      prop = (bdf_property_t*)_bdf_properties + propid;

    fp = font->props + font->props_used;

    fp->name    = prop->name;
    fp->format  = prop->format;
    fp->builtin = prop->builtin;

    switch ( prop->format )
    {
    case BDF_ATOM:
      fp->value.atom = 0;
      if ( value != 0 && value[0] )
      {
        if ( FT_STRDUP( fp->value.atom, value ) )
          goto Exit;
      }
      break;

    case BDF_INTEGER:
      fp->value.l = _bdf_atol( value, 0, 10 );
      break;

    case BDF_CARDINAL:
      fp->value.ul = _bdf_atoul( value, 0, 10 );
      break;
    }

    
    
    if ( ft_memcmp( name, "COMMENT", 7 ) != 0 )
    {
      
      error = hash_insert( fp->name,
                           font->props_used,
                           (hashtable *)font->internal,
                           memory );
      if ( error )
        goto Exit;
    }

    font->props_used++;

    
    
    
    
    
    if ( ft_memcmp( name, "DEFAULT_CHAR", 12 ) == 0 )
      font->default_char = fp->value.l;
    else if ( ft_memcmp( name, "FONT_ASCENT", 11 ) == 0 )
      font->font_ascent = fp->value.l;
    else if ( ft_memcmp( name, "FONT_DESCENT", 12 ) == 0 )
      font->font_descent = fp->value.l;
    else if ( ft_memcmp( name, "SPACING", 7 ) == 0 )
    {
      if ( !fp->value.atom )
      {
        FT_ERROR(( "_bdf_add_property: " ERRMSG8, lineno, "SPACING" ));
        error = BDF_Err_Invalid_File_Format;
        goto Exit;
      }

      if ( fp->value.atom[0] == 'p' || fp->value.atom[0] == 'P' )
        font->spacing = BDF_PROPORTIONAL;
      else if ( fp->value.atom[0] == 'm' || fp->value.atom[0] == 'M' )
        font->spacing = BDF_MONOWIDTH;
      else if ( fp->value.atom[0] == 'c' || fp->value.atom[0] == 'C' )
        font->spacing = BDF_CHARCELL;
    }

  Exit:
    return error;
  }


  static const unsigned char nibble_mask[8] =
  {
    0xFF, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE
  };


  
  static FT_Error
  _bdf_parse_glyphs( char*          line,
                     unsigned long  linelen,
                     unsigned long  lineno,
                     void*          call_data,
                     void*          client_data )
  {
    int                c, mask_index;
    char*              s;
    unsigned char*     bp;
    unsigned long      i, slen, nibbles;

    _bdf_parse_t*      p;
    bdf_glyph_t*       glyph;
    bdf_font_t*        font;

    FT_Memory          memory;
    FT_Error           error = BDF_Err_Ok;

    FT_UNUSED( call_data );
    FT_UNUSED( lineno );        


    p = (_bdf_parse_t *)client_data;

    font   = p->font;
    memory = font->memory;

    
    if ( ft_memcmp( line, "COMMENT", 7 ) == 0 )
    {
      linelen -= 7;

      s = line + 7;
      if ( *s != 0 )
      {
        s++;
        linelen--;
      }
      error = _bdf_add_comment( p->font, s, linelen );
      goto Exit;
    }

    
    if ( !( p->flags & _BDF_GLYPHS ) )
    {
      if ( ft_memcmp( line, "CHARS", 5 ) != 0 )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "CHARS" ));
        error = BDF_Err_Missing_Chars_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      p->cnt = font->glyphs_size = _bdf_atoul( p->list.field[1], 0, 10 );

      
      if ( p->cnt == 0 )
        font->glyphs_size = 64;

      
      
      if ( p->cnt >= 0x110000UL )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG5, lineno, "CHARS" ));
        error = BDF_Err_Invalid_Argument;
        goto Exit;
      }

      if ( FT_NEW_ARRAY( font->glyphs, font->glyphs_size ) )
        goto Exit;

      p->flags |= _BDF_GLYPHS;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "ENDFONT", 7 ) == 0 )
    {
      
      ft_qsort( (char *)font->glyphs,
                font->glyphs_used,
                sizeof ( bdf_glyph_t ),
                by_encoding );

      p->flags &= ~_BDF_START;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "ENDCHAR", 7 ) == 0 )
    {
      p->glyph_enc = 0;
      p->flags    &= ~_BDF_GLYPH_BITS;

      goto Exit;
    }

    
    
    if ( ( p->flags & _BDF_GLYPH )     &&
         p->glyph_enc            == -1 &&
         p->opts->keep_unencoded == 0  )
      goto Exit;

    
    if ( ft_memcmp( line, "STARTCHAR", 9 ) == 0 )
    {
      
      
      FT_FREE( p->glyph_name );

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      _bdf_list_shift( &p->list, 1 );

      s = _bdf_list_join( &p->list, ' ', &slen );

      if ( !s )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG8, lineno, "STARTCHAR" ));
        error = BDF_Err_Invalid_File_Format;
        goto Exit;
      }

      if ( FT_NEW_ARRAY( p->glyph_name, slen + 1 ) )
        goto Exit;

      FT_MEM_COPY( p->glyph_name, s, slen + 1 );

      p->flags |= _BDF_GLYPH;

      FT_TRACE4(( DBGMSG1, lineno, s ));

      goto Exit;
    }

    
    if ( ft_memcmp( line, "ENCODING", 8 ) == 0 )
    {
      if ( !( p->flags & _BDF_GLYPH ) )
      {
        
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "STARTCHAR" ));
        error = BDF_Err_Missing_Startchar_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      p->glyph_enc = _bdf_atol( p->list.field[1], 0, 10 );

      
      
      if ( p->glyph_enc < -1 )
        p->glyph_enc = -1;

      
      if ( p->glyph_enc == -1 && p->list.used > 2 )
        p->glyph_enc = _bdf_atol( p->list.field[2], 0, 10 );

      FT_TRACE4(( DBGMSG2, p->glyph_enc ));

      
      
      if ( p->glyph_enc > 0                               &&
           (size_t)p->glyph_enc >= sizeof ( p->have ) * 8 )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG5, lineno, "ENCODING" ));
        error = BDF_Err_Invalid_File_Format;
        goto Exit;
      }

      
      
      
      if ( p->glyph_enc >= 0 )
      {
        if ( _bdf_glyph_modified( p->have, p->glyph_enc ) )
        {
          
          
          FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG12,
                      p->glyph_enc, p->glyph_name ));
          p->glyph_enc = -1;
          font->modified = 1;
        }
        else
          _bdf_set_glyph_modified( p->have, p->glyph_enc );
      }

      if ( p->glyph_enc >= 0 )
      {
        
        
        if ( font->glyphs_used == font->glyphs_size )
        {
          if ( FT_RENEW_ARRAY( font->glyphs,
                               font->glyphs_size,
                               font->glyphs_size + 64 ) )
            goto Exit;

          font->glyphs_size += 64;
        }

        glyph           = font->glyphs + font->glyphs_used++;
        glyph->name     = p->glyph_name;
        glyph->encoding = p->glyph_enc;

        
        p->glyph_name = 0;
      }
      else
      {
        
        
        if ( p->opts->keep_unencoded != 0 )
        {
          
          if ( font->unencoded_used == font->unencoded_size )
          {
            if ( FT_RENEW_ARRAY( font->unencoded ,
                                 font->unencoded_size,
                                 font->unencoded_size + 4 ) )
              goto Exit;

            font->unencoded_size += 4;
          }

          glyph           = font->unencoded + font->unencoded_used;
          glyph->name     = p->glyph_name;
          glyph->encoding = font->unencoded_used++;
        }
        else
          
          
          FT_FREE( p->glyph_name );

        p->glyph_name = 0;
      }

      
      
      p->flags &= ~( _BDF_GLYPH_WIDTH_CHECK | _BDF_GLYPH_HEIGHT_CHECK );

      p->flags |= _BDF_ENCODING;

      goto Exit;
    }

    
    if ( p->glyph_enc == -1 )
      glyph = font->unencoded + ( font->unencoded_used - 1 );
    else
      glyph = font->glyphs + ( font->glyphs_used - 1 );

    
    if ( p->flags & _BDF_BITMAP )
    {
      
      
      if ( p->row >= (unsigned long)glyph->bbx.height )
      {
        if ( !( p->flags & _BDF_GLYPH_HEIGHT_CHECK ) )
        {
          FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG13, glyph->encoding ));
          p->flags |= _BDF_GLYPH_HEIGHT_CHECK;
          font->modified = 1;
        }

        goto Exit;
      }

      
      
      nibbles = glyph->bpr << 1;
      bp      = glyph->bitmap + p->row * glyph->bpr;

      for ( i = 0; i < nibbles; i++ )
      {
        c = line[i];
        if ( !isdigok( hdigits, c ) )
          break;
        *bp = (FT_Byte)( ( *bp << 4 ) + a2i[c] );
        if ( i + 1 < nibbles && ( i & 1 ) )
          *++bp = 0;
      }

      
      
      if ( i < nibbles                            &&
           !( p->flags & _BDF_GLYPH_WIDTH_CHECK ) )
      {
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG16, glyph->encoding ));
        p->flags       |= _BDF_GLYPH_WIDTH_CHECK;
        font->modified  = 1;
      }

      
      mask_index = ( glyph->bbx.width * p->font->bpp ) & 7;
      if ( glyph->bbx.width )
        *bp &= nibble_mask[mask_index];

      
      if ( i == nibbles                           &&
           isdigok( hdigits, line[nibbles] )      &&
           !( p->flags & _BDF_GLYPH_WIDTH_CHECK ) )
      {
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG14, glyph->encoding ));
        p->flags       |= _BDF_GLYPH_WIDTH_CHECK;
        font->modified  = 1;
      }

      p->row++;
      goto Exit;
    }

    
    if ( ft_memcmp( line, "SWIDTH", 6 ) == 0 )
    {
      if ( !( p->flags & _BDF_ENCODING ) )
        goto Missing_Encoding;

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      glyph->swidth = (unsigned short)_bdf_atoul( p->list.field[1], 0, 10 );
      p->flags |= _BDF_SWIDTH;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "DWIDTH", 6 ) == 0 )
    {
      if ( !( p->flags & _BDF_ENCODING ) )
        goto Missing_Encoding;

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      glyph->dwidth = (unsigned short)_bdf_atoul( p->list.field[1], 0, 10 );

      if ( !( p->flags & _BDF_SWIDTH ) )
      {
        
        
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG9, lineno ));

        glyph->swidth = (unsigned short)FT_MulDiv(
                          glyph->dwidth, 72000L,
                          (FT_Long)( font->point_size *
                                     font->resolution_x ) );
      }

      p->flags |= _BDF_DWIDTH;
      goto Exit;
    }

    
    if ( ft_memcmp( line, "BBX", 3 ) == 0 )
    {
      if ( !( p->flags & _BDF_ENCODING ) )
        goto Missing_Encoding;

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      glyph->bbx.width    = _bdf_atos( p->list.field[1], 0, 10 );
      glyph->bbx.height   = _bdf_atos( p->list.field[2], 0, 10 );
      glyph->bbx.x_offset = _bdf_atos( p->list.field[3], 0, 10 );
      glyph->bbx.y_offset = _bdf_atos( p->list.field[4], 0, 10 );

      
      glyph->bbx.ascent  = (short)( glyph->bbx.height + glyph->bbx.y_offset );
      glyph->bbx.descent = (short)( -glyph->bbx.y_offset );

      
      
      p->maxas    = (short)FT_MAX( glyph->bbx.ascent, p->maxas );
      p->maxds    = (short)FT_MAX( glyph->bbx.descent, p->maxds );

      p->rbearing = (short)( glyph->bbx.width + glyph->bbx.x_offset );

      p->maxrb    = (short)FT_MAX( p->rbearing, p->maxrb );
      p->minlb    = (short)FT_MIN( glyph->bbx.x_offset, p->minlb );
      p->maxlb    = (short)FT_MAX( glyph->bbx.x_offset, p->maxlb );

      if ( !( p->flags & _BDF_DWIDTH ) )
      {
        
        
        FT_TRACE2(( "_bdf_parse_glyphs: " ACMSG10, lineno ));
        glyph->dwidth = glyph->bbx.width;
      }

      
      
      if ( p->opts->correct_metrics != 0 )
      {
        
        unsigned short  sw = (unsigned short)FT_MulDiv(
                               glyph->dwidth, 72000L,
                               (FT_Long)( font->point_size *
                                          font->resolution_x ) );


        if ( sw != glyph->swidth )
        {
          glyph->swidth = sw;

          if ( p->glyph_enc == -1 )
            _bdf_set_glyph_modified( font->umod,
                                     font->unencoded_used - 1 );
          else
            _bdf_set_glyph_modified( font->nmod, glyph->encoding );

          p->flags       |= _BDF_SWIDTH_ADJ;
          font->modified  = 1;
        }
      }

      p->flags |= _BDF_BBX;
      goto Exit;
    }

    
    if ( ft_memcmp( line, "BITMAP", 6 ) == 0 )
    {
      unsigned long  bitmap_size;


      if ( !( p->flags & _BDF_BBX ) )
      {
        
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "BBX" ));
        error = BDF_Err_Missing_Bbx_Field;
        goto Exit;
      }

      
      glyph->bpr = ( glyph->bbx.width * p->font->bpp + 7 ) >> 3;

      bitmap_size = glyph->bpr * glyph->bbx.height;
      if ( glyph->bpr > 0xFFFFU || bitmap_size > 0xFFFFU )
      {
        FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG4, lineno ));
        error = BDF_Err_Bbx_Too_Big;
        goto Exit;
      }
      else
        glyph->bytes = (unsigned short)bitmap_size;

      if ( FT_NEW_ARRAY( glyph->bitmap, glyph->bytes ) )
        goto Exit;

      p->row    = 0;
      p->flags |= _BDF_BITMAP;

      goto Exit;
    }

    FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG9, lineno ));
    error = BDF_Err_Invalid_File_Format;
    goto Exit;

  Missing_Encoding:
    
    FT_ERROR(( "_bdf_parse_glyphs: " ERRMSG1, lineno, "ENCODING" ));
    error = BDF_Err_Missing_Encoding_Field;

  Exit:
    if ( error && ( p->flags & _BDF_GLYPH ) )
      FT_FREE( p->glyph_name );

    return error;
  }


  
  static FT_Error
  _bdf_parse_properties( char*          line,
                         unsigned long  linelen,
                         unsigned long  lineno,
                         void*          call_data,
                         void*          client_data )
  {
    unsigned long      vlen;
    _bdf_line_func_t*  next;
    _bdf_parse_t*      p;
    char*              name;
    char*              value;
    char               nbuf[128];
    FT_Error           error = BDF_Err_Ok;

    FT_UNUSED( lineno );


    next = (_bdf_line_func_t *)call_data;
    p    = (_bdf_parse_t *)    client_data;

    
    if ( ft_memcmp( line, "ENDPROPERTIES", 13 ) == 0 )
    {
      
      
      
      
      
      
      if ( bdf_get_font_property( p->font, "FONT_ASCENT" ) == 0 )
      {
        p->font->font_ascent = p->font->bbx.ascent;
        ft_sprintf( nbuf, "%hd", p->font->bbx.ascent );
        error = _bdf_add_property( p->font, (char *)"FONT_ASCENT",
                                   nbuf, lineno );
        if ( error )
          goto Exit;

        FT_TRACE2(( "_bdf_parse_properties: " ACMSG1, p->font->bbx.ascent ));
        p->font->modified = 1;
      }

      if ( bdf_get_font_property( p->font, "FONT_DESCENT" ) == 0 )
      {
        p->font->font_descent = p->font->bbx.descent;
        ft_sprintf( nbuf, "%hd", p->font->bbx.descent );
        error = _bdf_add_property( p->font, (char *)"FONT_DESCENT",
                                   nbuf, lineno );
        if ( error )
          goto Exit;

        FT_TRACE2(( "_bdf_parse_properties: " ACMSG2, p->font->bbx.descent ));
        p->font->modified = 1;
      }

      p->flags &= ~_BDF_PROPS;
      *next     = _bdf_parse_glyphs;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "_XFREE86_GLYPH_RANGES", 21 ) == 0 )
      goto Exit;

    
    
    if ( ft_memcmp( line, "COMMENT", 7 ) == 0 )
    {
      name = value = line;
      value += 7;
      if ( *value )
        *value++ = 0;
      error = _bdf_add_property( p->font, name, value, lineno );
      if ( error )
        goto Exit;
    }
    else if ( _bdf_is_atom( line, linelen, &name, &value, p->font ) )
    {
      error = _bdf_add_property( p->font, name, value, lineno );
      if ( error )
        goto Exit;
    }
    else
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      name = p->list.field[0];

      _bdf_list_shift( &p->list, 1 );
      value = _bdf_list_join( &p->list, ' ', &vlen );

      error = _bdf_add_property( p->font, name, value, lineno );
      if ( error )
        goto Exit;
    }

  Exit:
    return error;
  }


  
  static FT_Error
  _bdf_parse_start( char*          line,
                    unsigned long  linelen,
                    unsigned long  lineno,
                    void*          call_data,
                    void*          client_data )
  {
    unsigned long      slen;
    _bdf_line_func_t*  next;
    _bdf_parse_t*      p;
    bdf_font_t*        font;
    char               *s;

    FT_Memory          memory = NULL;
    FT_Error           error  = BDF_Err_Ok;

    FT_UNUSED( lineno );            


    next = (_bdf_line_func_t *)call_data;
    p    = (_bdf_parse_t *)    client_data;

    if ( p->font )
      memory = p->font->memory;

    
    
    if ( ft_memcmp( line, "COMMENT", 7 ) == 0 )
    {
      if ( p->opts->keep_comments != 0 && p->font != 0 )
      {
        linelen -= 7;

        s = line + 7;
        if ( *s != 0 )
        {
          s++;
          linelen--;
        }

        error = _bdf_add_comment( p->font, s, linelen );
        if ( error )
          goto Exit;
        
      }

      goto Exit;
    }

    if ( !( p->flags & _BDF_START ) )
    {
      memory = p->memory;

      if ( ft_memcmp( line, "STARTFONT", 9 ) != 0 )
      {
        
        
        error = BDF_Err_Missing_Startfont_Field;
        goto Exit;
      }

      p->flags = _BDF_START;
      font = p->font = 0;

      if ( FT_NEW( font ) )
        goto Exit;
      p->font = font;

      font->memory = p->memory;
      p->memory    = 0;

      { 
        size_t           i;
        bdf_property_t*  prop;


        error = hash_init( &(font->proptbl), memory );
        if ( error )
          goto Exit;
        for ( i = 0, prop = (bdf_property_t*)_bdf_properties;
              i < _num_bdf_properties; i++, prop++ )
        {
          error = hash_insert( prop->name, i,
                               &(font->proptbl), memory );
          if ( error )
            goto Exit;
        }
      }

      if ( FT_ALLOC( p->font->internal, sizeof ( hashtable ) ) )
        goto Exit;
      error = hash_init( (hashtable *)p->font->internal,memory );
      if ( error )
        goto Exit;
      p->font->spacing      = p->opts->font_spacing;
      p->font->default_char = -1;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "STARTPROPERTIES", 15 ) == 0 )
    {
      if ( !( p->flags & _BDF_FONT_BBX ) )
      {
        
        FT_ERROR(( "_bdf_parse_start: " ERRMSG1, lineno, "FONTBOUNDINGBOX" ));
        error = BDF_Err_Missing_Fontboundingbox_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      
      p->cnt = p->font->props_size = _bdf_atoul( p->list.field[1], 0, 10 );

      if ( FT_NEW_ARRAY( p->font->props, p->cnt ) )
        goto Exit;

      p->flags |= _BDF_PROPS;
      *next     = _bdf_parse_properties;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "FONTBOUNDINGBOX", 15 ) == 0 )
    {
      if ( !( p->flags & _BDF_SIZE ) )
      {
        
        FT_ERROR(( "_bdf_parse_start: " ERRMSG1, lineno, "SIZE" ));
        error = BDF_Err_Missing_Size_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      p->font->bbx.width  = _bdf_atos( p->list.field[1], 0, 10 );
      p->font->bbx.height = _bdf_atos( p->list.field[2], 0, 10 );

      p->font->bbx.x_offset = _bdf_atos( p->list.field[3], 0, 10 );
      p->font->bbx.y_offset = _bdf_atos( p->list.field[4], 0, 10 );

      p->font->bbx.ascent  = (short)( p->font->bbx.height +
                                      p->font->bbx.y_offset );

      p->font->bbx.descent = (short)( -p->font->bbx.y_offset );

      p->flags |= _BDF_FONT_BBX;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "FONT", 4 ) == 0 )
    {
      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;
      _bdf_list_shift( &p->list, 1 );

      s = _bdf_list_join( &p->list, ' ', &slen );

      if ( !s )
      {
        FT_ERROR(( "_bdf_parse_start: " ERRMSG8, lineno, "FONT" ));
        error = BDF_Err_Invalid_File_Format;
        goto Exit;
      }

      
      FT_FREE( p->font->name );

      if ( FT_NEW_ARRAY( p->font->name, slen + 1 ) )
        goto Exit;
      FT_MEM_COPY( p->font->name, s, slen + 1 );

      
      
      error = _bdf_set_default_spacing( p->font, p->opts, lineno );
      if ( error )
        goto Exit;

      p->flags |= _BDF_FONT_NAME;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "SIZE", 4 ) == 0 )
    {
      if ( !( p->flags & _BDF_FONT_NAME ) )
      {
        
        FT_ERROR(( "_bdf_parse_start: " ERRMSG1, lineno, "FONT" ));
        error = BDF_Err_Missing_Font_Field;
        goto Exit;
      }

      error = _bdf_list_split( &p->list, (char *)" +", line, linelen );
      if ( error )
        goto Exit;

      p->font->point_size   = _bdf_atoul( p->list.field[1], 0, 10 );
      p->font->resolution_x = _bdf_atoul( p->list.field[2], 0, 10 );
      p->font->resolution_y = _bdf_atoul( p->list.field[3], 0, 10 );

      
      if ( p->list.used == 5 )
      {
        unsigned short bitcount, i, shift;


        p->font->bpp = (unsigned short)_bdf_atos( p->list.field[4], 0, 10 );

        
        shift = p->font->bpp;
        bitcount = 0;
        for ( i = 0; shift > 0; i++ )
        {
          if ( shift & 1 )
            bitcount = i;
          shift >>= 1;
        }

        shift = (short)( ( bitcount > 3 ) ? 8 : ( 1 << bitcount ) );

        if ( p->font->bpp > shift || p->font->bpp != shift )
        {
          
          p->font->bpp = (unsigned short)( shift << 1 );
          FT_TRACE2(( "_bdf_parse_start: " ACMSG11, p->font->bpp ));
        }
      }
      else
        p->font->bpp = 1;

      p->flags |= _BDF_SIZE;

      goto Exit;
    }

    
    if ( ft_memcmp( line, "CHARS", 5 ) == 0 )
    {
      char  nbuf[128];


      if ( !( p->flags & _BDF_FONT_BBX ) )
      {
        
        FT_ERROR(( "_bdf_parse_start: " ERRMSG1, lineno, "FONTBOUNDINGBOX" ));
        error = BDF_Err_Missing_Fontboundingbox_Field;
        goto Exit;
      }

      
      
      p->font->font_ascent = p->font->bbx.ascent;
      ft_sprintf( nbuf, "%hd", p->font->bbx.ascent );
      error = _bdf_add_property( p->font, (char *)"FONT_ASCENT",
                                 nbuf, lineno );
      if ( error )
        goto Exit;
      FT_TRACE2(( "_bdf_parse_properties: " ACMSG1, p->font->bbx.ascent ));

      p->font->font_descent = p->font->bbx.descent;
      ft_sprintf( nbuf, "%hd", p->font->bbx.descent );
      error = _bdf_add_property( p->font, (char *)"FONT_DESCENT",
                                 nbuf, lineno );
      if ( error )
        goto Exit;
      FT_TRACE2(( "_bdf_parse_properties: " ACMSG2, p->font->bbx.descent ));

      p->font->modified = 1;

      *next = _bdf_parse_glyphs;

      
      error = -1;
      goto Exit;
    }

    FT_ERROR(( "_bdf_parse_start: " ERRMSG9, lineno ));
    error = BDF_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  
  
  
  
  


  FT_LOCAL_DEF( FT_Error )
  bdf_load_font( FT_Stream       stream,
                 FT_Memory       extmemory,
                 bdf_options_t*  opts,
                 bdf_font_t*    *font )
  {
    unsigned long  lineno = 0; 
    _bdf_parse_t   *p     = NULL;

    FT_Memory      memory = extmemory;
    FT_Error       error  = BDF_Err_Ok;


    if ( FT_NEW( p ) )
      goto Exit;

    memory    = NULL;
    p->opts   = (bdf_options_t*)( ( opts != 0 ) ? opts : &_bdf_opts );
    p->minlb  = 32767;
    p->memory = extmemory;  

    _bdf_list_init( &p->list, extmemory );

    error = _bdf_readstream( stream, _bdf_parse_start,
                             (void *)p, &lineno );
    if ( error )
      goto Fail;

    if ( p->font != 0 )
    {
      
      
      memory = p->font->memory;

      if ( p->font->spacing != BDF_PROPORTIONAL )
        p->font->monowidth = p->font->bbx.width;

      
      
      if ( p->cnt != p->font->glyphs_used + p->font->unencoded_used )
      {
        FT_TRACE2(( "bdf_load_font: " ACMSG15, p->cnt,
                    p->font->glyphs_used + p->font->unencoded_used ));
        p->font->modified = 1;
      }

      
      
      if ( p->opts->correct_metrics != 0 &&
           ( p->font->glyphs_used > 0 || p->font->unencoded_used > 0 ) )
      {
        if ( p->maxrb - p->minlb != p->font->bbx.width )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG3,
                      p->font->bbx.width, p->maxrb - p->minlb ));
          p->font->bbx.width = (unsigned short)( p->maxrb - p->minlb );
          p->font->modified  = 1;
        }

        if ( p->font->bbx.x_offset != p->minlb )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG4,
                      p->font->bbx.x_offset, p->minlb ));
          p->font->bbx.x_offset = p->minlb;
          p->font->modified     = 1;
        }

        if ( p->font->bbx.ascent != p->maxas )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG5,
                      p->font->bbx.ascent, p->maxas ));
          p->font->bbx.ascent = p->maxas;
          p->font->modified   = 1;
        }

        if ( p->font->bbx.descent != p->maxds )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG6,
                      p->font->bbx.descent, p->maxds ));
          p->font->bbx.descent  = p->maxds;
          p->font->bbx.y_offset = (short)( -p->maxds );
          p->font->modified     = 1;
        }

        if ( p->maxas + p->maxds != p->font->bbx.height )
        {
          FT_TRACE2(( "bdf_load_font: " ACMSG7,
                      p->font->bbx.height, p->maxas + p->maxds ));
          p->font->bbx.height = (unsigned short)( p->maxas + p->maxds );
        }

        if ( p->flags & _BDF_SWIDTH_ADJ )
          FT_TRACE2(( "bdf_load_font: " ACMSG8 ));
      }
    }

    if ( p->flags & _BDF_START )
    {
      
      if ( !( p->flags & _BDF_GLYPHS ) )
      {
        
        FT_ERROR(( "bdf_load_font: " ERRMSG2, lineno ));
        error = BDF_Err_Corrupted_Font_Header;
        goto Exit;
      }
      else
      {
        
        FT_ERROR(( "bdf_load_font: " ERRMSG3, lineno ));
        error = BDF_Err_Corrupted_Font_Glyphs;
        goto Exit;
      }
    }

    if ( p->font != 0 )
    {
      
      memory = p->font->memory;

      if ( p->font->comments_len > 0 )
      {
        if ( FT_RENEW_ARRAY( p->font->comments,
                             p->font->comments_len,
                             p->font->comments_len + 1 ) )
          goto Fail;

        p->font->comments[p->font->comments_len] = 0;
      }
    }
    else if ( error == BDF_Err_Ok )
      error = BDF_Err_Invalid_File_Format;

    *font = p->font;

  Exit:
    if ( p )
    {
      _bdf_list_done( &p->list );

      memory = extmemory;

      FT_FREE( p );
    }

    return error;

  Fail:
    bdf_free_font( p->font );

    memory = extmemory;

    FT_FREE( p->font );

    goto Exit;
  }


  FT_LOCAL_DEF( void )
  bdf_free_font( bdf_font_t*  font )
  {
    bdf_property_t*  prop;
    unsigned long    i;
    bdf_glyph_t*     glyphs;
    FT_Memory        memory;


    if ( font == 0 )
      return;

    memory = font->memory;

    FT_FREE( font->name );

    
    if ( font->internal )
    {
      hash_free( (hashtable *)font->internal, memory );
      FT_FREE( font->internal );
    }

    
    FT_FREE( font->comments );

    
    for ( i = 0; i < font->props_size; i++ )
    {
      if ( font->props[i].format == BDF_ATOM )
        FT_FREE( font->props[i].value.atom );
    }

    FT_FREE( font->props );

    
    for ( i = 0, glyphs = font->glyphs;
          i < font->glyphs_used; i++, glyphs++ )
    {
      FT_FREE( glyphs->name );
      FT_FREE( glyphs->bitmap );
    }

    for ( i = 0, glyphs = font->unencoded; i < font->unencoded_used;
          i++, glyphs++ )
    {
      FT_FREE( glyphs->name );
      FT_FREE( glyphs->bitmap );
    }

    FT_FREE( font->glyphs );
    FT_FREE( font->unencoded );

    
    for ( i = 0, glyphs = font->overflow.glyphs;
          i < font->overflow.glyphs_used; i++, glyphs++ )
    {
      FT_FREE( glyphs->name );
      FT_FREE( glyphs->bitmap );
    }

    FT_FREE( font->overflow.glyphs );

    
    hash_free( &(font->proptbl), memory );

    
    for ( prop = font->user_props, i = 0;
          i < font->nuser_props; i++, prop++ )
    {
      FT_FREE( prop->name );
      if ( prop->format == BDF_ATOM )
        FT_FREE( prop->value.atom );
    }

    FT_FREE( font->user_props );

     
  }


  FT_LOCAL_DEF( bdf_property_t * )
  bdf_get_font_property( bdf_font_t*  font,
                         const char*  name )
  {
    hashnode  hn;


    if ( font == 0 || font->props_size == 0 || name == 0 || *name == 0 )
      return 0;

    hn = hash_lookup( name, (hashtable *)font->internal );

    return hn ? ( font->props + hn->data ) : 0;
  }



