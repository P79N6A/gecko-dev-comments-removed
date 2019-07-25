

























#pragma once

#ifndef DISABLE_SEGCACHE

#include "Main.h"

namespace graphite2 {

class Segment;
class Slot;
class SegCacheEntry;
class SegCachePrefixEntry;

enum SegCacheParameters {
    
    ePrefixLength = 2,
    
    eAgeFactor = 4,
    

    ePurgeFactor = 5,
    

    eMaxSuffixCount = 15

};

class SegCacheCharInfo
{
public:
    uint16 m_unicode;
    uint16 m_before;
    uint16 m_after;
};





class SegCacheEntry
{
    friend class SegCachePrefixEntry;
public:
    SegCacheEntry() :
        m_glyphLength(0), m_unicode(NULL), m_glyph(NULL), m_attr(NULL),
        m_accessCount(0), m_lastAccess(0)
    {}
    SegCacheEntry(const uint16 * cmapGlyphs, size_t length, Segment * seg, size_t charOffset, long long cacheTime);
    ~SegCacheEntry() { clear(); };
    void clear();
    size_t glyphLength() const { return m_glyphLength; }
    const Slot * first() const { return m_glyph; }
    const Slot * last() const { return m_glyph + (m_glyphLength - 1); }

    void log(size_t unicodeLength) const;
    
    unsigned long long accessCount() const { return m_accessCount; }
    
    void accessed(unsigned long long cacheTime) const
    {
        m_lastAccess = cacheTime; ++m_accessCount;
    };

    int compareRank(const SegCacheEntry & entry) const
    {
        if (m_accessCount > entry.m_accessCount) return 1;
        else if (m_accessCount < entry.m_accessCount) return 1;
        else if (m_lastAccess > entry.m_lastAccess) return 1;
        else if (m_lastAccess < entry.m_lastAccess) return -1;
        return 0;
    }
    unsigned long long lastAccess() const { return m_lastAccess; };

    CLASS_NEW_DELETE;
private:

    size_t m_glyphLength;
    

    uint16 * m_unicode;
    
    Slot * m_glyph;
    uint16 * m_attr;
    mutable unsigned long long m_accessCount;
    mutable unsigned long long m_lastAccess;
};

} 

#endif
