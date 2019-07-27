




#ifndef GMPTimerChild_h_
#define GMPTimerChild_h_

#include "mozilla/gmp/PGMPTimerChild.h"
#include "mozilla/Monitor.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "gmp-errors.h"
#include "gmp-platform.h"

namespace mozilla {
namespace gmp {

class GMPChild;

class GMPTimerChild : public PGMPTimerChild
{
public:
  NS_INLINE_DECL_REFCOUNTING(GMPTimerChild)

  explicit GMPTimerChild(GMPChild* aPlugin);

  GMPErr SetTimer(GMPTask* aTask, int64_t aTimeoutMS);

protected:
  
  virtual bool RecvTimerExpired(const uint32_t& aTimerId) MOZ_OVERRIDE;

private:
  ~GMPTimerChild();

  nsDataHashtable<nsUint32HashKey, GMPTask*> mTimers;
  uint32_t mTimerCount;

  GMPChild* mPlugin;
};

} 
} 

#endif 
