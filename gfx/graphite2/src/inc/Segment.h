

























#pragma once

#include "inc/Main.h"

#include <cassert>

#include "inc/Slot.h"
#include "inc/CharInfo.h"
#include "inc/FeatureVal.h"
#include "inc/Silf.h"

#include "inc/List.h"

#define MAX_SEG_GROWTH_FACTOR  256

namespace graphite2 {

typedef Vector<Features>        FeatureList;
typedef Vector<Slot *>          SlotRope;
typedef Vector<int16 *>        AttributeRope;

#ifndef GRAPHITE2_NSEGCACHE
class SegmentScopeState;
#endif
class Segment;

enum SpliceParam {


    eMaxSpliceSize = 16
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
    void splice(size_t offset, size_t length, Slot * startSlot, Slot * endSlot, 
                const Slot * firstSpliceSlot, size_t numGlyphs);
#endif
    Slot *first() { return m_first; }
    void first(Slot *p) { m_first = p; }
    Slot *last() { return m_last; }
    void last(Slot *p) { m_last = p; }
    void appendSlot(int i, int cid, int gid, int fid, size_t coffset);
    Slot *newSlot();
    void freeSlot(Slot *);
    Position positionSlots(const Font *font, Slot *first=0, Slot *last=0);
    void linkClusters(Slot *first, Slot *last);
    uint16 getClassGlyph(uint16 cid, uint16 offset) const { return m_silf->getClassGlyph(cid, offset); }
    uint16 findClassIndex(uint16 cid, uint16 gid) const { return m_silf->findClassIndex(cid, gid); }
    int addFeatures(const Features& feats) { m_feats.push_back(feats); return m_feats.size() - 1; }
    uint16 getFeature(int index, uint8 findex) const { const FeatureRef* pFR=m_face->theSill().theFeatureMap().featureRef(findex); if (!pFR) return 0; else return pFR->getFeatureVal(m_feats[index]); }
    void dir(int8 val) { m_dir = val; }
    uint16 glyphAttr(uint16 gid, uint8 gattr) const { return m_face->glyphAttr(gid, gattr); }
    uint16 getGlyphMetric(Slot *iSlot, uint8 metric, uint8 attrLevel) const {
        if (attrLevel > 0)
        {
            Slot *is = findRoot(iSlot);
            return is->clusterMetric(this, metric, attrLevel);
        }
        else
            return m_face->getGlyphMetric(iSlot->gid(), metric);
    }
    float glyphAdvance(uint16 gid) const { return m_face->getAdvance(gid, 1.0); }
    const Rect &theGlyphBBoxTemporary(uint16 gid) const { return m_face->theBBoxTemporary(gid); }   
    Slot *findRoot(Slot *is) const { return is->attachedTo() ? findRoot(is->attachedTo()) : is; }
    int numAttrs() const { return m_silf->numUser(); }
    int defaultOriginal() const { return m_defaultOriginal; }
    const Face * getFace() const { return m_face; }
    const Features & getFeatures(unsigned int ) { assert(m_feats.size() == 1); return m_feats[0]; }
    void bidiPass(uint8 aBidi, int paradir, uint8 aMirror);

    CLASS_NEW_DELETE

public:       
    void read_text(const Face *face, const Features* pFeats, gr_encform enc, const void*pStart, size_t nChars);
    void prepare_pos(const Font *font);
    void finalise(const Font *font);
    void justify(Slot *pSlot, const Font *font, float width, enum justFlags flags, Slot *pFirst, Slot *pLast);
  
private:
    SlotRope m_slots;           
    Slot *m_freeSlots;          
    Slot *m_first;              
    Slot *m_last;               
    unsigned int m_bufSize;     
    unsigned int m_numGlyphs;
    unsigned int m_numCharinfo; 
    int m_defaultOriginal;      
    AttributeRope m_userAttrs;  
    CharInfo *m_charinfo;       

    const Face *m_face;       
    const Silf *m_silf;
    Position m_advance;         
    Rect m_bbox;                
    int8 m_dir;
    FeatureList m_feats;	

private:		
    Segment(const Segment&);
    Segment& operator=(const Segment&);
};

} 

struct gr_segment : public graphite2::Segment {};

