

















#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H

#include "psmodule.h"
#include "pstables.h"

#include "psnamerr.h"


#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES


#ifdef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST


#define VARIANT_BIT         0x80000000UL
#define BASE_GLYPH( code )  ( (code) & ~VARIANT_BIT )


  
  
  
  
  
  static FT_UInt32
  ps_unicode_value( const char*  glyph_name )
  {
    
    
    if ( glyph_name[0] == 'u' &&
         glyph_name[1] == 'n' &&
         glyph_name[2] == 'i' )
    {
      
      

      
      

      FT_Int       count;
      FT_ULong     value = 0;
      const char*  p     = glyph_name + 3;


      for ( count = 4; count > 0; count--, p++ )
      {
        char          c = *p;
        unsigned int  d;


        d = (unsigned char)c - '0';
        if ( d >= 10 )
        {
          d = (unsigned char)c - 'A';
          if ( d >= 6 )
            d = 16;
          else
            d += 10;
        }

        
        
        
        if ( d >= 16 )
          break;

        value = ( value << 4 ) + d;
      }

      
      if ( count == 0 )
      {
        if ( *p == '\0' )
          return value;
        if ( *p == '.' )
          return value | VARIANT_BIT;
      }
    }

    
    
    if ( glyph_name[0] == 'u' )
    {
      FT_Int       count;
      FT_ULong     value = 0;
      const char*  p     = glyph_name + 1;


      for ( count = 6; count > 0; count--, p++ )
      {
        char          c = *p;
        unsigned int  d;


        d = (unsigned char)c - '0';
        if ( d >= 10 )
        {
          d = (unsigned char)c - 'A';
          if ( d >= 6 )
            d = 16;
          else
            d += 10;
        }

        if ( d >= 16 )
          break;

        value = ( value << 4 ) + d;
      }

      if ( count <= 2 )
      {
        if ( *p == '\0' )
          return value;
        if ( *p == '.' )
          return value | VARIANT_BIT;
      }
    }

    
    
    {
      const char*  p   = glyph_name;
      const char*  dot = NULL;


      for ( ; *p; p++ )
      {
        if ( *p == '.' && p > glyph_name )
        {
          dot = p;
          break;
        }
      }

      
      if ( !dot )
        return ft_get_adobe_glyph_index( glyph_name, p );
      else
        return ft_get_adobe_glyph_index( glyph_name, dot ) | VARIANT_BIT;
    }
  }


  
  FT_CALLBACK_DEF( int )
  compare_uni_maps( const void*  a,
                    const void*  b )
  {
    PS_UniMap*  map1 = (PS_UniMap*)a;
    PS_UniMap*  map2 = (PS_UniMap*)b;
    FT_UInt32   unicode1 = BASE_GLYPH( map1->unicode );
    FT_UInt32   unicode2 = BASE_GLYPH( map2->unicode );


    
    if ( unicode1 == unicode2 )
      return map1->unicode - map2->unicode;
    else
      return unicode1 - unicode2;
  }


  

#define WGL_EXTRA_LIST_SIZE  8

  static const FT_UInt32  ft_wgl_extra_unicodes[WGL_EXTRA_LIST_SIZE] =
  {
    0x0394,
    0x03A9,
    0x2215,
    0x00AD,
    0x02C9,
    0x03BC,
    0x2219,
    0x00A0
  };

  static const char  ft_wgl_extra_glyph_names[] =
  {
    'D','e','l','t','a',0,
    'O','m','e','g','a',0,
    'f','r','a','c','t','i','o','n',0,
    'h','y','p','h','e','n',0,
    'm','a','c','r','o','n',0,
    'm','u',0,
    'p','e','r','i','o','d','c','e','n','t','e','r','e','d',0,
    's','p','a','c','e',0
  };

  static const FT_Int
  ft_wgl_extra_glyph_name_offsets[WGL_EXTRA_LIST_SIZE] =
  {
     0,
     6,
    12,
    21,
    28,
    35,
    38,
    53
  };


  static void
  ps_check_wgl_name( const char*  gname,
                     FT_UInt      glyph,
                     FT_UInt*     wgl_glyphs,
                     FT_UInt     *states )
  {
    FT_UInt  n;


    for ( n = 0; n < WGL_EXTRA_LIST_SIZE; n++ )
    {
      if ( ft_strcmp( ft_wgl_extra_glyph_names +
                        ft_wgl_extra_glyph_name_offsets[n], gname ) == 0 )
      {
        if ( states[n] == 0 )
        {
          
          states[n]     = 1;
          wgl_glyphs[n] = glyph;
        }

        return;
      }
    }
  }


  static void
  ps_check_wgl_unicode( FT_UInt32  uni_char,
                        FT_UInt   *states )
  {
    FT_UInt  n;


    for ( n = 0; n < WGL_EXTRA_LIST_SIZE; n++ )
    {
      if ( uni_char == ft_wgl_extra_unicodes[n] )
      {
        
        states[n] = 2;

        return;
      }
    }
  }


  
  static FT_Error
  ps_unicodes_init( FT_Memory             memory,
                    PS_Unicodes           table,
                    FT_UInt               num_glyphs,
                    PS_GetGlyphNameFunc   get_glyph_name,
                    PS_FreeGlyphNameFunc  free_glyph_name,
                    FT_Pointer            glyph_data )
  {
    FT_Error  error;

    FT_UInt  wgl_list_states[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    FT_UInt  wgl_glyphs[WGL_EXTRA_LIST_SIZE];


    
    table->num_maps = 0;
    table->maps     = 0;

    if ( !FT_NEW_ARRAY( table->maps, num_glyphs + WGL_EXTRA_LIST_SIZE ) )
    {
      FT_UInt     n;
      FT_UInt     count;
      PS_UniMap*  map;
      FT_UInt32   uni_char;


      map = table->maps;

      for ( n = 0; n < num_glyphs; n++ )
      {
        const char*  gname = get_glyph_name( glyph_data, n );


        if ( gname )
        {
          ps_check_wgl_name( gname, n, wgl_glyphs, wgl_list_states );
          uni_char = ps_unicode_value( gname );

          if ( BASE_GLYPH( uni_char ) != 0 )
          {
            ps_check_wgl_unicode( uni_char, wgl_list_states );
            map->unicode     = uni_char;
            map->glyph_index = n;
            map++;
          }

          if ( free_glyph_name )
            free_glyph_name( glyph_data, gname );
        }
      }

      for ( n = 0; n < WGL_EXTRA_LIST_SIZE; n++ )
      {
        if ( wgl_list_states[n] == 1 )
        {
          
          

          map->unicode     = ft_wgl_extra_unicodes[n];
          map->glyph_index = wgl_glyphs[n];
          map++;
        }
      }

      
      count = (FT_UInt)( map - table->maps );

      if ( count == 0 )
      {
        FT_FREE( table->maps );
        if ( !error )
          error = PSnames_Err_Invalid_Argument;  
      }
      else {
        
        if ( count < num_glyphs / 2 )
        {
          (void)FT_RENEW_ARRAY( table->maps, num_glyphs, count );
          error = PSnames_Err_Ok;
        }

        
        
        ft_qsort( table->maps, count, sizeof ( PS_UniMap ),
                  compare_uni_maps );
      }

      table->num_maps = count;
    }

    return error;
  }


  static FT_UInt
  ps_unicodes_char_index( PS_Unicodes  table,
                          FT_UInt32    unicode )
  {
    PS_UniMap  *min, *max, *mid, *result = NULL;


    

    min = table->maps;
    max = min + table->num_maps - 1;

    while ( min <= max )
    {
      FT_UInt32  base_glyph;


      mid = min + ( ( max - min ) >> 1 );

      if ( mid->unicode == unicode )
      {
        result = mid;
        break;
      }

      base_glyph = BASE_GLYPH( mid->unicode );

      if ( base_glyph == unicode )
        result = mid; 

      if ( min == max )
        break;

      if ( base_glyph < unicode )
        min = mid + 1;
      else
        max = mid - 1;
    }

    if ( result )
      return result->glyph_index;
    else
      return 0;
  }


  static FT_ULong
  ps_unicodes_char_next( PS_Unicodes  table,
                         FT_UInt32   *unicode )
  {
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *unicode + 1;


    {
      FT_UInt     min = 0;
      FT_UInt     max = table->num_maps;
      FT_UInt     mid;
      PS_UniMap*  map;
      FT_UInt32   base_glyph;


      while ( min < max )
      {
        mid = min + ( ( max - min ) >> 1 );
        map = table->maps + mid;

        if ( map->unicode == char_code )
        {
          result = map->glyph_index;
          goto Exit;
        }

        base_glyph = BASE_GLYPH( map->unicode );

        if ( base_glyph == char_code )
          result = map->glyph_index;

        if ( base_glyph < char_code )
          min = mid + 1;
        else
          max = mid;
      }

      if ( result )
        goto Exit;               

      
      char_code = 0;

      if ( min < table->num_maps )
      {
        map       = table->maps + min;
        result    = map->glyph_index;
        char_code = BASE_GLYPH( map->unicode );
      }
    }

  Exit:
    *unicode = char_code;
    return result;
  }


#endif 


  static const char*
  ps_get_macintosh_name( FT_UInt  name_index )
  {
    if ( name_index >= FT_NUM_MAC_NAMES )
      name_index = 0;

    return ft_standard_glyph_names + ft_mac_names[name_index];
  }


  static const char*
  ps_get_standard_strings( FT_UInt  sid )
  {
    if ( sid >= FT_NUM_SID_NAMES )
      return 0;

    return ft_standard_glyph_names + ft_sid_names[sid];
  }


  static
  const FT_Service_PsCMapsRec  pscmaps_interface =
  {
#ifdef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST

    (PS_Unicode_ValueFunc)     ps_unicode_value,
    (PS_Unicodes_InitFunc)     ps_unicodes_init,
    (PS_Unicodes_CharIndexFunc)ps_unicodes_char_index,
    (PS_Unicodes_CharNextFunc) ps_unicodes_char_next,

#else

    0,
    0,
    0,
    0,

#endif 

    (PS_Macintosh_NameFunc)    ps_get_macintosh_name,
    (PS_Adobe_Std_StringsFunc) ps_get_standard_strings,

    t1_standard_encoding,
    t1_expert_encoding
  };


  static const FT_ServiceDescRec  pscmaps_services[] =
  {
    { FT_SERVICE_ID_POSTSCRIPT_CMAPS, &pscmaps_interface },
    { NULL, NULL }
  };


  static FT_Pointer
  psnames_get_service( FT_Module    module,
                       const char*  service_id )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( pscmaps_services, service_id );
  }

#endif 



  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  psnames_module_class =
  {
    0,  
    sizeof ( FT_ModuleRec ),

    "psnames",  
    0x10000L,   
    0x20000L,   

#ifndef FT_CONFIG_OPTION_POSTSCRIPT_NAMES
    0,
    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  0
#else
    (void*)&pscmaps_interface,   
    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  psnames_get_service
#endif
  };



