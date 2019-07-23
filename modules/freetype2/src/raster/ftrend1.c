

















#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include "ftrend1.h"
#include "ftraster.h"

#include "rasterrs.h"


  
  static FT_Error
  ft_raster1_init( FT_Renderer  render )
  {
    FT_Library  library = FT_MODULE_LIBRARY( render );


    render->clazz->raster_class->raster_reset( render->raster,
                                               library->raster_pool,
                                               library->raster_pool_size );

    return Raster_Err_Ok;
  }


  
  static FT_Error
  ft_raster1_set_mode( FT_Renderer  render,
                       FT_ULong     mode_tag,
                       FT_Pointer   data )
  {
    
    return render->clazz->raster_class->raster_set_mode( render->raster,
                                                         mode_tag,
                                                         data );
  }


  
  static FT_Error
  ft_raster1_transform( FT_Renderer       render,
                        FT_GlyphSlot      slot,
                        const FT_Matrix*  matrix,
                        const FT_Vector*  delta )
  {
    FT_Error error = Raster_Err_Ok;


    if ( slot->format != render->glyph_format )
    {
      error = Raster_Err_Invalid_Argument;
      goto Exit;
    }

    if ( matrix )
      FT_Outline_Transform( &slot->outline, matrix );

    if ( delta )
      FT_Outline_Translate( &slot->outline, delta->x, delta->y );

  Exit:
    return error;
  }


  
  static void
  ft_raster1_get_cbox( FT_Renderer   render,
                       FT_GlyphSlot  slot,
                       FT_BBox*      cbox )
  {
    FT_MEM_ZERO( cbox, sizeof ( *cbox ) );

    if ( slot->format == render->glyph_format )
      FT_Outline_Get_CBox( &slot->outline, cbox );
  }


  
  static FT_Error
  ft_raster1_render( FT_Renderer       render,
                     FT_GlyphSlot      slot,
                     FT_Render_Mode    mode,
                     const FT_Vector*  origin )
  {
    FT_Error     error;
    FT_Outline*  outline;
    FT_BBox      cbox;
    FT_UInt      width, height, pitch;
    FT_Bitmap*   bitmap;
    FT_Memory    memory;

    FT_Raster_Params  params;


    
    if ( slot->format != render->glyph_format )
    {
      error = Raster_Err_Invalid_Argument;
      goto Exit;
    }

    
    if ( mode != FT_RENDER_MODE_MONO )
    {
      
      if ( render->clazz == &ft_raster1_renderer_class )
        return Raster_Err_Cannot_Render_Glyph;
    }
    else
    {
      
      if ( render->clazz == &ft_raster5_renderer_class )
        return Raster_Err_Cannot_Render_Glyph;
    }

    outline = &slot->outline;

    
    if ( origin )
      FT_Outline_Translate( outline, origin->x, origin->y );

    
    FT_Outline_Get_CBox( outline, &cbox );

    cbox.xMin = FT_PIX_FLOOR( cbox.xMin );
    cbox.yMin = FT_PIX_FLOOR( cbox.yMin );
    cbox.xMax = FT_PIX_CEIL( cbox.xMax );
    cbox.yMax = FT_PIX_CEIL( cbox.yMax );

    width  = (FT_UInt)( ( cbox.xMax - cbox.xMin ) >> 6 );
    height = (FT_UInt)( ( cbox.yMax - cbox.yMin ) >> 6 );
    bitmap = &slot->bitmap;
    memory = render->root.memory;

    
    if ( slot->internal->flags & FT_GLYPH_OWN_BITMAP )
    {
      FT_FREE( bitmap->buffer );
      slot->internal->flags &= ~FT_GLYPH_OWN_BITMAP;
    }

    
    if ( !( mode & FT_RENDER_MODE_MONO ) )
    {
      
      pitch              = FT_PAD_CEIL( width, 4 );
      bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
      bitmap->num_grays  = 256;
    }
    else
    {
      pitch              = ( ( width + 15 ) >> 4 ) << 1;
      bitmap->pixel_mode = FT_PIXEL_MODE_MONO;
    }

    bitmap->width = width;
    bitmap->rows  = height;
    bitmap->pitch = pitch;

    if ( FT_ALLOC_MULT( bitmap->buffer, pitch, height ) )
      goto Exit;

    slot->internal->flags |= FT_GLYPH_OWN_BITMAP;

    
    FT_Outline_Translate( outline, -cbox.xMin, -cbox.yMin );

    
    params.target = bitmap;
    params.source = outline;
    params.flags  = 0;

    if ( bitmap->pixel_mode == FT_PIXEL_MODE_GRAY )
      params.flags |= FT_RASTER_FLAG_AA;

    
    error = render->raster_render( render->raster, &params );

    FT_Outline_Translate( outline, cbox.xMin, cbox.yMin );

    if ( error )
      goto Exit;

    slot->format      = FT_GLYPH_FORMAT_BITMAP;
    slot->bitmap_left = (FT_Int)( cbox.xMin >> 6 );
    slot->bitmap_top  = (FT_Int)( cbox.yMax >> 6 );

  Exit:
    return error;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_raster1_renderer_class =
  {
    {
      FT_MODULE_RENDERER,
      sizeof( FT_RendererRec ),

      "raster1",
      0x10000L,
      0x20000L,

      0,    

      (FT_Module_Constructor)ft_raster1_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    FT_GLYPH_FORMAT_OUTLINE,

    (FT_Renderer_RenderFunc)   ft_raster1_render,
    (FT_Renderer_TransformFunc)ft_raster1_transform,
    (FT_Renderer_GetCBoxFunc)  ft_raster1_get_cbox,
    (FT_Renderer_SetModeFunc)  ft_raster1_set_mode,

    (FT_Raster_Funcs*)    &ft_standard_raster
  };


  
  
  
  
  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_raster5_renderer_class =
  {
    {
      FT_MODULE_RENDERER,
      sizeof( FT_RendererRec ),

      "raster5",
      0x10000L,
      0x20000L,

      0,    

      (FT_Module_Constructor)ft_raster1_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    FT_GLYPH_FORMAT_OUTLINE,

    (FT_Renderer_RenderFunc)   ft_raster1_render,
    (FT_Renderer_TransformFunc)ft_raster1_transform,
    (FT_Renderer_GetCBoxFunc)  ft_raster1_get_cbox,
    (FT_Renderer_SetModeFunc)  ft_raster1_set_mode,

    (FT_Raster_Funcs*)    &ft_standard_raster
  };



