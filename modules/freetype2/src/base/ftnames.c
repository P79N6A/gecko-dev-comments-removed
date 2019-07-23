




















#include <ft2build.h>
#include FT_SFNT_NAMES_H
#include FT_INTERNAL_TRUETYPE_TYPES_H
#include FT_INTERNAL_STREAM_H


#ifdef TT_CONFIG_OPTION_SFNT_NAMES


  

  FT_EXPORT_DEF( FT_UInt )
  FT_Get_Sfnt_Name_Count( FT_Face  face )
  {
    return (face && FT_IS_SFNT( face )) ? ((TT_Face)face)->num_names : 0;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Sfnt_Name( FT_Face       face,
                    FT_UInt       idx,
                    FT_SfntName  *aname )
  {
    FT_Error  error = FT_Err_Invalid_Argument;


    if ( aname && face && FT_IS_SFNT( face ) )
    {
      TT_Face  ttface = (TT_Face)face;


      if ( idx < (FT_UInt)ttface->num_names )
      {
        TT_NameEntryRec*  entry = ttface->name_table.names + idx;


        
        if ( entry->stringLength > 0 && entry->string == NULL )
        {
          FT_Memory  memory = face->memory;
          FT_Stream  stream = face->stream;


          if ( FT_NEW_ARRAY  ( entry->string, entry->stringLength ) ||
               FT_STREAM_SEEK( entry->stringOffset )                ||
               FT_STREAM_READ( entry->string, entry->stringLength ) )
          {
            FT_FREE( entry->string );
            entry->stringLength = 0;
          }
        }

        aname->platform_id = entry->platformID;
        aname->encoding_id = entry->encodingID;
        aname->language_id = entry->languageID;
        aname->name_id     = entry->nameID;
        aname->string      = (FT_Byte*)entry->string;
        aname->string_len  = entry->stringLength;

        error = FT_Err_Ok;
      }
    }

    return error;
  }


#endif 



