




#pragma once

#include "mozwrlbase.h"

#include <windows.system.h>
#include <windows.ui.core.h>
#include <Windows.ApplicationModel.core.h>
#include <Windows.ApplicationModel.h>
#include <Windows.Applicationmodel.Activation.h>

class MetroWidget;

namespace mozilla {
namespace widget {
namespace winrt {

class FrameworkView;

class MetroApp : public Microsoft::WRL::RuntimeClass<ABI::Windows::ApplicationModel::Core::IFrameworkViewSource>
{
  InspectableClass(L"MetroApp", TrustLevel::BaseTrust)

  typedef ABI::Windows::UI::Core::CoreDispatcherPriority CoreDispatcherPriority;
  typedef ABI::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs LaunchActivatedEventArgs;
  typedef ABI::Windows::ApplicationModel::ISuspendingEventArgs ISuspendingEventArgs;
  typedef ABI::Windows::ApplicationModel::Core::IFrameworkView IFrameworkView;
  typedef ABI::Windows::ApplicationModel::Core::ICoreApplication ICoreApplication;

public:
  
  STDMETHODIMP CreateView(IFrameworkView **viewProvider);

  
  HRESULT OnSuspending(IInspectable* aSender, ISuspendingEventArgs* aArgs);
  HRESULT OnResuming(IInspectable* aSender, IInspectable* aArgs);

  
  HRESULT OnAsyncTileCreated(ABI::Windows::Foundation::IAsyncOperation<bool>* aOperation, AsyncStatus aStatus);

  void Initialize();
  void CoreExit();

  void ShutdownXPCOM();

  
  static void SetBaseWidget(MetroWidget* aPtr);
  static void PostSuspendResumeProcessNotification(bool aIsSuspend);
  static void PostSleepWakeNotification(bool aIsSuspend);

private:
  EventRegistrationToken mSuspendEvent;
  EventRegistrationToken mResumeEvent;
};

} } }
