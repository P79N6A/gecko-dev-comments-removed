






#ifndef SkImageGeneratorPriv_DEFINED
#define SkImageGeneratorPriv_DEFINED

#include "SkImageGenerator.h"
#include "SkDiscardableMemory.h"























bool SkInstallDiscardablePixelRef(SkImageGenerator*, SkBitmap* destination,
                                  SkDiscardableMemory::Factory* factory);

#endif
