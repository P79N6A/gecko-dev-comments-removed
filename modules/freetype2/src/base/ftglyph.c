
















  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_INTERNAL_OBJECTS_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_glyph


  
  
  
  
  
  
  

  FT_CALLBACK_DEF( FT_Error )
  ft_bitmap_glyph_init( FT_Glyph      bitmap_glyph,
                        FT_GlyphSlot  slot )
  {
    FT_BitmapGlyph  glyph   = (FT_BitmapGlyph)bitmap_glyph;
    FT_Error        error   = FT_Err_Ok;
    FT_Library      library = FT_GLYPH( glyph )->library;


    if ( slot->format != FT_GLYPH_FORMAT_BITMAP )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    glyph->left = slot->bitmap_left;
    glyph->top  = slot->bitmap_top;

    
    if ( slot->internal->flags & FT_GLYPH_OWN_BITMAP )
    {
      glyph->bitmap = slot->bitmap;
      slot->internal->flags &= ~FT_GLYPH_OWN_BITMAP;
    }
    else
    {
      FT_Bitmap_New( &glyph->bitmap );
      error = FT_Bitmap_Copy( library, &slot->bitmap, &glyph->bitmap );
    }

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  ft_bitmap_glyph_copy( FT_Glyph  bitmap_source,
                        FT_Glyph  bitmap_target )
  {
    FT_Library      library = bitmap_source->library;
    FT_BitmapGlyph  source  = (FT_BitmapGlyph)bitmap_source;
    FT_BitmapGlyph  target  = (FT_BitmapGlyph)bitmap_target;


    target->left = source->left;
    target->top  = source->top;

    return FT_Bitmap_Copy( library, &source->bitmap, &target->bitmap );
  }


  FT_CALLBACK_DEF( void )
  ft_bitmap_glyph_done( FT_Glyph  bitmap_glyph )
  {
    FT_BitmapGlyph  glyph   = (FT_BitmapGlyph)bitmap_glyph;
    FT_Library      library = FT_GLYPH( glyph )->library;


    FT_Bitmap_Done( library, &glyph->bitmap );
  }


  FT_CALLBACK_DEF( void )
  ft_bitmap_glyph_bbox( FT_Glyph  bitmap_glyph,
                        FT_BBox*  cbox )
  {
    FT_BitmapGlyph  glyph = (FT_BitmapGlyph)bitmap_glyph;


    cbox->xMin = glyph->left << 6;
    cbox->xMax = cbox->xMin + ( glyph->bitmap.width << 6 );
    cbox->yMax = glyph->top << 6;
    cbox->yMin = cbox->yMax - ( glyph->bitmap.rows << 6 );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Glyph_Class  ft_bitmap_glyph_class =
  {
    sizeof ( FT_BitmapGlyphRec ),
    FT_GLYPH_FORMAT_BITMAP,

    ft_bitmap_glyph_init,
    ft_bitmap_glyph_done,
    ft_bitmap_glyph_copy,
    0,                          
    ft_bitmap_glyph_bbox,
    0                           
  };


  
  
  
  
  
  
  


  FT_CALLBACK_DEF( FT_Error )
  ft_outline_glyph_init( FT_Glyph      outline_glyph,
                         FT_GlyphSlot  slot )
  {
    FT_OutlineGlyph  glyph   = (FT_OutlineGlyph)outline_glyph;
    FT_Error         error   = FT_Err_Ok;
    FT_Library       library = FT_GLYPH( glyph )->library;
    FT_Outline*      source  = &slot->outline;
    FT_Outline*      target  = &glyph->outline;


    
    if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    
    error = FT_Outline_New( library, source->n_points, source->n_contours,
                            &glyph->outline );
    if ( error )
      goto Exit;

    FT_Outline_Copy( source, target );

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( void )
  ft_outline_glyph_done( FT_Glyph  outline_glyph )
  {
    FT_OutlineGlyph  glyph = (FT_OutlineGlyph)outline_glyph;


    FT_Outline_Done( FT_GLYPH( glyph )->library, &glyph->outline );
  }


  FT_CALLBACK_DEF( FT_Error )
  ft_outline_glyph_copy( FT_Glyph  outline_source,
                         FT_Glyph  outline_target )
  {
    FT_OutlineGlyph  source  = (FT_OutlineGlyph)outline_source;
    FT_OutlineGlyph  target  = (FT_OutlineGlyph)outline_target;
    FT_Error         error;
    FT_Library       library = FT_GLYPH( source )->library;


    error = FT_Outline_New( library, source->outline.n_points,
                            source->outline.n_contours, &target->outline );
    if ( !error )
      FT_Outline_Copy( &source->outline, &target->outline );

    return error;
  }


  FT_CALLBACK_DEF( void )
  ft_outline_glyph_transform( FT_Glyph          outline_glyph,
                              const FT_Matrix*  matrix,
                              const FT_Vector*  delta )
  {
    FT_OutlineGlyph  glyph = (FT_OutlineGlyph)outline_glyph;


    if ( matrix )
      FT_Outline_Transform( &glyph->outline, matrix );

    if ( delta )
      FT_Outline_Translate( &glyph->outline, delta->x, delta->y );
  }


  FT_CALLBACK_DEF( void )
  ft_outline_glyph_bbox( FT_Glyph  outline_glyph,
                         FT_BBox*  bbox )
  {
    FT_OutlineGlyph  glyph = (FT_OutlineGlyph)outline_glyph;


    FT_Outline_Get_CBox( &glyph->outline, bbox );
  }


  FT_CALLBACK_DEF( FT_Error )
  ft_outline_glyph_prepare( FT_Glyph      outline_glyph,
                            FT_GlyphSlot  slot )
  {
    FT_OutlineGlyph  glyph = (FT_OutlineGlyph)outline_glyph;


    slot->format         = FT_GLYPH_FORMAT_OUTLINE;
    slot->outline        = glyph->outline;
    slot->outline.flags &= ~FT_OUTLINE_OWNER;

    return FT_Err_Ok;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Glyph_Class  ft_outline_glyph_class =
  {
    sizeof ( FT_OutlineGlyphRec ),
    FT_GLYPH_FORMAT_OUTLINE,

    ft_outline_glyph_init,
    ft_outline_glyph_done,
    ft_outline_glyph_copy,
    ft_outline_glyph_transform,
    ft_outline_glyph_bbox,
    ft_outline_glyph_prepare
  };


  
  
  
  
  
  
  

   static FT_Error
   ft_new_glyph( FT_Library             library,
                 const FT_Glyph_Class*  clazz,
                 FT_Glyph*              aglyph )
   {
     FT_Memory  memory = library->memory;
     FT_Error   error;
     FT_Glyph   glyph;


     *aglyph = 0;

     if ( !FT_ALLOC( glyph, clazz->glyph_size ) )
     {
       glyph->library = library;
       glyph->clazz   = clazz;
       glyph->format  = clazz->glyph_format;

       *aglyph = glyph;
     }

     return error;
   }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_Copy( FT_Glyph   source,
                 FT_Glyph  *target )
  {
    FT_Glyph               copy;
    FT_Error               error;
    const FT_Glyph_Class*  clazz;


    
    if ( !target )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    *target = 0;

    if ( !source || !source->clazz )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    clazz = source->clazz;
    error = ft_new_glyph( source->library, clazz, &copy );
    if ( error )
      goto Exit;

    copy->advance = source->advance;
    copy->format  = source->format;

    if ( clazz->glyph_copy )
      error = clazz->glyph_copy( source, copy );

    if ( error )
      FT_Done_Glyph( copy );
    else
      *target = copy;

  Exit:
    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Glyph( FT_GlyphSlot  slot,
                FT_Glyph     *aglyph )
  {
    FT_Library  library;
    FT_Error    error;
    FT_Glyph    glyph;

    const FT_Glyph_Class*  clazz = 0;


    if ( !slot )
      return FT_Err_Invalid_Slot_Handle;

    library = slot->library;

    if ( !aglyph )
      return FT_Err_Invalid_Argument;

    
    if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
      clazz = &ft_bitmap_glyph_class;

    
    else if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
      clazz = &ft_outline_glyph_class;

    else
    {
      
      FT_Renderer  render = FT_Lookup_Renderer( library, slot->format, 0 );


      if ( render )
        clazz = &render->glyph_class;
    }

    if ( !clazz )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    
    error = ft_new_glyph( library, clazz, &glyph );
    if ( error )
      goto Exit;

    
    glyph->advance.x = slot->advance.x << 10;
    glyph->advance.y = slot->advance.y << 10;

    
    error = clazz->glyph_init( glyph, slot );

    
    if ( error )
      FT_Done_Glyph( glyph );
    else
      *aglyph = glyph;

  Exit:
    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_Transform( FT_Glyph    glyph,
                      FT_Matrix*  matrix,
                      FT_Vector*  delta )
  {
    const FT_Glyph_Class*  clazz;
    FT_Error               error = FT_Err_Ok;


    if ( !glyph || !glyph->clazz )
      error = FT_Err_Invalid_Argument;
    else
    {
      clazz = glyph->clazz;
      if ( clazz->glyph_transform )
      {
        
        clazz->glyph_transform( glyph, matrix, delta );

        
        if ( matrix )
          FT_Vector_Transform( &glyph->advance, matrix );
      }
      else
        error = FT_Err_Invalid_Glyph_Format;
    }
    return error;
  }


  

  FT_EXPORT_DEF( void )
  FT_Glyph_Get_CBox( FT_Glyph  glyph,
                     FT_UInt   bbox_mode,
                     FT_BBox  *acbox )
  {
    const FT_Glyph_Class*  clazz;


    if ( !acbox )
      return;

    acbox->xMin = acbox->yMin = acbox->xMax = acbox->yMax = 0;

    if ( !glyph || !glyph->clazz )
      return;
    else
    {
      clazz = glyph->clazz;
      if ( !clazz->glyph_bbox )
        return;
      else
      {
        
        clazz->glyph_bbox( glyph, acbox );

        
        if ( bbox_mode == FT_GLYPH_BBOX_GRIDFIT ||
             bbox_mode == FT_GLYPH_BBOX_PIXELS  )
        {
          acbox->xMin = FT_PIX_FLOOR( acbox->xMin );
          acbox->yMin = FT_PIX_FLOOR( acbox->yMin );
          acbox->xMax = FT_PIX_CEIL( acbox->xMax );
          acbox->yMax = FT_PIX_CEIL( acbox->yMax );
        }

        
        if ( bbox_mode == FT_GLYPH_BBOX_TRUNCATE ||
             bbox_mode == FT_GLYPH_BBOX_PIXELS   )
        {
          acbox->xMin >>= 6;
          acbox->yMin >>= 6;
          acbox->xMax >>= 6;
          acbox->yMax >>= 6;
        }
      }
    }
    return;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_To_Bitmap( FT_Glyph*       the_glyph,
                      FT_Render_Mode  render_mode,
                      FT_Vector*      origin,
                      FT_Bool         destroy )
  {
    FT_GlyphSlotRec           dummy;
    FT_GlyphSlot_InternalRec  dummy_internal;
    FT_Error                  error = FT_Err_Ok;
    FT_Glyph                  glyph;
    FT_BitmapGlyph            bitmap = NULL;

    const FT_Glyph_Class*     clazz;


    
    if ( !the_glyph )
      goto Bad;

    
    

    glyph = *the_glyph;
    if ( !glyph )
      goto Bad;

    clazz = glyph->clazz;

    
    if ( clazz == &ft_bitmap_glyph_class )
      goto Exit;

    if ( !clazz || !clazz->glyph_prepare )
      goto Bad;

    FT_MEM_ZERO( &dummy, sizeof ( dummy ) );
    FT_MEM_ZERO( &dummy_internal, sizeof ( dummy_internal ) );
    dummy.internal = &dummy_internal;
    dummy.library  = glyph->library;
    dummy.format   = clazz->glyph_format;

    
    error = ft_new_glyph( glyph->library, &ft_bitmap_glyph_class,
                          (FT_Glyph*)(void*)&bitmap );
    if ( error )
      goto Exit;

#if 1
    
    if ( origin )
      FT_Glyph_Transform( glyph, 0, origin );
#else
    FT_UNUSED( origin );
#endif

    
    error = clazz->glyph_prepare( glyph, &dummy );
    if ( !error )
      error = FT_Render_Glyph_Internal( glyph->library, &dummy, render_mode );

#if 1
    if ( !destroy && origin )
    {
      FT_Vector  v;


      v.x = -origin->x;
      v.y = -origin->y;
      FT_Glyph_Transform( glyph, 0, &v );
    }
#endif

    if ( error )
      goto Exit;

    
    error = ft_bitmap_glyph_init( (FT_Glyph)bitmap, &dummy );
    if ( error )
      goto Exit;

    
    bitmap->root.advance = glyph->advance;

    if ( destroy )
      FT_Done_Glyph( glyph );

    *the_glyph = FT_GLYPH( bitmap );

  Exit:
    if ( error && bitmap )
      FT_Done_Glyph( FT_GLYPH( bitmap ) );

    return error;

  Bad:
    error = FT_Err_Invalid_Argument;
    goto Exit;
  }


  

  FT_EXPORT_DEF( void )
  FT_Done_Glyph( FT_Glyph  glyph )
  {
    if ( glyph )
    {
      FT_Memory              memory = glyph->library->memory;
      const FT_Glyph_Class*  clazz  = glyph->clazz;


      if ( clazz->glyph_done )
        clazz->glyph_done( glyph );

      FT_FREE( glyph );
    }
  }



