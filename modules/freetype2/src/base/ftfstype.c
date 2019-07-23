
















#include <ft2build.h>
#include FT_TYPE1_TABLES_H
#include FT_TRUETYPE_TABLES_H
#include FT_INTERNAL_SERVICE_H
#include FT_SERVICE_POSTSCRIPT_INFO_H


  

  FT_EXPORT_DEF( FT_UShort )
  FT_Get_FSType_Flags( FT_Face  face )
  {
    TT_OS2*  os2;


    
    if ( face )
    {
      FT_Service_PsInfo  service = NULL;


      FT_FACE_FIND_SERVICE( face, service, POSTSCRIPT_INFO );

      if ( service && service->ps_get_font_extra )
      {
        PS_FontExtraRec  extra;


        if ( !service->ps_get_font_extra( face, &extra ) &&
             extra.fs_type != 0                          )
          return extra.fs_type;
      }
    }

    

    if ( ( os2 = (TT_OS2*)FT_Get_Sfnt_Table( face, ft_sfnt_os2 ) ) != NULL &&
         os2->version != 0xFFFFU                                           )
      return os2->fsType;

    return 0;
  }



