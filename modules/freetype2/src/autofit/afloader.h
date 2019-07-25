

















#ifndef __AFLOADER_H__
#define __AFLOADER_H__

#include "afhints.h"
#include "afglobal.h"


FT_BEGIN_HEADER

  typedef struct AF_LoaderRec_
  {
    FT_Face           face;           
    AF_FaceGlobals    globals;        
    FT_GlyphLoader    gloader;        
    AF_GlyphHintsRec  hints;
    AF_ScriptMetrics  metrics;
    FT_Bool           transformed;
    FT_Matrix         trans_matrix;
    FT_Vector         trans_delta;
    FT_Vector         pp1;
    FT_Vector         pp2;
    

  } AF_LoaderRec, *AF_Loader;


  FT_LOCAL( FT_Error )
  af_loader_init( AF_Loader  loader,
                  FT_Memory  memory );


  FT_LOCAL( FT_Error )
  af_loader_reset( AF_Loader  loader,
                   FT_Face    face );


  FT_LOCAL( void )
  af_loader_done( AF_Loader  loader );


  FT_LOCAL( FT_Error )
  af_loader_load_glyph( AF_Loader  loader,
                        FT_Face    face,
                        FT_UInt    gindex,
                        FT_Int32   load_flags );




FT_END_HEADER

#endif 



