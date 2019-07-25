

























#pragma once

#ifndef DISABLE_SEGCACHE

#include "Main.h"
#include "CmapCache.h"
#include "SegCache.h"

namespace graphite2 {

class SegCache;
class Face;

class SilfSegCache
{
public:
    SilfSegCache() : m_caches(NULL), m_cacheCount(0) {};
    ~SilfSegCache()
    {
        assert(m_caches == NULL);
    }
    void clear(SegCacheStore * cacheStore)
    {
        for (size_t i = 0; i < m_cacheCount; i++)
        {
            m_caches[i]->clear(cacheStore);
            delete m_caches[i];
        }
        free(m_caches);
        m_caches = NULL;
        m_cacheCount = 0;
    }
    SegCache * getOrCreate(SegCacheStore * cacheStore, const Features & features)
    {
        for (size_t i = 0; i < m_cacheCount; i++)
        {
            if (m_caches[i]->features() == features)
                return m_caches[i];
        }
        SegCache ** newData = gralloc<SegCache*>(m_cacheCount+1);
        if (newData)
        {
            if (m_cacheCount > 0)
            {
                memcpy(newData, m_caches, sizeof(SegCache*) * m_cacheCount);
                free(m_caches);
            }
            m_caches = newData;
            m_caches[m_cacheCount] = new SegCache(cacheStore, features);
            m_cacheCount++;
            return m_caches[m_cacheCount - 1];
        }
        return NULL;
    }
    CLASS_NEW_DELETE
private:
    SegCache ** m_caches;
    size_t m_cacheCount;
};

class SegCacheStore
{
public:
    SegCacheStore(const Face *face, unsigned int numSilf, size_t maxSegments);
    ~SegCacheStore()
    {
        for (size_t i = 0; i < m_numSilf; i++)
        {
            m_caches[i].clear(this);
        }
        delete [] m_caches;
        m_caches = NULL;
    }
    SegCache * getOrCreate(unsigned int i, const Features & features)
    {
        return m_caches[i].getOrCreate(this, features);
    }
    bool isSpaceGlyph(uint16 gid) const { return (gid == m_spaceGid) || (gid == m_zwspGid); }
    uint16 maxCmapGid() const { return m_maxCmapGid; }
    uint32 maxSegmentCount() const { return m_maxSegments; };

    CLASS_NEW_DELETE
private:
    SilfSegCache * m_caches;
    uint8 m_numSilf;
    uint32 m_maxSegments;
    uint16 m_maxCmapGid;
    uint16 m_spaceGid;
    uint16 m_zwspGid;
};

} 

#endif

