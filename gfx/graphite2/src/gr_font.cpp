

























#include "graphite2/Font.h"
#include "inc/Font.h"


using namespace graphite2;

extern "C" {

void gr_engine_version(int *nMajor, int *nMinor, int *nBugFix)
{
    if (nMajor) *nMajor = GR2_VERSION_MAJOR;
    if (nMinor) *nMinor = GR2_VERSION_MINOR;
    if (nBugFix) *nBugFix = GR2_VERSION_BUGFIX;
}

gr_font* gr_make_font(float ppm, const gr_face *face)
{
    Font * const res = new SimpleFont(ppm, face);
    return static_cast<gr_font*>(res);
}


gr_font* gr_make_font_with_advance_fn(float ppm, const void* appFontHandle, gr_advance_fn advance, const gr_face *face)
{                 
    Font * const res = new HintedFont(ppm, appFontHandle, advance, face);
    return static_cast<gr_font*>(res);
}


void gr_font_destroy(gr_font *font)
{
    delete font;
}


} 




