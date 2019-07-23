

















#include <ft2build.h>
#include FT_SYNTHESIS_H
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include FT_BITMAP_H


  
  
  
  
  
  
  

  

  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Oblique( FT_GlyphSlot  slot )
  {
    FT_Matrix    transform;
    FT_Outline*  outline = &slot->outline;


    
    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
      return;

    

    
    

    transform.xx = 0x10000L;
    transform.yx = 0x00000L;

    transform.xy = 0x06000L;
    transform.yy = 0x10000L;

    FT_Outline_Transform( outline, &transform );
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


  

  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Embolden( FT_GlyphSlot  slot )
  {
    FT_Library  library = slot->library;
    FT_Face     face    = FT_SLOT_FACE( slot );
    FT_Error    error;
    FT_Pos      xstr, ystr;


    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE &&
         slot->format != FT_GLYPH_FORMAT_BITMAP )
      return;

    
    xstr = FT_MulFix( face->units_per_EM,
                      face->size->metrics.y_scale ) / 24;
    ystr = xstr;

    if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
    {
      error = FT_Outline_Embolden( &slot->outline, xstr );
      

      
      
      xstr = xstr * 2;
      ystr = xstr;
    }
    else if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
    {
      xstr = FT_PIX_FLOOR( xstr );
      if ( xstr == 0 )
        xstr = 1 << 6;
      ystr = FT_PIX_FLOOR( ystr );

      error = FT_GlyphSlot_Own_Bitmap( slot );
      if ( error )
        return;

      error = FT_Bitmap_Embolden( library, &slot->bitmap, xstr, ystr );
      if ( error )
        return;
    }

    if ( slot->advance.x )
      slot->advance.x += xstr;

    if ( slot->advance.y )
      slot->advance.y += ystr;

    slot->metrics.width        += xstr;
    slot->metrics.height       += ystr;
    slot->metrics.horiBearingY += ystr;
    slot->metrics.horiAdvance  += xstr;
    slot->metrics.vertBearingX -= xstr / 2;
    slot->metrics.vertBearingY += ystr;
    slot->metrics.vertAdvance  += ystr;

    if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
      slot->bitmap_top += ystr >> 6;
  }



