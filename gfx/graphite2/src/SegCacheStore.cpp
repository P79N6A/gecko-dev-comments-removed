


























#ifndef GRAPHITE2_NSEGCACHE

#include "inc/SegCacheStore.h"
#include "inc/Face.h"


using namespace graphite2;

SegCacheStore::SegCacheStore(const Face & face, unsigned int numSilf, size_t maxSegments)
: m_caches(new SilfSegCache[numSilf]),
  m_numSilf(numSilf),
  m_maxSegments(maxSegments),
  m_maxCmapGid(face.glyphs().numGlyphs()),
  m_spaceGid(face.cmap()[0x20]),
  m_zwspGid(face.cmap()[0x200B])
{
}

#endif

