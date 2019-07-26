


























#ifndef GRAPHITE2_NSEGCACHE

#include <graphite2/Segment.h>
#include "inc/CachedFace.h"
#include "inc/SegCacheStore.h"


using namespace graphite2;

CachedFace::CachedFace(const void* appFaceHandle, const gr_face_ops & ops)
: Face(appFaceHandle, ops), m_cacheStore(0)
{
}

CachedFace::~CachedFace()
{
    delete m_cacheStore;
}

bool CachedFace::setupCache(unsigned int cacheSize)
{
    m_cacheStore = new SegCacheStore(*this, m_numSilf, cacheSize);
    return bool(m_cacheStore);
}


bool CachedFace::runGraphite(Segment *seg, const Silf *pSilf) const
{
    assert(pSilf);
    pSilf->runGraphite(seg, 0, pSilf->substitutionPass());

    unsigned int silfIndex = 0;
    for (; silfIndex < m_numSilf && &(m_silfs[silfIndex]) != pSilf; ++silfIndex);
    if (silfIndex == m_numSilf)  return false;
    SegCache * const segCache = m_cacheStore->getOrCreate(silfIndex, seg->getFeatures(0));
    if (!segCache)
    	return false;

    assert(m_cacheStore);
    
    Slot * subSegStartSlot = seg->first();
    Slot * subSegEndSlot = subSegStartSlot;
    uint16 cmapGlyphs[eMaxSpliceSize];
    int subSegStart = 0;
    for (unsigned int i = 0; i < seg->charInfoCount(); ++i)
    {
    	const unsigned int length = i - subSegStart + 1;
        if (length < eMaxSpliceSize)
            cmapGlyphs[length-1] = subSegEndSlot->gid();
        else return false;
        const bool spaceOnly = m_cacheStore->isSpaceGlyph(subSegEndSlot->gid());
        
        const int	breakWeight = seg->charinfo(i)->breakWeight(),
        		 	nextBreakWeight = (i + 1 < seg->charInfoCount())?
        		 			seg->charinfo(i+1)->breakWeight() : 0;
        const uint8 f = seg->charinfo(i)->flags();
        if (((spaceOnly
				|| (breakWeight > 0 && breakWeight <= gr_breakWord)
				|| i + 1 == seg->charInfoCount()
				|| ((nextBreakWeight < 0 && nextBreakWeight >= gr_breakBeforeWord)
					|| (subSegEndSlot->next() && m_cacheStore->isSpaceGlyph(subSegEndSlot->next()->gid()))))
				&& f != 1)
			|| f == 2)
        {
            
            Slot * nextSlot = subSegEndSlot->next();
            
            if (!spaceOnly)
            {
                
                const SegCacheEntry * entry = segCache->find(cmapGlyphs, length);
                
                if (!entry)
                {
                    SegmentScopeState scopeState = seg->setScope(subSegStartSlot, subSegEndSlot, length);
                    pSilf->runGraphite(seg, pSilf->substitutionPass(), pSilf->numPasses());
                    if (length < eMaxSpliceSize)
                    {
                        seg->associateChars();
                        entry = segCache->cache(m_cacheStore, cmapGlyphs, length, seg, subSegStart);
                    }
                    seg->removeScope(scopeState);
                }
                else
                    seg->splice(subSegStart, length, subSegStartSlot, subSegEndSlot,
                        entry->first(), entry->glyphLength());
            }
            subSegStartSlot = subSegEndSlot = nextSlot;
            subSegStart = i + 1;
        }
        else
        {
            subSegEndSlot = subSegEndSlot->next();
        }
    }
    return true;
}

#endif

