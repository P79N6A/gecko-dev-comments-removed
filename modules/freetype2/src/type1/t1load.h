

















#ifndef __T1LOAD_H__
#define __T1LOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_MULTIPLE_MASTERS_H

#include "t1parse.h"


FT_BEGIN_HEADER


  typedef struct  T1_Loader_
  {
    T1_ParserRec  parser;          

    FT_Int        num_chars;       
    PS_TableRec   encoding_table;  
                                   

    FT_Int        num_glyphs;
    PS_TableRec   glyph_names;
    PS_TableRec   charstrings;
    PS_TableRec   swap_table;      

    FT_Int        num_subrs;
    PS_TableRec   subrs;
    FT_Bool       fontdata;

    FT_UInt       keywords_encountered; 

  } T1_LoaderRec, *T1_Loader;


  
  

#define T1_PRIVATE                ( 1 << 0 )
#define T1_FONTDIR_AFTER_PRIVATE  ( 1 << 1 )


  FT_LOCAL( FT_Error )
  T1_Open_Face( T1_Face  face );

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT

  FT_LOCAL( FT_Error )
  T1_Get_Multi_Master( T1_Face           face,
                       FT_Multi_Master*  master );

  FT_LOCAL_DEF( FT_Error )
  T1_Get_MM_Var( T1_Face      face,
                 FT_MM_Var*  *master );

  FT_LOCAL( FT_Error )
  T1_Set_MM_Blend( T1_Face    face,
                   FT_UInt    num_coords,
                   FT_Fixed*  coords );

  FT_LOCAL( FT_Error )
  T1_Set_MM_Design( T1_Face   face,
                    FT_UInt   num_coords,
                    FT_Long*  coords );

  FT_LOCAL_DEF( FT_Error )
  T1_Set_Var_Design( T1_Face    face,
                     FT_UInt    num_coords,
                     FT_Fixed*  coords );

  FT_LOCAL( void )
  T1_Done_Blend( T1_Face  face );

#endif 


FT_END_HEADER

#endif 



