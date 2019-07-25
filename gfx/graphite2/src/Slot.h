

























#pragma once

#include "graphite2/Types.h"
#include "graphite2/Segment.h"
#include "Main.h"
#include "Font.h"


#define SLOT_DELETED    1
#define SLOT_INSERT	2
#define SLOT_COPIED     4
#define SLOT_POSITIONED 8

namespace graphite2 {

typedef gr_attrCode attrCode;

class Segment;

class Slot
{
public:
    unsigned short gid() const { return m_glyphid; }
    Position origin() const { return m_position; }
    float advance() const { return m_advance.x; }
    Position advancePos() const { return m_advance; }
    int before() const { return m_before; }
    int after() const { return m_after; }
    uint32 index() const { return m_index; }
    void index(uint32 val) { m_index = val; }

    Slot();
    void set(const Slot & slot, int charOffset, uint8 numUserAttr);
    Slot *next() const { return m_next; }
    void next(Slot *s) { m_next = s; }
    Slot *prev() const { return m_prev; }
    void prev(Slot *s) { m_prev = s; }
    uint16 glyph() const { return m_realglyphid ? m_realglyphid : m_glyphid; }
    void setGlyph(Segment *seg, uint16 glyphid, const GlyphFace * theGlyph = NULL);
    void setRealGid(uint16 realGid) { m_realglyphid = realGid; }
    void adjKern(const Position &pos) { m_shift = m_shift + pos; m_advance = m_advance + pos; }
    void origin(const Position &pos) { m_position = pos + m_shift; }
    void originate(int ind) { m_original = ind; }
    int original() const { return m_original; }
    void before(int ind) { m_before = ind; }
    void after(int ind) { m_after = ind; }
    bool isBase() const { return (!m_parent); }
    void update(int numSlots, int numCharInfo, Position &relpos);
    Position finalise(const Segment* seg, const Font* font, Position* base, Rect* bbox, float* cMin, uint8 attrLevel, float *clusterMin);
    bool isDeleted() const { return (m_flags & SLOT_DELETED) ? true : false; }
    void markDeleted(bool state) { if (state) m_flags |= SLOT_DELETED; else m_flags &= ~SLOT_DELETED; }
    bool isCopied() const { return (m_flags & SLOT_COPIED) ? true : false; }
    void markCopied(bool state) { if (state) m_flags |= SLOT_COPIED; else m_flags &= ~SLOT_COPIED; }
    bool isPositioned() const { return (m_flags & SLOT_POSITIONED) ? true : false; }
    void markPositioned(bool state) { if (state) m_flags |= SLOT_POSITIONED; else m_flags &= ~SLOT_POSITIONED; }
    bool isInsertBefore() const { return !(m_flags & SLOT_INSERT); }
    uint8 getBidiLevel() const { return m_bidiLevel; }
    void setBidiLevel(uint8 level) { m_bidiLevel = level; }
    uint8 getBidiClass() const { return m_bidiCls; }
    void setBidiClass(uint8 cls) { m_bidiCls = cls; }
    uint16 *userAttrs() { return m_userAttr; }
    void userAttrs(uint16 *p) { m_userAttr = p; }
    void markInsertBefore(bool state) { if (!state) m_flags |= SLOT_INSERT; else m_flags &= ~SLOT_INSERT; }
    void setAttr(Segment* seg, attrCode ind, uint8 subindex, int16 val, const SlotMap & map);
    int getAttr(const Segment *seg, attrCode ind, uint8 subindex) const;
    void attachTo(Slot *ap) { m_parent = ap; }
    Slot *attachedTo() const { return m_parent; }
    Slot* firstChild() const { return m_child; }
    bool child(Slot *ap);
    Slot* nextSibling() const { return m_sibling; }
    bool sibling(Slot *ap);
    Slot *attachTo() const { return m_parent; }
    uint32 clusterMetric(const Segment* seg, uint8 metric, uint8 attrLevel);
    void positionShift(Position a) { m_position += a; }
    void floodShift(Position adj);
    float just() { return m_just; }
    void just(float j) { m_just = j; }

    CLASS_NEW_DELETE

private:
    Slot *m_next;           
    Slot *m_prev;
    unsigned short m_glyphid;        
    uint16 m_realglyphid;
    uint32 m_original;	    
    uint32 m_before;        
    uint32 m_after;         
    uint32 m_index;         
    Slot *m_parent;         
    Slot *m_child;          
    Slot *m_sibling;        
    Position m_position;    
    Position m_shift;       
    Position m_advance;     
    Position m_attach;      
    Position m_with;	    
    float    m_just;        
    uint8    m_flags;       
    byte     m_attLevel;    
    byte     m_bidiCls;     
    byte     m_bidiLevel;   
    uint16  *m_userAttr;     
};

} 

struct gr_slot : public graphite2::Slot {};
