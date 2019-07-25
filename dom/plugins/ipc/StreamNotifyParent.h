





































#ifndef mozilla_plugins_StreamNotifyParent_h
#define mozilla_plugins_StreamNotifyParent_h

#include "mozilla/plugins/PStreamNotifyParent.h"

namespace mozilla {
namespace plugins {

class StreamNotifyParent : public PStreamNotifyParent
{
  friend class PluginInstanceParent;

  StreamNotifyParent()
    : mDestructionFlag(NULL)
  { }
  ~StreamNotifyParent() {
    if (mDestructionFlag)
      *mDestructionFlag = true;
  }

public:
  
  
  void SetDestructionFlag(bool* flag) {
    mDestructionFlag = flag;
  }
  void ClearDestructionFlag() {
    mDestructionFlag = NULL;
  }

private:
  bool RecvRedirectNotifyResponse(const bool& allow);

  bool* mDestructionFlag;
};

} 
} 

#endif
