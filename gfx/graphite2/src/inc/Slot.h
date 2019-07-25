

























#pragma once

#include "graphite2/Types.h"
#include "graphite2/Segment.h"
#include "inc/Main.h"
#include "inc/Font.h"



namespace graphite2 {

typedef gr_attrCode attrCode;

class Segment;

class Slot
{
	enum Flag
	{
		DELETED 	= 1,
		INSERTED 	= 2,
		COPIED		= 4,
		POSITIONED	= 8,
		ATTACHED 	= 16
	};

public:
	struct iterator;

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
    Position finalise(const Segment* seg, const Font* font, Position & base, Rect & bbox, float & cMin, uint8 attrLevel, float & clusterMin);
    bool isDeleted() const { return (m_flags & DELETED) ? true : false; }
    void markDeleted(bool state) { if (state) m_flags |= DELETED; else m_flags &= ~DELETED; }
    bool isCopied() const { return (m_flags & COPIED) ? true : false; }
    void markCopied(bool state) { if (state) m_flags |= COPIED; else m_flags &= ~COPIED; }
    bool isPositioned() const { return (m_flags & POSITIONED) ? true : false; }
    void markPositioned(bool state) { if (state) m_flags |= POSITIONED; else m_flags &= ~POSITIONED; }
    bool isInsertBefore() const { return !(m_flags & INSERTED); }
    uint8 getBidiLevel() const { return m_bidiLevel; }
    void setBidiLevel(uint8 level) { m_bidiLevel = level; }
    uint8 getBidiClass() const { return m_bidiCls; }
    void setBidiClass(uint8 cls) { m_bidiCls = cls; }
    int16 *userAttrs() { return m_userAttr; }
    void userAttrs(int16 *p) { m_userAttr = p; }
    void markInsertBefore(bool state) { if (!state) m_flags |= INSERTED; else m_flags &= ~INSERTED; }
    void setAttr(Segment* seg, attrCode ind, uint8 subindex, int16 val, const SlotMap & map);
    int getAttr(const Segment *seg, attrCode ind, uint8 subindex) const;
    void attachTo(Slot *ap) { m_parent = ap; }
    Slot *attachedTo() const { return m_parent; }
    Position attachOffset() const { return m_attach - m_with; }
    Slot* firstChild() const { return m_child; }
    bool child(Slot *ap);
    Slot* nextSibling() const { return m_sibling; }
    bool sibling(Slot *ap);
    uint32 clusterMetric(const Segment* seg, uint8 metric, uint8 attrLevel);
    void positionShift(Position a) { m_position += a; }
    void floodShift(Position adj);
    float just() const { return m_just; }
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
    int16   *m_userAttr;     
};

} 

struct gr_slot : public graphite2::Slot {};
