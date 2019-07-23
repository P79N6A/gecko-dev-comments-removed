

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H

#include "cidgload.h"
#include "cidload.h"

#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H

#include "ciderrs.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidobjs


  
  
  
  
  

  FT_LOCAL_DEF( void )
  cid_slot_done( FT_GlyphSlot  slot )
  {
    slot->internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF( FT_Error )
  cid_slot_init( FT_GlyphSlot  slot )
  {
    CID_Face          face;
    PSHinter_Service  pshinter;


    face     = (CID_Face)slot->face;
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


  
  
  
  
  


  static PSH_Globals_Funcs
  cid_size_get_globals_funcs( CID_Size  size )
  {
    CID_Face          face     = (CID_Face)size->root.face;
    PSHinter_Service  pshinter = (PSHinter_Service)face->pshinter;
    FT_Module         module;


    module = FT_Get_Module( size->root.face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0;
  }


  FT_LOCAL_DEF( void )
  cid_size_done( FT_Size  cidsize )         
  {
    CID_Size  size = (CID_Size)cidsize;


    if ( cidsize->internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = cid_size_get_globals_funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)cidsize->internal );

      cidsize->internal = 0;
    }
  }


  FT_LOCAL_DEF( FT_Error )
  cid_size_init( FT_Size  cidsize )     
  {
    CID_Size           size  = (CID_Size)cidsize;
    FT_Error           error = 0;
    PSH_Globals_Funcs  funcs = cid_size_get_globals_funcs( size );


    if ( funcs )
    {
      PSH_Globals   globals;
      CID_Face      face = (CID_Face)cidsize->face;
      CID_FaceDict  dict = face->cid.font_dicts + face->root.face_index;
      PS_Private    priv = &dict->private_dict;


      error = funcs->create( cidsize->face->memory, priv, &globals );
      if ( !error )
        cidsize->internal = (FT_Size_Internal)(void*)globals;
    }

    return error;
  }


  FT_LOCAL( FT_Error )
  cid_size_request( FT_Size          size,
                    FT_Size_Request  req )
  {
    PSH_Globals_Funcs  funcs;


    FT_Request_Metrics( size->face, req );

    funcs = cid_size_get_globals_funcs( (CID_Size)size );

    if ( funcs )
      funcs->set_scale( (PSH_Globals)size->internal,
                        size->metrics.x_scale,
                        size->metrics.y_scale,
                        0, 0 );

    return CID_Err_Ok;
  }


  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  cid_face_done( FT_Face  cidface )         
  {
    CID_Face   face = (CID_Face)cidface;
    FT_Memory  memory;


    if ( face )
    {
      CID_FaceInfo  cid  = &face->cid;
      PS_FontInfo   info = &cid->font_info;


      memory = cidface->memory;

      
      if ( face->subrs )
      {
        FT_Int  n;


        for ( n = 0; n < cid->num_dicts; n++ )
        {
          CID_Subrs  subr = face->subrs + n;


          if ( subr->code )
          {
            FT_FREE( subr->code[0] );
            FT_FREE( subr->code );
          }
        }

        FT_FREE( face->subrs );
      }

      
      FT_FREE( info->version );
      FT_FREE( info->notice );
      FT_FREE( info->full_name );
      FT_FREE( info->family_name );
      FT_FREE( info->weight );

      
      FT_FREE( cid->font_dicts );
      cid->num_dicts = 0;

      
      FT_FREE( cid->cid_font_name );
      FT_FREE( cid->registry );
      FT_FREE( cid->ordering );

      cidface->family_name = 0;
      cidface->style_name  = 0;

      FT_FREE( face->binary_data );
      FT_FREE( face->cid_stream );
    }
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  cid_face_init( FT_Stream      stream,
                 FT_Face        cidface,        
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    CID_Face          face = (CID_Face)cidface;
    FT_Error          error;
    PSAux_Service     psaux;
    PSHinter_Service  pshinter;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( stream );


    cidface->num_faces = 1;

    psaux = (PSAux_Service)face->psaux;
    if ( !psaux )
    {
      psaux = (PSAux_Service)FT_Get_Module_Interface(
                FT_FACE_LIBRARY( face ), "psaux" );

      face->psaux = psaux;
    }

    pshinter = (PSHinter_Service)face->pshinter;
    if ( !pshinter )
    {
      pshinter = (PSHinter_Service)FT_Get_Module_Interface(
                   FT_FACE_LIBRARY( face ), "pshinter" );

      face->pshinter = pshinter;
    }

    
    if ( FT_STREAM_SEEK( 0 ) )
      goto Exit;

    error = cid_face_open( face, face_index );
    if ( error )
      goto Exit;

    
    if ( face_index < 0 )
      goto Exit;

    
    if ( face_index != 0 )
    {
      FT_ERROR(( "cid_face_init: invalid face index\n" ));
      error = CID_Err_Invalid_Argument;
      goto Exit;
    }

    

    

    
    {
      CID_FaceInfo  cid  = &face->cid;
      PS_FontInfo   info = &cid->font_info;


      cidface->num_glyphs   = cid->cid_count;
      cidface->num_charmaps = 0;

      cidface->face_index = face_index;
      cidface->face_flags = FT_FACE_FLAG_SCALABLE   | 
                            FT_FACE_FLAG_HORIZONTAL | 
                            FT_FACE_FLAG_HINTER;      

      if ( info->is_fixed_pitch )
        cidface->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      

      
      
      cidface->family_name = info->family_name;
      
      cidface->style_name = (char *)"Regular";
      if ( cidface->family_name )
      {
        char*  full   = info->full_name;
        char*  family = cidface->family_name;


        if ( full )
        {
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
                if ( !*family )
                  cidface->style_name = full;
                break;
              }
            }
          }
        }
      }
      else
      {
        
        if ( cid->cid_font_name )
          cidface->family_name = cid->cid_font_name;
      }

      
      cidface->style_flags = 0;
      if ( info->italic_angle )
        cidface->style_flags |= FT_STYLE_FLAG_ITALIC;
      if ( info->weight )
      {
        if ( !ft_strcmp( info->weight, "Bold"  ) ||
             !ft_strcmp( info->weight, "Black" ) )
          cidface->style_flags |= FT_STYLE_FLAG_BOLD;
      }

      
      cidface->num_fixed_sizes = 0;
      cidface->available_sizes = 0;

      cidface->bbox.xMin =   cid->font_bbox.xMin             >> 16;
      cidface->bbox.yMin =   cid->font_bbox.yMin             >> 16;
      cidface->bbox.xMax = ( cid->font_bbox.xMax + 0xFFFFU ) >> 16;
      cidface->bbox.yMax = ( cid->font_bbox.yMax + 0xFFFFU ) >> 16;

      if ( !cidface->units_per_EM )
        cidface->units_per_EM = 1000;

      cidface->ascender  = (FT_Short)( cidface->bbox.yMax );
      cidface->descender = (FT_Short)( cidface->bbox.yMin );

      cidface->height = (FT_Short)( ( cidface->units_per_EM * 12 ) / 10 );
      if ( cidface->height < cidface->ascender - cidface->descender )
        cidface->height = (FT_Short)( cidface->ascender - cidface->descender );

      cidface->underline_position  = (FT_Short)info->underline_position;
      cidface->underline_thickness = (FT_Short)info->underline_thickness;
    }

  Exit:
    return error;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  cid_driver_init( FT_Module  driver )
  {
    FT_UNUSED( driver );

    return CID_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( void )
  cid_driver_done( FT_Module  driver )
  {
    FT_UNUSED( driver );
  }



