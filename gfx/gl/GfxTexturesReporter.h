





#ifndef GFXTEXTURESREPORTER_H_
#define GFXTEXTURESREPORTER_H_

#include "nsIMemoryReporter.h"
#include "GLTypes.h"

namespace mozilla {
namespace gl {

class GfxTexturesReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
    NS_DECL_ISUPPORTS

    GfxTexturesReporter()
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

    NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                              nsISupports* aData)
    {
        return MOZ_COLLECT_REPORT(
            "gfx-textures", KIND_OTHER, UNITS_BYTES, sAmount,
            "Memory used for storing GL textures.");
    }

private:
    static int64_t sAmount;
};

}
}

#endif 
