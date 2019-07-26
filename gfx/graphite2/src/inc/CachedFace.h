

























#pragma once

#ifndef GRAPHITE2_NSEGCACHE

#include "inc/Face.h"

namespace graphite2 {

class SegCacheStore;
class SegCache;

class CachedFace : public Face
{
    CachedFace(const CachedFace &);
    CachedFace & operator = (const CachedFace &);

public:
    CachedFace(const void* appFaceHandle, const gr_face_ops & ops);
    bool setupCache(unsigned int cacheSize);
    virtual ~CachedFace();
    virtual bool runGraphite(Segment *seg, const Silf *silf) const;
    SegCacheStore * cacheStore() { return m_cacheStore; }
private:
    SegCacheStore * m_cacheStore;
};

} 

#endif

