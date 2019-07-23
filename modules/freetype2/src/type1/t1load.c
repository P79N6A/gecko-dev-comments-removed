

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_CONFIG_CONFIG_H
#include FT_MULTIPLE_MASTERS_H
#include FT_INTERNAL_TYPE1_TYPES_H

#include "t1load.h"
#include "t1errors.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1load


#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT


  
  
  
  
  
  
  

  static FT_Error
  t1_allocate_blend( T1_Face  face,
                     FT_UInt  num_designs,
                     FT_UInt  num_axis )
  {
    PS_Blend   blend;
    FT_Memory  memory = face->root.memory;
    FT_Error   error  = T1_Err_Ok;


    blend = face->blend;
    if ( !blend )
    {
      if ( FT_NEW( blend ) )
        goto Exit;

      blend->num_default_design_vector = 0;

      face->blend = blend;
    }

    
    if ( num_designs > 0 )
    {
      if ( blend->num_designs == 0 )
      {
        FT_UInt  nn;


        
        if ( FT_NEW_ARRAY( blend->font_infos[1], num_designs     ) ||
             FT_NEW_ARRAY( blend->privates[1], num_designs       ) ||
             FT_NEW_ARRAY( blend->bboxes[1], num_designs         ) ||
             FT_NEW_ARRAY( blend->weight_vector, num_designs * 2 ) )
          goto Exit;

        blend->default_weight_vector = blend->weight_vector + num_designs;

        blend->font_infos[0] = &face->type1.font_info;
        blend->privates  [0] = &face->type1.private_dict;
        blend->bboxes    [0] = &face->type1.font_bbox;

        for ( nn = 2; nn <= num_designs; nn++ )
        {
          blend->privates[nn]   = blend->privates  [nn - 1] + 1;
          blend->font_infos[nn] = blend->font_infos[nn - 1] + 1;
          blend->bboxes[nn]     = blend->bboxes    [nn - 1] + 1;
        }

        blend->num_designs   = num_designs;
      }
      else if ( blend->num_designs != num_designs )
        goto Fail;
    }

    
    if ( num_axis > 0 )
    {
      if ( blend->num_axis != 0 && blend->num_axis != num_axis )
        goto Fail;

      blend->num_axis = num_axis;
    }

    
    num_designs = blend->num_designs;
    num_axis    = blend->num_axis;
    if ( num_designs && num_axis && blend->design_pos[0] == 0 )
    {
      FT_UInt  n;


      if ( FT_NEW_ARRAY( blend->design_pos[0], num_designs * num_axis ) )
        goto Exit;

      for ( n = 1; n < num_designs; n++ )
        blend->design_pos[n] = blend->design_pos[0] + num_axis * n;
    }

  Exit:
    return error;

  Fail:
    error = T1_Err_Invalid_File_Format;
    goto Exit;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Get_Multi_Master( T1_Face           face,
                       FT_Multi_Master*  master )
  {
    PS_Blend  blend = face->blend;
    FT_UInt   n;
    FT_Error  error;


    error = T1_Err_Invalid_Argument;

    if ( blend )
    {
      master->num_axis    = blend->num_axis;
      master->num_designs = blend->num_designs;

      for ( n = 0; n < blend->num_axis; n++ )
      {
        FT_MM_Axis*   axis = master->axis + n;
        PS_DesignMap  map = blend->design_map + n;


        axis->name    = blend->axis_names[n];
        axis->minimum = map->design_points[0];
        axis->maximum = map->design_points[map->num_points - 1];
      }

      error = T1_Err_Ok;
    }

    return error;
  }


#define FT_INT_TO_FIXED( a )  ( (a) << 16 )
#define FT_FIXED_TO_INT( a )  ( FT_RoundFix( a ) >> 16 )


  
  
  
  
  
  FT_LOCAL_DEF( FT_Fixed )
  mm_axis_unmap( PS_DesignMap  axismap,
                 FT_Fixed      ncv )
  {
    int  j;


    if ( ncv <= axismap->blend_points[0] )
      return axismap->design_points[0];

    for ( j = 1; j < axismap->num_points; ++j )
    {
      if ( ncv <= axismap->blend_points[j] )
      {
        FT_Fixed  t = FT_MulDiv( ncv - axismap->blend_points[j - 1],
                                 0x10000L,
                                 axismap->blend_points[j] -
                                   axismap->blend_points[j - 1] );


        return axismap->design_points[j - 1] +
                 FT_MulDiv( t,
                            axismap->design_points[j] -
                              axismap->design_points[j - 1],
                            1L );
      }
    }

    return axismap->design_points[axismap->num_points - 1];
  }


  
  
  
  
  
  FT_LOCAL_DEF( void )
  mm_weights_unmap( FT_Fixed*  weights,
                    FT_Fixed*  axiscoords,
                    FT_UInt    axis_count )
  {
    FT_ASSERT( axis_count <= T1_MAX_MM_AXIS );

    if ( axis_count == 1 )
      axiscoords[0] = weights[1];

    else if ( axis_count == 2 )
    {
      axiscoords[0] = weights[3] + weights[1];
      axiscoords[1] = weights[3] + weights[2];
    }

    else if ( axis_count == 3 )
    {
      axiscoords[0] = weights[7] + weights[5] + weights[3] + weights[1];
      axiscoords[1] = weights[7] + weights[6] + weights[3] + weights[2];
      axiscoords[2] = weights[7] + weights[6] + weights[5] + weights[4];
    }

    else
    {
      axiscoords[0] = weights[15] + weights[13] + weights[11] + weights[9] +
                        weights[7] + weights[5] + weights[3] + weights[1];
      axiscoords[1] = weights[15] + weights[14] + weights[11] + weights[10] +
                        weights[7] + weights[6] + weights[3] + weights[2];
      axiscoords[2] = weights[15] + weights[14] + weights[13] + weights[12] +
                        weights[7] + weights[6] + weights[5] + weights[4];
      axiscoords[3] = weights[15] + weights[14] + weights[13] + weights[12] +
                        weights[11] + weights[10] + weights[9] + weights[8];
    }
  }


  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  T1_Get_MM_Var( T1_Face      face,
                 FT_MM_Var*  *master )
  {
    FT_Memory        memory = face->root.memory;
    FT_MM_Var       *mmvar;
    FT_Multi_Master  mmaster;
    FT_Error         error;
    FT_UInt          i;
    FT_Fixed         axiscoords[T1_MAX_MM_AXIS];
    PS_Blend         blend = face->blend;


    error = T1_Get_Multi_Master( face, &mmaster );
    if ( error )
      goto Exit;
    if ( FT_ALLOC( mmvar,
                   sizeof ( FT_MM_Var ) +
                     mmaster.num_axis * sizeof ( FT_Var_Axis ) ) )
      goto Exit;

    mmvar->num_axis        = mmaster.num_axis;
    mmvar->num_designs     = mmaster.num_designs;
    mmvar->num_namedstyles = (FT_UInt)-1;                
    mmvar->axis            = (FT_Var_Axis*)&mmvar[1];
                                      
    mmvar->namedstyle      = NULL;

    for ( i = 0 ; i < mmaster.num_axis; ++i )
    {
      mmvar->axis[i].name    = mmaster.axis[i].name;
      mmvar->axis[i].minimum = FT_INT_TO_FIXED( mmaster.axis[i].minimum);
      mmvar->axis[i].maximum = FT_INT_TO_FIXED( mmaster.axis[i].maximum);
      mmvar->axis[i].def     = ( mmvar->axis[i].minimum +
                                   mmvar->axis[i].maximum ) / 2;
                            
      mmvar->axis[i].strid   = 0xFFFFFFFFUL;   
      mmvar->axis[i].tag     = 0xFFFFFFFFUL;   

      if ( ft_strcmp( mmvar->axis[i].name, "Weight" ) == 0 )
        mmvar->axis[i].tag = FT_MAKE_TAG( 'w', 'g', 'h', 't' );
      else if ( ft_strcmp( mmvar->axis[i].name, "Width" ) == 0 )
        mmvar->axis[i].tag = FT_MAKE_TAG( 'w', 'd', 't', 'h' );
      else if ( ft_strcmp( mmvar->axis[i].name, "OpticalSize" ) == 0 )
        mmvar->axis[i].tag = FT_MAKE_TAG( 'o', 'p', 's', 'z' );
    }

    if ( blend->num_designs == 1U << blend->num_axis )
    {
      mm_weights_unmap( blend->default_weight_vector,
                        axiscoords,
                        blend->num_axis );

      for ( i = 0; i < mmaster.num_axis; ++i )
        mmvar->axis[i].def =
          FT_INT_TO_FIXED( mm_axis_unmap( &blend->design_map[i],
                                          axiscoords[i] ) );
    }

    *master = mmvar;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Set_MM_Blend( T1_Face    face,
                   FT_UInt    num_coords,
                   FT_Fixed*  coords )
  {
    PS_Blend  blend = face->blend;
    FT_Error  error;
    FT_UInt   n, m;


    error = T1_Err_Invalid_Argument;

    if ( blend && blend->num_axis == num_coords )
    {
      
      error = T1_Err_Ok;

      for ( n = 0; n < blend->num_designs; n++ )
      {
        FT_Fixed  result = 0x10000L;  


        for ( m = 0; m < blend->num_axis; m++ )
        {
          FT_Fixed  factor;


          
          factor = coords[m];
          if ( factor < 0 )        factor = 0;
          if ( factor > 0x10000L ) factor = 0x10000L;

          if ( ( n & ( 1 << m ) ) == 0 )
            factor = 0x10000L - factor;

          result = FT_MulFix( result, factor );
        }
        blend->weight_vector[n] = result;
      }

      error = T1_Err_Ok;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Set_MM_Design( T1_Face   face,
                    FT_UInt   num_coords,
                    FT_Long*  coords )
  {
    PS_Blend  blend = face->blend;
    FT_Error  error;
    FT_UInt   n, p;


    error = T1_Err_Invalid_Argument;
    if ( blend && blend->num_axis == num_coords )
    {
      
      FT_Fixed  final_blends[T1_MAX_MM_DESIGNS];


      for ( n = 0; n < blend->num_axis; n++ )
      {
        FT_Long       design  = coords[n];
        FT_Fixed      the_blend;
        PS_DesignMap  map     = blend->design_map + n;
        FT_Long*      designs = map->design_points;
        FT_Fixed*     blends  = map->blend_points;
        FT_Int        before  = -1, after = -1;


        for ( p = 0; p < (FT_UInt)map->num_points; p++ )
        {
          FT_Long  p_design = designs[p];


          
          if ( design == p_design )
          {
            the_blend = blends[p];
            goto Found;
          }

          if ( design < p_design )
          {
            after = p;
            break;
          }

          before = p;
        }

        
        if ( before < 0 )
          the_blend = blends[0];

        else if ( after < 0 )
          the_blend = blends[map->num_points - 1];

        else
          the_blend = FT_MulDiv( design         - designs[before],
                                 blends [after] - blends [before],
                                 designs[after] - designs[before] );

      Found:
        final_blends[n] = the_blend;
      }

      error = T1_Set_MM_Blend( face, num_coords, final_blends );
    }

    return error;
  }


  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  T1_Set_Var_Design( T1_Face    face,
                     FT_UInt    num_coords,
                     FT_Fixed*  coords )
  {
     FT_Long   lcoords[4];          
     FT_UInt   i;
     FT_Error  error;


     error = T1_Err_Invalid_Argument;
     if ( num_coords <= 4 && num_coords > 0 )
     {
       for ( i = 0; i < num_coords; ++i )
         lcoords[i] = FT_FIXED_TO_INT( coords[i] );
       error = T1_Set_MM_Design( face, num_coords, lcoords );
     }

     return error;
  }


  FT_LOCAL_DEF( void )
  T1_Done_Blend( T1_Face  face )
  {
    FT_Memory  memory = face->root.memory;
    PS_Blend   blend  = face->blend;


    if ( blend )
    {
      FT_UInt  num_designs = blend->num_designs;
      FT_UInt  num_axis    = blend->num_axis;
      FT_UInt  n;


      
      FT_FREE( blend->design_pos[0] );
      for ( n = 1; n < num_designs; n++ )
        blend->design_pos[n] = 0;

      
      FT_FREE( blend->privates[1] );
      FT_FREE( blend->font_infos[1] );
      FT_FREE( blend->bboxes[1] );

      for ( n = 0; n < num_designs; n++ )
      {
        blend->privates  [n] = 0;
        blend->font_infos[n] = 0;
        blend->bboxes    [n] = 0;
      }

      
      FT_FREE( blend->weight_vector );
      blend->default_weight_vector = 0;

      
      for ( n = 0; n < num_axis; n++ )
        FT_FREE( blend->axis_names[n] );

      
      for ( n = 0; n < num_axis; n++ )
      {
        PS_DesignMap  dmap = blend->design_map + n;


        FT_FREE( dmap->design_points );
        dmap->num_points = 0;
      }

      FT_FREE( face->blend );
    }
  }


  static void
  parse_blend_axis_types( T1_Face    face,
                          T1_Loader  loader )
  {
    T1_TokenRec  axis_tokens[T1_MAX_MM_AXIS];
    FT_Int       n, num_axis;
    FT_Error     error = T1_Err_Ok;
    PS_Blend     blend;
    FT_Memory    memory;


    
    T1_ToTokenArray( &loader->parser, axis_tokens,
                     T1_MAX_MM_AXIS, &num_axis );
    if ( num_axis < 0 )
    {
      error = T1_Err_Ignore;
      goto Exit;
    }
    if ( num_axis == 0 || num_axis > T1_MAX_MM_AXIS )
    {
      FT_ERROR(( "parse_blend_axis_types: incorrect number of axes: %d\n",
                 num_axis ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    
    error = t1_allocate_blend( face, 0, (FT_UInt)num_axis );
    if ( error )
      goto Exit;

    blend  = face->blend;
    memory = face->root.memory;

    
    for ( n = 0; n < num_axis; n++ )
    {
      T1_Token    token = axis_tokens + n;
      FT_Byte*    name;
      FT_PtrDist  len;


      
      if ( token->start[0] == '/' )
        token->start++;

      len = token->limit - token->start;
      if ( len == 0 )
      {
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      if ( FT_ALLOC( blend->axis_names[n], len + 1 ) )
        goto Exit;

      name = (FT_Byte*)blend->axis_names[n];
      FT_MEM_COPY( name, token->start, len );
      name[len] = 0;
    }

  Exit:
    loader->parser.root.error = error;
  }


  static void
  parse_blend_design_positions( T1_Face    face,
                                T1_Loader  loader )
  {
    T1_TokenRec  design_tokens[T1_MAX_MM_DESIGNS];
    FT_Int       num_designs;
    FT_Int       num_axis;
    T1_Parser    parser = &loader->parser;

    FT_Error     error = T1_Err_Ok;
    PS_Blend     blend;


    
    T1_ToTokenArray( parser, design_tokens,
                     T1_MAX_MM_DESIGNS, &num_designs );
    if ( num_designs < 0 )
    {
      error = T1_Err_Ignore;
      goto Exit;
    }
    if ( num_designs == 0 || num_designs > T1_MAX_MM_DESIGNS )
    {
      FT_ERROR(( "parse_blend_design_positions:" ));
      FT_ERROR(( " incorrect number of designs: %d\n",
                 num_designs ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    {
      FT_Byte*  old_cursor = parser->root.cursor;
      FT_Byte*  old_limit  = parser->root.limit;
      FT_Int    n;


      blend    = face->blend;
      num_axis = 0;  

      for ( n = 0; n < num_designs; n++ )
      {
        T1_TokenRec  axis_tokens[T1_MAX_MM_AXIS];
        T1_Token     token;
        FT_Int       axis, n_axis;


        
        token = design_tokens + n;
        parser->root.cursor = token->start;
        parser->root.limit  = token->limit;
        T1_ToTokenArray( parser, axis_tokens, T1_MAX_MM_AXIS, &n_axis );

        if ( n == 0 )
        {
          if ( n_axis <= 0 || n_axis > T1_MAX_MM_AXIS )
          {
            FT_ERROR(( "parse_blend_design_positions:" ));
            FT_ERROR(( "  invalid number of axes: %d\n",
                       n_axis ));
            error = T1_Err_Invalid_File_Format;
            goto Exit;
          }

          num_axis = n_axis;
          error = t1_allocate_blend( face, num_designs, num_axis );
          if ( error )
            goto Exit;
          blend = face->blend;
        }
        else if ( n_axis != num_axis )
        {
          FT_ERROR(( "parse_blend_design_positions: incorrect table\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }

        
        for ( axis = 0; axis < n_axis; axis++ )
        {
          T1_Token  token2 = axis_tokens + axis;


          parser->root.cursor = token2->start;
          parser->root.limit  = token2->limit;
          blend->design_pos[n][axis] = T1_ToFixed( parser, 0 );
        }
      }

      loader->parser.root.cursor = old_cursor;
      loader->parser.root.limit  = old_limit;
    }

  Exit:
    loader->parser.root.error = error;
  }


  static void
  parse_blend_design_map( T1_Face    face,
                          T1_Loader  loader )
  {
    FT_Error     error  = T1_Err_Ok;
    T1_Parser    parser = &loader->parser;
    PS_Blend     blend;
    T1_TokenRec  axis_tokens[T1_MAX_MM_AXIS];
    FT_Int       n, num_axis;
    FT_Byte*     old_cursor;
    FT_Byte*     old_limit;
    FT_Memory    memory = face->root.memory;


    T1_ToTokenArray( parser, axis_tokens,
                     T1_MAX_MM_AXIS, &num_axis );
    if ( num_axis < 0 )
    {
      error = T1_Err_Ignore;
      goto Exit;
    }
    if ( num_axis == 0 || num_axis > T1_MAX_MM_AXIS )
    {
      FT_ERROR(( "parse_blend_design_map: incorrect number of axes: %d\n",
                 num_axis ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    old_cursor = parser->root.cursor;
    old_limit  = parser->root.limit;

    error = t1_allocate_blend( face, 0, num_axis );
    if ( error )
      goto Exit;
    blend = face->blend;

    
    for ( n = 0; n < num_axis; n++ )
    {
      PS_DesignMap  map = blend->design_map + n;
      T1_Token      axis_token;
      T1_TokenRec   point_tokens[T1_MAX_MM_MAP_POINTS];
      FT_Int        p, num_points;


      axis_token = axis_tokens + n;

      parser->root.cursor = axis_token->start;
      parser->root.limit  = axis_token->limit;
      T1_ToTokenArray( parser, point_tokens,
                       T1_MAX_MM_MAP_POINTS, &num_points );

      if ( num_points <= 0 || num_points > T1_MAX_MM_MAP_POINTS )
      {
        FT_ERROR(( "parse_blend_design_map: incorrect table\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      
      if ( FT_NEW_ARRAY( map->design_points, num_points * 2 ) )
        goto Exit;
      map->blend_points = map->design_points + num_points;
      map->num_points   = (FT_Byte)num_points;

      for ( p = 0; p < num_points; p++ )
      {
        T1_Token  point_token;


        point_token = point_tokens + p;

        
        parser->root.cursor = point_token->start + 1;
        parser->root.limit  = point_token->limit - 1;

        map->design_points[p] = T1_ToInt( parser );
        map->blend_points [p] = T1_ToFixed( parser, 0 );
      }
    }

    parser->root.cursor = old_cursor;
    parser->root.limit  = old_limit;

  Exit:
    parser->root.error = error;
  }


  static void
  parse_weight_vector( T1_Face    face,
                       T1_Loader  loader )
  {
    T1_TokenRec  design_tokens[T1_MAX_MM_DESIGNS];
    FT_Int       num_designs;
    FT_Error     error  = T1_Err_Ok;
    T1_Parser    parser = &loader->parser;
    PS_Blend     blend  = face->blend;
    T1_Token     token;
    FT_Int       n;
    FT_Byte*     old_cursor;
    FT_Byte*     old_limit;


    T1_ToTokenArray( parser, design_tokens,
                     T1_MAX_MM_DESIGNS, &num_designs );
    if ( num_designs < 0 )
    {
      error = T1_Err_Ignore;
      goto Exit;
    }
    if ( num_designs == 0 || num_designs > T1_MAX_MM_DESIGNS )
    {
      FT_ERROR(( "parse_weight_vector:" ));
      FT_ERROR(( " incorrect number of designs: %d\n",
                 num_designs ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    if ( !blend || !blend->num_designs )
    {
      error = t1_allocate_blend( face, num_designs, 0 );
      if ( error )
        goto Exit;
      blend = face->blend;
    }
    else if ( blend->num_designs != (FT_UInt)num_designs )
    {
      FT_ERROR(( "parse_weight_vector:"
                 " /BlendDesignPosition and /WeightVector have\n" ));
      FT_ERROR(( "                    "
                 " different number of elements!\n" ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    old_cursor = parser->root.cursor;
    old_limit  = parser->root.limit;

    for ( n = 0; n < num_designs; n++ )
    {
      token = design_tokens + n;
      parser->root.cursor = token->start;
      parser->root.limit  = token->limit;

      blend->default_weight_vector[n] =
      blend->weight_vector[n]         = T1_ToFixed( parser, 0 );
    }

    parser->root.cursor = old_cursor;
    parser->root.limit  = old_limit;

  Exit:
    parser->root.error = error;
  }


  
  
  static void
  parse_buildchar( T1_Face    face,
                   T1_Loader  loader )
  {
    face->len_buildchar = T1_ToFixedArray( &loader->parser, 0, NULL, 0 );

    return;
  }


#endif 




  
  
  
  
  
  
  

  static FT_Error
  t1_load_keyword( T1_Face         face,
                   T1_Loader       loader,
                   const T1_Field  field )
  {
    FT_Error  error;
    void*     dummy_object;
    void**    objects;
    FT_UInt   max_objects;
    PS_Blend  blend = face->blend;


    
    if ( field->type == T1_FIELD_TYPE_CALLBACK )
    {
      field->reader( (FT_Face)face, loader );
      error = loader->parser.root.error;
      goto Exit;
    }

    
    
    switch ( field->location )
    {
    case T1_FIELD_LOCATION_FONT_INFO:
      dummy_object = &face->type1.font_info;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->font_infos;
        max_objects = blend->num_designs;
      }
      break;

    case T1_FIELD_LOCATION_PRIVATE:
      dummy_object = &face->type1.private_dict;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->privates;
        max_objects = blend->num_designs;
      }
      break;

    case T1_FIELD_LOCATION_BBOX:
      dummy_object = &face->type1.font_bbox;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->bboxes;
        max_objects = blend->num_designs;
      }
      break;

    case T1_FIELD_LOCATION_LOADER:
      dummy_object = loader;
      objects      = &dummy_object;
      max_objects  = 0;
      break;

    case T1_FIELD_LOCATION_FACE:
      dummy_object = face;
      objects      = &dummy_object;
      max_objects  = 0;
      break;

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT
    case T1_FIELD_LOCATION_BLEND:
      dummy_object = face->blend;
      objects      = &dummy_object;
      max_objects  = 0;
      break;
#endif

    default:
      dummy_object = &face->type1;
      objects      = &dummy_object;
      max_objects  = 0;
    }

    if ( field->type == T1_FIELD_TYPE_INTEGER_ARRAY ||
         field->type == T1_FIELD_TYPE_FIXED_ARRAY   )
      error = T1_Load_Field_Table( &loader->parser, field,
                                   objects, max_objects, 0 );
    else
      error = T1_Load_Field( &loader->parser, field,
                             objects, max_objects, 0 );

  Exit:
    return error;
  }


  static void
  parse_private( T1_Face    face,
                 T1_Loader  loader )
  {
    FT_UNUSED( face );

    loader->keywords_encountered |= T1_PRIVATE;
  }


  static int
  read_binary_data( T1_Parser  parser,
                    FT_Long*   size,
                    FT_Byte**  base )
  {
    FT_Byte*  cur;
    FT_Byte*  limit = parser->root.limit;


    
    
    
    
    

    T1_Skip_Spaces( parser );

    cur = parser->root.cursor;

    if ( cur < limit && ft_isdigit( *cur ) )
    {
      *size = T1_ToInt( parser );

      T1_Skip_PS_Token( parser );   

      
      
      *base = parser->root.cursor + 1;

      parser->root.cursor += *size + 1;
      return !parser->root.error;
    }

    FT_ERROR(( "read_binary_data: invalid size field\n" ));
    parser->root.error = T1_Err_Invalid_File_Format;
    return 0;
  }


  
  

  static void
  parse_font_matrix( T1_Face    face,
                     T1_Loader  loader )
  {
    T1_Parser   parser = &loader->parser;
    FT_Matrix*  matrix = &face->type1.font_matrix;
    FT_Vector*  offset = &face->type1.font_offset;
    FT_Face     root   = (FT_Face)&face->root;
    FT_Fixed    temp[6];
    FT_Fixed    temp_scale;
    FT_Int      result;


    result = T1_ToFixedArray( parser, 6, temp, 3 );

    if ( result < 0 )
    {
      parser->root.error = T1_Err_Invalid_File_Format;
      return;
    }

    temp_scale = FT_ABS( temp[3] );

    if ( temp_scale == 0 )
    {
      FT_ERROR(( "parse_font_matrix: invalid font matrix\n" ));
      parser->root.error = T1_Err_Invalid_File_Format;
      return;
    }

    
    
    

    root->units_per_EM = (FT_UShort)( FT_DivFix( 1000 * 0x10000L,
                                                 temp_scale ) >> 16 );

    
    if ( temp_scale != 0x10000L )
    {
      temp[0] = FT_DivFix( temp[0], temp_scale );
      temp[1] = FT_DivFix( temp[1], temp_scale );
      temp[2] = FT_DivFix( temp[2], temp_scale );
      temp[4] = FT_DivFix( temp[4], temp_scale );
      temp[5] = FT_DivFix( temp[5], temp_scale );
      temp[3] = 0x10000L;
    }

    matrix->xx = temp[0];
    matrix->yx = temp[1];
    matrix->xy = temp[2];
    matrix->yy = temp[3];

    
    offset->x = temp[4] >> 16;
    offset->y = temp[5] >> 16;
  }


  static void
  parse_encoding( T1_Face    face,
                  T1_Loader  loader )
  {
    T1_Parser  parser = &loader->parser;
    FT_Byte*   cur;
    FT_Byte*   limit  = parser->root.limit;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    T1_Skip_Spaces( parser );
    cur = parser->root.cursor;
    if ( cur >= limit )
    {
      FT_ERROR(( "parse_encoding: out of bounds!\n" ));
      parser->root.error = T1_Err_Invalid_File_Format;
      return;
    }

    
    
    if ( ft_isdigit( *cur ) || *cur == '[' )
    {
      T1_Encoding  encode          = &face->type1.encoding;
      FT_Int       count, n;
      PS_Table     char_table      = &loader->encoding_table;
      FT_Memory    memory          = parser->root.memory;
      FT_Error     error;
      FT_Bool      only_immediates = 0;


      
      if ( *cur == '[' )
      {
        count           = 256;
        only_immediates = 1;
        parser->root.cursor++;
      }
      else
        count = (FT_Int)T1_ToInt( parser );

      T1_Skip_Spaces( parser );
      if ( parser->root.cursor >= limit )
        return;

      
      loader->num_chars = encode->num_chars = count;
      if ( FT_NEW_ARRAY( encode->char_index, count )     ||
           FT_NEW_ARRAY( encode->char_name,  count )     ||
           FT_SET_ERROR( psaux->ps_table_funcs->init(
                           char_table, count, memory ) ) )
      {
        parser->root.error = error;
        return;
      }

      
      for ( n = 0; n < count; n++ )
      {
        char*  notdef = (char *)".notdef";


        T1_Add_Table( char_table, n, notdef, 8 );
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      n = 0;
      T1_Skip_Spaces( parser );

      while ( parser->root.cursor < limit )
      {
        cur = parser->root.cursor;

        
        if ( *cur == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e'         &&
               cur[2] == 'f'         &&
               IS_PS_DELIM( cur[3] ) )
          {
            FT_TRACE6(( "encoding end\n" ));
            cur += 3;
            break;
          }
        }
        if ( *cur == ']' )
        {
          FT_TRACE6(( "encoding end\n" ));
          cur++;
          break;
        }

        
        if ( ft_isdigit( *cur ) || only_immediates )
        {
          FT_Int  charcode;


          if ( only_immediates )
            charcode = n;
          else
          {
            charcode = (FT_Int)T1_ToInt( parser );
            T1_Skip_Spaces( parser );
          }

          cur = parser->root.cursor;

          if ( *cur == '/' && cur + 2 < limit && n < count )
          {
            FT_PtrDist  len;


            cur++;

            parser->root.cursor = cur;
            T1_Skip_PS_Token( parser );
            if ( parser->root.error )
              return;

            len = parser->root.cursor - cur;

            parser->root.error = T1_Add_Table( char_table, charcode,
                                               cur, len + 1 );
            if ( parser->root.error )
              return;
            char_table->elements[charcode][len] = '\0';

            n++;
          }
        }
        else
        {
          T1_Skip_PS_Token( parser );
          if ( parser->root.error )
            return;
        }

        T1_Skip_Spaces( parser );
      }

      face->type1.encoding_type = T1_ENCODING_TYPE_ARRAY;
      parser->root.cursor       = cur;
    }

    
    
    else
    {
      if ( cur + 17 < limit                                            &&
           ft_strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_STANDARD;

      else if ( cur + 15 < limit                                          &&
                ft_strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_EXPERT;

      else if ( cur + 18 < limit                                             &&
                ft_strncmp( (const char*)cur, "ISOLatin1Encoding", 17 ) == 0 )
        face->type1.encoding_type = T1_ENCODING_TYPE_ISOLATIN1;

      else
        parser->root.error = T1_Err_Ignore;
    }
  }


  static void
  parse_subrs( T1_Face    face,
               T1_Loader  loader )
  {
    T1_Parser  parser = &loader->parser;
    PS_Table   table  = &loader->subrs;
    FT_Memory  memory = parser->root.memory;
    FT_Error   error;
    FT_Int     n, num_subrs;

    PSAux_Service  psaux  = (PSAux_Service)face->psaux;


    T1_Skip_Spaces( parser );

    
    if ( parser->root.cursor < parser->root.limit &&
         *parser->root.cursor == '['              )
    {
      T1_Skip_PS_Token( parser );
      T1_Skip_Spaces  ( parser );
      if ( parser->root.cursor >= parser->root.limit ||
           *parser->root.cursor != ']'               )
        parser->root.error = T1_Err_Invalid_File_Format;
      return;
    }

    num_subrs = (FT_Int)T1_ToInt( parser );

    
    T1_Skip_PS_Token( parser );         
    if ( parser->root.error )
      return;
    T1_Skip_Spaces( parser );

    
    
    if ( !loader->num_subrs )
    {
      error = psaux->ps_table_funcs->init( table, num_subrs, memory );
      if ( error )
        goto Fail;
    }

    
    
    
    
    for ( n = 0; n < num_subrs; n++ )
    {
      FT_Long   idx, size;
      FT_Byte*  base;


      
      
      if ( ft_strncmp( (char*)parser->root.cursor, "dup", 3 ) != 0 )
        break;

      T1_Skip_PS_Token( parser );       

      idx = T1_ToInt( parser );

      if ( !read_binary_data( parser, &size, &base ) )
        return;

      
      
      
      
      T1_Skip_PS_Token( parser );   
      if ( parser->root.error )
        return;
      T1_Skip_Spaces  ( parser );

      if ( ft_strncmp( (char*)parser->root.cursor, "put", 3 ) == 0 )
      {
        T1_Skip_PS_Token( parser ); 
        T1_Skip_Spaces  ( parser );
      }

      
      if ( loader->num_subrs )
        continue;

      
      
      
      
      
      if ( face->type1.private_dict.lenIV >= 0 )
      {
        FT_Byte*  temp;


        
        
        
        if ( size < face->type1.private_dict.lenIV )
        {
          error = T1_Err_Invalid_File_Format;
          goto Fail;
        }

        
        if ( FT_ALLOC( temp, size ) )
          goto Fail;
        FT_MEM_COPY( temp, base, size );
        psaux->t1_decrypt( temp, size, 4330 );
        size -= face->type1.private_dict.lenIV;
        error = T1_Add_Table( table, (FT_Int)idx,
                              temp + face->type1.private_dict.lenIV, size );
        FT_FREE( temp );
      }
      else
        error = T1_Add_Table( table, (FT_Int)idx, base, size );
      if ( error )
        goto Fail;
    }

    if ( !loader->num_subrs )
      loader->num_subrs = num_subrs;

    return;

  Fail:
    parser->root.error = error;
  }


#define TABLE_EXTEND  5


  static void
  parse_charstrings( T1_Face    face,
                     T1_Loader  loader )
  {
    T1_Parser      parser       = &loader->parser;
    PS_Table       code_table   = &loader->charstrings;
    PS_Table       name_table   = &loader->glyph_names;
    PS_Table       swap_table   = &loader->swap_table;
    FT_Memory      memory       = parser->root.memory;
    FT_Error       error;

    PSAux_Service  psaux        = (PSAux_Service)face->psaux;

    FT_Byte*       cur;
    FT_Byte*       limit        = parser->root.limit;
    FT_Int         n, num_glyphs;
    FT_UInt        notdef_index = 0;
    FT_Byte        notdef_found = 0;


    num_glyphs = (FT_Int)T1_ToInt( parser );
    
    
    if ( num_glyphs == 0 || parser->root.error )
      return;

    
    
    

    
    
    
    if ( !loader->num_glyphs )
    {
      error = psaux->ps_table_funcs->init(
                code_table, num_glyphs + 1 + TABLE_EXTEND, memory );
      if ( error )
        goto Fail;

      error = psaux->ps_table_funcs->init(
                name_table, num_glyphs + 1 + TABLE_EXTEND, memory );
      if ( error )
        goto Fail;

      
      

      error = psaux->ps_table_funcs->init( swap_table, 4, memory );
      if ( error )
        goto Fail;
    }

    n = 0;

    for (;;)
    {
      FT_Long   size;
      FT_Byte*  base;


      
      

      T1_Skip_Spaces( parser );

      cur = parser->root.cursor;
      if ( cur >= limit )
        break;

      
      if ( cur + 3 < limit && IS_PS_DELIM( cur[3] ) )
      {
        if ( cur[0] == 'd' &&
             cur[1] == 'e' &&
             cur[2] == 'f' )
        {
          
          
          
          
          
          
          
          
          
          
          if ( n )
            break;
        }

        if ( cur[0] == 'e' &&
             cur[1] == 'n' &&
             cur[2] == 'd' )
          break;
      }

      T1_Skip_PS_Token( parser );
      if ( parser->root.error )
        return;

      if ( *cur == '/' )
      {
        FT_PtrDist  len;


        if ( cur + 1 >= limit )
        {
          error = T1_Err_Invalid_File_Format;
          goto Fail;
        }

        cur++;                              
        len = parser->root.cursor - cur;

        if ( !read_binary_data( parser, &size, &base ) )
          return;

        
        
        
        if ( loader->num_glyphs )
          continue;

        error = T1_Add_Table( name_table, n, cur, len + 1 );
        if ( error )
          goto Fail;

        
        name_table->elements[n][len] = '\0';

        
        if ( *cur == '.'                                              &&
             ft_strcmp( ".notdef",
                        (const char*)(name_table->elements[n]) ) == 0 )
        {
          notdef_index = n;
          notdef_found = 1;
        }

        if ( face->type1.private_dict.lenIV >= 0 &&
             n < num_glyphs + TABLE_EXTEND       )
        {
          FT_Byte*  temp;


          if ( size <= face->type1.private_dict.lenIV )
          {
            error = T1_Err_Invalid_File_Format;
            goto Fail;
          }

          
          if ( FT_ALLOC( temp, size ) )
            goto Fail;
          FT_MEM_COPY( temp, base, size );
          psaux->t1_decrypt( temp, size, 4330 );
          size -= face->type1.private_dict.lenIV;
          error = T1_Add_Table( code_table, n,
                                temp + face->type1.private_dict.lenIV, size );
          FT_FREE( temp );
        }
        else
          error = T1_Add_Table( code_table, n, base, size );
        if ( error )
          goto Fail;

        n++;
      }
    }

    if ( loader->num_glyphs )
      return;
    else
      loader->num_glyphs = n;

    
    if ( ft_strcmp( (const char*)".notdef",
                    (const char*)name_table->elements[0] ) &&
         notdef_found                                      )
    {
      
      
      
      
      

      
      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      
      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      
      error = T1_Add_Table( swap_table, 2,
                            name_table->elements[notdef_index],
                            name_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      
      error = T1_Add_Table( swap_table, 3,
                            code_table->elements[notdef_index],
                            code_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, notdef_index,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, notdef_index,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0,
                            swap_table->elements[2],
                            swap_table->lengths [2] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0,
                            swap_table->elements[3],
                            swap_table->lengths [3] );
      if ( error )
        goto Fail;

    }
    else if ( !notdef_found )
    {
      
      
      
      

      
      FT_Byte  notdef_glyph[] = {0x8B, 0xF7, 0xE1, 0x0D, 0x0E};
      char*    notdef_name    = (char *)".notdef";


      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0, notdef_name, 8 );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0, notdef_glyph, 5 );

      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, n,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, n,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      
      loader->num_glyphs = n + 1;
    }

    return;

  Fail:
    parser->root.error = error;
  }


  
  
  
  
  
  


  static
  const T1_FieldRec  t1_keywords[] =
  {

#include "t1tokens.h"

    
    T1_FIELD_CALLBACK( "FontMatrix",           parse_font_matrix,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "Encoding",             parse_encoding,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "Subrs",                parse_subrs,
                       T1_FIELD_DICT_PRIVATE )
    T1_FIELD_CALLBACK( "CharStrings",          parse_charstrings,
                       T1_FIELD_DICT_PRIVATE )
    T1_FIELD_CALLBACK( "Private",              parse_private,
                       T1_FIELD_DICT_FONTDICT )

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT
    T1_FIELD_CALLBACK( "BlendDesignPositions", parse_blend_design_positions,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "BlendDesignMap",       parse_blend_design_map,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "BlendAxisTypes",       parse_blend_axis_types,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "WeightVector",         parse_weight_vector,
                       T1_FIELD_DICT_FONTDICT )
    T1_FIELD_CALLBACK( "BuildCharArray",       parse_buildchar,
                       T1_FIELD_DICT_PRIVATE )
#endif

    { 0, T1_FIELD_LOCATION_CID_INFO, T1_FIELD_TYPE_NONE, 0, 0, 0, 0, 0, 0 }
  };


#define T1_FIELD_COUNT                                           \
          ( sizeof ( t1_keywords ) / sizeof ( t1_keywords[0] ) )


  static FT_Error
  parse_dict( T1_Face    face,
              T1_Loader  loader,
              FT_Byte*   base,
              FT_Long    size )
  {
    T1_Parser  parser = &loader->parser;
    FT_Byte   *limit, *start_binary = NULL;
    FT_Bool    have_integer = 0;


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = T1_Err_Ok;

    limit = parser->root.limit;

    T1_Skip_Spaces( parser );

    while ( parser->root.cursor < limit )
    {
      FT_Byte*  cur;


      cur = parser->root.cursor;

      
      if ( IS_PS_TOKEN( cur, limit, "eexec" ) )
        break;

      
      else if ( IS_PS_TOKEN( cur, limit, "closefile" ) )
        break;

      
      
      else if ( IS_PS_TOKEN( cur, limit, "FontDirectory" ) )
      {
        if ( loader->keywords_encountered & T1_PRIVATE )
          loader->keywords_encountered |=
            T1_FONTDIR_AFTER_PRIVATE;
        parser->root.cursor += 13;
      }

      
      else if ( ft_isdigit( *cur ) )
      {
        start_binary = cur;
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;
        have_integer = 1;
      }

      
      
      
      else if ( *cur == 'R' && cur + 6 < limit && *(cur + 1) == 'D' &&
                have_integer )
      {
        FT_Long   s;
        FT_Byte*  b;


        parser->root.cursor = start_binary;
        if ( !read_binary_data( parser, &s, &b ) )
          return T1_Err_Invalid_File_Format;
        have_integer = 0;
      }

      else if ( *cur == '-' && cur + 6 < limit && *(cur + 1) == '|' &&
                have_integer )
      {
        FT_Long   s;
        FT_Byte*  b;


        parser->root.cursor = start_binary;
        if ( !read_binary_data( parser, &s, &b ) )
          return T1_Err_Invalid_File_Format;
        have_integer = 0;
      }

      
      else if ( *cur == '/' && cur + 2 < limit )
      {
        FT_PtrDist  len;


        cur++;

        parser->root.cursor = cur;
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;

        len = parser->root.cursor - cur;

        if ( len > 0 && len < 22 && parser->root.cursor < limit )
        {
          
          T1_Field  keyword = (T1_Field)t1_keywords;


          for (;;)
          {
            FT_Byte*  name;


            name = (FT_Byte*)keyword->ident;
            if ( !name )
              break;

            if ( cur[0] == name[0]                                  &&
                 len == (FT_PtrDist)ft_strlen( (const char *)name ) &&
                 ft_memcmp( cur, name, len ) == 0                   )
            {
              
              
              
              
              
              
              

              
              
              
              
              

              
              
              
              

              
              
              
              
              
              
              

              const FT_UInt dict =
                ( loader->keywords_encountered & T1_PRIVATE )
                    ? T1_FIELD_DICT_PRIVATE
                    : T1_FIELD_DICT_FONTDICT;

              if ( !( dict & keyword->dict ) )
              {
                FT_TRACE1(( "parse_dict: found %s but ignoring it "
                            "since it is in the wrong dictionary\n",
                            keyword->ident ));
                break;
              }

              if ( !( loader->keywords_encountered &
                      T1_FONTDIR_AFTER_PRIVATE     )                  ||
                   ft_strcmp( (const char*)name, "CharStrings" ) == 0 )
              {
                parser->root.error = t1_load_keyword( face,
                                                      loader,
                                                      keyword );
                if ( parser->root.error != T1_Err_Ok )
                {
                  if ( FT_ERROR_BASE( parser->root.error ) == FT_Err_Ignore )
                    parser->root.error = T1_Err_Ok;
                  else
                    return parser->root.error;
                }
              }
              break;
            }

            keyword++;
          }
        }

        have_integer = 0;
      }
      else
      {
        T1_Skip_PS_Token( parser );
        if ( parser->root.error )
          goto Exit;
        have_integer = 0;
      }

      T1_Skip_Spaces( parser );
    }

  Exit:
    return parser->root.error;
  }


  static void
  t1_init_loader( T1_Loader  loader,
                  T1_Face    face )
  {
    FT_UNUSED( face );

    FT_MEM_ZERO( loader, sizeof ( *loader ) );
    loader->num_glyphs = 0;
    loader->num_chars  = 0;

    
    loader->encoding_table.init  = 0;
    loader->charstrings.init     = 0;
    loader->glyph_names.init     = 0;
    loader->subrs.init           = 0;
    loader->swap_table.init      = 0;
    loader->fontdata             = 0;
    loader->keywords_encountered = 0;
  }


  static void
  t1_done_loader( T1_Loader  loader )
  {
    T1_Parser  parser = &loader->parser;


    
    T1_Release_Table( &loader->encoding_table );
    T1_Release_Table( &loader->charstrings );
    T1_Release_Table( &loader->glyph_names );
    T1_Release_Table( &loader->swap_table );
    T1_Release_Table( &loader->subrs );

    
    T1_Finalize_Parser( parser );
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Open_Face( T1_Face  face )
  {
    T1_LoaderRec   loader;
    T1_Parser      parser;
    T1_Font        type1 = &face->type1;
    PS_Private     priv  = &type1->private_dict;
    FT_Error       error;

    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    t1_init_loader( &loader, face );

    
    face->ndv_idx          = -1;
    face->cdv_idx          = -1;
    face->len_buildchar    = 0;

    priv->blue_shift       = 7;
    priv->blue_fuzz        = 1;
    priv->lenIV            = 4;
    priv->expansion_factor = (FT_Fixed)( 0.06 * 0x10000L );
    priv->blue_scale       = (FT_Fixed)( 0.039625 * 0x10000L * 1000 );

    parser = &loader.parser;
    error  = T1_New_Parser( parser,
                            face->root.stream,
                            face->root.memory,
                            psaux );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader,
                        parser->base_dict, parser->base_len );
    if ( error )
      goto Exit;

    error = T1_Get_Private_Dict( parser, psaux );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader,
                        parser->private_dict, parser->private_len );
    if ( error )
      goto Exit;

    
    priv->num_blue_values &= ~1;

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT

    if ( face->blend                                                     &&
         face->blend->num_default_design_vector != 0                     &&
         face->blend->num_default_design_vector != face->blend->num_axis )
    {
      
      FT_ERROR(( "T1_Open_Face(): /DesignVector contains %u entries "
                 "while there are %u axes.\n",
                 face->blend->num_default_design_vector,
                 face->blend->num_axis ));

      face->blend->num_default_design_vector = 0;
    }

    
    
    if ( face->blend                                             &&
         ( !face->blend->num_designs || !face->blend->num_axis ) )
      T1_Done_Blend( face );

    
    if ( face->blend )
    {
      FT_UInt  i;


      for ( i = 0; i < face->blend->num_axis; i++ )
        if ( !face->blend->design_map[i].num_points )
        {
          T1_Done_Blend( face );
          break;
        }
    }

    if ( face->blend )
    {
      if ( face->len_buildchar > 0 )
      {
        FT_Memory  memory = face->root.memory;


        if ( FT_NEW_ARRAY( face->buildchar, face->len_buildchar ) )
        {
          FT_ERROR(( "T1_Open_Face: cannot allocate BuildCharArray\n" ));
          face->len_buildchar = 0;
          goto Exit;
        }
      }
    }

#endif 

    
    
    type1->num_glyphs = loader.num_glyphs;

    if ( loader.subrs.init )
    {
      loader.subrs.init  = 0;
      type1->num_subrs   = loader.num_subrs;
      type1->subrs_block = loader.subrs.block;
      type1->subrs       = loader.subrs.elements;
      type1->subrs_len   = loader.subrs.lengths;
    }

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    if ( !face->root.internal->incremental_interface )
#endif
      if ( !loader.charstrings.init )
      {
        FT_ERROR(( "T1_Open_Face: no `/CharStrings' array in face!\n" ));
        error = T1_Err_Invalid_File_Format;
      }

    loader.charstrings.init  = 0;
    type1->charstrings_block = loader.charstrings.block;
    type1->charstrings       = loader.charstrings.elements;
    type1->charstrings_len   = loader.charstrings.lengths;

    
    
    type1->glyph_names_block    = loader.glyph_names.block;
    type1->glyph_names          = (FT_String**)loader.glyph_names.elements;
    loader.glyph_names.block    = 0;
    loader.glyph_names.elements = 0;

    
    if ( type1->encoding_type == T1_ENCODING_TYPE_ARRAY )
    {
      FT_Int    charcode, idx, min_char, max_char;
      FT_Byte*  char_name;
      FT_Byte*  glyph_name;


      
      
      
      

      min_char = +32000;
      max_char = -32000;

      charcode = 0;
      for ( ; charcode < loader.encoding_table.max_elems; charcode++ )
      {
        type1->encoding.char_index[charcode] = 0;
        type1->encoding.char_name [charcode] = (char *)".notdef";

        char_name = loader.encoding_table.elements[charcode];
        if ( char_name )
          for ( idx = 0; idx < type1->num_glyphs; idx++ )
          {
            glyph_name = (FT_Byte*)type1->glyph_names[idx];
            if ( ft_strcmp( (const char*)char_name,
                            (const char*)glyph_name ) == 0 )
            {
              type1->encoding.char_index[charcode] = (FT_UShort)idx;
              type1->encoding.char_name [charcode] = (char*)glyph_name;

              
              
              if ( ft_strcmp( (const char*)".notdef",
                              (const char*)glyph_name ) != 0 )
              {
                if ( charcode < min_char )
                  min_char = charcode;
                if ( charcode > max_char )
                  max_char = charcode;
              }
              break;
            }
          }
      }

      




      if ( min_char > max_char )
      {
        min_char = 0;
        max_char = loader.encoding_table.max_elems;
      }

      type1->encoding.code_first = min_char;
      type1->encoding.code_last  = max_char;
      type1->encoding.num_chars  = loader.num_chars;
    }

  Exit:
    t1_done_loader( &loader );
    return error;
  }



