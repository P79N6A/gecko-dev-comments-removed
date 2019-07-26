

















#include <ft2build.h>
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_IDS_H

#include "t1gload.h"
#include "t1load.h"

#include "t1errors.h"

#ifndef T1_CONFIG_OPTION_NO_AFM
#include "t1afm.h"
#endif

#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1objs


  
  
  
  
  
  
  
  


  static PSH_Globals_Funcs
  T1_Size_Get_Globals_Funcs( T1_Size  size )
  {
    T1_Face           face     = (T1_Face)size->root.face;
    PSHinter_Service  pshinter = (PSHinter_Service)face->pshinter;
    FT_Module         module;


    module = FT_Get_Module( size->root.face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0 ;
  }


  FT_LOCAL_DEF( void )
  T1_Size_Done( FT_Size  t1size )          
  {
    T1_Size  size = (T1_Size)t1size;


    if ( size->root.internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = T1_Size_Get_Globals_Funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)size->root.internal );

      size->root.internal = 0;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Size_Init( FT_Size  t1size )      
  {
    T1_Size            size  = (T1_Size)t1size;
    FT_Error           error = FT_Err_Ok;
    PSH_Globals_Funcs  funcs = T1_Size_Get_Globals_Funcs( size );


    if ( funcs )
    {
      PSH_Globals  globals;
      T1_Face      face = (T1_Face)size->root.face;


      error = funcs->create( size->root.face->memory,
                             &face->type1.private_dict, &globals );
      if ( !error )
        size->root.internal = (FT_Size_Internal)(void*)globals;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Size_Request( FT_Size          t1size,     
                   FT_Size_Request  req )
  {
    T1_Size            size  = (T1_Size)t1size;
    PSH_Globals_Funcs  funcs = T1_Size_Get_Globals_Funcs( size );


    FT_Request_Metrics( size->root.face, req );

    if ( funcs )
      funcs->set_scale( (PSH_Globals)size->root.internal,
                        size->root.metrics.x_scale,
                        size->root.metrics.y_scale,
                        0, 0 );

    return FT_Err_Ok;
  }


  
  
  
  
  

  FT_LOCAL_DEF( void )
  T1_GlyphSlot_Done( FT_GlyphSlot  slot )
  {
    slot->internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_GlyphSlot_Init( FT_GlyphSlot  slot )
  {
    T1_Face           face;
    PSHinter_Service  pshinter;


    face     = (T1_Face)slot->face;
    pshinter = (PSHinter_Service)face->pshinter;

    if ( pshinter )
    {
      FT_Module  module;


      module = FT_Get_Module( slot->face->driver->root.library,
                              "pshinter" );
      if ( module )
      {
        T1_Hints_Funcs  funcs;


        funcs = pshinter->get_t1_funcs( module );
        slot->internal->glyph_hints = (void*)funcs;
      }
    }

    return 0;
  }


  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  T1_Face_Done( FT_Face  t1face )         
  {
    T1_Face    face = (T1_Face)t1face;
    FT_Memory  memory;
    T1_Font    type1;


    if ( !face )
      return;

    memory = face->root.memory;
    type1  = &face->type1;

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT
    
    FT_ASSERT( ( face->len_buildchar == 0 ) == ( face->buildchar == NULL ) );

    if ( face->buildchar )
    {
      FT_FREE( face->buildchar );

      face->buildchar     = NULL;
      face->len_buildchar = 0;
    }

    T1_Done_Blend( face );
    face->blend = 0;
#endif

    
    {
      PS_FontInfo  info = &type1->font_info;


      FT_FREE( info->version );
      FT_FREE( info->notice );
      FT_FREE( info->full_name );
      FT_FREE( info->family_name );
      FT_FREE( info->weight );
    }

    
    FT_FREE( type1->charstrings_len );
    FT_FREE( type1->charstrings );
    FT_FREE( type1->glyph_names );

    FT_FREE( type1->subrs );
    FT_FREE( type1->subrs_len );

    FT_FREE( type1->subrs_block );
    FT_FREE( type1->charstrings_block );
    FT_FREE( type1->glyph_names_block );

    FT_FREE( type1->encoding.char_index );
    FT_FREE( type1->encoding.char_name );
    FT_FREE( type1->font_name );

#ifndef T1_CONFIG_OPTION_NO_AFM
    
    if ( face->afm_data )
      T1_Done_Metrics( memory, (AFM_FontInfo)face->afm_data );
#endif

    
#if 0
    FT_FREE( face->unicode_map_rec.maps );
    face->unicode_map_rec.num_maps = 0;
    face->unicode_map              = NULL;
#endif

    face->root.family_name = NULL;
    face->root.style_name  = NULL;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  T1_Face_Init( FT_Stream      stream,
                FT_Face        t1face,          
                FT_Int         face_index,
                FT_Int         num_params,
                FT_Parameter*  params )
  {
    T1_Face             face = (T1_Face)t1face;
    FT_Error            error;
    FT_Service_PsCMaps  psnames;
    PSAux_Service       psaux;
    T1_Font             type1 = &face->type1;
    PS_FontInfo         info = &type1->font_info;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( stream );


    face->root.num_faces = 1;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    face->psnames = psnames;

    face->psaux = FT_Get_Module_Interface( FT_FACE_LIBRARY( face ),
                                           "psaux" );
    psaux = (PSAux_Service)face->psaux;
    if ( !psaux )
    {
      FT_ERROR(( "T1_Face_Init: cannot access `psaux' module\n" ));
      error = FT_THROW( Missing_Module );
      goto Exit;
    }

    face->pshinter = FT_Get_Module_Interface( FT_FACE_LIBRARY( face ),
                                              "pshinter" );

    FT_TRACE2(( "Type 1 driver\n" ));

    
    error = T1_Open_Face( face );
    if ( error )
      goto Exit;

    
    if ( face_index < 0 )
      goto Exit;

    
    if ( face_index > 0 )
    {
      FT_ERROR(( "T1_Face_Init: invalid face index\n" ));
      error = FT_THROW( Invalid_Argument );
      goto Exit;
    }

    

    

    
    {
      FT_Face  root = (FT_Face)&face->root;


      root->num_glyphs = type1->num_glyphs;
      root->face_index = 0;

      root->face_flags |= FT_FACE_FLAG_SCALABLE    |
                          FT_FACE_FLAG_HORIZONTAL  |
                          FT_FACE_FLAG_GLYPH_NAMES |
                          FT_FACE_FLAG_HINTER;

      if ( info->is_fixed_pitch )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      if ( face->blend )
        root->face_flags |= FT_FACE_FLAG_MULTIPLE_MASTERS;

      


      
      
      
      
      

      
      
      root->family_name = info->family_name;
      root->style_name  = NULL;

      if ( root->family_name )
      {
        char*  full   = info->full_name;
        char*  family = root->family_name;


        if ( full )
        {
          FT_Bool  the_same = TRUE;


          while ( *full )
          {
            if ( *full == *family )
            {
              family++;
              full++;
            }
            else
            {
              if ( *full == ' ' || *full == '-' )
                full++;
              else if ( *family == ' ' || *family == '-' )
                family++;
              else
              {
                the_same = FALSE;

                if ( !*family )
                  root->style_name = full;
                break;
              }
            }
          }

          if ( the_same )
            root->style_name = (char *)"Regular";
        }
      }
      else
      {
        
        if ( type1->font_name )
          root->family_name = type1->font_name;
      }

      if ( !root->style_name )
      {
        if ( info->weight )
          root->style_name = info->weight;
        else
          
          root->style_name = (char *)"Regular";
      }

      
      root->style_flags = 0;
      if ( info->italic_angle )
        root->style_flags |= FT_STYLE_FLAG_ITALIC;
      if ( info->weight )
      {
        if ( !ft_strcmp( info->weight, "Bold"  ) ||
             !ft_strcmp( info->weight, "Black" ) )
          root->style_flags |= FT_STYLE_FLAG_BOLD;
      }

      
      root->num_fixed_sizes = 0;
      root->available_sizes = 0;

      root->bbox.xMin =   type1->font_bbox.xMin            >> 16;
      root->bbox.yMin =   type1->font_bbox.yMin            >> 16;
      
      root->bbox.xMax = ( type1->font_bbox.xMax + 0xFFFF ) >> 16;
      root->bbox.yMax = ( type1->font_bbox.yMax + 0xFFFF ) >> 16;

      
      if ( !root->units_per_EM )
        root->units_per_EM = 1000;

      root->ascender  = (FT_Short)( root->bbox.yMax );
      root->descender = (FT_Short)( root->bbox.yMin );

      root->height = (FT_Short)( ( root->units_per_EM * 12 ) / 10 );
      if ( root->height < root->ascender - root->descender )
        root->height = (FT_Short)( root->ascender - root->descender );

      
      root->max_advance_width =
        (FT_Short)( root->bbox.xMax );
      {
        FT_Pos  max_advance;


        error = T1_Compute_Max_Advance( face, &max_advance );

        
        if ( !error )
          root->max_advance_width = (FT_Short)FIXED_TO_INT( max_advance );
        else
          error = FT_Err_Ok;   
      }

      root->max_advance_height = root->height;

      root->underline_position  = (FT_Short)info->underline_position;
      root->underline_thickness = (FT_Short)info->underline_thickness;
    }

    {
      FT_Face  root = &face->root;


      if ( psnames )
      {
        FT_CharMapRec    charmap;
        T1_CMap_Classes  cmap_classes = psaux->t1_cmap_classes;
        FT_CMap_Class    clazz;


        charmap.face = root;

        
        charmap.platform_id = TT_PLATFORM_MICROSOFT;
        charmap.encoding_id = TT_MS_ID_UNICODE_CS;
        charmap.encoding    = FT_ENCODING_UNICODE;

        error = FT_CMap_New( cmap_classes->unicode, NULL, &charmap, NULL );
        if ( error                                      &&
             FT_ERR_NEQ( error, No_Unicode_Glyph_Name ) )
          goto Exit;
        error = FT_Err_Ok;

        
        charmap.platform_id = TT_PLATFORM_ADOBE;
        clazz               = NULL;

        switch ( type1->encoding_type )
        {
        case T1_ENCODING_TYPE_STANDARD:
          charmap.encoding    = FT_ENCODING_ADOBE_STANDARD;
          charmap.encoding_id = TT_ADOBE_ID_STANDARD;
          clazz               = cmap_classes->standard;
          break;

        case T1_ENCODING_TYPE_EXPERT:
          charmap.encoding    = FT_ENCODING_ADOBE_EXPERT;
          charmap.encoding_id = TT_ADOBE_ID_EXPERT;
          clazz               = cmap_classes->expert;
          break;

        case T1_ENCODING_TYPE_ARRAY:
          charmap.encoding    = FT_ENCODING_ADOBE_CUSTOM;
          charmap.encoding_id = TT_ADOBE_ID_CUSTOM;
          clazz               = cmap_classes->custom;
          break;

        case T1_ENCODING_TYPE_ISOLATIN1:
          charmap.encoding    = FT_ENCODING_ADOBE_LATIN_1;
          charmap.encoding_id = TT_ADOBE_ID_LATIN_1;
          clazz               = cmap_classes->unicode;
          break;

        default:
          ;
        }

        if ( clazz )
          error = FT_CMap_New( clazz, NULL, &charmap, NULL );

#if 0
        
        if (root->num_charmaps)
          root->charmap = root->charmaps[0];
#endif
      }
    }

  Exit:
    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  T1_Driver_Init( FT_Module  driver )
  {
    FT_UNUSED( driver );

    return FT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  T1_Driver_Done( FT_Module  driver )
  {
    FT_UNUSED( driver );
  }



