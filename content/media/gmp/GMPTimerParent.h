




#ifndef GMPTimerParent_h_
#define GMPTimerParent_h_

#include "mozilla/gmp/PGMPTimerParent.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"
#include "mozilla/Monitor.h"
#include "nsIThread.h"

namespace mozilla {
namespace gmp {

class GMPTimerParent : public PGMPTimerParent {
public:
  NS_INLINE_DECL_REFCOUNTING(GMPTimerParent)
  explicit GMPTimerParent(nsIThread* aGMPThread);

  void Shutdown();

protected:
  virtual bool RecvSetTimer(const uint32_t& aTimerId,
                            const uint32_t& aTimeoutMs) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  ~GMPTimerParent() {}

  static void GMPTimerExpired(nsITimer *aTimer, void *aClosure);

  struct Context {
    Context() {
      MOZ_COUNT_CTOR(Context);
    }
    ~Context() {
      MOZ_COUNT_DTOR(Context);
    }
    nsCOMPtr<nsITimer> mTimer;
    nsRefPtr<GMPTimerParent> mParent; 
    uint32_t mId;
  };

  static PLDHashOperator
  CancelTimers(nsPtrHashKey<Context>* aContext, void* aClosure);

  void TimerExpired(Context* aContext);

  nsTHashtable<nsPtrHashKey<Context>> mTimers;

  nsCOMPtr<nsIThread> mGMPThread;

  bool mIsOpen;
};

} 
} 

#endif 
