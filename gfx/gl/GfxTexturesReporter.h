





#ifndef GFXTEXTURESREPORTER_H_
#define GFXTEXTURESREPORTER_H_

#include "mozilla/Atomics.h"
#include "nsIMemoryReporter.h"
#include "GLTypes.h"

namespace mozilla {
namespace gl {

class GfxTexturesReporter final : public nsIMemoryReporter
{
    ~GfxTexturesReporter() {}

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
                             int32_t tileWidth, int32_t tileHeight);

    static void UpdateWasteAmount(int32_t delta) {
      sTileWasteAmount += delta;
    }

    NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                              nsISupports* aData, bool aAnonymize) override
    {
        MOZ_COLLECT_REPORT("gfx-tiles-waste", KIND_OTHER, UNITS_BYTES,
            sTileWasteAmount,
            "Memory lost due to tiles extending past content boundaries");
        return MOZ_COLLECT_REPORT(
            "gfx-textures", KIND_OTHER, UNITS_BYTES, sAmount,
            "Memory used for storing GL textures.");
    }

private:
    static Atomic<int32_t> sAmount;
    
    static Atomic<int32_t> sTileWasteAmount;
};

class GfxTextureWasteTracker {
public:
  GfxTextureWasteTracker()
    : mBytes(0)
  {
    MOZ_COUNT_CTOR(GfxTextureWasteTracker);
  }

  void Update(int32_t aPixelArea, int32_t aBytesPerPixel) {
    GfxTexturesReporter::UpdateWasteAmount(-mBytes);
    mBytes = aPixelArea * aBytesPerPixel;
    GfxTexturesReporter::UpdateWasteAmount(mBytes);
  }

  ~GfxTextureWasteTracker() {
    GfxTexturesReporter::UpdateWasteAmount(-mBytes);
    MOZ_COUNT_DTOR(GfxTextureWasteTracker);
  }
private:
  GfxTextureWasteTracker(const GfxTextureWasteTracker& aRef);

  int32_t mBytes;
};

} 
} 

#endif 
