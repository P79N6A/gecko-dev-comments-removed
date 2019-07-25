

















#include <ft2build.h>
#include FT_SYNTHESIS_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include FT_BITMAP_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_synth

  
  
  
  
  
  
  

  

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


  
  
  
  
  
  
  


  

  FT_EXPORT_DEF( void )
  FT_GlyphSlot_Embolden( FT_GlyphSlot  slot )
  {
    FT_Library  library = slot->library;
    FT_Face     face    = slot->face;
    FT_Error    error;
    FT_Pos      xstr, ystr;


    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE &&
         slot->format != FT_GLYPH_FORMAT_BITMAP  )
      return;

    
    xstr = FT_MulFix( face->units_per_EM,
                      face->size->metrics.y_scale ) / 24;
    ystr = xstr;

    if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
    {
      
      (void)FT_Outline_Embolden( &slot->outline, xstr );

      
      
      xstr = xstr * 2;
      ystr = xstr;
    }
    else 
    {
      
      xstr &= ~63;
      if ( xstr == 0 )
        xstr = 1 << 6;
      ystr &= ~63;

      





      if ( ( ystr >> 6 ) > FT_INT_MAX || ( ystr >> 6 ) < FT_INT_MIN )
      {
        FT_TRACE1(( "FT_GlyphSlot_Embolden:" ));
        FT_TRACE1(( "too strong embolding parameter ystr=%d\n", ystr ));
        return;
      }
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
      slot->bitmap_top += (FT_Int)( ystr >> 6 );
  }



