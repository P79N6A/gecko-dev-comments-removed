

















  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttpost.h"
#include "ttload.h"

#include "sferrors.h"


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttpost


  
  

#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES


#include FT_SERVICE_POSTSCRIPT_CMAPS_H

#define MAC_NAME( x )  ( (FT_String*)psnames->macintosh_name( x ) )


#else 


   
   
   

#define MAC_NAME( x )  tt_post_default_names[x]

  

  static const FT_String*  tt_post_default_names[258] =
  {
    
    ".notdef", ".null", "CR", "space", "exclam",
    "quotedbl", "numbersign", "dollar", "percent", "ampersand",
    
    "quotesingle", "parenleft", "parenright", "asterisk", "plus",
    "comma", "hyphen", "period", "slash", "zero",
    
    "one", "two", "three", "four", "five",
    "six", "seven", "eight", "nine", "colon",
    
    "semicolon", "less", "equal", "greater", "question",
    "at", "A", "B", "C", "D",
    
    "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N",
    
    "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X",
    
    "Y", "Z", "bracketleft", "backslash", "bracketright",
    "asciicircum", "underscore", "grave", "a", "b",
    
    "c", "d", "e", "f", "g",
    "h", "i", "j", "k", "l",
    
    "m", "n", "o", "p", "q",
    "r", "s", "t", "u", "v",
    
    "w", "x", "y", "z", "braceleft",
    "bar", "braceright", "asciitilde", "Adieresis", "Aring",
    
    "Ccedilla", "Eacute", "Ntilde", "Odieresis", "Udieresis",
    "aacute", "agrave", "acircumflex", "adieresis", "atilde",
    
    "aring", "ccedilla", "eacute", "egrave", "ecircumflex",
    "edieresis", "iacute", "igrave", "icircumflex", "idieresis",
    
    "ntilde", "oacute", "ograve", "ocircumflex", "odieresis",
    "otilde", "uacute", "ugrave", "ucircumflex", "udieresis",
    
    "dagger", "degree", "cent", "sterling", "section",
    "bullet", "paragraph", "germandbls", "registered", "copyright",
    
    "trademark", "acute", "dieresis", "notequal", "AE",
    "Oslash", "infinity", "plusminus", "lessequal", "greaterequal",
    
    "yen", "mu", "partialdiff", "summation", "product",
    "pi", "integral", "ordfeminine", "ordmasculine", "Omega",
    
    "ae", "oslash", "questiondown", "exclamdown", "logicalnot",
    "radical", "florin", "approxequal", "Delta", "guillemotleft",
    
    "guillemotright", "ellipsis", "nbspace", "Agrave", "Atilde",
    "Otilde", "OE", "oe", "endash", "emdash",
    
    "quotedblleft", "quotedblright", "quoteleft", "quoteright", "divide",
    "lozenge", "ydieresis", "Ydieresis", "fraction", "currency",
    
    "guilsinglleft", "guilsinglright", "fi", "fl", "daggerdbl",
    "periodcentered", "quotesinglbase", "quotedblbase", "perthousand", "Acircumflex",
    
    "Ecircumflex", "Aacute", "Edieresis", "Egrave", "Iacute",
    "Icircumflex", "Idieresis", "Igrave", "Oacute", "Ocircumflex",
    
    "apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave",
    "dotlessi", "circumflex", "tilde", "macron", "breve",
    
    "dotaccent", "ring", "cedilla", "hungarumlaut", "ogonek",
    "caron", "Lslash", "lslash", "Scaron", "scaron",
    
    "Zcaron", "zcaron", "brokenbar", "Eth", "eth",
    "Yacute", "yacute", "Thorn", "thorn", "minus",
    
    "multiply", "onesuperior", "twosuperior", "threesuperior", "onehalf",
    "onequarter", "threequarters", "franc", "Gbreve", "gbreve",
    
    "Idot", "Scedilla", "scedilla", "Cacute", "cacute",
    "Ccaron", "ccaron", "dmacron",
  };


#endif 


  static FT_Error
  load_format_20( TT_Face    face,
                  FT_Stream  stream )
  {
    FT_Memory   memory = stream->memory;
    FT_Error    error;

    FT_Int      num_glyphs;
    FT_UShort   num_names;

    FT_UShort*  glyph_indices = 0;
    FT_Char**   name_strings  = 0;


    if ( FT_READ_USHORT( num_glyphs ) )
      goto Exit;

    
    

    
    

    if ( num_glyphs > face->max_profile.numGlyphs )
    {
      error = SFNT_Err_Invalid_File_Format;
      goto Exit;
    }

    
    {
      FT_Int  n;


      if ( FT_NEW_ARRAY ( glyph_indices, num_glyphs ) ||
           FT_FRAME_ENTER( num_glyphs * 2L )          )
        goto Fail;

      for ( n = 0; n < num_glyphs; n++ )
        glyph_indices[n] = FT_GET_USHORT();

      FT_FRAME_EXIT();
    }

    
    {
      FT_Int  n;


      num_names = 0;

      for ( n = 0; n < num_glyphs; n++ )
      {
        FT_Int  idx;


        idx = glyph_indices[n];
        if ( idx >= 258 )
        {
          idx -= 257;
          if ( idx > num_names )
            num_names = (FT_UShort)idx;
        }
      }
    }

    
    {
      FT_UShort  n;


      if ( FT_NEW_ARRAY( name_strings, num_names ) )
        goto Fail;

      for ( n = 0; n < num_names; n++ )
      {
        FT_UInt  len;


        if ( FT_READ_BYTE  ( len )                    ||
             FT_NEW_ARRAY( name_strings[n], len + 1 ) ||
             FT_STREAM_READ  ( name_strings[n], len ) )
          goto Fail1;

        name_strings[n][len] = '\0';
      }
    }

    
    {
      TT_Post_20  table = &face->postscript_names.names.format_20;


      table->num_glyphs    = (FT_UShort)num_glyphs;
      table->num_names     = (FT_UShort)num_names;
      table->glyph_indices = glyph_indices;
      table->glyph_names   = name_strings;
    }
    return SFNT_Err_Ok;

  Fail1:
    {
      FT_UShort  n;


      for ( n = 0; n < num_names; n++ )
        FT_FREE( name_strings[n] );
    }

  Fail:
    FT_FREE( name_strings );
    FT_FREE( glyph_indices );

  Exit:
    return error;
  }


  static FT_Error
  load_format_25( TT_Face    face,
                  FT_Stream  stream )
  {
    FT_Memory  memory = stream->memory;
    FT_Error   error;

    FT_Int     num_glyphs;
    FT_Char*   offset_table = 0;


    
    if ( FT_READ_USHORT( num_glyphs ) )
      goto Exit;

    
    if ( num_glyphs > face->max_profile.numGlyphs || num_glyphs > 258 )
    {
      error = SFNT_Err_Invalid_File_Format;
      goto Exit;
    }

    if ( FT_NEW_ARRAY( offset_table, num_glyphs )   ||
         FT_STREAM_READ( offset_table, num_glyphs ) )
      goto Fail;

    
    {
      FT_Int  n;


      for ( n = 0; n < num_glyphs; n++ )
      {
        FT_Long  idx = (FT_Long)n + offset_table[n];


        if ( idx < 0 || idx > num_glyphs )
        {
          error = SFNT_Err_Invalid_File_Format;
          goto Fail;
        }
      }
    }

    
    {
      TT_Post_25  table = &face->postscript_names.names.format_25;


      table->num_glyphs = (FT_UShort)num_glyphs;
      table->offsets    = offset_table;
    }

    return SFNT_Err_Ok;

  Fail:
    FT_FREE( offset_table );

  Exit:
    return error;
  }


  static FT_Error
  load_post_names( TT_Face  face )
  {
    FT_Stream  stream;
    FT_Error   error;
    FT_Fixed   format;


    
    stream = face->root.stream;

    
    error = face->goto_table( face, TTAG_post, stream, 0 );
    if ( error )
      goto Exit;

    format = face->postscript.FormatType;

    
    if ( FT_STREAM_SKIP( 32 ) )
      goto Exit;

    
    if ( format == 0x00020000L )
      error = load_format_20( face, stream );
    else if ( format == 0x00028000L )
      error = load_format_25( face, stream );
    else
      error = SFNT_Err_Invalid_File_Format;

    face->postscript_names.loaded = 1;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  tt_face_free_ps_names( TT_Face  face )
  {
    FT_Memory      memory = face->root.memory;
    TT_Post_Names  names  = &face->postscript_names;
    FT_Fixed       format;


    if ( names->loaded )
    {
      format = face->postscript.FormatType;

      if ( format == 0x00020000L )
      {
        TT_Post_20  table = &names->names.format_20;
        FT_UShort   n;


        FT_FREE( table->glyph_indices );
        table->num_glyphs = 0;

        for ( n = 0; n < table->num_names; n++ )
          FT_FREE( table->glyph_names[n] );

        FT_FREE( table->glyph_names );
        table->num_names = 0;
      }
      else if ( format == 0x00028000L )
      {
        TT_Post_25  table = &names->names.format_25;


        FT_FREE( table->offsets );
        table->num_glyphs = 0;
      }
    }
    names->loaded = 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  tt_face_get_ps_name( TT_Face      face,
                       FT_UInt      idx,
                       FT_String**  PSname )
  {
    FT_Error         error;
    TT_Post_Names    names;
    FT_Fixed         format;

#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES
    FT_Service_PsCMaps  psnames;
#endif


    if ( !face )
      return SFNT_Err_Invalid_Face_Handle;

    if ( idx >= (FT_UInt)face->max_profile.numGlyphs )
      return SFNT_Err_Invalid_Glyph_Index;

#ifdef FT_CONFIG_OPTION_POSTSCRIPT_NAMES
    psnames = (FT_Service_PsCMaps)face->psnames;
    if ( !psnames )
      return SFNT_Err_Unimplemented_Feature;
#endif

    names = &face->postscript_names;

    
    *PSname = MAC_NAME( 0 );

    format = face->postscript.FormatType;

    if ( format == 0x00010000L )
    {
      if ( idx < 258 )                    
        *PSname = MAC_NAME( idx );
    }
    else if ( format == 0x00020000L )
    {
      TT_Post_20  table = &names->names.format_20;


      if ( !names->loaded )
      {
        error = load_post_names( face );
        if ( error )
          goto End;
      }

      if ( idx < (FT_UInt)table->num_glyphs )
      {
        FT_UShort  name_index = table->glyph_indices[idx];


        if ( name_index < 258 )
          *PSname = MAC_NAME( name_index );
        else
          *PSname = (FT_String*)table->glyph_names[name_index - 258];
      }
    }
    else if ( format == 0x00028000L )
    {
      TT_Post_25  table = &names->names.format_25;


      if ( !names->loaded )
      {
        error = load_post_names( face );
        if ( error )
          goto End;
      }

      if ( idx < (FT_UInt)table->num_glyphs )    
      {
        idx    += table->offsets[idx];
        *PSname = MAC_NAME( idx );
      }
    }

    

  End:
    return SFNT_Err_Ok;
  }



