


























#ifndef GRAPHITE2_NSEGCACHE

#include <graphite2/Segment.h>
#include "inc/CachedFace.h"
#include "inc/SegCacheStore.h"


using namespace graphite2;

CachedFace::CachedFace(const void* appFaceHandle, gr_get_table_fn getTable2)
: Face(appFaceHandle, getTable2), m_cacheStore(0) 
{
}

CachedFace::~CachedFace()
{
    delete m_cacheStore;
}

bool CachedFace::setupCache(unsigned int cacheSize)
{
    m_cacheStore = new SegCacheStore(this, m_numSilf, cacheSize);
    return (m_cacheStore != NULL);
}


bool CachedFace::runGraphite(Segment *seg, const Silf *pSilf) const
{
    assert(pSilf);
    pSilf->runGraphite(seg, 0, pSilf->substitutionPass());

    SegCache * segCache = NULL;
    unsigned int silfIndex = 0;

    for (unsigned int i = 0; i < m_numSilf; i++)
    {
        if (&(m_silfs[i]) == pSilf)
        {
            break;
        }
    }
    assert(silfIndex < m_numSilf);
    assert(m_cacheStore);
    segCache = m_cacheStore->getOrCreate(silfIndex, seg->getFeatures(0));
    
    Slot * subSegStartSlot = seg->first();
    Slot * subSegEndSlot = subSegStartSlot;
    uint16 cmapGlyphs[eMaxSpliceSize];
    int subSegStart = 0;
    bool spaceOnly = true;
    for (unsigned int i = 0; i < seg->charInfoCount(); i++)
    {
        if (i - subSegStart < eMaxSpliceSize)
        {
            cmapGlyphs[i-subSegStart] = subSegEndSlot->gid();
        }
        if (!m_cacheStore->isSpaceGlyph(subSegEndSlot->gid()))
        {
            spaceOnly = false;
        }
        
        int breakWeight = seg->charinfo(i)->breakWeight();
        int nextBreakWeight = (i + 1 < seg->charInfoCount())?
            seg->charinfo(i+1)->breakWeight() : 0;
        if (((breakWeight > 0) &&
             (breakWeight <= gr_breakWord)) ||
            (i + 1 == seg->charInfoCount()) ||
             m_cacheStore->isSpaceGlyph(subSegEndSlot->gid()) ||
            ((i + 1 < seg->charInfoCount()) &&
             (((nextBreakWeight < 0) &&
              (nextBreakWeight >= gr_breakBeforeWord)) ||
              (subSegEndSlot->next() && m_cacheStore->isSpaceGlyph(subSegEndSlot->next()->gid())))))
        {
            
            Slot * nextSlot = subSegEndSlot->next();
            if (spaceOnly)
            {
                
            }
            else
            {
                
                const SegCacheEntry * entry = (segCache)?
                    segCache->find(cmapGlyphs, i - subSegStart + 1) : NULL;
                
                if (!entry)
                {
                    unsigned int length = i - subSegStart + 1;
                    SegmentScopeState scopeState = seg->setScope(subSegStartSlot, subSegEndSlot, length);
                    pSilf->runGraphite(seg, pSilf->substitutionPass(), pSilf->numPasses());
                    
                    
                    
                    if ((length < eMaxSpliceSize) && segCache)
                        entry = segCache->cache(m_cacheStore, cmapGlyphs, length, seg, subSegStart);
                    seg->removeScope(scopeState);
                }
                else
                {
                    
                    seg->splice(subSegStart, i - subSegStart + 1, subSegStartSlot, subSegEndSlot,
                        entry->first(), entry->glyphLength());
                }
            }
            subSegEndSlot = nextSlot;
            subSegStartSlot = nextSlot;
            subSegStart = i + 1;
            spaceOnly = true;
        }
        else
        {
            subSegEndSlot = subSegEndSlot->next();
        }
    }
    return true;
}

#endif

