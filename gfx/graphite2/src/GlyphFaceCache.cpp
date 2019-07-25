

























#include "GlyphFaceCache.h"
#include "graphite2/Font.h"
#include "Face.h"     
#include "Endian.h"

using namespace graphite2;

 bool GlyphFaceCacheHeader::initialize(const Face & face, const bool dumb_font)    
{
    if ((m_pLoca = face.getTable(Tag::loca, &m_lLoca)) == NULL) return false;
    if ((m_pHead = face.getTable(Tag::head)) == NULL) return false;
    if ((m_pGlyf = face.getTable(Tag::glyf, &m_lGlyf)) == NULL) return false;
    if ((m_pHmtx = face.getTable(Tag::hmtx, &m_lHmtx)) == NULL) return false;
    if ((m_pHHea = face.getTable(Tag::hhea)) == NULL) return false;

    const void* pMaxp = face.getTable(Tag::maxp);
    if (pMaxp == NULL) return false;
    m_nGlyphs = m_nGlyphsWithGraphics = (unsigned short)TtfUtil::GlyphCount(pMaxp);
    
    if (!dumb_font)
    {
		if ((m_pGlat = face.getTable(Tag::Glat, &m_lGlat)) == NULL) return false;
		m_fGlat = be::peek<uint32>(m_pGlat);
		size_t lGloc;
		if ((m_pGloc = face.getTable(Tag::Gloc, &lGloc)) == NULL) return false;
		if (lGloc < 6) return false;
		int version = be::peek<uint32>(m_pGloc);
		if (version != 0x00010000) return false;

		m_numAttrs = be::swap<uint16>(((uint16 *)m_pGloc)[3]);
		if (m_numAttrs > 0x1000) return false;                  

		unsigned short locFlags = be::swap<uint16>(((uint16 *)m_pGloc)[2]);
		if (locFlags & 1)
		{
			m_locFlagsUse32Bit = true;
			m_nGlyphsWithAttributes = (unsigned short)((lGloc - 12) / 4);
		}
		else
		{
			m_locFlagsUse32Bit = false;
			m_nGlyphsWithAttributes = (unsigned short)((lGloc - 10) / 2);
		}

		if (m_nGlyphsWithAttributes > m_nGlyphs)
	        m_nGlyphs = m_nGlyphsWithAttributes;
    }

    return true;
}

GlyphFaceCache* GlyphFaceCache::makeCache(const GlyphFaceCacheHeader& hdr)
{
    return new (hdr) GlyphFaceCache(hdr);
}

GlyphFaceCache::GlyphFaceCache(const GlyphFaceCacheHeader& hdr)
:   GlyphFaceCacheHeader(hdr)
{
    unsigned int nGlyphs = numGlyphs();
    
    for (unsigned int i = 0; i < nGlyphs; i++)
    {
         *glyphPtrDirect(i) = NULL;
    }
}

GlyphFaceCache::~GlyphFaceCache()
{
    unsigned int nGlyphs = numGlyphs();
    int deltaPointers = (*glyphPtrDirect(nGlyphs-1u) - *glyphPtrDirect(0u));
    if ((nGlyphs > 0u) && (deltaPointers == static_cast<int>(nGlyphs - 1)))
    {
        for (unsigned int i=0 ; i<nGlyphs; ++i)
        {
            GlyphFace *p = *glyphPtrDirect(i);
            assert (p);
            p->~GlyphFace();
        }
        free (*glyphPtrDirect(0));
    }
    else
    {
        for (unsigned int i=0 ; i<nGlyphs; ++i)
        {
            GlyphFace *p = *glyphPtrDirect(i);
            if (p)
            {
                p->~GlyphFace();
                free(p);
            }
        }
    }
}

void GlyphFaceCache::loadAllGlyphs()
{
    unsigned int nGlyphs = numGlyphs();

    GlyphFace * glyphs = gralloc<GlyphFace>(nGlyphs);
    for (unsigned short glyphid = 0; glyphid < nGlyphs; glyphid++)
    {
        GlyphFace **p = glyphPtrDirect(glyphid);
        *p = &(glyphs[glyphid]);
        new(*p) GlyphFace(*this, glyphid);

    }


}

 const GlyphFace *GlyphFaceCache::glyph(unsigned short glyphid) const      
{ 
    GlyphFace **p = glyphPtrDirect(glyphid);
    if (*p)
        return *p;

    *p = (GlyphFace*)malloc(sizeof(GlyphFace));
    new(*p) GlyphFace(*this, glyphid);
    return *p;
}
