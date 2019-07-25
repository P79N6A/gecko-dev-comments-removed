


























#ifndef DISABLE_SEGCACHE

#include "Main.h"
#include "Slot.h"
#include "Segment.h"
#include "SegCache.h"
#include "SegCacheEntry.h"


using namespace graphite2;

SegCacheEntry::SegCacheEntry(const uint16* cmapGlyphs, size_t length, Segment * seg, size_t charOffset, long long cacheTime)
    : m_glyphLength(0), m_unicode(gralloc<uint16>(length)), m_glyph(NULL),
    m_attr(NULL),
    m_accessCount(0), m_lastAccess(cacheTime)
{
    for (uint16 i = 0; i < length; i++)
    {
        m_unicode[i] = cmapGlyphs[i];
    }
    size_t glyphCount = seg->slotCount();
    const Slot * slot = seg->first();
    m_glyph = new Slot[glyphCount];
    m_attr = gralloc<uint16>(glyphCount * seg->numAttrs());
    m_glyphLength = glyphCount;
    Slot * slotCopy = m_glyph;
    m_glyph->prev(NULL);
    struct Index2Slot {
        Index2Slot(uint16 i, const Slot * s) : m_i(i), m_slot(s) {};
        Index2Slot() : m_i(0), m_slot(NULL) {};
        uint16 m_i;
        const Slot * m_slot;
    };
    struct Index2Slot parentGlyphs[eMaxSpliceSize];
    struct Index2Slot childGlyphs[eMaxSpliceSize];
    uint16 numParents = 0;
    uint16 numChildren = 0;
    uint16 pos = 0;
    while (slot)
    {
        slotCopy->userAttrs(m_attr + pos * seg->numAttrs());
        slotCopy->set(*slot, -static_cast<int32>(charOffset), seg->numAttrs());
        if (slot->firstChild())
        {
            new(parentGlyphs + numParents) Index2Slot( pos, slot );
            ++numParents;
        }
        if (slot->attachedTo())
        {
            new(childGlyphs + numChildren) Index2Slot( pos, slot );
            ++numChildren;
        }
        slot = slot->next();
        ++slotCopy;
        ++pos;
        if (slot)
        {
            slotCopy->prev(slotCopy-1);
            (slotCopy-1)->next(slotCopy);
        }
    }
    
    for (int16 i = 0; i < numChildren; i++)
    {
        if (childGlyphs[i].m_slot->nextSibling())
        {
            for (int16 j = i; j < numChildren; j++)
            {
                if (childGlyphs[i].m_slot->nextSibling() == childGlyphs[j].m_slot)
                {
                    m_glyph[childGlyphs[i].m_i].sibling(m_glyph + childGlyphs[j].m_i);
                    break;
                }
            }
            if (!m_glyph[childGlyphs[i].m_i].nextSibling())
            {
                
                for (int16 j = i-1; j >= 0; j--)
                {
                    if (childGlyphs[i].m_slot->nextSibling() == childGlyphs[j].m_slot)
                    {
                        m_glyph[childGlyphs[i].m_i].sibling(m_glyph + childGlyphs[j].m_i);
                        break;
                    }
                }
            }
        }
        
        for (int16 j = 0; j < numParents; j++)
        {
            if (childGlyphs[i].m_slot->attachedTo() == parentGlyphs[j].m_slot)
            {
                m_glyph[childGlyphs[i].m_i].attachTo(m_glyph + parentGlyphs[j].m_i);
                if (parentGlyphs[j].m_slot->firstChild() == childGlyphs[i].m_slot)
                {
                    m_glyph[parentGlyphs[j].m_i].child(m_glyph + childGlyphs[i].m_i);
                }
            }
        }
    }
}

void SegCacheEntry::log(GR_MAYBE_UNUSED size_t unicodeLength) const
{
#ifndef DISABLE_TRACING
    if (XmlTraceLog::get().active())
    {
        XmlTraceLog::get().openElement(ElementSegCacheEntry);
        XmlTraceLog::get().addAttribute(AttrAccessCount, m_accessCount);
        XmlTraceLog::get().addAttribute(AttrLastAccess, m_lastAccess);
        for (size_t i = 0; i < unicodeLength; i++)
        {
            XmlTraceLog::get().openElement(ElementText);
            XmlTraceLog::get().addAttribute(AttrGlyphId, m_unicode[i]);
            XmlTraceLog::get().closeElement(ElementText);
        }
        for (size_t i = 0; i < m_glyphLength; i++)
        {
            XmlTraceLog::get().openElement(ElementGlyph);
            XmlTraceLog::get().addAttribute(AttrGlyphId, m_glyph[i].gid());
            XmlTraceLog::get().addAttribute(AttrX, m_glyph[i].origin().x);
            XmlTraceLog::get().addAttribute(AttrY, m_glyph[i].origin().y);
            XmlTraceLog::get().addAttribute(AttrBefore, m_glyph[i].before());
            XmlTraceLog::get().addAttribute(AttrAfter, m_glyph[i].after());
            XmlTraceLog::get().closeElement(ElementGlyph);
        }
        XmlTraceLog::get().closeElement(ElementSegCacheEntry);
    }
#endif
}

void SegCacheEntry::clear()
{
    free(m_unicode);
    free(m_attr);
    delete [] m_glyph;
    m_unicode = NULL;
    m_glyph = NULL;
    m_glyphLength = 0;
    m_attr = NULL;
}

#endif

