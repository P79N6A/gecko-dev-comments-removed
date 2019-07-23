

















  
  
  
  
  
  


#ifndef __AUTOHINT_H__
#define __AUTOHINT_H__












































#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  typedef struct FT_AutoHinterRec_  *FT_AutoHinter;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*FT_AutoHinter_GlobalGetFunc)( FT_AutoHinter  hinter,
                                  FT_Face        face,
                                  void**         global_hints,
                                  long*          global_len );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*FT_AutoHinter_GlobalDoneFunc)( FT_AutoHinter  hinter,
                                   void*          global );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef void
  (*FT_AutoHinter_GlobalResetFunc)( FT_AutoHinter  hinter,
                                    FT_Face        face );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_Error
  (*FT_AutoHinter_GlyphLoadFunc)( FT_AutoHinter  hinter,
                                  FT_GlyphSlot   slot,
                                  FT_Size        size,
                                  FT_UInt        glyph_index,
                                  FT_Int32       load_flags );


  
  
  
  
  
  
  
  
  typedef struct  FT_AutoHinter_ServiceRec_
  {
    FT_AutoHinter_GlobalResetFunc  reset_face;
    FT_AutoHinter_GlobalGetFunc    get_global_hints;
    FT_AutoHinter_GlobalDoneFunc   done_global_hints;
    FT_AutoHinter_GlyphLoadFunc    load_glyph;

  } FT_AutoHinter_ServiceRec, *FT_AutoHinter_Service;


FT_END_HEADER

#endif 



