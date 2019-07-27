






#ifndef nsFrameState_h_
#define nsFrameState_h_

#include <stdint.h>

#ifdef DEBUG
#include "nsString.h"

class nsIFrame;
#endif

typedef uint64_t nsFrameState_size_t;

#define NS_FRAME_STATE_BIT(n_) (nsFrameState(nsFrameState_size_t(1) << (n_)))

#if (_MSC_VER == 1600)














typedef nsFrameState_size_t nsFrameState;

#define FRAME_STATE_BIT(group_, value_, name_) \
const nsFrameState name_ = NS_FRAME_STATE_BIT(value_);
#include "nsFrameStateBits.h"
#undef FRAME_STATE_BIT

#else

enum nsFrameState : nsFrameState_size_t {
#define FRAME_STATE_BIT(group_, value_, name_) \
  name_ = NS_FRAME_STATE_BIT(value_),
#include "nsFrameStateBits.h"
#undef FRAME_STATE_BIT
};

inline nsFrameState operator|(nsFrameState aLeft, nsFrameState aRight)
{
  return nsFrameState(nsFrameState_size_t(aLeft) | nsFrameState_size_t(aRight));
}

inline nsFrameState operator&(nsFrameState aLeft, nsFrameState aRight)
{
  return nsFrameState(nsFrameState_size_t(aLeft) & nsFrameState_size_t(aRight));
}

inline nsFrameState& operator|=(nsFrameState& aLeft, nsFrameState aRight)
{
  aLeft = aLeft | aRight;
  return aLeft;
}

inline nsFrameState& operator&=(nsFrameState& aLeft, nsFrameState aRight)
{
  aLeft = aLeft & aRight;
  return aLeft;
}

inline nsFrameState operator~(nsFrameState aRight)
{
  return nsFrameState(~nsFrameState_size_t(aRight));
}

inline nsFrameState operator^(nsFrameState aLeft, nsFrameState aRight)
{
  return nsFrameState(nsFrameState_size_t(aLeft) ^ nsFrameState_size_t(aRight));
}

inline nsFrameState& operator^=(nsFrameState& aLeft, nsFrameState aRight)
{
  aLeft = aLeft ^ aRight;
  return aLeft;
}

#endif


#define NS_FRAME_IMPL_RESERVED                      nsFrameState(0xF0000000FFF00000)
#define NS_FRAME_RESERVED                           ~NS_FRAME_IMPL_RESERVED

namespace mozilla {
#ifdef DEBUG
nsCString GetFrameState(nsIFrame* aFrame);
void PrintFrameState(nsIFrame* aFrame);
#endif
} 

#endif  
