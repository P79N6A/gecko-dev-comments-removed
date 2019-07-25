

































#ifndef gfxCrashReporterUtils_h__
#define gfxCrashReporterUtils_h__

#include "gfxCore.h"

namespace mozilla {












class NS_GFX ScopedGfxFeatureReporter
{
public:
  ScopedGfxFeatureReporter(const char *aFeature) : mFeature(aFeature), mStatusChar('-')
  {
    WriteAppNote('?');
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
