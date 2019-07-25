

























#include "graphite2/Segment.h"
#include "Segment.h"
#include "Slot.h"
#include "Font.h"


extern "C" {


const gr_slot* gr_slot_next_in_segment(const gr_slot* p)
{
    assert(p);
    return static_cast<const gr_slot*>(p->next());
}

const gr_slot* gr_slot_prev_in_segment(const gr_slot* p)
{
    assert(p);
    return static_cast<const gr_slot*>(p->prev());
}

const gr_slot* gr_slot_attached_to(const gr_slot* p)        
{
    assert(p);
    return static_cast<const gr_slot*>(p->attachTo());
}


const gr_slot* gr_slot_first_attachment(const gr_slot* p)        
{        
    assert(p);
    return static_cast<const gr_slot*>(p->firstChild());
}

    
const gr_slot* gr_slot_next_sibling_attachment(const gr_slot* p)        
{        
    assert(p);
    return static_cast<const gr_slot*>(p->nextSibling());
}


unsigned short gr_slot_gid(const gr_slot* p)
{
    assert(p);
    return p->glyph();
}


float gr_slot_origin_X(const gr_slot* p)
{
    assert(p);
    return p->origin().x;
}


float gr_slot_origin_Y(const gr_slot* p)
{
    assert(p);
    return p->origin().y;
}


float gr_slot_advance_X(const gr_slot* p, const gr_face *face, const gr_font *font)
{
    assert(p);
    float scale = 1.0;
    float res = p->advance();
    if (font)
    {
        scale = font->scale();
        if (face && font->isHinted())
            res = (res - face->advance(p->gid())) * scale + font->advance(p->gid());
        else
            res = res * scale;
    }
    return res;
}

float gr_slot_advance_Y(const gr_slot *p, const gr_face *face, const gr_font *font)
{
    assert(p);
    float res = p->advancePos().y;
    if (font && (face || !face))
        return res * font->scale();
    else
        return res;
}
        
int gr_slot_before(const gr_slot* p)
{
    assert(p);
    return p->before();
}


int gr_slot_after(const gr_slot* p)
{
    assert(p);
    return p->after();
}

unsigned int gr_slot_index(const gr_slot *p)
{
    assert(p);
    return p->index();
}

int gr_slot_attr(const gr_slot* p, const gr_segment* pSeg, gr_attrCode index, gr_uint8 subindex)
{
    assert(p);
    return p->getAttr(pSeg, index, subindex);
}


int gr_slot_can_insert_before(const gr_slot* p)
{
    assert(p);
    return (p->isInsertBefore())? 1 : 0;
}


int gr_slot_original(const gr_slot* p)
{
    assert(p);
    return p->original();
}

void gr_slot_linebreak_before(gr_slot* p)
{
    assert(p);
    gr_slot *prev = (gr_slot *)p->prev();
    prev->next(NULL);
    prev->sibling(NULL);
    p->prev(NULL);
}

#if 0       
size_t id(const gr_slot* p)
{
    return (size_t)p->id();
}
#endif


} 

