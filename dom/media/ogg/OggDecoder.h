




#if !defined(OggDecoder_h_)
#define OggDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {

class OggDecoder : public MediaDecoder
{
public:
  OggDecoder()
    : mShutdownBitMonitor("mShutdownBitMonitor")
    , mShutdownBit(false)
  {}

  virtual MediaDecoder* Clone() {
    if (!IsOggEnabled()) {
      return nullptr;
    }
    return new OggDecoder();
  }
  virtual MediaDecoderStateMachine* CreateStateMachine();

  
  
  
  
  bool IsOggDecoderShutdown() override
  {
    MonitorAutoLock lock(mShutdownBitMonitor);
    return mShutdownBit;
  }

protected:
  void ShutdownBitChanged() override
  {
    MonitorAutoLock lock(mShutdownBitMonitor);
    mShutdownBit = mStateMachineIsShutdown;
  }

  Monitor mShutdownBitMonitor;
  bool mShutdownBit;
};

} 

#endif
