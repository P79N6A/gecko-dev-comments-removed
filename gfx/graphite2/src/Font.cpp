

























#include "Font.h"


using namespace graphite2;

Font::Font(float ppm, const Face *face) :
    m_scale(ppm / face->upem())
{
    size_t nGlyphs=face->numGlyphs();
    m_advances = gralloc<float>(nGlyphs);
    if (m_advances)
    {
        float *advp = m_advances;
        for (size_t i = 0; i < nGlyphs; i++)
        { *advp++ = INVALID_ADVANCE; }
    }
}


 Font::~Font()
{
	free(m_advances);
}


SimpleFont::SimpleFont(float ppm, const Face *face) :
  Font(ppm, face),
  m_face(face)
{
}
  
  
 float SimpleFont::computeAdvance(unsigned short glyphid) const
{
    return m_face->getAdvance(glyphid, m_scale);
}



HintedFont::HintedFont(float ppm, const void* appFontHandle, gr_advance_fn advance2, const Face *face) :
    Font(ppm, face), 
    m_appFontHandle(appFontHandle),
    m_advance(advance2)
{
}


 float HintedFont::computeAdvance(unsigned short glyphid) const
{
    return (*m_advance)(m_appFontHandle, glyphid);
}



