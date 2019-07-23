





































#ifndef mozilla_plugins_StreamNotifyChild_h
#define mozilla_plugins_StreamNotifyChild_h

#include "mozilla/plugins/PStreamNotifyChild.h"

namespace mozilla {
namespace plugins {

class StreamNotifyChild : public PStreamNotifyChild
{
  friend class PluginInstanceChild;
  friend class BrowserStreamChild;

public:
  StreamNotifyChild(const nsCString& aURL)
    : mURL(aURL)
    , mClosure(NULL)
  { }

  void SetValid(void* aClosure) {
    mClosure = aClosure;
  }

  bool Answer__delete__(const NPReason& reason);

private:
  nsCString mURL;
  void* mClosure;
};

} 
} 

#endif
