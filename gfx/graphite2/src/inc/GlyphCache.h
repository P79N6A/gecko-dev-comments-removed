

























#pragma once


#include "graphite2/Font.h"
#include "inc/Main.h"

namespace graphite2 {

class Face;
class FeatureVal;
class GlyphFace;
class Segment;

class GlyphCache
{
    class Loader;

	GlyphCache(const GlyphCache&);
    GlyphCache& operator=(const GlyphCache&);

public:
    GlyphCache(const Face & face, const uint32 face_options);
    ~GlyphCache();

    size_t numGlyphs() const throw();
    size_t numAttrs() const throw();
    size_t unitsPerEm() const throw();

    const GlyphFace *glyph(unsigned short glyphid) const;      
    const GlyphFace *glyphSafe(unsigned short glyphid) const;
    uint16 glyphAttr(uint16 gid, uint16 gattr) const;

    CLASS_NEW_DELETE;
    
private:
    const Loader              * _glyph_loader;
    const GlyphFace *   *       _glyphs;
    unsigned short              _num_glyphs,
                                _num_attrs,
                                _upem;
};

inline
size_t GlyphCache::numGlyphs() const throw()
{
    return _num_glyphs;
}

inline
size_t GlyphCache::numAttrs() const throw()
{
    return _num_attrs;
}

inline
size_t GlyphCache::unitsPerEm() const throw()
{
    return _upem;
}

inline
const GlyphFace *GlyphCache::glyphSafe(unsigned short glyphid) const
{
    return glyphid < _num_glyphs ? glyph(glyphid) : NULL;
}

} 
