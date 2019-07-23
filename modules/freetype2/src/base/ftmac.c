





















  












































#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_STREAM_H

  
  
  
#if !HAVE_ANSI_OS_INLINE
#undef  OS_INLINE
#define OS_INLINE  static __inline__
#endif

  
#ifndef HAVE_TYPE_RESOURCE_INDEX
typedef short ResourceIndex;
#endif

#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <sys/syslimits.h> 

#define FT_DEPRECATED_ATTRIBUTE

#include FT_MAC_H

  
#undef FT_GetFile_From_Mac_Name( a, b, c )
#undef FT_GetFile_From_Mac_ATS_Name( a, b, c )
#undef FT_New_Face_From_FOND( a, b, c, d )
#undef FT_New_Face_From_FSSpec( a, b, c, d )
#undef FT_New_Face_From_FSRef( a, b, c, d )

#ifndef kATSOptionFlagsUnRestrictedScope 
#define kATSOptionFlagsUnRestrictedScope kATSOptionFlagsDefault
#endif


  


#ifndef PREFER_LWFN
#define PREFER_LWFN  1
#endif


  
  FT_EXPORT_DEF( FT_Error )
  FT_GetFile_From_Mac_Name( const char*  fontName,
                            FSSpec*      pathSpec,
                            FT_Long*     face_index )
  {
    FT_UNUSED( fontName );
    FT_UNUSED( pathSpec );
    FT_UNUSED( face_index );

    return FT_Err_Unimplemented_Feature;
  }


  
  
  
  
  
  static OSStatus
  FT_ATSFontGetFileReference( ATSFontRef  ats_font_id,
                              FSRef*      ats_font_ref )
  {
#if defined( MAC_OS_X_VERSION_10_5 ) && \
    ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
 
    OSStatus  err;

    err = ATSFontGetFileReference( ats_font_id, ats_font_ref );

    return err;
#elif __LP64__ 
    FT_UNUSED( ats_font_id );
    FT_UNUSED( ats_font_ref );


    return fnfErr;
#else 
    OSStatus  err;
    FSSpec    spec;


    err = ATSFontGetFileSpecification( ats_font_id, &spec );
    if ( noErr == err )
      err = FSpMakeFSRef( &spec, ats_font_ref );

    return err;
#endif
  }


  static FT_Error
  FT_GetFileRef_From_Mac_ATS_Name( const char*  fontName,
                                   FSRef*       ats_font_ref,
                                   FT_Long*     face_index )
  {
    CFStringRef  cf_fontName;
    ATSFontRef   ats_font_id;


    *face_index = 0;

    cf_fontName = CFStringCreateWithCString( NULL, fontName,
                                             kCFStringEncodingMacRoman );
    ats_font_id = ATSFontFindFromName( cf_fontName,
                                       kATSOptionFlagsUnRestrictedScope );
    CFRelease( cf_fontName );

    if ( ats_font_id == 0 || ats_font_id == 0xFFFFFFFFUL )
      return FT_Err_Unknown_File_Format;

    if ( noErr != FT_ATSFontGetFileReference( ats_font_id, ats_font_ref ) )
      return FT_Err_Unknown_File_Format;

    
    
    {
      ATSFontRef  id2 = ats_font_id - 1;
      FSRef       ref2;


      while ( id2 > 0 )
      {
        if ( noErr != FT_ATSFontGetFileReference( id2, &ref2 ) )
          break;
        if ( noErr != FSCompareFSRefs( ats_font_ref, &ref2 ) )
          break;

        id2 --;
      }
      *face_index = ats_font_id - ( id2 + 1 );
    }

    return FT_Err_Ok;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_GetFilePath_From_Mac_ATS_Name( const char*  fontName,
                                    UInt8*       path,
                                    UInt32       maxPathSize,
                                    FT_Long*     face_index )
  {
    FSRef     ref;
    FT_Error  err;


    err = FT_GetFileRef_From_Mac_ATS_Name( fontName, &ref, face_index );
    if ( FT_Err_Ok != err )
      return err;

    if ( noErr != FSRefMakePath( &ref, path, maxPathSize ) )
      return FT_Err_Unknown_File_Format;

    return FT_Err_Ok;
  }


  
  FT_EXPORT_DEF( FT_Error )
  FT_GetFile_From_Mac_ATS_Name( const char*  fontName,
                                FSSpec*      pathSpec,
                                FT_Long*     face_index )
  {
#if ( __LP64__ ) || ( defined( MAC_OS_X_VERSION_10_5 ) && \
      ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 ) )
    FT_UNUSED( fontName );
    FT_UNUSED( pathSpec );
    FT_UNUSED( face_index );

    return FT_Err_Unimplemented_Feature;
#else
    FSRef     ref;
    FT_Error  err;


    err = FT_GetFileRef_From_Mac_ATS_Name( fontName, &ref, face_index );
    if ( FT_Err_Ok != err )
      return err;

    if ( noErr != FSGetCatalogInfo( &ref, kFSCatInfoNone, NULL, NULL,
                                    pathSpec, NULL ) )
      return FT_Err_Unknown_File_Format;

    return FT_Err_Ok;
#endif
  }


  static OSErr
  FT_FSPathMakeRes( const UInt8*    pathname,
                    ResFileRefNum*  res )
  {
    OSErr  err;
    FSRef  ref;


    if ( noErr != FSPathMakeRef( pathname, &ref, FALSE ) )
      return FT_Err_Cannot_Open_Resource;

    
    err = FSOpenResourceFile( &ref, 0, NULL, fsRdPerm, res );
    if ( noErr == err )
      return err;

    
    *res = FSOpenResFile( &ref, fsRdPerm );
    err  = ResError();

    return err;
  }


  
  static OSType
  get_file_type_from_path( const UInt8*  pathname )
  {
    FSRef          ref;
    FSCatalogInfo  info;


    if ( noErr != FSPathMakeRef( pathname, &ref, FALSE ) )
      return ( OSType ) 0;

    if ( noErr != FSGetCatalogInfo( &ref, kFSCatInfoFinderInfo, &info,
                                    NULL, NULL, NULL ) )
      return ( OSType ) 0;

    return ((FInfo *)(info.finderInfo))->fdType;
  }


  
  static void
  create_lwfn_name( char*   ps_name,
                    Str255  lwfn_file_name )
  {
    int       max = 5, count = 0;
    FT_Byte*  p = lwfn_file_name;
    FT_Byte*  q = (FT_Byte*)ps_name;


    lwfn_file_name[0] = 0;

    while ( *q )
    {
      if ( ft_isupper( *q ) )
      {
        if ( count )
          max = 3;
        count = 0;
      }
      if ( count < max && ( ft_isalnum( *q ) || *q == '_' ) )
      {
        *++p = *q;
        lwfn_file_name[0]++;
        count++;
      }
      q++;
    }
  }


  static short
  count_faces_sfnt( char*  fond_data )
  {
    
    

    return EndianS16_BtoN( *( (short*)( fond_data +
                                        sizeof ( FamRec ) ) ) ) + 1;
  }


  static short
  count_faces_scalable( char*  fond_data )
  {
    AsscEntry*  assoc;
    FamRec*     fond;
    short       i, face, face_all;


    fond     = (FamRec*)fond_data;
    face_all = EndianS16_BtoN( *( (short *)( fond_data +
                                             sizeof ( FamRec ) ) ) ) + 1;
    assoc    = (AsscEntry*)( fond_data + sizeof ( FamRec ) + 2 );
    face     = 0;

    for ( i = 0; i < face_all; i++ )
    {
      if ( 0 == EndianS16_BtoN( assoc[i].fontSize ) )
        face++;
    }
    return face;
  }


  







  static void
  parse_fond( char*   fond_data,
              short*  have_sfnt,
              ResID*  sfnt_id,
              Str255  lwfn_file_name,
              short   face_index )
  {
    AsscEntry*  assoc;
    AsscEntry*  base_assoc;
    FamRec*     fond;


    *sfnt_id          = 0;
    *have_sfnt        = 0;
    lwfn_file_name[0] = 0;

    fond       = (FamRec*)fond_data;
    assoc      = (AsscEntry*)( fond_data + sizeof ( FamRec ) + 2 );
    base_assoc = assoc;

    
    if ( 47 < face_index )
      return;

    
    if ( face_index < count_faces_sfnt( fond_data ) )
    {
      assoc += face_index;        

      

      if ( EndianS16_BtoN( assoc->fontSize ) == 0 )
      {
        *have_sfnt = 1;
        *sfnt_id   = EndianS16_BtoN( assoc->fontID );
      }
      else if ( base_assoc->fontSize == 0 )
      {
        *have_sfnt = 1;
        *sfnt_id   = EndianS16_BtoN( base_assoc->fontID );
      }
    }

    if ( EndianS32_BtoN( fond->ffStylOff ) )
    {
      unsigned char*  p = (unsigned char*)fond_data;
      StyleTable*     style;
      unsigned short  string_count;
      char            ps_name[256];
      unsigned char*  names[64];
      int             i;


      p += EndianS32_BtoN( fond->ffStylOff );
      style = (StyleTable*)p;
      p += sizeof ( StyleTable );
      string_count = EndianS16_BtoN( *(short*)(p) );
      p += sizeof ( short );

      for ( i = 0; i < string_count && i < 64; i++ )
      {
        names[i] = p;
        p       += names[i][0];
        p++;
      }

      {
        size_t  ps_name_len = (size_t)names[0][0];


        if ( ps_name_len != 0 )
        {
          ft_memcpy(ps_name, names[0] + 1, ps_name_len);
          ps_name[ps_name_len] = 0;
        }
        if ( style->indexes[face_index] > 1 &&
             style->indexes[face_index] <= FT_MIN( string_count, 64 ) )
        {
          unsigned char*  suffixes = names[style->indexes[face_index] - 1];


          for ( i = 1; i <= suffixes[0]; i++ )
          {
            unsigned char*  s;
            size_t          j = suffixes[i] - 1;


            if ( j < string_count && ( s = names[j] ) != NULL )
            {
              size_t  s_len = (size_t)s[0];


              if ( s_len != 0 && ps_name_len + s_len < sizeof ( ps_name ) )
              {
                ft_memcpy( ps_name + ps_name_len, s + 1, s_len );
                ps_name_len += s_len;
                ps_name[ps_name_len] = 0;
              }
            }
          }
        }
      }

      create_lwfn_name( ps_name, lwfn_file_name );
    }
  }


  static  FT_Error
  lookup_lwfn_by_fond( const UInt8*      path_fond,
                       ConstStr255Param  base_lwfn,
                       UInt8*            path_lwfn,
                       size_t            path_size )
  {
    FSRef   ref, par_ref;
    size_t  dirname_len;


    
    

    if ( noErr != FSPathMakeRef( path_fond, &ref, FALSE ) )
      return FT_Err_Invalid_Argument;

    if ( noErr != FSGetCatalogInfo( &ref, kFSCatInfoNone,
                                    NULL, NULL, NULL, &par_ref ) )
      return FT_Err_Invalid_Argument;

    if ( noErr != FSRefMakePath( &par_ref, path_lwfn, path_size ) )
      return FT_Err_Invalid_Argument;

    if ( ft_strlen( (char *)path_lwfn ) + 1 + base_lwfn[0] > path_size )
      return FT_Err_Invalid_Argument;

    
    ft_strcat( (char *)path_lwfn, "/" );
    dirname_len = ft_strlen( (char *)path_lwfn );
    ft_strcat( (char *)path_lwfn, (char *)base_lwfn + 1 );
    path_lwfn[dirname_len + base_lwfn[0]] = '\0';

    if ( noErr != FSPathMakeRef( path_lwfn, &ref, FALSE ) )
      return FT_Err_Cannot_Open_Resource;

    if ( noErr != FSGetCatalogInfo( &ref, kFSCatInfoNone,
                                    NULL, NULL, NULL, NULL ) )
      return FT_Err_Cannot_Open_Resource;

    return FT_Err_Ok;
  }


  static short
  count_faces( Handle        fond,
               const UInt8*  pathname )
  {
    ResID     sfnt_id;
    short     have_sfnt, have_lwfn;
    Str255    lwfn_file_name;
    UInt8     buff[PATH_MAX];
    FT_Error  err;
    short     num_faces;


    have_sfnt = have_lwfn = 0;

    parse_fond( *fond, &have_sfnt, &sfnt_id, lwfn_file_name, 0 );

    if ( lwfn_file_name[0] )
    {
      err = lookup_lwfn_by_fond( pathname, lwfn_file_name,
                                 buff, sizeof ( buff )  );
      if ( FT_Err_Ok == err )
        have_lwfn = 1;
    }

    if ( have_lwfn && ( !have_sfnt || PREFER_LWFN ) )
      num_faces = 1;
    else
      num_faces = count_faces_scalable( *fond );

    return num_faces;
  }


  




  static FT_Error
  read_lwfn( FT_Memory      memory,
             ResFileRefNum  res,
             FT_Byte**      pfb_data,
             FT_ULong*      size )
  {
    FT_Error       error = FT_Err_Ok;
    ResID          res_id;
    unsigned char  *buffer, *p, *size_p = NULL;
    FT_ULong       total_size = 0;
    FT_ULong       old_total_size = 0;
    FT_ULong       post_size, pfb_chunk_size;
    Handle         post_data;
    char           code, last_code;


    UseResFile( res );

    
    
    res_id    = 501;
    last_code = -1;

    for (;;)
    {
      post_data = Get1Resource( FT_MAKE_TAG( 'P', 'O', 'S', 'T' ),
                                res_id++ );
      if ( post_data == NULL )
        break;  

      code = (*post_data)[0];

      if ( code != last_code )
      {
        if ( code == 5 )
          total_size += 2; 
        else
          total_size += 6; 
      }

      total_size += GetHandleSize( post_data ) - 2;
      last_code = code;

      
      if ( total_size < old_total_size )
      {
        error = FT_Err_Array_Too_Large;
        goto Error;
      }

      old_total_size = total_size;
    }

    if ( FT_ALLOC( buffer, (FT_Long)total_size ) )
      goto Error;

    
    
    p              = buffer;
    res_id         = 501;
    last_code      = -1;
    pfb_chunk_size = 0;

    for (;;)
    {
      post_data = Get1Resource( FT_MAKE_TAG( 'P', 'O', 'S', 'T' ),
                                res_id++ );
      if ( post_data == NULL )
        break;  

      post_size = (FT_ULong)GetHandleSize( post_data ) - 2;
      code = (*post_data)[0];

      if ( code != last_code )
      {
        if ( last_code != -1 )
        {
          
          if ( size_p != NULL )
          {
            *size_p++ = (FT_Byte)(   pfb_chunk_size         & 0xFF );
            *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 8  ) & 0xFF );
            *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 16 ) & 0xFF );
            *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 24 ) & 0xFF );
          }
          pfb_chunk_size = 0;
        }

        *p++ = 0x80;
        if ( code == 5 )
          *p++ = 0x03;  
        else if ( code == 2 )
          *p++ = 0x02;  
        else
          *p++ = 0x01;  

        if ( code != 5 )
        {
          size_p = p;   
          p += 4;       
        }
      }

      ft_memcpy( p, *post_data + 2, post_size );
      pfb_chunk_size += post_size;
      p += post_size;
      last_code = code;
    }

    *pfb_data = buffer;
    *size = total_size;

  Error:
    CloseResFile( res );
    return error;
  }


  

  static void
  memory_stream_close( FT_Stream  stream )
  {
    FT_Memory  memory = stream->memory;


    FT_FREE( stream->base );

    stream->size  = 0;
    stream->base  = 0;
    stream->close = 0;
  }


  
  static FT_Error
  new_memory_stream( FT_Library           library,
                     FT_Byte*             base,
                     FT_ULong             size,
                     FT_Stream_CloseFunc  close,
                     FT_Stream*           astream )
  {
    FT_Error   error;
    FT_Memory  memory;
    FT_Stream  stream;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !base )
      return FT_Err_Invalid_Argument;

    *astream = 0;
    memory = library->memory;
    if ( FT_NEW( stream ) )
      goto Exit;

    FT_Stream_OpenMemory( stream, base, size );

    stream->close = close;

    *astream = stream;

  Exit:
    return error;
  }


  
  static FT_Error
  open_face_from_buffer( FT_Library   library,
                         FT_Byte*     base,
                         FT_ULong     size,
                         FT_Long      face_index,
                         const char*  driver_name,
                         FT_Face*     aface )
  {
    FT_Open_Args  args;
    FT_Error      error;
    FT_Stream     stream;
    FT_Memory     memory = library->memory;


    error = new_memory_stream( library,
                               base,
                               size,
                               memory_stream_close,
                               &stream );
    if ( error )
    {
      FT_FREE( base );
      return error;
    }

    args.flags  = FT_OPEN_STREAM;
    args.stream = stream;
    if ( driver_name )
    {
      args.flags  = args.flags | FT_OPEN_DRIVER;
      args.driver = FT_Get_Module( library, driver_name );
    }

    
    
    
    

    if ( face_index > 0 )
      face_index = 0;

    error = FT_Open_Face( library, &args, face_index, aface );
    if ( error == FT_Err_Ok )
      (*aface)->face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;
    else
      FT_Stream_Free( stream, 0 );

    return error;
  }


  
  static FT_Error
  FT_New_Face_From_LWFN( FT_Library    library,
                         const UInt8*  pathname,
                         FT_Long       face_index,
                         FT_Face*      aface )
  {
    FT_Byte*       pfb_data;
    FT_ULong       pfb_size;
    FT_Error       error;
    ResFileRefNum  res;


    if ( noErr != FT_FSPathMakeRes( pathname, &res ) )
      return FT_Err_Cannot_Open_Resource;

    pfb_data = NULL;
    pfb_size = 0;
    error = read_lwfn( library->memory, res, &pfb_data, &pfb_size );
    CloseResFile( res ); 
    if ( error )
      return error;

    return open_face_from_buffer( library,
                                  pfb_data,
                                  pfb_size,
                                  face_index,
                                  "type1",
                                  aface );
  }


  
  static FT_Error
  FT_New_Face_From_SFNT( FT_Library  library,
                         ResID       sfnt_id,
                         FT_Long     face_index,
                         FT_Face*    aface )
  {
    Handle     sfnt = NULL;
    FT_Byte*   sfnt_data;
    size_t     sfnt_size;
    FT_Error   error  = FT_Err_Ok;
    FT_Memory  memory = library->memory;
    int        is_cff;


    sfnt = GetResource( FT_MAKE_TAG( 's', 'f', 'n', 't' ), sfnt_id );
    if ( sfnt == NULL )
      return FT_Err_Invalid_Handle;

    sfnt_size = (FT_ULong)GetHandleSize( sfnt );
    if ( FT_ALLOC( sfnt_data, (FT_Long)sfnt_size ) )
    {
      ReleaseResource( sfnt );
      return error;
    }

    ft_memcpy( sfnt_data, *sfnt, sfnt_size );
    ReleaseResource( sfnt );

    is_cff = sfnt_size > 4 && sfnt_data[0] == 'O' &&
                              sfnt_data[1] == 'T' &&
                              sfnt_data[2] == 'T' &&
                              sfnt_data[3] == 'O';

    return open_face_from_buffer( library,
                                  sfnt_data,
                                  sfnt_size,
                                  face_index,
                                  is_cff ? "cff" : "truetype",
                                  aface );
  }


  
  static FT_Error
  FT_New_Face_From_Suitcase( FT_Library    library,
                             const UInt8*  pathname,
                             FT_Long       face_index,
                             FT_Face*      aface )
  {
    FT_Error       error = FT_Err_Cannot_Open_Resource;
    ResFileRefNum  res_ref;
    ResourceIndex  res_index;
    Handle         fond;
    short          num_faces_in_res, num_faces_in_fond;


    if ( noErr != FT_FSPathMakeRes( pathname, &res_ref ) )
      return FT_Err_Cannot_Open_Resource;

    UseResFile( res_ref );
    if ( ResError() )
      return FT_Err_Cannot_Open_Resource;

    num_faces_in_res = 0;
    for ( res_index = 1; ; ++res_index )
    {
      fond = Get1IndResource( FT_MAKE_TAG( 'F', 'O', 'N', 'D' ),
                              res_index );
      if ( ResError() )
        break;

      num_faces_in_fond  = count_faces( fond, pathname );
      num_faces_in_res  += num_faces_in_fond;

      if ( 0 <= face_index && face_index < num_faces_in_fond && error )
        error = FT_New_Face_From_FOND( library, fond, face_index, aface );

      face_index -= num_faces_in_fond;
    }

    CloseResFile( res_ref );
    if ( FT_Err_Ok == error && NULL != aface && NULL != *aface )
      (*aface)->num_faces = num_faces_in_res;
    return error;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_New_Face_From_FOND( FT_Library  library,
                         Handle      fond,
                         FT_Long     face_index,
                         FT_Face*    aface )
  {
    short     have_sfnt, have_lwfn = 0;
    ResID     sfnt_id, fond_id;
    OSType    fond_type;
    Str255    fond_name;
    Str255    lwfn_file_name;
    UInt8     path_lwfn[PATH_MAX];
    OSErr     err;
    FT_Error  error = FT_Err_Ok;


    GetResInfo( fond, &fond_id, &fond_type, fond_name );
    if ( ResError() != noErr || fond_type != FT_MAKE_TAG( 'F', 'O', 'N', 'D' ) )
      return FT_Err_Invalid_File_Format;

    parse_fond( *fond, &have_sfnt, &sfnt_id, lwfn_file_name, face_index );

    if ( lwfn_file_name[0] )
    {
      ResFileRefNum  res;


      res = HomeResFile( fond );
      if ( noErr != ResError() )
        goto found_no_lwfn_file;

      {
        UInt8  path_fond[PATH_MAX];
        FSRef  ref;


        err = FSGetForkCBInfo( res, kFSInvalidVolumeRefNum,
                               NULL, NULL, NULL, &ref, NULL );
        if ( noErr != err )
          goto found_no_lwfn_file;

        err = FSRefMakePath( &ref, path_fond, sizeof ( path_fond ) );
        if ( noErr != err )
          goto found_no_lwfn_file;

        error = lookup_lwfn_by_fond( path_fond, lwfn_file_name,
                                     path_lwfn, sizeof ( path_lwfn ) );
        if ( FT_Err_Ok == error )
          have_lwfn = 1;
      }
    }

    if ( have_lwfn && ( !have_sfnt || PREFER_LWFN ) )
      error = FT_New_Face_From_LWFN( library,
                                     path_lwfn,
                                     face_index,
                                     aface );
    else
      error = FT_Err_Unknown_File_Format;

  found_no_lwfn_file:
    if ( have_sfnt && FT_Err_Ok != error )
      error = FT_New_Face_From_SFNT( library,
                                     sfnt_id,
                                     face_index,
                                     aface );

    return error;
  }


  
  static FT_Error
  FT_New_Face_From_Resource( FT_Library    library,
                             const UInt8*  pathname,
                             FT_Long       face_index,
                             FT_Face*      aface )
  {
    OSType    file_type;
    FT_Error  error;


    
    file_type = get_file_type_from_path( pathname );
    if ( file_type == FT_MAKE_TAG( 'L', 'W', 'F', 'N' ) )
      return FT_New_Face_From_LWFN( library, pathname, face_index, aface );

    
    
    

    error = FT_New_Face_From_Suitcase( library, pathname, face_index, aface );
    if ( error == 0 )
      return error;

    
    
    *aface = NULL;
    return 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT_DEF( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  pathname,
               FT_Long      face_index,
               FT_Face*     aface )
  {
    FT_Open_Args  args;
    FT_Error      error;


    
    if ( !pathname )
      return FT_Err_Invalid_Argument;

    error  = FT_Err_Ok;
    *aface = NULL;

    
    error = FT_New_Face_From_Resource( library, (UInt8 *)pathname,
                                       face_index, aface );
    if ( error != 0 || *aface != NULL )
      return error;

    
    args.flags    = FT_OPEN_PATHNAME;
    args.pathname = (char*)pathname;
    return FT_Open_Face( library, &args, face_index, aface );
  }


  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT_DEF( FT_Error )
  FT_New_Face_From_FSRef( FT_Library    library,
                          const FSRef*  ref,
                          FT_Long       face_index,
                          FT_Face*      aface )
  {
    FT_Error      error;
    FT_Open_Args  args;
    OSErr   err;
    UInt8   pathname[PATH_MAX];


    if ( !ref )
      return FT_Err_Invalid_Argument;

    err = FSRefMakePath( ref, pathname, sizeof ( pathname ) );
    if ( err )
      error = FT_Err_Cannot_Open_Resource;

    error = FT_New_Face_From_Resource( library, pathname, face_index, aface );
    if ( error != 0 || *aface != NULL )
      return error;

    
    args.flags    = FT_OPEN_PATHNAME;
    args.pathname = (char*)pathname;
    return FT_Open_Face( library, &args, face_index, aface );
  }


  
  
  
  
  
  
  
  
  
  
  FT_EXPORT_DEF( FT_Error )
  FT_New_Face_From_FSSpec( FT_Library     library,
                           const FSSpec*  spec,
                           FT_Long        face_index,
                           FT_Face*       aface )
  {
#if ( __LP64__ ) || ( defined( MAC_OS_X_VERSION_10_5 ) && \
      ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 ) )
    FT_UNUSED( library );
    FT_UNUSED( spec );
    FT_UNUSED( face_index );
    FT_UNUSED( aface );

    return FT_Err_Unimplemented_Feature;
#else
    FSRef  ref;


    if ( !spec || FSpMakeFSRef( spec, &ref ) != noErr )
      return FT_Err_Invalid_Argument;
    else
      return FT_New_Face_From_FSRef( library, &ref, face_index, aface );
#endif
  }



