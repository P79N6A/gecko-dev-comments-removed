




#include "MetroApp.h"
#include "MetroWidget.h"
#include "mozilla/widget/AudioSession.h"
#include "nsIRunnable.h"
#include "MetroUtils.h"
#include "MetroAppShell.h"
#include "nsICommandLineRunner.h"
#include "FrameworkView.h"
#include "nsAppDirectoryServiceDefs.h"
#include "GeckoProfiler.h"
#include <shellapi.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::System;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace mozilla::widget;



extern nsresult XRE_metroStartup(bool runXREMain);
extern void XRE_metroShutdown();

static const char* gGeckoThreadName = "GeckoMain";

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif

namespace mozilla {
namespace widget {
namespace winrt {

ComPtr<FrameworkView> sFrameworkView;
ComPtr<MetroApp> sMetroApp;
ComPtr<ICoreApplication> sCoreApp;
bool MetroApp::sGeckoShuttingDown = false;





HRESULT
MetroApp::CreateView(ABI::Windows::ApplicationModel::Core::IFrameworkView **aViewProvider)
{
  
  
  

  LogFunction();

  sFrameworkView.Get()->AddRef();
  *aViewProvider = sFrameworkView.Get();
  return !sFrameworkView ? E_FAIL : S_OK;
}




void
MetroApp::Run()
{
  LogThread();

  
  
  char aLocal;
  PR_SetCurrentThreadName(gGeckoThreadName);
  profiler_register_thread(gGeckoThreadName, &aLocal);

  HRESULT hr;
  hr = sCoreApp->add_Suspending(Callback<__FIEventHandler_1_Windows__CApplicationModel__CSuspendingEventArgs_t>(
    this, &MetroApp::OnSuspending).Get(), &mSuspendEvent);
  AssertHRESULT(hr);

  hr = sCoreApp->add_Resuming(Callback<__FIEventHandler_1_IInspectable_t>(
    this, &MetroApp::OnResuming).Get(), &mResumeEvent);
  AssertHRESULT(hr);

  WinUtils::Log("XPCOM startup initialization began");
  nsresult rv = XRE_metroStartup(true);
  WinUtils::Log("XPCOM startup initialization complete");
  if (NS_FAILED(rv)) {
    WinUtils::Log("XPCOM startup initialization failed, bailing. rv=%X", rv);
    CoreExit();
  }
}



void
MetroApp::ShutdownXPCOM()
{
  LogThread();

  if (sCoreApp) {
    sCoreApp->remove_Suspending(mSuspendEvent);
    sCoreApp->remove_Resuming(mResumeEvent);
  }

  if (sFrameworkView) {
    sFrameworkView->ShutdownXPCOM();
  }

  MetroApp::sGeckoShuttingDown = true;

  
  XRE_metroShutdown();

  
  profiler_unregister_thread();
}


void
MetroApp::CoreExit()
{
  LogFunction();
  HRESULT hr;
  ComPtr<ICoreApplicationExit> coreExit;
  HStringReference className(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
  hr = GetActivationFactory(className.Get(), coreExit.GetAddressOf());
  NS_ASSERTION(SUCCEEDED(hr), "Activation of ICoreApplicationExit");
  if (SUCCEEDED(hr)) {
    coreExit->Exit();
  }
}




HRESULT
MetroApp::OnSuspending(IInspectable* aSender, ISuspendingEventArgs* aArgs)
{
  LogThread();
  PostSuspendResumeProcessNotification(true);
  return S_OK;
}

HRESULT
MetroApp::OnResuming(IInspectable* aSender, IInspectable* aArgs)
{
  LogThread();
  PostSuspendResumeProcessNotification(false);
  return S_OK;
}

HRESULT
MetroApp::OnAsyncTileCreated(ABI::Windows::Foundation::IAsyncOperation<bool>* aOperation,
                             AsyncStatus aStatus)
{
  WinUtils::Log("Async operation status: %d", aStatus);
  return S_OK;
}


void
MetroApp::SetBaseWidget(MetroWidget* aPtr)
{
  LogThread();

  NS_ASSERTION(aPtr, "setting null base widget?");

  
  aPtr->SetView(sFrameworkView.Get());
  sFrameworkView->SetWidget(aPtr);
}


void
MetroApp::PostSuspendResumeProcessNotification(const bool aIsSuspend)
{
  static bool isSuspend = false;
  if (isSuspend == aIsSuspend) {
    return;
  }
  isSuspend = aIsSuspend;
  MetroUtils::FireObserver(aIsSuspend ? "suspend_process_notification" :
                                        "resume_process_notification");
}


void
MetroApp::PostSleepWakeNotification(const bool aIsSleep)
{
  static bool isSleep = false;
  if (isSleep == aIsSleep) {
    return;
  }
  isSleep = aIsSleep;
  MetroUtils::FireObserver(aIsSleep ? "sleep_notification" :
                                      "wake_notification");
}

} } }


static bool
IsBackgroundSessionClosedStartup()
{
  int argc;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  bool backgroundSessionClosed = argc > 1 && !wcsicmp(argv[1], L"-BackgroundSessionClosed");
  LocalFree(argv);
  return backgroundSessionClosed;
}

bool
XRE_MetroCoreApplicationRun()
{
  HRESULT hr;
  LogThread();

  using namespace mozilla::widget::winrt;

  sMetroApp = Make<MetroApp>();

  HStringReference className(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
  hr = GetActivationFactory(className.Get(), sCoreApp.GetAddressOf());
  if (FAILED(hr)) {
    LogHRESULT(hr);
    return false;
  }

  
  
  
  if (IsBackgroundSessionClosedStartup() && SUCCEEDED(XRE_metroStartup(false))) {

    
    
    nsCOMPtr<nsIFile> sessionBAK;
    if (NS_FAILED(NS_GetSpecialDirectory("ProfDS", getter_AddRefs(sessionBAK)))) {
      return false;
    }

    sessionBAK->AppendNative(nsDependentCString("sessionstore.bak"));
    bool exists;
    if (NS_SUCCEEDED(sessionBAK->Exists(&exists)) && exists) {
      sessionBAK->Remove(false);
    }
    return false;
  }

  sFrameworkView = Make<FrameworkView>(sMetroApp.Get());
  hr = sCoreApp->Run(sMetroApp.Get());
  sFrameworkView = nullptr;

  WinUtils::Log("Exiting CoreApplication::Run");

  sCoreApp = nullptr;
  sMetroApp = nullptr;

  return true;
}

