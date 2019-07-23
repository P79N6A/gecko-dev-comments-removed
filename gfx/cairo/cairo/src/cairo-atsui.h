


































#ifndef CAIRO_ATSUI_H
#define CAIRO_ATSUI_H

#include <cairo.h>

#if CAIRO_HAS_ATSUI_FONT



#include <Carbon/Carbon.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_font_face_t *
cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the atsui font backend
#endif 

#endif 
