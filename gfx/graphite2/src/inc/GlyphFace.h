

























#pragma once

#include "inc/Main.h"
#include "inc/Position.h"
#include "inc/Sparse.h"

namespace graphite2 {

enum metrics {
    kgmetLsb = 0, kgmetRsb,
    kgmetBbTop, kgmetBbBottom, kgmetBbLeft, kgmetBbRight,
    kgmetBbHeight, kgmetBbWidth,
    kgmetAdvWidth, kgmetAdvHeight,
    kgmetAscent, kgmetDescent
};

class Rect
{
public :
    Rect() {}
    Rect(const Position& botLeft, const Position& topRight): bl(botLeft), tr(topRight) {}
    Rect widen(const Rect& other) { return Rect(Position(bl.x > other.bl.x ? other.bl.x : bl.x, bl.y > other.bl.y ? other.bl.y : bl.y), Position(tr.x > other.tr.x ? tr.x : other.tr.x, tr.y > other.tr.y ? tr.y : other.tr.y)); }
    Rect operator + (const Position &a) const { return Rect(Position(bl.x + a.x, bl.y + a.y), Position(tr.x + a.x, tr.y + a.y)); }
    Rect operator * (float m) const { return Rect(Position(bl.x, bl.y) * m, Position(tr.x, tr.y) * m); }

    Position bl;
    Position tr;
};

class GlyphFaceCacheHeader;

class GlyphFace
{
private:
friend class GlyphFaceCache;
    GlyphFace(const GlyphFaceCacheHeader& hdr, unsigned short glyphid);
    ~GlyphFace() throw();

public:

    const Position    & theAdvance() const;
    const Rect &theBBox() const { return m_bbox; }
    uint16  getAttr(uint8 index) const { 
    	return m_attrs ? m_attrs[index] : 0;
    }
    uint16  getMetric(uint8 metric) const;

private:
    Rect     m_bbox;        
    Position m_advance;     
    sparse   m_attrs;
};


inline GlyphFace::~GlyphFace() throw() {
}

inline const Position & GlyphFace::theAdvance() const { 
    return m_advance;
}

} 
