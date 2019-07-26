





#ifndef mozilla_plugins_PluginHangUIParent_h
#define mozilla_plugins_PluginHangUIParent_h

#include "nsString.h"

#include "base/process.h"
#include "base/process_util.h"

#include "mozilla/Mutex.h"
#include "mozilla/plugins/PluginMessageUtils.h"

#include "MiniShmParent.h"

namespace mozilla {
namespace plugins {

class PluginModuleParent;











class PluginHangUIParent : public MiniShmObserver
{
public:
  PluginHangUIParent(PluginModuleParent* aModule,
                     const int32_t aHangUITimeoutPref,
                     const int32_t aChildTimeoutPref);
  virtual ~PluginHangUIParent();

  







  bool
  Init(const nsString& aPluginName);

  






  bool
  Cancel();

  




  bool
  IsShowing() const { return mIsShowing; }

  





  bool
  WasShown() const { return mIsShowing || mLastUserResponse != 0; }

  




  bool
  DontShowAgain() const;

  






  bool
  WasLastHangStopped() const;

  



  unsigned int
  LastUserResponse() const { return mLastUserResponse; }

  




  unsigned int
  LastShowDurationMs() const;

  virtual void
  OnMiniShmEvent(MiniShmBase* aMiniShmObj) MOZ_OVERRIDE;

  virtual void
  OnMiniShmConnect(MiniShmBase* aMiniShmObj) MOZ_OVERRIDE;

private:
  nsresult
  GetHangUIOwnerWindowHandle(NativeWindowHandle& windowHandle);

  bool
  SendCancel();

  bool
  RecvUserResponse(const unsigned int& aResponse);

  bool
  UnwatchHangUIChildProcess(bool aWait);

  static
  VOID CALLBACK SOnHangUIProcessExit(PVOID aContext, BOOLEAN aIsTimer);

private:
  Mutex mMutex;
  PluginModuleParent* mModule;
  const uint32_t mTimeoutPrefMs;
  const uint32_t mIPCTimeoutMs;
  MessageLoop* mMainThreadMessageLoop;
  bool mIsShowing;
  unsigned int mLastUserResponse;
  base::ProcessHandle mHangUIProcessHandle;
  NativeWindowHandle mMainWindowHandle;
  HANDLE mRegWait;
  HANDLE mShowEvent;
  DWORD mShowTicks;
  DWORD mResponseTicks;
  MiniShmParent mMiniShm;

  DISALLOW_COPY_AND_ASSIGN(PluginHangUIParent);
};

} 
} 

#endif 

