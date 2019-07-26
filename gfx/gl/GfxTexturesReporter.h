





#ifndef GFXTEXTURESREPORTER_H_
#define GFXTEXTURESREPORTER_H_

#include "nsIMemoryReporter.h"
#include "GLTypes.h"

namespace mozilla {
namespace gl {

class GfxTexturesReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    GfxTexturesReporter()
      : MemoryReporterBase("gfx-textures", KIND_OTHER, UNITS_BYTES,
                           "Memory used for storing GL textures.")
    {
#ifdef DEBUG
        
        
        static bool hasRun = false;
        MOZ_ASSERT(!hasRun);
        hasRun = true;
#endif
    }

    enum MemoryUse {
        
        MemoryAllocated,
        
        MemoryFreed
    };

    
    
    static void UpdateAmount(MemoryUse action, GLenum format, GLenum type,
                             uint16_t tileSize);

private:
    int64_t Amount() MOZ_OVERRIDE { return sAmount; }

    static int64_t sAmount;
};

}
}

#endif 
