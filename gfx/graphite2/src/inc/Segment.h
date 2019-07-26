

























#pragma once

#include "inc/Main.h"

#include <cassert>

#include "inc/CharInfo.h"
#include "inc/Face.h"
#include "inc/FeatureVal.h"
#include "inc/GlyphCache.h"
#include "inc/GlyphFace.h"

#include "inc/Slot.h"
#include "inc/Position.h"
#include "inc/List.h"

#define MAX_SEG_GROWTH_FACTOR  256

namespace graphite2 {

typedef Vector<Features>        FeatureList;
typedef Vector<Slot *>          SlotRope;
typedef Vector<int16 *>        AttributeRope;
typedef Vector<SlotJustify *>   JustifyRope;

#ifndef GRAPHITE2_NSEGCACHE
class SegmentScopeState;
#endif
class Font;
class Segment;
class Silf;

enum SpliceParam {


    eMaxSpliceSize = 96
};

enum justFlags {
    gr_justStartInline = 1,
    gr_justEndInline = 2
};

class SegmentScopeState
{
private:
    friend class Segment;
    Slot * realFirstSlot;
    Slot * slotBeforeScope;
    Slot * slotAfterScope;
    Slot * realLastSlot;
    size_t numGlyphsOutsideScope;
};

class Segment
{
    
    Segment(const Segment&);
    Segment& operator=(const Segment&);

public:
    unsigned int slotCount() const { return m_numGlyphs; }      
    void extendLength(int num) { m_numGlyphs += num; }
    Position advance() const { return m_advance; }
    bool runGraphite() { if (m_silf) return m_face->runGraphite(this, m_silf); else return true;};
    void chooseSilf(uint32 script) { m_silf = m_face->chooseSilf(script); }
    const Silf *silf() const { return m_silf; }
    unsigned int charInfoCount() const { return m_numCharinfo; }
    const CharInfo *charinfo(unsigned int index) const { return index < m_numCharinfo ? m_charinfo + index : NULL; }
    CharInfo *charinfo(unsigned int index) { return index < m_numCharinfo ? m_charinfo + index : NULL; }
    int8 dir() const { return m_dir; }

    Segment(unsigned int numchars, const Face* face, uint32 script, int dir);
    ~Segment();
#ifndef GRAPHITE2_NSEGCACHE
    SegmentScopeState setScope(Slot * firstSlot, Slot * lastSlot, size_t subLength);
    void removeScope(SegmentScopeState & state);
    void append(const Segment &other);
    void splice(size_t offset, size_t length, Slot * const startSlot,
            Slot * endSlot, const Slot * srcSlot,
            const size_t numGlyphs);
#endif
    Slot *first() { return m_first; }
    void first(Slot *p) { m_first = p; }
    Slot *last() { return m_last; }
    void last(Slot *p) { m_last = p; }
    void appendSlot(int i, int cid, int gid, int fid, size_t coffset);
    Slot *newSlot();
    void freeSlot(Slot *);
    SlotJustify *newJustify();
    void freeJustify(SlotJustify *aJustify);
    Position positionSlots(const Font *font, Slot *first=0, Slot *last=0);
    void associateChars();
    void linkClusters(Slot *first, Slot *last);
    uint16 getClassGlyph(uint16 cid, uint16 offset) const { return m_silf->getClassGlyph(cid, offset); }
    uint16 findClassIndex(uint16 cid, uint16 gid) const { return m_silf->findClassIndex(cid, gid); }
    int addFeatures(const Features& feats) { m_feats.push_back(feats); return m_feats.size() - 1; }
    uint32 getFeature(int index, uint8 findex) const { const FeatureRef* pFR=m_face->theSill().theFeatureMap().featureRef(findex); if (!pFR) return 0; else return pFR->getFeatureVal(m_feats[index]); }
    void dir(int8 val) { m_dir = val; }
    uint16 glyphAttr(uint16 gid, uint16 gattr) const { return m_face->glyphs().glyphAttr(gid, gattr); }
    uint16 getGlyphMetric(Slot *iSlot, uint8 metric, uint8 attrLevel) const;
    float glyphAdvance(uint16 gid) const { return m_face->glyphs().glyph(gid)->theAdvance().x; }
    const Rect &theGlyphBBoxTemporary(uint16 gid) const { return m_face->glyphs().glyph(gid)->theBBox(); }   
    Slot *findRoot(Slot *is) const { return is->attachedTo() ? findRoot(is->attachedTo()) : is; }
    int numAttrs() const { return m_silf->numUser(); }
    int defaultOriginal() const { return m_defaultOriginal; }
    const Face * getFace() const { return m_face; }
    const Features & getFeatures(unsigned int ) { assert(m_feats.size() == 1); return m_feats[0]; }
    void bidiPass(uint8 aBidi, int paradir, uint8 aMirror);
    Slot *addLineEnd(Slot *nSlot);
    void delLineEnd(Slot *s);
    bool hasJustification() const { return m_justifies.size() != 0; }

    bool isWhitespace(const int cid) const;

    CLASS_NEW_DELETE

public:       
    void read_text(const Face *face, const Features* pFeats, gr_encform enc, const void*pStart, size_t nChars);
    void prepare_pos(const Font *font);
    void finalise(const Font *font);
    float justify(Slot *pSlot, const Font *font, float width, enum justFlags flags, Slot *pFirst, Slot *pLast);
  
private:
    Rect            m_bbox;             
    Position        m_advance;          
    SlotRope        m_slots;            
    AttributeRope   m_userAttrs;        
    JustifyRope     m_justifies;        
    FeatureList     m_feats;            
    Slot          * m_freeSlots;        
    SlotJustify   * m_freeJustifies;    
    CharInfo      * m_charinfo;         
    const Face    * m_face;             
    const Silf    * m_silf;
    Slot          * m_first;            
    Slot          * m_last;             
    unsigned int    m_bufSize,          
                    m_numGlyphs,
                    m_numCharinfo;      
    int             m_defaultOriginal;  
    int8            m_dir;
};



inline
void Segment::finalise(const Font *font)
{
	if (!m_first) return;

    m_advance = positionSlots(font);
    associateChars();
    linkClusters(m_first, m_last);
}

inline
uint16 Segment::getGlyphMetric(Slot *iSlot, uint8 metric, uint8 attrLevel) const {
    if (attrLevel > 0)
    {
        Slot *is = findRoot(iSlot);
        return is->clusterMetric(this, metric, attrLevel);
    }
    else
        return m_face->getGlyphMetric(iSlot->gid(), metric);
}

inline
bool Segment::isWhitespace(const int cid) const
{
    return ((cid >= 0x0009) * (cid <= 0x000D)
         + (cid == 0x0020)
         + (cid == 0x0085)
         + (cid == 0x00A0)
         + (cid == 0x1680)
         + (cid == 0x180E)
         + (cid >= 0x2000) * (cid <= 0x200A)
         + (cid == 0x2028)
         + (cid == 0x2029)
         + (cid == 0x202F)
         + (cid == 0x205F)
         + (cid == 0x3000)) != 0;
}

























































} 

struct gr_segment : public graphite2::Segment {};

