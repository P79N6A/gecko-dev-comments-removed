

























#pragma once

#ifndef DISABLE_SEGCACHE

#include <graphite2/Segment.h>
#include "Main.h"
#include "Slot.h"
#include "FeatureVal.h"
#include "SegCacheEntry.h"
#include "Segment.h"

namespace graphite2 {

class SegCache;
class SegCacheEntry;
class SegCacheStore;






class SegCachePrefixEntry
{
public:
    SegCachePrefixEntry() { memset(this, 0, sizeof(SegCachePrefixEntry)); }
    ~SegCachePrefixEntry()
    {
        for (size_t j = 0; j < eMaxSpliceSize; j++)
        {
            if (m_entryCounts[j])
            {
                assert(m_entries[j]);
                for (size_t k = 0; k < m_entryCounts[j]; k++)
                {
                    m_entries[j][k].log(j);
                    m_entries[j][k].clear();
                }
                free(m_entries[j]);
            }
        }
    }
    const SegCacheEntry * find(const uint16 * cmapGlyphs, size_t length) const
    {
        if (length <= ePrefixLength)
        {
            assert(m_entryCounts[length-1] <= 1);
            if (m_entries[length-1])
                return m_entries[length-1];
            return NULL;
        }
        SegCacheEntry * entry = NULL;
        findPosition(cmapGlyphs, length, &entry);
        return entry;
    }
    SegCacheEntry * cache(const uint16* cmapGlyphs, size_t length, Segment * seg, size_t charOffset, unsigned long long totalAccessCount)
    {
        size_t listSize = m_entryBSIndex[length-1]? (m_entryBSIndex[length-1] << 1) - 1 : 0;
        SegCacheEntry * newEntries = NULL;
        if (m_entryCounts[length-1] + 1u > listSize)
        {
            if (m_entryCounts[length-1] == 0)
            {
                listSize = 1;
            }
            else
            {
                
                if (listSize >= eMaxSuffixCount)
                    return NULL;
                listSize = (m_entryBSIndex[length-1] << 2) - 1;
            }
            newEntries = gralloc<SegCacheEntry>(listSize);
            if (!newEntries)
            {
                return NULL;
            }
        }        

        uint16 insertPos = 0;
        if (m_entryCounts[length-1] > 0)
        {
            insertPos = findPosition(cmapGlyphs, length, NULL);
            if (!newEntries)
            {
                
                memmove(m_entries[length-1] + insertPos + 1, m_entries[length-1] + insertPos,
                    sizeof(SegCacheEntry) * (m_entryCounts[length-1] - insertPos));
            }
            else
            {
                memcpy(newEntries, m_entries[length-1], sizeof(SegCacheEntry) * (insertPos));
                memcpy(newEntries + insertPos + 1, m_entries[length-1] + insertPos,
                   sizeof(SegCacheEntry) * (m_entryCounts[length-1] - insertPos));
                
                free(m_entries[length-1]);
                m_entries[length-1] = newEntries;
                assert (m_entryBSIndex[length-1]);
                m_entryBSIndex[length-1] <<= 1;
            }
        } 
        else
        {
            m_entryBSIndex[length-1] = 1;
            m_entries[length-1] = newEntries;
        }
        m_entryCounts[length-1] += 1;
        new (m_entries[length-1] + insertPos)
            SegCacheEntry(cmapGlyphs, length, seg, charOffset, totalAccessCount);
        return m_entries[length-1]  + insertPos;
    }
    uint32 purge(unsigned long long minAccessCount, unsigned long long oldAccessTime,
        unsigned long long currentTime);
    CLASS_NEW_DELETE
private:
    uint16 findPosition(const uint16 * cmapGlyphs, uint16 length, SegCacheEntry ** entry) const
    {
        int dir = 0;
        if (m_entryCounts[length-1] == 0)
        {
            if (entry) *entry = NULL;
            return 0;
        }
        else if (m_entryCounts[length-1] == 1)
        {
            
            for (int i = ePrefixLength; i < length; i++)
            {
                if (cmapGlyphs[i] > m_entries[length-1][0].m_unicode[i])
                {
                    return 1;
                }
                else if (cmapGlyphs[i] < m_entries[length-1][0].m_unicode[i])
                {
                    return 0;
                }
            }
            if (entry)
                *entry = m_entries[length-1];
            return 0;
        }
        uint16 searchIndex = m_entryBSIndex[length-1] - 1;
        uint16 stepSize = m_entryBSIndex[length-1] >> 1;
        size_t prevIndex = searchIndex;
        do
        {
            dir = 0;
            if (searchIndex >= m_entryCounts[length-1])
            {
                dir = -1;
                searchIndex -= stepSize;
                stepSize >>= 1;
            }
            else
            {
                for (int i = ePrefixLength; i < length; i++)
                {
                    if (cmapGlyphs[i] > m_entries[length-1][searchIndex].m_unicode[i])
                    {
                        dir = 1;
                        searchIndex += stepSize;
                        stepSize >>= 1;
                        break;
                    }
                    else if (cmapGlyphs[i] < m_entries[length-1][searchIndex].m_unicode[i])
                    {
                        dir = -1;
                        searchIndex -= stepSize;
                        stepSize >>= 1;
                        break;
                    }
                }
            }
            if (prevIndex == searchIndex)
                break;
            prevIndex = searchIndex;
        } while (dir != 0);
        if (entry)
        {
            if (dir == 0)
                *entry = m_entries[length-1] + searchIndex;
            else
                *entry = NULL;
        }
        else
        {
            
            
            assert(dir != 0);
            if (dir > 0)
                ++searchIndex;
        }
        return searchIndex;
    }
    
    uint16 m_entryCounts[eMaxSpliceSize];
    uint16 m_entryBSIndex[eMaxSpliceSize];
    SegCacheEntry * m_entries[eMaxSpliceSize];
    unsigned long long m_lastPurge;
};

union SegCachePrefixArray;

#define SEG_CACHE_MIN_INDEX (store->maxCmapGid())
#define SEG_CACHE_MAX_INDEX (store->maxCmapGid()+1u)
#define SEG_CACHE_UNSET_INDEX (store->maxCmapGid()+2u)

union SegCachePrefixArray
{
    void ** raw;
    SegCachePrefixArray * array;
    SegCachePrefixEntry ** prefixEntries;
    uintptr * range;
};

class SegCache
{
public:
    SegCache(const SegCacheStore * store, const Features& features);
    ~SegCache();

    const SegCacheEntry * find(const uint16 * cmapGlyphs, size_t length) const;
    SegCacheEntry * cache(SegCacheStore * store, const uint16 * cmapGlyphs, size_t length, Segment * seg, size_t charOffset);
    void purge(SegCacheStore * store);

    long long totalAccessCount() const { return m_totalAccessCount; }
    size_t segmentCount() const { return m_segmentCount; }
    const Features & features() const { return m_features; }
    void clear(SegCacheStore * store);

    CLASS_NEW_DELETE
private:
    void freeLevel(SegCacheStore * store, SegCachePrefixArray prefixes, size_t level);
    void purgeLevel(SegCacheStore * store, SegCachePrefixArray prefixes, size_t level,
                    unsigned long long minAccessCount, unsigned long long oldAccessTime);

    uint16 m_prefixLength;
    uint16 m_maxCachedSegLength;
    size_t m_segmentCount;
    SegCachePrefixArray m_prefixes;
    Features m_features;
    mutable unsigned long long m_totalAccessCount;
    mutable unsigned long long m_totalMisses;
    float m_purgeFactor;
};

inline const SegCacheEntry * SegCache::find(const uint16 * cmapGlyphs, size_t length) const
{
    uint16 pos = 0;
    if (!length || length > eMaxSpliceSize) return NULL;
    SegCachePrefixArray pEntry = m_prefixes.array[cmapGlyphs[0]];
    while (++pos < m_prefixLength - 1)
    {
        if (!pEntry.raw)
        {
            ++m_totalMisses;
            return NULL;
        }
        pEntry = pEntry.array[(pos < length)? cmapGlyphs[pos] : 0];
    }
    if (!pEntry.raw)
    {
        ++m_totalMisses;
        return NULL;
    }
    SegCachePrefixEntry * prefixEntry = pEntry.prefixEntries[(pos < length)? cmapGlyphs[pos] : 0];
    if (!prefixEntry)
    {
        ++m_totalMisses;
        return NULL;
    }
    const SegCacheEntry * entry = prefixEntry->find(cmapGlyphs, length);
    if (entry)
    {
        ++m_totalAccessCount;
        entry->accessed(m_totalAccessCount);
    }
    else
    {
        ++m_totalMisses;
    }   
    return entry;
}
    
} 

#endif

