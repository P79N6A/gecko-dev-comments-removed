




#include "MetroApp.h"
#include "MetroWidget.h"
#include "mozilla/widget/AudioSession.h"
#include "nsIRunnable.h"
#include "MetroUtils.h"
#include "MetroAppShell.h"
#include "nsICommandLineRunner.h"
#include "FrameworkView.h"
#include "nsAppDirectoryServiceDefs.h"
#include <shellapi.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::System;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;



extern nsresult XRE_metroStartup(bool runXREMain);
extern void XRE_metroShutdown();

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif

namespace mozilla {
namespace widget {
namespace winrt {

ComPtr<FrameworkView> sFrameworkView;
ComPtr<MetroApp> sMetroApp;
ComPtr<ICoreApplication> sCoreApp;





HRESULT
MetroApp::CreateView(ABI::Windows::ApplicationModel::Core::IFrameworkView **aViewProvider)
{
  
  
  

  LogFunction();

  *aViewProvider = sFrameworkView.Get();
  return !sFrameworkView ? E_FAIL : S_OK;
}





void
MetroApp::Initialize()
{
  HRESULT hr;
  LogThread();

  static bool xpcomInit;
  if (!xpcomInit) {
    xpcomInit = true;
    Log(L"XPCOM startup initialization began");
    nsresult rv = XRE_metroStartup(true);
    Log(L"XPCOM startup initialization complete");
    if (NS_FAILED(rv)) {
      Log(L"XPCOM startup initialization failed, bailing. rv=%X", rv);
      CoreExit();
      return;
    }
  }

  sFrameworkView->SetupContracts();

  hr = sCoreApp->add_Suspending(Callback<__FIEventHandler_1_Windows__CApplicationModel__CSuspendingEventArgs_t>(
    this, &MetroApp::OnSuspending).Get(), &mSuspendEvent);
  AssertHRESULT(hr);

  hr = sCoreApp->add_Resuming(Callback<__FIEventHandler_1_IInspectable_t>(
    this, &MetroApp::OnResuming).Get(), &mResumeEvent);
  AssertHRESULT(hr);

  mozilla::widget::StartAudioSession();
}



void
MetroApp::ShutdownXPCOM()
{
  LogThread();

  mozilla::widget::StopAudioSession();

  sCoreApp->remove_Suspending(mSuspendEvent);
  sCoreApp->remove_Resuming(mResumeEvent);

  MetroApp::GetView()->ShutdownXPCOM();

  
  XRE_metroShutdown();
}


void
MetroApp::CoreExit()
{
  HRESULT hr;
  ComPtr<ICoreApplicationExit> coreExit;
  HStringReference className(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
  hr = GetActivationFactory(className.Get(), coreExit.GetAddressOf());
  NS_ASSERTION(SUCCEEDED(hr), "Activation of ICoreApplicationExit");
  if (SUCCEEDED(hr)) {
    coreExit->Exit();
  }
}


FrameworkView*
MetroApp::GetView()
{
  NS_ASSERTION(sFrameworkView, "view has not been created.");
  return sFrameworkView.Get();
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
  Log(L"Async operation status: %d", aStatus);
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

#ifdef PR_LOGGING
  if (!gWindowsLog) {
    gWindowsLog = PR_NewLogModule("nsWindow");
  }
#endif

  sMetroApp = Make<MetroApp>();

  HStringReference className(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
  hr = GetActivationFactory(className.Get(), sCoreApp.GetAddressOf());
  if (FAILED(hr)) {
    LogHRESULT(hr);
    return false;
  }

  sFrameworkView = Make<FrameworkView>(sMetroApp.Get());

  
  
  
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

  hr = sCoreApp->Run(sMetroApp.Get());

  Log(L"Exiting CoreApplication::Run");

  sFrameworkView = nullptr;
  sCoreApp = nullptr;
  sMetroApp = nullptr;

  return true;
}

