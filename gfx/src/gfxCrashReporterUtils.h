

































#ifndef gfxCrashReporterUtils_h__
#define gfxCrashReporterUtils_h__

#include "gfxCore.h"

namespace mozilla {












class NS_GFX ScopedGfxFeatureReporter
{
public:
  ScopedGfxFeatureReporter(const char *aFeature, bool force = false)
    : mFeature(aFeature), mStatusChar('-')
  {
    WriteAppNote(force ? '!' : '?');
  }
  ~ScopedGfxFeatureReporter() {
    WriteAppNote(mStatusChar);
  }
  void SetSuccessful() { mStatusChar = '+'; }

protected:
  const char *mFeature;
  char mStatusChar;

private:
  void WriteAppNote(char statusChar);
};

} 

#endif 
