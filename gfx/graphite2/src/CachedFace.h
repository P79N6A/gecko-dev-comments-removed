

























#pragma once

#ifndef DISABLE_SEGCACHE

#include "Face.h"

namespace graphite2 {

class SegCacheStore;

class CachedFace : public Face
{
public:
    CachedFace(const void* appFaceHandle, gr_get_table_fn getTable2);
    bool setupCache(unsigned int cacheSize);
    virtual ~CachedFace();
    virtual bool runGraphite(Segment *seg, const Silf *silf) const;
    SegCacheStore * cacheStore() { return m_cacheStore; }
private:
    SegCacheStore * m_cacheStore;
};

} 

#endif

