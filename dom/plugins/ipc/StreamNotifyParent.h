





#ifndef mozilla_plugins_StreamNotifyParent_h
#define mozilla_plugins_StreamNotifyParent_h

#include "mozilla/plugins/PStreamNotifyParent.h"

namespace mozilla {
namespace plugins {

class StreamNotifyParent : public PStreamNotifyParent
{
  friend class PluginInstanceParent;

  StreamNotifyParent()
    : mDestructionFlag(nullptr)
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
    mDestructionFlag = nullptr;
  }

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  bool RecvRedirectNotifyResponse(const bool& allow) override;

  bool* mDestructionFlag;
};

} 
} 

#endif
