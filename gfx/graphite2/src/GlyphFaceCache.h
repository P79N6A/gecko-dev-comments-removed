

























#pragma once

#include "GlyphFace.h"
#include "graphite2/Font.h"

namespace graphite2 {

class Segment;
class Face;
class FeatureVal;


class GlyphFaceCacheHeader
{
public:
    bool initialize(const Face & face, const bool dumb_font);    
    unsigned short numGlyphs() const { return m_nGlyphs; }
    unsigned short numAttrs() const { return m_numAttrs; }

private:
friend class Face;
friend class GlyphFace;
    const byte* m_pHead,
    		  * m_pHHea,
    		  * m_pHmtx,
    		  * m_pGlat,
    		  * m_pGloc,
    		  * m_pGlyf,
    		  * m_pLoca;
    size_t		m_lHmtx,
    			m_lGlat,
    			m_lGlyf,
    			m_lLoca;

    uint32			m_fGlat;
    unsigned short 	m_numAttrs,					
    				m_nGlyphsWithGraphics,		
    				m_nGlyphsWithAttributes,
    				m_nGlyphs;					
    bool 			m_locFlagsUse32Bit;
};

class GlyphFaceCache : public GlyphFaceCacheHeader
{
public:
    static GlyphFaceCache* makeCache(const GlyphFaceCacheHeader& hdr );

    GlyphFaceCache(const GlyphFaceCacheHeader& hdr);
    ~GlyphFaceCache();

    const GlyphFace *glyphSafe(unsigned short glyphid) const { return glyphid<numGlyphs()?glyph(glyphid):NULL; }
    uint16 glyphAttr(uint16 gid, uint8 gattr) const { if (gattr>=numAttrs()) return 0; const GlyphFace*p=glyphSafe(gid); return p?p->getAttr(gattr):0; }

    void * operator new (size_t s, const GlyphFaceCacheHeader& hdr)
    {
        return malloc(s + sizeof(GlyphFace*)*hdr.numGlyphs());
    }
    
    void operator delete(void* p, const GlyphFaceCacheHeader& ) throw()
    {
        free(p);
    }

    const GlyphFace *glyph(unsigned short glyphid) const;      
    void loadAllGlyphs();

    CLASS_NEW_DELETE
    
private:
    GlyphFace **glyphPtrDirect(unsigned short glyphid) const { return (GlyphFace **)((const char*)(this)+sizeof(GlyphFaceCache)+sizeof(GlyphFace*)*glyphid);}

private:      
    GlyphFaceCache(const GlyphFaceCache&);
    GlyphFaceCache& operator=(const GlyphFaceCache&);
};

} 
