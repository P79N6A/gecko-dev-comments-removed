

















#include <ft2build.h>
#include FT_INTERNAL_GLYPH_LOADER_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_OBJECTS_H

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gloader


  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_New( FT_Memory        memory,
                      FT_GlyphLoader  *aloader )
  {
    FT_GlyphLoader  loader;
    FT_Error        error;


    if ( !FT_NEW( loader ) )
    {
      loader->memory = memory;
      *aloader       = loader;
    }
    return error;
  }


  
  FT_BASE_DEF( void )
  FT_GlyphLoader_Rewind( FT_GlyphLoader  loader )
  {
    FT_GlyphLoad  base    = &loader->base;
    FT_GlyphLoad  current = &loader->current;


    base->outline.n_points   = 0;
    base->outline.n_contours = 0;
    base->num_subglyphs      = 0;

    *current = *base;
  }


  
  
  FT_BASE_DEF( void )
  FT_GlyphLoader_Reset( FT_GlyphLoader  loader )
  {
    FT_Memory memory = loader->memory;


    FT_FREE( loader->base.outline.points );
    FT_FREE( loader->base.outline.tags );
    FT_FREE( loader->base.outline.contours );
    FT_FREE( loader->base.extra_points );
    FT_FREE( loader->base.subglyphs );

    loader->base.extra_points2 = NULL;

    loader->max_points    = 0;
    loader->max_contours  = 0;
    loader->max_subglyphs = 0;

    FT_GlyphLoader_Rewind( loader );
  }


  
  FT_BASE_DEF( void )
  FT_GlyphLoader_Done( FT_GlyphLoader  loader )
  {
    if ( loader )
    {
      FT_Memory memory = loader->memory;


      FT_GlyphLoader_Reset( loader );
      FT_FREE( loader );
    }
  }


  
  static void
  FT_GlyphLoader_Adjust_Points( FT_GlyphLoader  loader )
  {
    FT_Outline*  base    = &loader->base.outline;
    FT_Outline*  current = &loader->current.outline;


    current->points   = base->points   + base->n_points;
    current->tags     = base->tags     + base->n_points;
    current->contours = base->contours + base->n_contours;

    
    if ( loader->use_extra )
    {
      loader->current.extra_points  = loader->base.extra_points +
                                      base->n_points;

      loader->current.extra_points2 = loader->base.extra_points2 +
                                      base->n_points;
    }
  }


  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_CreateExtra( FT_GlyphLoader  loader )
  {
    FT_Error   error;
    FT_Memory  memory = loader->memory;


    if ( !FT_NEW_ARRAY( loader->base.extra_points, 2 * loader->max_points ) )
    {
      loader->use_extra          = 1;
      loader->base.extra_points2 = loader->base.extra_points +
                                   loader->max_points;

      FT_GlyphLoader_Adjust_Points( loader );
    }
    return error;
  }


  
  static void
  FT_GlyphLoader_Adjust_Subglyphs( FT_GlyphLoader  loader )
  {
    FT_GlyphLoad  base    = &loader->base;
    FT_GlyphLoad  current = &loader->current;


    current->subglyphs = base->subglyphs + base->num_subglyphs;
  }


  
  
  
  
  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_CheckPoints( FT_GlyphLoader  loader,
                              FT_UInt         n_points,
                              FT_UInt         n_contours )
  {
    FT_Memory    memory  = loader->memory;
    FT_Error     error   = FT_Err_Ok;
    FT_Outline*  base    = &loader->base.outline;
    FT_Outline*  current = &loader->current.outline;
    FT_Bool      adjust  = 0;

    FT_UInt      new_max, old_max;


    
    new_max = base->n_points + current->n_points + n_points;
    old_max = loader->max_points;

    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 8 );

      if ( FT_RENEW_ARRAY( base->points, old_max, new_max ) ||
           FT_RENEW_ARRAY( base->tags,   old_max, new_max ) )
        goto Exit;

      if ( loader->use_extra )
      {
        if ( FT_RENEW_ARRAY( loader->base.extra_points,
                             old_max * 2, new_max * 2 ) )
          goto Exit;

        FT_ARRAY_MOVE( loader->base.extra_points + new_max,
                       loader->base.extra_points + old_max,
                       old_max );

        loader->base.extra_points2 = loader->base.extra_points + new_max;
      }

      adjust = 1;
      loader->max_points = new_max;
    }

    
    old_max = loader->max_contours;
    new_max = base->n_contours + current->n_contours +
              n_contours;
    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 4 );
      if ( FT_RENEW_ARRAY( base->contours, old_max, new_max ) )
        goto Exit;

      adjust = 1;
      loader->max_contours = new_max;
    }

    if ( adjust )
      FT_GlyphLoader_Adjust_Points( loader );

  Exit:
    return error;
  }


  
  
  
  
  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_CheckSubGlyphs( FT_GlyphLoader  loader,
                                 FT_UInt         n_subs )
  {
    FT_Memory     memory = loader->memory;
    FT_Error      error  = FT_Err_Ok;
    FT_UInt       new_max, old_max;

    FT_GlyphLoad  base    = &loader->base;
    FT_GlyphLoad  current = &loader->current;


    new_max = base->num_subglyphs + current->num_subglyphs + n_subs;
    old_max = loader->max_subglyphs;
    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 2 );
      if ( FT_RENEW_ARRAY( base->subglyphs, old_max, new_max ) )
        goto Exit;

      loader->max_subglyphs = new_max;

      FT_GlyphLoader_Adjust_Subglyphs( loader );
    }

  Exit:
    return error;
  }


  
  FT_BASE_DEF( void )
  FT_GlyphLoader_Prepare( FT_GlyphLoader  loader )
  {
    FT_GlyphLoad  current = &loader->current;


    current->outline.n_points   = 0;
    current->outline.n_contours = 0;
    current->num_subglyphs      = 0;

    FT_GlyphLoader_Adjust_Points   ( loader );
    FT_GlyphLoader_Adjust_Subglyphs( loader );
  }


  
  FT_BASE_DEF( void )
  FT_GlyphLoader_Add( FT_GlyphLoader  loader )
  {
    FT_GlyphLoad  base;
    FT_GlyphLoad  current;

    FT_UInt       n_curr_contours;
    FT_UInt       n_base_points;
    FT_UInt       n;


    if ( !loader )
      return;

    base    = &loader->base;
    current = &loader->current;

    n_curr_contours = current->outline.n_contours;
    n_base_points   = base->outline.n_points;

    base->outline.n_points =
      (short)( base->outline.n_points + current->outline.n_points );
    base->outline.n_contours =
      (short)( base->outline.n_contours + current->outline.n_contours );

    base->num_subglyphs += current->num_subglyphs;

    
    for ( n = 0; n < n_curr_contours; n++ )
      current->outline.contours[n] =
        (short)( current->outline.contours[n] + n_base_points );

    
    FT_GlyphLoader_Prepare( loader );
  }


  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_CopyPoints( FT_GlyphLoader  target,
                             FT_GlyphLoader  source )
  {
    FT_Error  error;
    FT_UInt   num_points   = source->base.outline.n_points;
    FT_UInt   num_contours = source->base.outline.n_contours;


    error = FT_GlyphLoader_CheckPoints( target, num_points, num_contours );
    if ( !error )
    {
      FT_Outline*  out = &target->base.outline;
      FT_Outline*  in  = &source->base.outline;


      FT_ARRAY_COPY( out->points, in->points,
                     num_points );
      FT_ARRAY_COPY( out->tags, in->tags,
                     num_points );
      FT_ARRAY_COPY( out->contours, in->contours,
                     num_contours );

      
      if ( target->use_extra && source->use_extra )
      {
        FT_ARRAY_COPY( target->base.extra_points, source->base.extra_points,
                       num_points );
        FT_ARRAY_COPY( target->base.extra_points2, source->base.extra_points2,
                       num_points );
      }

      out->n_points   = (short)num_points;
      out->n_contours = (short)num_contours;

      FT_GlyphLoader_Adjust_Points( target );
    }

    return error;
  }



