




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
#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "nsIThread.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "prio.h"

class nsIPrefBranch;
#endif

namespace mozilla {
namespace net {

#ifdef MOZ_USE_WIFI_TICKLER


#define NS_TICKLER_IID \
{ 0x8f769ed6, 0x207c, 0x4af9, \
  { 0x9f, 0x7e, 0x9e, 0x83, 0x2d, 0xa3, 0x75, 0x4e } }

class Tickler MOZ_FINAL : public nsSupportsWeakReference
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_TICKLER_IID)

  
  Tickler();
  void Cancel();
  nsresult Init();
  void SetIPV4Address(uint32_t address);
  void SetIPV4Port(uint16_t port);

  
  
  void Tickle();

private:
  ~Tickler();

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

NS_DEFINE_STATIC_IID_ACCESSOR(Tickler, NS_TICKLER_IID)

#else 

class Tickler MOZ_FINAL : public nsISupports
{
  ~Tickler() { }
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  Tickler() { }
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
