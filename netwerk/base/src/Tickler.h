




#ifndef mozilla_net_Tickler_h
#define mozilla_net_Tickler_h






















#if defined(ANDROID) && !defined(MOZ_B2G)
#define MOZ_USE_WIFI_TICKLER
#endif

#include "mozilla/Attributes.h"
#include "nsISupports.h"
#include <stdint.h>

#ifdef MOZ_USE_WIFI_TICKLER
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/WeakPtr.h"
#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "nsIThread.h"
#include "nsITimer.h"
#include "prio.h"

class nsIPrefBranch;
#endif

namespace mozilla {
namespace net {

#ifdef MOZ_USE_WIFI_TICKLER

class Tickler MOZ_FINAL : public SupportsWeakPtr<Tickler>
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Tickler)

  
  Tickler();
  ~Tickler();
  void Cancel();
  nsresult Init();
  void SetIPV4Address(uint32_t address);
  void SetIPV4Port(uint16_t port);

  
  
  void Tickle();

private:
  friend class TicklerTimer;
  Mutex mLock;
  nsCOMPtr<nsIThread> mThread;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIPrefBranch> mPrefs;

  bool mActive;
  bool mCanceled;
  bool mEnabled;
  uint32_t mDelay;
  TimeDuration mDuration;
  PRFileDesc* mFD;

  TimeStamp mLastTickle;
  PRNetAddr mAddr;

  
  void PostCheckTickler();
  void MaybeStartTickler();
  void MaybeStartTicklerUnlocked();

  
  void CheckTickler();
  void StartTickler();
  void StopTickler();
};

#else 

class Tickler MOZ_FINAL
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Tickler)

  Tickler() { }
  ~Tickler() { }
  nsresult Init() { return NS_ERROR_NOT_IMPLEMENTED; }
  void Cancel() { }
  void SetIPV4Address(uint32_t) { };
  void SetIPV4Port(uint16_t) { }
  void Tickle() { }
};

#endif 

} 
} 

#endif 
