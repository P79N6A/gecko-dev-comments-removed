

























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
    return gr_make_font_with_advance_fn(ppm, 0, 0, face);
}


gr_font* gr_make_font_with_ops(float ppm, const void* appFontHandle, const gr_font_ops * font_ops, const gr_face * face)
{                 
	if (face == 0)	return 0;

	Font * const res = new Font(ppm, *face, appFontHandle, font_ops);
    return static_cast<gr_font*>(res);
}

gr_font* gr_make_font_with_advance_fn(float ppm, const void* appFontHandle, gr_advance_fn getAdvance, const gr_face * face)
{
    const gr_font_ops ops = {sizeof(gr_font_ops), getAdvance, NULL};
    return gr_make_font_with_ops(ppm, appFontHandle, &ops, face);
}

void gr_font_destroy(gr_font *font)
{
    delete font;
}


} 




