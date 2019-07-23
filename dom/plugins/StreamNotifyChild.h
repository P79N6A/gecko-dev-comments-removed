





































#ifndef mozilla_plugins_StreamNotifyChild_h
#define mozilla_plugins_StreamNotifyChild_h

#include "mozilla/plugins/PStreamNotifyChild.h"

namespace mozilla {
namespace plugins {

class StreamNotifyChild : public PStreamNotifyChild
{
  friend class PluginInstanceChild;

public:
  StreamNotifyChild(const nsCString& aURL, void* aClosure)
    : mURL(aURL)
    , mClosure(aClosure)
  { }

private:
  nsCString mURL;
  void* mClosure;
};

} 
} 

#endif
