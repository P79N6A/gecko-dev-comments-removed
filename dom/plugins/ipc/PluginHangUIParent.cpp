





#include "PluginHangUI.h"

#include "PluginHangUIParent.h"

#include "mozilla/Telemetry.h"
#include "mozilla/plugins/PluginModuleParent.h"

#include "nsContentUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsIProperties.h"
#include "nsIWindowMediator.h"
#include "nsIWinTaskbar.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

#include "WidgetUtils.h"

#define NS_TASKBAR_CONTRACTID "@mozilla.org/windows-taskbar;1"

using base::ProcessHandle;

using mozilla::widget::WidgetUtils;

using std::string;
using std::vector;

namespace {
class nsPluginHangUITelemetry : public nsRunnable
{
public:
  nsPluginHangUITelemetry(int aResponseCode, int aDontAskCode,
                          uint32_t aResponseTimeMs, uint32_t aTimeoutMs)
    : mResponseCode(aResponseCode),
      mDontAskCode(aDontAskCode),
      mResponseTimeMs(aResponseTimeMs),
      mTimeoutMs(aTimeoutMs)
  {
  }

  NS_IMETHOD
  Run()
  {
    mozilla::Telemetry::Accumulate(
              mozilla::Telemetry::PLUGIN_HANG_UI_USER_RESPONSE, mResponseCode);
    mozilla::Telemetry::Accumulate(
              mozilla::Telemetry::PLUGIN_HANG_UI_DONT_ASK, mDontAskCode);
    mozilla::Telemetry::Accumulate(
              mozilla::Telemetry::PLUGIN_HANG_UI_RESPONSE_TIME, mResponseTimeMs);
    mozilla::Telemetry::Accumulate(
              mozilla::Telemetry::PLUGIN_HANG_TIME, mTimeoutMs + mResponseTimeMs);
    return NS_OK;
  }

private:
  int mResponseCode;
  int mDontAskCode;
  uint32_t mResponseTimeMs;
  uint32_t mTimeoutMs;
};
} 

namespace mozilla {
namespace plugins {

PluginHangUIParent::PluginHangUIParent(PluginModuleParent* aModule,
                                       const int32_t aHangUITimeoutPref,
                                       const int32_t aChildTimeoutPref)
  : mMutex("mozilla::plugins::PluginHangUIParent::mMutex"),
    mModule(aModule),
    mTimeoutPrefMs(static_cast<uint32_t>(aHangUITimeoutPref) * 1000U),
    mIPCTimeoutMs(static_cast<uint32_t>(aChildTimeoutPref) * 1000U),
    mMainThreadMessageLoop(MessageLoop::current()),
    mIsShowing(false),
    mLastUserResponse(0),
    mHangUIProcessHandle(nullptr),
    mMainWindowHandle(nullptr),
    mRegWait(nullptr),
    mShowEvent(nullptr),
    mShowTicks(0),
    mResponseTicks(0)
{
}

PluginHangUIParent::~PluginHangUIParent()
{
  { 
    MutexAutoLock lock(mMutex);
    UnwatchHangUIChildProcess(true);
  }
  if (mShowEvent) {
    ::CloseHandle(mShowEvent);
  }
  if (mHangUIProcessHandle) {
    ::CloseHandle(mHangUIProcessHandle);
  }
}

bool
PluginHangUIParent::DontShowAgain() const
{
  return (mLastUserResponse & HANGUI_USER_RESPONSE_DONT_SHOW_AGAIN);
}

bool
PluginHangUIParent::WasLastHangStopped() const
{
  return (mLastUserResponse & HANGUI_USER_RESPONSE_STOP);
}

unsigned int
PluginHangUIParent::LastShowDurationMs() const
{
  
  if (!mLastUserResponse) {
    return 0;
  }
  return static_cast<unsigned int>(mResponseTicks - mShowTicks);
}

bool
PluginHangUIParent::Init(const nsString& aPluginName)
{
  if (mHangUIProcessHandle) {
    return false;
  }

  nsresult rv;
  rv = mMiniShm.Init(this, ::IsDebuggerPresent() ? INFINITE : mIPCTimeoutMs);
  NS_ENSURE_SUCCESS(rv, false);
  nsCOMPtr<nsIProperties>
    directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
  if (!directoryService) {
    return false;
  }
  nsCOMPtr<nsIFile> greDir;
  rv = directoryService->Get(NS_GRE_DIR,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(greDir));
  if (NS_FAILED(rv)) {
    return false;
  }
  nsAutoString path;
  greDir->GetPath(path);

  FilePath exePath(path.get());
  exePath = exePath.AppendASCII(MOZ_HANGUI_PROCESS_NAME);
  CommandLine commandLine(exePath.value());

  nsXPIDLString localizedStr;
  const char16_t* formatParams[] = { aPluginName.get() };
  rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                             "PluginHangUIMessage",
                                             formatParams,
                                             localizedStr);
  if (NS_FAILED(rv)) {
    return false;
  }
  commandLine.AppendLooseValue(localizedStr.get());

  const char* keys[] = { "PluginHangUITitle",
                         "PluginHangUIWaitButton",
                         "PluginHangUIStopButton",
                         "DontAskAgain" };
  for (unsigned int i = 0; i < ArrayLength(keys); ++i) {
    rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                            keys[i],
                                            localizedStr);
    if (NS_FAILED(rv)) {
      return false;
    }
    commandLine.AppendLooseValue(localizedStr.get());
  }

  rv = GetHangUIOwnerWindowHandle(mMainWindowHandle);
  if (NS_FAILED(rv)) {
    return false;
  }
  nsAutoString hwndStr;
  hwndStr.AppendPrintf("%p", mMainWindowHandle);
  commandLine.AppendLooseValue(hwndStr.get());

  ScopedHandle procHandle(::OpenProcess(SYNCHRONIZE,
                                        TRUE,
                                        GetCurrentProcessId()));
  if (!procHandle.IsValid()) {
    return false;
  }
  nsAutoString procHandleStr;
  procHandleStr.AppendPrintf("%p", procHandle.Get());
  commandLine.AppendLooseValue(procHandleStr.get());

  
  
  
  nsCOMPtr<nsIWinTaskbar> taskbarInfo = do_GetService(NS_TASKBAR_CONTRACTID);
  if (taskbarInfo) {
    bool isSupported = false;
    taskbarInfo->GetAvailable(&isSupported);
    nsAutoString appId;
    if (isSupported && NS_SUCCEEDED(taskbarInfo->GetDefaultGroupId(appId))) {
      commandLine.AppendLooseValue(appId.get());
    } else {
      commandLine.AppendLooseValue(L"-");
    }
  } else {
    commandLine.AppendLooseValue(L"-");
  }

  nsAutoString ipcTimeoutStr;
  ipcTimeoutStr.AppendInt(mIPCTimeoutMs);
  commandLine.AppendLooseValue(ipcTimeoutStr.get());

  std::wstring ipcCookie;
  rv = mMiniShm.GetCookie(ipcCookie);
  if (NS_FAILED(rv)) {
    return false;
  }
  commandLine.AppendLooseValue(ipcCookie);

  ScopedHandle showEvent(::CreateEventW(nullptr, FALSE, FALSE, nullptr));
  if (!showEvent.IsValid()) {
    return false;
  }
  mShowEvent = showEvent.Get();

  MutexAutoLock lock(mMutex);
  STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };
  PROCESS_INFORMATION processInfo = { nullptr };
  BOOL isProcessCreated = ::CreateProcess(exePath.value().c_str(),
                                          const_cast<wchar_t*>(commandLine.command_line_string().c_str()),
                                          nullptr,
                                          nullptr,
                                          TRUE,
                                          DETACHED_PROCESS,
                                          nullptr,
                                          nullptr,
                                          &startupInfo,
                                          &processInfo);
  if (isProcessCreated) {
    ::CloseHandle(processInfo.hThread);
    mHangUIProcessHandle = processInfo.hProcess;
    ::RegisterWaitForSingleObject(&mRegWait,
                                  processInfo.hProcess,
                                  &SOnHangUIProcessExit,
                                  this,
                                  INFINITE,
                                  WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);
    ::WaitForSingleObject(mShowEvent, ::IsDebuggerPresent() ? INFINITE
                                                            : mIPCTimeoutMs);
    
    
    
    
    
    mIsShowing = true;
  }
  mShowEvent = nullptr;
  return !(!isProcessCreated);
}


VOID CALLBACK PluginHangUIParent::SOnHangUIProcessExit(PVOID aContext,
                                                       BOOLEAN aIsTimer)
{
  PluginHangUIParent* object = static_cast<PluginHangUIParent*>(aContext);
  MutexAutoLock lock(object->mMutex);
  
  if (object->IsShowing()) {
    object->RecvUserResponse(HANGUI_USER_RESPONSE_CANCEL);
    
    
    ::EnableWindow(object->mMainWindowHandle, TRUE);
  }
}


bool
PluginHangUIParent::UnwatchHangUIChildProcess(bool aWait)
{
  mMutex.AssertCurrentThreadOwns();
  if (mRegWait) {
    
    
    ScopedHandle completionEvent;
    if (aWait) {
      completionEvent.Set(::CreateEventW(nullptr, FALSE, FALSE, nullptr));
      if (!completionEvent.IsValid()) {
        return false;
      }
    }

    
    
    
    if (::UnregisterWaitEx(mRegWait, completionEvent) ||
        !aWait && ::GetLastError() == ERROR_IO_PENDING) {
      mRegWait = nullptr;
      if (aWait) {
        
        
        MutexAutoUnlock unlock(mMutex);
        ::WaitForSingleObject(completionEvent, INFINITE);
      }
      return true;
    }
  }
  return false;
}

bool
PluginHangUIParent::Cancel()
{
  MutexAutoLock lock(mMutex);
  bool result = mIsShowing && SendCancel();
  if (result) {
    mIsShowing = false;
  }
  return result;
}

bool
PluginHangUIParent::SendCancel()
{
  PluginHangUICommand* cmd = nullptr;
  nsresult rv = mMiniShm.GetWritePtr(cmd);
  if (NS_FAILED(rv)) {
    return false;
  }
  cmd->mCode = PluginHangUICommand::HANGUI_CMD_CANCEL;
  return NS_SUCCEEDED(mMiniShm.Send());
}


bool
PluginHangUIParent::RecvUserResponse(const unsigned int& aResponse)
{
  mMutex.AssertCurrentThreadOwns();
  if (!mIsShowing && !(aResponse & HANGUI_USER_RESPONSE_CANCEL)) {
    
    return true;
  }
  mLastUserResponse = aResponse;
  mResponseTicks = ::GetTickCount();
  mIsShowing = false;
  
  int responseCode;
  if (aResponse & HANGUI_USER_RESPONSE_STOP) {
    
    mModule->TerminateChildProcess(mMainThreadMessageLoop);
    responseCode = 1;
  } else if(aResponse & HANGUI_USER_RESPONSE_CONTINUE) {
    
    responseCode = 2;
  } else {
    
    responseCode = 3;
  }
  int dontAskCode = (aResponse & HANGUI_USER_RESPONSE_DONT_SHOW_AGAIN) ? 1 : 0;
  nsCOMPtr<nsIRunnable> workItem = new nsPluginHangUITelemetry(responseCode,
                                                               dontAskCode,
                                                               LastShowDurationMs(),
                                                               mTimeoutPrefMs);
  NS_DispatchToMainThread(workItem);
  return true;
}

nsresult
PluginHangUIParent::GetHangUIOwnerWindowHandle(NativeWindowHandle& windowHandle)
{
  windowHandle = nullptr;

  nsresult rv;
  nsCOMPtr<nsIWindowMediator> winMediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID,
                                                        &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> navWin;
  rv = winMediator->GetMostRecentWindow(MOZ_UTF16("navigator:browser"),
                                        getter_AddRefs(navWin));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!navWin) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWidget> widget = WidgetUtils::DOMWindowToWidget(navWin);
  if (!widget) {
    return NS_ERROR_FAILURE;
  }

  windowHandle = reinterpret_cast<NativeWindowHandle>(widget->GetNativeData(NS_NATIVE_WINDOW));
  if (!windowHandle) {
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

void
PluginHangUIParent::OnMiniShmEvent(MiniShmBase *aMiniShmObj)
{
  const PluginHangUIResponse* response = nullptr;
  nsresult rv = aMiniShmObj->GetReadPtr(response);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "Couldn't obtain read pointer OnMiniShmEvent");
  if (NS_SUCCEEDED(rv)) {
    
    
    MutexAutoLock lock(mMutex);
    UnwatchHangUIChildProcess(false);
    RecvUserResponse(response->mResponseBits);
  }
}

void
PluginHangUIParent::OnMiniShmConnect(MiniShmBase* aMiniShmObj)
{
  PluginHangUICommand* cmd = nullptr;
  nsresult rv = aMiniShmObj->GetWritePtr(cmd);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "Couldn't obtain write pointer OnMiniShmConnect");
  if (NS_FAILED(rv)) {
    return;
  }
  cmd->mCode = PluginHangUICommand::HANGUI_CMD_SHOW;
  if (NS_SUCCEEDED(aMiniShmObj->Send())) {
    mShowTicks = ::GetTickCount();
  }
  ::SetEvent(mShowEvent);
}

} 
} 
