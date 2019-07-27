




#ifndef GreekCasing_h_
#define GreekCasing_h_

#include <stdint.h>
#include "mozilla/Attributes.h"

namespace mozilla {

class GreekCasing {
  
  
  
  
  
private:
  enum GreekStates {
    kStart,
    kAlpha,
    kEpsilon,
    kEta,
    kIota,
    kOmicron,
    kUpsilon,
    kOmega,
    kAlphaAcc,
    kEpsilonAcc,
    kEtaAcc,
    kIotaAcc,
    kOmicronAcc,
    kUpsilonAcc,
    kOmegaAcc,
    kOmicronUpsilon,
    kDiaeresis
  };

public:
  class State {
  public:
    State()
      : mState(kStart)
    {
    }

    MOZ_IMPLICIT State(const GreekStates& aState)
      : mState(aState)
    {
    }

    void Reset()
    {
      mState = kStart;
    }

    operator GreekStates() const
    {
      return mState;
    }

  private:
    GreekStates mState;
  };

  static uint32_t UpperCase(uint32_t aCh, State& aState);
};

} 

#endif
