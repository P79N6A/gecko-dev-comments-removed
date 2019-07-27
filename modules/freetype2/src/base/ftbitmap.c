

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H

#include FT_BITMAP_H
#include FT_IMAGE_H
#include FT_INTERNAL_OBJECTS_H


  static
  const FT_Bitmap  null_bitmap = { 0, 0, 0, 0, 0, 0, 0, 0 };


  

  FT_EXPORT_DEF( void )
  FT_Bitmap_New( FT_Bitmap  *abitmap )
  {
    if ( abitmap )
      *abitmap = null_bitmap;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Bitmap_Copy( FT_Library        library,
                  const FT_Bitmap  *source,
                  FT_Bitmap        *target)
  {
    FT_Memory  memory;
    FT_Error   error  = FT_Err_Ok;

    FT_Int    pitch;
    FT_ULong  size;

    FT_Int  source_pitch_sign, target_pitch_sign;


    if ( !library )
      return FT_THROW( Invalid_Library_Handle );

    if ( !source || !target )
      return FT_THROW( Invalid_Argument );

    if ( source == target )
      return FT_Err_Ok;

    source_pitch_sign = source->pitch < 0 ? -1 : 1;
    target_pitch_sign = target->pitch < 0 ? -1 : 1;

    if ( source->buffer == NULL )
    {
      *target = *source;
      if ( source_pitch_sign != target_pitch_sign )
        target->pitch = -target->pitch;

      return FT_Err_Ok;
    }

    memory = library->memory;
    pitch  = source->pitch;

    if ( pitch < 0 )
      pitch = -pitch;
    size = (FT_ULong)pitch * source->rows;

    if ( target->buffer )
    {
      FT_Int    target_pitch = target->pitch;
      FT_ULong  target_size;


      if ( target_pitch < 0 )
        target_pitch = -target_pitch;
      target_size = (FT_ULong)target_pitch * target->rows;

      if ( target_size != size )
        (void)FT_QREALLOC( target->buffer, target_size, size );
    }
    else
      (void)FT_QALLOC( target->buffer, size );

    if ( !error )
    {
      unsigned char *p;


      p = target->buffer;
      *target = *source;
      target->buffer = p;

      if ( source_pitch_sign == target_pitch_sign )
        FT_MEM_COPY( target->buffer, source->buffer, size );
      else
      {
        
        FT_UInt   i;
        FT_Byte*  s = source->buffer;
        FT_Byte*  t = target->buffer;


        t += pitch * ( target->rows - 1 );

        for ( i = target->rows; i > 0; i-- )
        {
          FT_ARRAY_COPY( t, s, pitch );

          s += pitch;
          t -= pitch;
        }
      }
    }

    return error;
  }


  
  

  static FT_Error
  ft_bitmap_assure_buffer( FT_Memory   memory,
                           FT_Bitmap*  bitmap,
                           FT_UInt     xpixels,
                           FT_UInt     ypixels )
  {
    FT_Error        error;
    int             pitch;
    int             new_pitch;
    FT_UInt         bpp;
    FT_UInt         i, width, height;
    unsigned char*  buffer = NULL;


    width  = bitmap->width;
    height = bitmap->rows;
    pitch  = bitmap->pitch;
    if ( pitch < 0 )
      pitch = -pitch;

    switch ( bitmap->pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
      bpp       = 1;
      new_pitch = ( width + xpixels + 7 ) >> 3;
      break;
    case FT_PIXEL_MODE_GRAY2:
      bpp       = 2;
      new_pitch = ( width + xpixels + 3 ) >> 2;
      break;
    case FT_PIXEL_MODE_GRAY4:
      bpp       = 4;
      new_pitch = ( width + xpixels + 1 ) >> 1;
      break;
    case FT_PIXEL_MODE_GRAY:
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
      bpp       = 8;
      new_pitch = ( width + xpixels );
      break;
    default:
      return FT_THROW( Invalid_Glyph_Format );
    }

    
    if ( ypixels == 0 && new_pitch <= pitch )
    {
      
      FT_UInt  bit_width = pitch * 8;
      FT_UInt  bit_last  = ( width + xpixels ) * bpp;


      if ( bit_last < bit_width )
      {
        FT_Byte*  line  = bitmap->buffer + ( bit_last >> 3 );
        FT_Byte*  end   = bitmap->buffer + pitch;
        FT_UInt   shift = bit_last & 7;
        FT_UInt   mask  = 0xFF00U >> shift;
        FT_UInt   count = height;


        for ( ; count > 0; count--, line += pitch, end += pitch )
        {
          FT_Byte*  write = line;


          if ( shift > 0 )
          {
            write[0] = (FT_Byte)( write[0] & mask );
            write++;
          }
          if ( write < end )
            FT_MEM_ZERO( write, end - write );
        }
      }

      return FT_Err_Ok;
    }

    
    if ( FT_QALLOC_MULT( buffer, new_pitch, bitmap->rows + ypixels ) )
      return error;

    
    
    if ( bitmap->pitch > 0 )
    {
      FT_UInt  len = ( width * bpp + 7 ) >> 3;


      for ( i = 0; i < bitmap->rows; i++ )
        FT_MEM_COPY( buffer + new_pitch * ( ypixels + i ),
                     bitmap->buffer + pitch * i, len );
    }
    else
    {
      FT_UInt  len = ( width * bpp + 7 ) >> 3;


      for ( i = 0; i < bitmap->rows; i++ )
        FT_MEM_COPY( buffer + new_pitch * i,
                     bitmap->buffer + pitch * i, len );
    }

    FT_FREE( bitmap->buffer );
    bitmap->buffer = buffer;

    if ( bitmap->pitch < 0 )
      new_pitch = -new_pitch;

    
    bitmap->pitch = new_pitch;

    return FT_Err_Ok;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Bitmap_Embolden( FT_Library  library,
                      FT_Bitmap*  bitmap,
                      FT_Pos      xStrength,
                      FT_Pos      yStrength )
  {
    FT_Error        error;
    unsigned char*  p;
    FT_Int          i, x, pitch;
    FT_UInt         y;
    FT_Int          xstr, ystr;


    if ( !library )
      return FT_THROW( Invalid_Library_Handle );

    if ( !bitmap || !bitmap->buffer )
      return FT_THROW( Invalid_Argument );

    if ( ( ( FT_PIX_ROUND( xStrength ) >> 6 ) > FT_INT_MAX ) ||
         ( ( FT_PIX_ROUND( yStrength ) >> 6 ) > FT_INT_MAX ) )
      return FT_THROW( Invalid_Argument );

    xstr = (FT_Int)FT_PIX_ROUND( xStrength ) >> 6;
    ystr = (FT_Int)FT_PIX_ROUND( yStrength ) >> 6;

    if ( xstr == 0 && ystr == 0 )
      return FT_Err_Ok;
    else if ( xstr < 0 || ystr < 0 )
      return FT_THROW( Invalid_Argument );

    switch ( bitmap->pixel_mode )
    {
    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
      {
        FT_Bitmap  tmp;


        
        FT_Bitmap_New( &tmp );
        error = FT_Bitmap_Convert( library, bitmap, &tmp, 1 );
        if ( error )
          return error;

        FT_Bitmap_Done( library, bitmap );
        *bitmap = tmp;
      }
      break;

    case FT_PIXEL_MODE_MONO:
      if ( xstr > 8 )
        xstr = 8;
      break;

    case FT_PIXEL_MODE_LCD:
      xstr *= 3;
      break;

    case FT_PIXEL_MODE_LCD_V:
      ystr *= 3;
      break;

    case FT_PIXEL_MODE_BGRA:
      
      return FT_Err_Ok;
    }

    error = ft_bitmap_assure_buffer( library->memory, bitmap, xstr, ystr );
    if ( error )
      return error;

    
    pitch = bitmap->pitch;
    if ( pitch > 0 )
      p = bitmap->buffer + pitch * ystr;
    else
    {
      pitch = -pitch;
      p = bitmap->buffer + pitch * ( bitmap->rows - 1 );
    }

    
    for ( y = 0; y < bitmap->rows ; y++ )
    {
      





      for ( x = pitch - 1; x >= 0; x-- )
      {
        unsigned char  tmp;


        tmp = p[x];
        for ( i = 1; i <= xstr; i++ )
        {
          if ( bitmap->pixel_mode == FT_PIXEL_MODE_MONO )
          {
            p[x] |= tmp >> i;

            
            if ( x > 0 )
              p[x] |= p[x - 1] << ( 8 - i );

#if 0
            if ( p[x] == 0xFF )
              break;
#endif
          }
          else
          {
            if ( x - i >= 0 )
            {
              if ( p[x] + p[x - i] > bitmap->num_grays - 1 )
              {
                p[x] = (unsigned char)( bitmap->num_grays - 1 );
                break;
              }
              else
              {
                p[x] = (unsigned char)( p[x] + p[x - i] );
                if ( p[x] == bitmap->num_grays - 1 )
                  break;
              }
            }
            else
              break;
          }
        }
      }

      




      for ( x = 1; x <= ystr; x++ )
      {
        unsigned char*  q;


        q = p - bitmap->pitch * x;
        for ( i = 0; i < pitch; i++ )
          q[i] |= p[i];
      }

      p += bitmap->pitch;
    }

    bitmap->width += xstr;
    bitmap->rows += ystr;

    return FT_Err_Ok;
  }


  static FT_Byte
  ft_gray_for_premultiplied_srgb_bgra( const FT_Byte*  bgra )
  {
    FT_UInt  a = bgra[3];
    FT_UInt  l;


    
    if ( !a )
      return 0;

    













    l = (  4732UL  * bgra[0] * bgra[0] +
          46871UL  * bgra[1] * bgra[1] +
          13933UL  * bgra[2] * bgra[2] ) >> 16;

    












    return (FT_Byte)( a - l / a );
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Bitmap_Convert( FT_Library        library,
                     const FT_Bitmap  *source,
                     FT_Bitmap        *target,
                     FT_Int            alignment )
  {
    FT_Error   error = FT_Err_Ok;
    FT_Memory  memory;

    FT_Byte*  s;
    FT_Byte*  t;


    if ( !library )
      return FT_THROW( Invalid_Library_Handle );

    if ( !source || !target )
      return FT_THROW( Invalid_Argument );

    memory = library->memory;

    switch ( source->pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
    case FT_PIXEL_MODE_GRAY:
    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
    case FT_PIXEL_MODE_BGRA:
      {
        FT_Int    pad, old_target_pitch, target_pitch;
        FT_ULong  old_size;


        old_target_pitch = target->pitch;
        if ( old_target_pitch < 0 )
          old_target_pitch = -old_target_pitch;

        old_size = target->rows * old_target_pitch;

        target->pixel_mode = FT_PIXEL_MODE_GRAY;
        target->rows       = source->rows;
        target->width      = source->width;

        pad = 0;
        if ( alignment > 0 )
        {
          pad = source->width % alignment;
          if ( pad != 0 )
            pad = alignment - pad;
        }

        target_pitch = source->width + pad;

        if ( target_pitch > 0                                     &&
             (FT_ULong)target->rows > FT_ULONG_MAX / target_pitch )
          return FT_THROW( Invalid_Argument );

        if ( target->rows * target_pitch > old_size               &&
             FT_QREALLOC( target->buffer,
                          old_size, target->rows * target_pitch ) )
          return error;

        target->pitch = target->pitch < 0 ? -target_pitch : target_pitch;
      }
      break;

    default:
      error = FT_THROW( Invalid_Argument );
    }

    s = source->buffer;
    t = target->buffer;

    
    if ( source->pitch < 0 )
      s -= source->pitch * ( source->rows - 1 );
    if ( target->pitch < 0 )
      t -= target->pitch * ( target->rows - 1 );

    switch ( source->pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
      {
        FT_UInt  i;


        target->num_grays = 2;

        for ( i = source->rows; i > 0; i-- )
        {
          FT_Byte*  ss = s;
          FT_Byte*  tt = t;
          FT_UInt   j;


          
          for ( j = source->width >> 3; j > 0; j-- )
          {
            FT_Int  val = ss[0]; 


            tt[0] = (FT_Byte)( ( val & 0x80 ) >> 7 );
            tt[1] = (FT_Byte)( ( val & 0x40 ) >> 6 );
            tt[2] = (FT_Byte)( ( val & 0x20 ) >> 5 );
            tt[3] = (FT_Byte)( ( val & 0x10 ) >> 4 );
            tt[4] = (FT_Byte)( ( val & 0x08 ) >> 3 );
            tt[5] = (FT_Byte)( ( val & 0x04 ) >> 2 );
            tt[6] = (FT_Byte)( ( val & 0x02 ) >> 1 );
            tt[7] = (FT_Byte)(   val & 0x01 );

            tt += 8;
            ss += 1;
          }

          
          j = source->width & 7;
          if ( j > 0 )
          {
            FT_Int  val = *ss;


            for ( ; j > 0; j-- )
            {
              tt[0] = (FT_Byte)( ( val & 0x80 ) >> 7);
              val <<= 1;
              tt   += 1;
            }
          }

          s += source->pitch;
          t += target->pitch;
        }
      }
      break;


    case FT_PIXEL_MODE_GRAY:
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
      {
        FT_Int   width = source->width;
        FT_UInt  i;


        target->num_grays = 256;

        for ( i = source->rows; i > 0; i-- )
        {
          FT_ARRAY_COPY( t, s, width );

          s += source->pitch;
          t += target->pitch;
        }
      }
      break;


    case FT_PIXEL_MODE_GRAY2:
      {
        FT_UInt  i;


        target->num_grays = 4;

        for ( i = source->rows; i > 0; i-- )
        {
          FT_Byte*  ss = s;
          FT_Byte*  tt = t;
          FT_UInt   j;


          
          for ( j = source->width >> 2; j > 0; j-- )
          {
            FT_Int  val = ss[0];


            tt[0] = (FT_Byte)( ( val & 0xC0 ) >> 6 );
            tt[1] = (FT_Byte)( ( val & 0x30 ) >> 4 );
            tt[2] = (FT_Byte)( ( val & 0x0C ) >> 2 );
            tt[3] = (FT_Byte)( ( val & 0x03 ) );

            ss += 1;
            tt += 4;
          }

          j = source->width & 3;
          if ( j > 0 )
          {
            FT_Int  val = ss[0];


            for ( ; j > 0; j-- )
            {
              tt[0]  = (FT_Byte)( ( val & 0xC0 ) >> 6 );
              val  <<= 2;
              tt    += 1;
            }
          }

          s += source->pitch;
          t += target->pitch;
        }
      }
      break;


    case FT_PIXEL_MODE_GRAY4:
      {
        FT_UInt  i;


        target->num_grays = 16;

        for ( i = source->rows; i > 0; i-- )
        {
          FT_Byte*  ss = s;
          FT_Byte*  tt = t;
          FT_UInt   j;


          
          for ( j = source->width >> 1; j > 0; j-- )
          {
            FT_Int  val = ss[0];


            tt[0] = (FT_Byte)( ( val & 0xF0 ) >> 4 );
            tt[1] = (FT_Byte)( ( val & 0x0F ) );

            ss += 1;
            tt += 2;
          }

          if ( source->width & 1 )
            tt[0] = (FT_Byte)( ( ss[0] & 0xF0 ) >> 4 );

          s += source->pitch;
          t += target->pitch;
        }
      }
      break;


    case FT_PIXEL_MODE_BGRA:
      {
        FT_UInt  i;


        target->num_grays = 256;

        for ( i = source->rows; i > 0; i-- )
        {
          FT_Byte*  ss = s;
          FT_Byte*  tt = t;
          FT_UInt   j;


          for ( j = source->width; j > 0; j-- )
          {
            tt[0] = ft_gray_for_premultiplied_srgb_bgra( ss );

            ss += 4;
            tt += 1;
          }

          s += source->pitch;
          t += target->pitch;
        }
      }
      break;

    default:
      ;
    }

    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_GlyphSlot_Own_Bitmap( FT_GlyphSlot  slot )
  {
    if ( slot && slot->format == FT_GLYPH_FORMAT_BITMAP   &&
         !( slot->internal->flags & FT_GLYPH_OWN_BITMAP ) )
    {
      FT_Bitmap  bitmap;
      FT_Error   error;


      FT_Bitmap_New( &bitmap );
      error = FT_Bitmap_Copy( slot->library, &slot->bitmap, &bitmap );
      if ( error )
        return error;

      slot->bitmap = bitmap;
      slot->internal->flags |= FT_GLYPH_OWN_BITMAP;
    }

    return FT_Err_Ok;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Bitmap_Done( FT_Library  library,
                  FT_Bitmap  *bitmap )
  {
    FT_Memory  memory;


    if ( !library )
      return FT_THROW( Invalid_Library_Handle );

    if ( !bitmap )
      return FT_THROW( Invalid_Argument );

    memory = library->memory;

    FT_FREE( bitmap->buffer );
    *bitmap = null_bitmap;

    return FT_Err_Ok;
  }



