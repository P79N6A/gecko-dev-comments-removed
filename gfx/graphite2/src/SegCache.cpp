


























#include "Main.h"
#include "TtfTypes.h"
#include "TtfUtil.h"
#include "SegCache.h"
#include "SegCacheEntry.h"
#include "SegCacheStore.h"
#include "CmapCache.h"


using namespace graphite2;

#ifndef DISABLE_SEGCACHE

SegCache::SegCache(const SegCacheStore * store, const Features & feats)
    :
    m_prefixLength(ePrefixLength),
    m_maxCachedSegLength(eMaxSpliceSize),
    m_segmentCount(0),
    m_features(feats),
    m_totalAccessCount(0l), m_totalMisses(0l),
    m_purgeFactor(1.0f / (ePurgeFactor * store->maxSegmentCount()))
{
    m_prefixes.raw = grzeroalloc<void*>(store->maxCmapGid() + 2);
    m_prefixes.range[SEG_CACHE_MIN_INDEX] = SEG_CACHE_UNSET_INDEX;
    m_prefixes.range[SEG_CACHE_MAX_INDEX] = SEG_CACHE_UNSET_INDEX;
}

void SegCache::freeLevel(SegCacheStore * store, SegCachePrefixArray prefixes, size_t level)
{
    for (size_t i = 0; i < store->maxCmapGid(); i++)
    {
        if (prefixes.array[i].raw)
        {
            if (level + 1 < ePrefixLength)
                freeLevel(store, prefixes.array[i], level + 1);
            else
            {
                SegCachePrefixEntry * prefixEntry = prefixes.prefixEntries[i];
                delete prefixEntry;
            }
        }
    }
    free(prefixes.raw);
}

void SegCache::clear(SegCacheStore * store)
{
    #ifndef DISABLE_TRACING
    if (XmlTraceLog::get().active())
    {
        XmlTraceLog::get().openElement(ElementSegCache);
        XmlTraceLog::get().addAttribute(AttrNum, m_segmentCount);
        XmlTraceLog::get().addAttribute(AttrAccessCount, m_totalAccessCount);
        XmlTraceLog::get().addAttribute(AttrMisses, m_totalMisses);
    }
#endif
    freeLevel(store, m_prefixes, 0);
#ifndef DISABLE_TRACING
    if (XmlTraceLog::get().active())
    {
        XmlTraceLog::get().closeElement(ElementSegCache);
    }
#endif
    m_prefixes.raw = NULL;
}

SegCache::~SegCache()
{
    assert(m_prefixes.raw == NULL);
}

SegCacheEntry* SegCache::cache(SegCacheStore * store, const uint16* cmapGlyphs, size_t length, Segment * seg, size_t charOffset)
{
    uint16 pos = 0;
    if (!length) return NULL;
    assert(length < m_maxCachedSegLength);
    SegCachePrefixArray pArray = m_prefixes;
    while (pos + 1 < m_prefixLength)
    {
        uint16 gid = (pos < length)? cmapGlyphs[pos] : 0;
        if (!pArray.array[gid].raw)
        {
            pArray.array[gid].raw = grzeroalloc<void*>(store->maxCmapGid() + 2);
            if (!pArray.array[gid].raw)
                return NULL; 
            if (pArray.range[SEG_CACHE_MIN_INDEX] == SEG_CACHE_UNSET_INDEX)
            {
                pArray.range[SEG_CACHE_MIN_INDEX] = gid;
                pArray.range[SEG_CACHE_MAX_INDEX] = gid;
            }
            else
            {
                if (gid < pArray.range[SEG_CACHE_MIN_INDEX])
                    pArray.range[SEG_CACHE_MIN_INDEX] = gid;
                else if (gid > pArray.range[SEG_CACHE_MAX_INDEX])
                    pArray.range[SEG_CACHE_MAX_INDEX] = gid;
            }
        }
        pArray = pArray.array[gid];
        ++pos;
    }
    uint16 gid = (pos < length)? cmapGlyphs[pos] : 0;
    SegCachePrefixEntry * prefixEntry = pArray.prefixEntries[gid];
    if (!prefixEntry)
    {
        prefixEntry = new SegCachePrefixEntry();
        pArray.prefixEntries[gid] = prefixEntry;
        if (pArray.range[SEG_CACHE_MIN_INDEX] == SEG_CACHE_UNSET_INDEX)
        {
            pArray.range[SEG_CACHE_MIN_INDEX] = gid;
            pArray.range[SEG_CACHE_MAX_INDEX] = gid;
        }
        else
        {
            if (gid < pArray.range[SEG_CACHE_MIN_INDEX])
                pArray.range[SEG_CACHE_MIN_INDEX] = gid;
            else if (gid > pArray.range[SEG_CACHE_MAX_INDEX])
                pArray.range[SEG_CACHE_MAX_INDEX] = gid;
        }
    }
    if (!prefixEntry) return NULL;
    
    if (m_segmentCount + 1 > store->maxSegmentCount())
    {
        purge(store);
        assert(m_segmentCount < store->maxSegmentCount());
    }
    SegCacheEntry * pEntry = prefixEntry->cache(cmapGlyphs, length, seg, charOffset, m_totalAccessCount);
    if (pEntry) ++m_segmentCount;
    return pEntry;
}

void SegCache::purge(SegCacheStore * store)
{
    unsigned long long minAccessCount = m_totalAccessCount * m_purgeFactor + 1;
    if (minAccessCount < 2) minAccessCount = 2;
    unsigned long long oldAccessTime = m_totalAccessCount - store->maxSegmentCount() / eAgeFactor;
    purgeLevel(store, m_prefixes, 0, minAccessCount, oldAccessTime);
}

void SegCache::purgeLevel(SegCacheStore * store, SegCachePrefixArray prefixes, size_t level,
                          unsigned long long minAccessCount, unsigned long long oldAccessTime)
{
    if (prefixes.range[SEG_CACHE_MIN_INDEX] == SEG_CACHE_UNSET_INDEX) return;
    size_t maxGlyphCached = prefixes.range[SEG_CACHE_MAX_INDEX];
    for (size_t i = prefixes.range[SEG_CACHE_MIN_INDEX]; i <= maxGlyphCached; i++)
    {
        if (prefixes.array[i].raw)
        {
            if (level + 1 < ePrefixLength)
                purgeLevel(store, prefixes.array[i], level + 1, minAccessCount, oldAccessTime);
            else
            {
                SegCachePrefixEntry * prefixEntry = prefixes.prefixEntries[i];
                m_segmentCount -= prefixEntry->purge(minAccessCount,
                    oldAccessTime, m_totalAccessCount);
            }
        }
    }
}

uint32 SegCachePrefixEntry::purge(unsigned long long minAccessCount,
                                              unsigned long long oldAccessTime,
                                              unsigned long long currentTime)
{
    
    
    

    uint32 totalPurged = 0;
    
    for (uint16 length = 0; length < eMaxSpliceSize; length++)
    {
        if (m_entryCounts[length] == 0)
            continue;
        uint16 purgeCount = 0;
        uint16 newIndex = 0;
        for (uint16 j = 0; j < m_entryCounts[length]; j++)
        {
            SegCacheEntry & tempEntry = m_entries[length][j];
            
            
            if (tempEntry.accessCount() <= minAccessCount &&
                tempEntry.lastAccess() <= oldAccessTime)
            {
                tempEntry.clear();
                ++purgeCount;
            }
            else
            {
                m_entries[length][newIndex++] = m_entries[length][j];
            }
        }
        if (purgeCount == m_entryCounts[length])
        {
            assert(newIndex == 0);
            m_entryCounts[length] = 0;
            m_entryBSIndex[length] = 0;
            free(m_entries[length]);
            m_entries[length] = NULL;
        }
        else if (purgeCount > 0)
        {
            assert(m_entryCounts[length] == newIndex + purgeCount);
            m_entryCounts[length] = newIndex;
        }
        totalPurged += purgeCount;
    }
    m_lastPurge = currentTime;
    return totalPurged;
}

#endif
