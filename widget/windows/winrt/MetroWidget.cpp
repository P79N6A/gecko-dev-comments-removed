




#include "LayerManagerD3D10.h"
#include "MetroWidget.h"
#include "MetroApp.h"
#include "mozilla/Preferences.h"
#include "nsToolkit.h"
#include "KeyboardLayout.h"
#include "MetroUtils.h"
#include "WinUtils.h"
#include "nsToolkitCompsCID.h"
#include "nsIAppStartup.h"
#include "../resource.h"
#include "nsIWidgetListener.h"
#include "nsPrintfCString.h"
#include "nsWindowDefs.h"
#include "FrameworkView.h"
#include "nsTextStore.h"
#include "Layers.h"
#include "BasicLayers.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

using namespace mozilla;
using namespace mozilla::widget;
using namespace mozilla::layers;
using namespace mozilla::widget::winrt;

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::UI::Input;
using namespace ABI::Windows::Devices::Input;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::System;
using namespace ABI::Windows::Foundation;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif

static uint32_t gInstanceCount = 0;
const PRUnichar* kMetroSubclassThisProp = L"MetroSubclassThisProp";
static const UINT sDefaultBrowserMsgID = RegisterWindowMessageW(L"DefaultBrowserClosing");

namespace {

  void SendInputs(uint32_t aModifiers, INPUT* aExtraInputs, uint32_t aExtraInputsLen)
  {
    
    
    nsAutoTArray<KeyPair,32> keySequence;
    for (uint32_t i = 0; i < ArrayLength(sModifierKeyMap); ++i) {
      const uint32_t* map = sModifierKeyMap[i];
      if (aModifiers & map[0]) {
        keySequence.AppendElement(KeyPair(map[1], map[2]));
      }
    }

    uint32_t const len = keySequence.Length() * 2 + aExtraInputsLen;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    INPUT* inputs = new INPUT[len];
    memset(inputs, 0, len * sizeof(INPUT));
    for (uint32_t i = 0; i < keySequence.Length(); ++i) {
      inputs[i].type = inputs[len-i-1].type = INPUT_KEYBOARD;
      inputs[i].ki.wVk = inputs[len-i-1].ki.wVk = keySequence[i].mSpecific
                                                ? keySequence[i].mSpecific
                                                : keySequence[i].mGeneral;
      inputs[len-i-1].ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    for (uint32_t i = 0; i < aExtraInputsLen; i++) {
      inputs[keySequence.Length()+i] = aExtraInputs[i];
    }
    Log(L"  Sending inputs");
    for (uint32_t i = 0; i < len; i++) {
      if (inputs[i].type == INPUT_KEYBOARD) {
        Log(L"    Key press: 0x%x %s",
            inputs[i].ki.wVk,
            inputs[i].ki.dwFlags & KEYEVENTF_KEYUP
            ? L"UP"
            : L"DOWN");
      } else if(inputs[i].type == INPUT_MOUSE) {
        Log(L"    Mouse input: 0x%x 0x%x",
            inputs[i].mi.dwFlags,
            inputs[i].mi.mouseData);
      } else {
        Log(L"    Unknown input type!");
      }
    }
    ::SendInput(len, inputs, sizeof(INPUT));
    delete[] inputs;

    
    
    
    
    Log(L"  Inputs sent. Waiting for input messages to clear");
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
      Log(L"    Dispatched 0x%x 0x%x 0x%x", msg.message, msg.wParam, msg.lParam);
    }
    Log(L"  No more input messages");
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(MetroWidget, nsBaseWidget)

MetroWidget::MetroWidget() :
  mTransparencyMode(eTransparencyOpaque),
  mWnd(NULL),
  mMetroWndProc(NULL),
  mTempBasicLayerInUse(false),
  nsWindowBase()
{
  
  if (!gInstanceCount) {
    UserActivity();
    nsTextStore::Initialize();
  } 
  gInstanceCount++;
}

MetroWidget::~MetroWidget()
{
  LogThis();

  gInstanceCount--;

  
  if (!gInstanceCount) {
    nsTextStore::Terminate();
  } 
}

static bool gTopLevelAssigned = false;
NS_IMETHODIMP
MetroWidget::Create(nsIWidget *aParent,
                    nsNativeWidget aNativeParent,
                    const nsIntRect &aRect,
                    nsDeviceContext *aContext,
                    nsWidgetInitData *aInitData)
{
  LogFunction();

  nsWidgetInitData defaultInitData;
  if (!aInitData)
    aInitData = &defaultInitData;

  mWindowType = aInitData->mWindowType;

  
  nsToolkit::GetToolkit();

  BaseCreate(aParent, aRect, aContext, aInitData);

  if (mWindowType != eWindowType_toplevel) {
    switch(mWindowType) {
      case eWindowType_dialog:
      Log(L"eWindowType_dialog window requested, returning failure.");
      break;
      case eWindowType_child:
      Log(L"eWindowType_child window requested, returning failure.");
      break;
      case eWindowType_popup:
      Log(L"eWindowType_popup window requested, returning failure.");
      break;
      case eWindowType_plugin:
      Log(L"eWindowType_plugin window requested, returning failure.");
      break;
      
      case eWindowType_invisible:
      Log(L"eWindowType_invisible window requested, this doesn't actually exist!");
      return NS_OK;
    }
    NS_WARNING("Invalid window type requested.");
    return NS_ERROR_FAILURE;
  }

  if (gTopLevelAssigned) {
    
    
    NS_WARNING("New eWindowType_toplevel window requested after FrameworkView widget created.");
    NS_WARNING("Widget created but the physical window does not exist! Fix me!");
    return NS_OK;
  }

  
  gTopLevelAssigned = true;
  MetroApp::SetBaseWidget(this);

  if (mWidgetListener) {
    mWidgetListener->WindowActivated();
  }

  return NS_OK;
}

void
MetroWidget::SetView(FrameworkView* aView)
{
  mView = aView;
  
  
  mLayerManager = nullptr;
}

NS_IMETHODIMP
MetroWidget::Destroy()
{
  if (mOnDestroyCalled)
    return NS_OK;
  Log(L"[%X] %s mWnd=%X type=%d", this, __WFUNCTION__, mWnd, mWindowType);
  mOnDestroyCalled = true;
  RemoveSubclass();
  mView = nullptr;
  mIdleService = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::SetParent(nsIWidget *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::Show(bool bState)
{
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::IsVisible(bool & aState)
{
  aState = mView->IsVisible();
  return NS_OK;
}

bool
MetroWidget::IsVisible() const
{
  if (!mView)
    return false;
  return mView->IsVisible();
}

NS_IMETHODIMP
MetroWidget::IsEnabled(bool *aState)
{
  *aState = mView->IsEnabled();
  return NS_OK;
}

bool
MetroWidget::IsEnabled() const
{
  if (!mView)
    return false;
  return mView->IsEnabled();
}

NS_IMETHODIMP
MetroWidget::Enable(bool bState)
{
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::GetBounds(nsIntRect &aRect)
{
  if (mView) {
    mView->GetBounds(aRect);
  } else {
    nsIntRect rect(0,0,0,0);
    aRect = rect;
  }
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::GetScreenBounds(nsIntRect &aRect)
{
  if (mView) {
    mView->GetBounds(aRect);
  } else {
    nsIntRect rect(0,0,0,0);
    aRect = rect;
  }
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::GetClientBounds(nsIntRect &aRect)
{
  if (mView) {
    mView->GetBounds(aRect);
  } else {
    nsIntRect rect(0,0,0,0);
    aRect = rect;
  }
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::SetCursor(nsCursor aCursor)
{
  if (!mView)
    return NS_ERROR_FAILURE;

  switch (aCursor) {
    case eCursor_select:
      mView->SetCursor(CoreCursorType::CoreCursorType_IBeam);
      break;
    case eCursor_wait:
      mView->SetCursor(CoreCursorType::CoreCursorType_Wait);
      break;
    case eCursor_hyperlink:
      mView->SetCursor(CoreCursorType::CoreCursorType_Hand);
      break;
    case eCursor_standard:
      mView->SetCursor(CoreCursorType::CoreCursorType_Arrow);
      break;
    case eCursor_n_resize:
    case eCursor_s_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeNorthSouth);
      break;
    case eCursor_w_resize:
    case eCursor_e_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeWestEast);
      break;
    case eCursor_nw_resize:
    case eCursor_se_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeNorthwestSoutheast);
      break;
    case eCursor_ne_resize:
    case eCursor_sw_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeNortheastSouthwest);
      break;
    case eCursor_crosshair:
      mView->SetCursor(CoreCursorType::CoreCursorType_Cross);
      break;
    case eCursor_move:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeAll);
      break;
    case eCursor_help:
      mView->SetCursor(CoreCursorType::CoreCursorType_Help);
      break;
    
    case eCursor_copy:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_COPY);
      break;
    case eCursor_alias:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_ALIAS);
      break;
    case eCursor_cell:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_CELL);
      break;
    case eCursor_grab:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_GRAB);
      break;
    case eCursor_grabbing:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_GRABBING);
      break;
    case eCursor_spinning:
      mView->SetCursor(CoreCursorType::CoreCursorType_Wait);
      break;
    case eCursor_context_menu:
      mView->SetCursor(CoreCursorType::CoreCursorType_Arrow);
      break;
    case eCursor_zoom_in:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_ZOOMIN);
      break;
    case eCursor_zoom_out:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_ZOOMOUT);
      break;
    case eCursor_not_allowed:
    case eCursor_no_drop:
      mView->SetCursor(CoreCursorType::CoreCursorType_UniversalNo);
      break;
    case eCursor_col_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_COLRESIZE);
      break;
    case eCursor_row_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_ROWRESIZE);
      break;
    case eCursor_vertical_text:
      mView->SetCursor(CoreCursorType::CoreCursorType_Custom, IDC_VERTICALTEXT);
      break;
    case eCursor_all_scroll:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeAll);
      break;
    case eCursor_nesw_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeNortheastSouthwest);
      break;
    case eCursor_nwse_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeNorthwestSoutheast);
      break;
    case eCursor_ns_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeNorthSouth);
      break;
    case eCursor_ew_resize:
      mView->SetCursor(CoreCursorType::CoreCursorType_SizeWestEast);
      break;
    case eCursor_none:
      mView->ClearCursor();
      break;
    default:
      NS_WARNING("Invalid cursor type");
      break;
  }
  return NS_OK;
}

nsresult
MetroWidget::SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                      int32_t aNativeKeyCode,
                                      uint32_t aModifierFlags,
                                      const nsAString& aCharacters,
                                      const nsAString& aUnmodifiedCharacters)
{
  Log(L"ENTERED SynthesizeNativeKeyEvent");

  
  
  NS_ENSURE_ARG_RANGE(aNativeKeyCode, 1, 254);

  
  int32_t const numKeyboardLayouts = GetKeyboardLayoutList(0, NULL);
  if (numKeyboardLayouts == 0) {
    return NS_ERROR_FAILURE;
  }
  HKL* keyboardLayoutList = new HKL[numKeyboardLayouts];
  GetKeyboardLayoutList(numKeyboardLayouts, keyboardLayoutList);

  
  HKL const oldKeyboardLayout = ::GetKeyboardLayout(0);
  Log(L"  Current keyboard layout: %08x", oldKeyboardLayout);
  Log(L"  Loading keyboard layout: %08x", aNativeKeyboardLayout);

  
  nsPrintfCString layoutName("%08x", aNativeKeyboardLayout);
  HKL const newKeyboardLayout = ::LoadKeyboardLayoutA(layoutName.get(),
                                                      KLF_REPLACELANG);
  Log(L"  ::LoadKeyboardLayoutA returned %08x", newKeyboardLayout);

  
  
  
  
  
  
  
  
  bool haveLoaded = true;
  bool haveActivated = false;
  bool haveReplaced = false;
  if (GetKeyboardLayoutList(0, NULL) == numKeyboardLayouts) {
    haveReplaced = true;
    for (int32_t i = 0; i < numKeyboardLayouts; i++) {
      if (keyboardLayoutList[i] == newKeyboardLayout) {
        Log(L"  %08x found in list of loaded keyboard layouts", newKeyboardLayout);
        haveLoaded = false;
        haveReplaced = false;
        break;
      }
    }
  }

  
  
  if (oldKeyboardLayout != newKeyboardLayout) {
    Log(L"  %08x != %08x", oldKeyboardLayout, newKeyboardLayout);
    haveActivated = true;
    Log(L"  Activating keyboard layout: %08x", newKeyboardLayout);
    HKL ret = ::ActivateKeyboardLayout(newKeyboardLayout, KLF_SETFORPROCESS);
    Log(L"  ::ActivateKeyboardLayout returned %08x", ret);
  }

  INPUT inputs[2];
  memset(&inputs, 0, 2*sizeof(INPUT));
  inputs[0].type = inputs[1].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = inputs[1].ki.wVk = aNativeKeyCode;
  inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;
  SendInputs(aModifierFlags, inputs, 2);

  
  
  
  
  if (haveActivated) {
    
    
    if (haveReplaced) {
      Log(L"  Loading all previous layouts");
      for (int32_t i = 0; i < numKeyboardLayouts; i++) {
        nsPrintfCString layoutName("%08x", keyboardLayoutList[i]);
        HKL ret = ::LoadKeyboardLayoutA(layoutName.get(), KLF_REPLACELANG);
        Log(L"    ::LoadKeyboardLayoutA returned %08x", ret);
      }
    }
    
    
    
    Log(L"  Activating previous layout %08x", oldKeyboardLayout);
    HKL ret = ::ActivateKeyboardLayout(oldKeyboardLayout, KLF_SETFORPROCESS);
    Log(L"  ::ActivateKeyboardLayout returned %08x", ret);
    
    
    if (haveLoaded && !haveReplaced) {
      Log(L"  Unloading keyboard layout %08x", newKeyboardLayout);
      ::UnloadKeyboardLayout(newKeyboardLayout);
    }
  }

  delete[] keyboardLayoutList;

  Log(L"EXITING SynthesizeNativeKeyEvent");
  return NS_OK;
}

nsresult
MetroWidget::SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                        uint32_t aNativeMessage,
                                        uint32_t aModifierFlags)
{
  Log(L"ENTERED SynthesizeNativeMouseEvent");

  INPUT inputs[2];
  memset(inputs, 0, 2*sizeof(INPUT));
  inputs[0].type = inputs[1].type = INPUT_MOUSE;
  inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  
  
  
  
  inputs[0].mi.dx = (aPoint.x * 65535) / ::GetSystemMetrics(SM_CXSCREEN);
  inputs[0].mi.dy = (aPoint.y * 65535) / ::GetSystemMetrics(SM_CYSCREEN);
  inputs[1].mi.dwFlags = aNativeMessage;
  SendInputs(aModifierFlags, inputs, 2);

  Log(L"Exiting SynthesizeNativeMouseEvent");
  return NS_OK;
}

nsresult
MetroWidget::SynthesizeNativeMouseScrollEvent(nsIntPoint aPoint,
                                           uint32_t aNativeMessage,
                                           double aDeltaX,
                                           double aDeltaY,
                                           double aDeltaZ,
                                           uint32_t aModifierFlags,
                                           uint32_t aAdditionalFlags)
{
  Log(L"ENTERED SynthesizeNativeMouseScrollEvent");

  int32_t mouseData = 0;
  if (aNativeMessage == MOUSEEVENTF_WHEEL) {
    mouseData = static_cast<int32_t>(aDeltaY);
    Log(L"  Vertical scroll, delta %d", mouseData);
  } else if (aNativeMessage == MOUSEEVENTF_HWHEEL) {
    mouseData = static_cast<int32_t>(aDeltaX);
    Log(L"  Horizontal scroll, delta %d", mouseData);
  } else {
    Log(L"ERROR Unrecognized scroll event");
    return NS_ERROR_INVALID_ARG;
  }

  INPUT inputs[2];
  memset(inputs, 0, 2*sizeof(INPUT));
  inputs[0].type = inputs[1].type = INPUT_MOUSE;
  inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  
  
  
  
  inputs[0].mi.dx = (aPoint.x * 65535) / ::GetSystemMetrics(SM_CXSCREEN);
  inputs[0].mi.dy = (aPoint.y * 65535) / ::GetSystemMetrics(SM_CYSCREEN);
  inputs[1].mi.dwFlags = aNativeMessage;
  inputs[1].mi.mouseData = mouseData;
  SendInputs(aModifierFlags, inputs, 2);

  Log(L"EXITING SynthesizeNativeMouseScrollEvent");
  return NS_OK;
}

static void
CloseGesture()
{
  Log(L"shuting down due to close gesture.\n");
  nsCOMPtr<nsIAppStartup> appStartup =
    do_GetService(NS_APPSTARTUP_CONTRACTID);
  if (appStartup) {
    appStartup->Quit(nsIAppStartup::eForceQuit);
  }
}


LRESULT CALLBACK
MetroWidget::StaticWindowProcedure(HWND aWnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam)
{
  MetroWidget* self = reinterpret_cast<MetroWidget*>(
    GetProp(aWnd, kMetroSubclassThisProp));
  if (!self) {
    NS_NOTREACHED("Missing 'this' prop on subclassed metro window, this is bad.");
    return 0;
  }
  return self->WindowProcedure(aWnd, aMsg, aWParam, aLParam);
}

LRESULT
MetroWidget::WindowProcedure(HWND aWnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam)
{
  if(sDefaultBrowserMsgID == aMsg) {
    CloseGesture();
  }

  
  
  bool processDefault = true;
  
  LRESULT processResult = 0;

  switch (aMsg) {
    case WM_PAINT:
    {
      HRGN rgn = CreateRectRgn(0, 0, 0, 0);
      GetUpdateRgn(mWnd, rgn, false);
      nsIntRegion region = WinUtils::ConvertHRGNToRegion(rgn);
      DeleteObject(rgn);
      if (region.IsEmpty())
        break;
      mView->Render(region);
      break;
    }

    case WM_POWERBROADCAST:
    {
      switch (aWParam)
      {
        case PBT_APMSUSPEND:
          MetroApp::PostSleepWakeNotification(true);
          break;
        case PBT_APMRESUMEAUTOMATIC:
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
          MetroApp::PostSleepWakeNotification(false);
          break;
      }
      break;
    }

    default:
    {
      if (aWParam == WM_USER_TSF_TEXTCHANGE) {
        nsTextStore::OnTextChangeMsg();
      }
      break;
    }
  }

  if (processDefault) {
    return CallWindowProc(mMetroWndProc, aWnd, aMsg, aWParam,
                          aLParam);
  }
  return processResult;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  WCHAR className[56];
  if (GetClassNameW(hwnd, className, sizeof(className)/sizeof(WCHAR)) &&
      !wcscmp(L"Windows.UI.Core.CoreWindow", className)) {
    DWORD processID = 0;
    GetWindowThreadProcessId(hwnd, &processID);
    if (processID && processID == GetCurrentProcessId()) {
      *((HWND*)lParam) = hwnd;
      return FALSE;
    }
  }
  return TRUE;
}

void
MetroWidget::FindMetroWindow()
{
  LogFunction();
  if (mWnd)
    return;
  EnumWindows(EnumWindowsProc, (LPARAM)&mWnd);
  NS_ASSERTION(mWnd, "Couldn't find our metro CoreWindow, this is bad.");

  
  SetSubclass();
  return;
}

void
MetroWidget::SetSubclass()
{
  if (!mWnd) {
    NS_NOTREACHED("SetSubclass called without a valid hwnd.");
    return;
  }

  WNDPROC wndProc = reinterpret_cast<WNDPROC>(
    GetWindowLongPtr(mWnd, GWLP_WNDPROC));
  if (wndProc != StaticWindowProcedure) {
      if (!SetPropW(mWnd, kMetroSubclassThisProp, this)) {
        NS_NOTREACHED("SetProp failed, can't continue.");
        return;
      }
      mMetroWndProc =
        reinterpret_cast<WNDPROC>(
          SetWindowLongPtr(mWnd, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(StaticWindowProcedure)));
      NS_ASSERTION(mMetroWndProc != StaticWindowProcedure, "WTF?");
  }
}

void
MetroWidget::RemoveSubclass()
{
  if (!mWnd)
    return;
  WNDPROC wndProc = reinterpret_cast<WNDPROC>(
    GetWindowLongPtr(mWnd, GWLP_WNDPROC));
  if (wndProc == StaticWindowProcedure) {
      NS_ASSERTION(mMetroWndProc, "Should have old proc here.");
      SetWindowLongPtr(mWnd, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(mMetroWndProc));
      mMetroWndProc = NULL;
  }
  RemovePropW(mWnd, kMetroSubclassThisProp);
}

bool
MetroWidget::ShouldUseOffMainThreadCompositing()
{
  
  if (!mView) {
    return false;
  }
  
  return (CompositorParent::CompositorLoop() && mWindowType == eWindowType_toplevel);
}

bool
MetroWidget::ShouldUseMainThreadD3D10Manager()
{
  
  if (!mView) {
    return false;
  }
  return (!CompositorParent::CompositorLoop() && mWindowType == eWindowType_toplevel);
}

bool
MetroWidget::ShouldUseBasicManager()
{
  
  return (mWindowType != eWindowType_toplevel);
}

LayerManager*
MetroWidget::GetLayerManager(PLayersChild* aShadowManager,
                             LayersBackend aBackendHint,
                             LayerManagerPersistence aPersistence,
                             bool* aAllowRetaining)
{
  bool retaining = true;

  
  if (mLayerManager &&
      mTempBasicLayerInUse &&
      ShouldUseOffMainThreadCompositing()) {
    mLayerManager = nullptr;
    mTempBasicLayerInUse = false;
    retaining = false;
  }

  
  if (mLayerManager) {
    if (mLayerManager->GetBackendType() == LAYERS_D3D10) {
      LayerManagerD3D10 *layerManagerD3D10 =
        static_cast<LayerManagerD3D10*>(mLayerManager.get());
      if (layerManagerD3D10->device() !=
          gfxWindowsPlatform::GetPlatform()->GetD3D10Device()) {
        MOZ_ASSERT(!mLayerManager->IsInTransaction());

        mLayerManager->Destroy();
        mLayerManager = nullptr;
        retaining = false;
      }
    }
  }

  
  
  if (!mLayerManager) {
    if (ShouldUseOffMainThreadCompositing()) {
      NS_ASSERTION(aShadowManager == nullptr, "Async Compositor not supported with e10s");
      CreateCompositor();
    } else if (ShouldUseMainThreadD3D10Manager()) {
      nsRefPtr<mozilla::layers::LayerManagerD3D10> layerManager =
        new mozilla::layers::LayerManagerD3D10(this);
      if (layerManager->Initialize(true)) {
        mLayerManager = layerManager;
      }
    } else if (ShouldUseBasicManager()) {
      mLayerManager = CreateBasicLayerManager();
    }

    
    
    if (!mLayerManager) {
      if (!mView) {
        NS_WARNING("Using temporary basic layer manager.");
        mLayerManager = new BasicShadowLayerManager(this);
        mTempBasicLayerInUse = true;
      } else {
        NS_RUNTIMEABORT("Couldn't create layer manager");
      }
    }
  }

  if (aAllowRetaining) {
    *aAllowRetaining = retaining;
  }

  return mLayerManager;
}

NS_IMETHODIMP
MetroWidget::Invalidate(bool aEraseBackground,
                        bool aUpdateNCArea,
                        bool aIncludeChildren)
{
  nsIntRect rect;
  if (mView) {
    mView->GetBounds(rect);
  }
  Invalidate(rect);
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::Invalidate(const nsIntRect & aRect)
{
  if (mWnd) {
    RECT rect;
    rect.left   = aRect.x;
    rect.top    = aRect.y;
    rect.right  = aRect.x + aRect.width;
    rect.bottom = aRect.y + aRect.height;
    InvalidateRect(mWnd, &rect, FALSE);
  }

  return NS_OK;
}

nsTransparencyMode
MetroWidget::GetTransparencyMode()
{
  return mTransparencyMode;
}

void
MetroWidget::SetTransparencyMode(nsTransparencyMode aMode)
{
  mTransparencyMode = aMode;
}

nsIWidgetListener*
MetroWidget::GetPaintListener()
{
  if (mOnDestroyCalled)
    return nullptr;
  return mAttachedWidgetListener ? mAttachedWidgetListener :
    mWidgetListener;
}

void MetroWidget::Paint(const nsIntRegion& aInvalidRegion)
{
  nsIWidgetListener* listener = GetPaintListener();
  if (!listener)
    return;

  listener->WillPaintWindow(this);

  
  listener = GetPaintListener();
  if (!listener)
    return;

  listener->PaintWindow(this, aInvalidRegion, 0);

  listener = GetPaintListener();
  if (!listener)
    return;

  listener->DidPaintWindow();
}

void MetroWidget::UserActivity()
{
  
  if (!mIdleService) {
    mIdleService = do_GetService("@mozilla.org/widget/idleservice;1");
  }

  
  if (mIdleService) {
    mIdleService->ResetIdleTimeOut(0);
  }
}

void
MetroWidget::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
  if (!aPoint) {
    event.refPoint.x = event.refPoint.y = 0;
  } else {  
    event.refPoint.x = aPoint->x;
    event.refPoint.y = aPoint->y;
  }
  event.time = ::GetMessageTime();
}

bool
MetroWidget::DispatchWindowEvent(nsGUIEvent* aEvent)
{
  nsEventStatus aStatus;
  if (!aEvent || NS_FAILED(DispatchEvent(aEvent, aStatus)))
    return false;
  return true;
}

NS_IMETHODIMP
MetroWidget::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
  if (NS_IS_INPUT_EVENT(event)) {
    UserActivity();
  }

  aStatus = nsEventStatus_eIgnore;

  
  
  
  
  if (mAttachedWidgetListener) {
    aStatus = mAttachedWidgetListener->HandleEvent(event, mUseAttachedEvents);
  }
  else if (mWidgetListener) {
    aStatus = mWidgetListener->HandleEvent(event, mUseAttachedEvents);
  }

  
  
  
  if (mOnDestroyCalled)
    aStatus = nsEventStatus_eConsumeNoDefault;
  return NS_OK;
}

#ifdef ACCESSIBILITY
mozilla::a11y::Accessible*
MetroWidget::GetRootAccessible()
{
  
  
  
  
  
  
  
  static int accForceDisable = -1;

  if (accForceDisable == -1) {
    const char* kPrefName = "accessibility.win32.force_disabled";
    if (Preferences::GetBool(kPrefName, false)) {
      accForceDisable = 1;
    } else {
      accForceDisable = 0;
    }
  }

  
  if (accForceDisable)
      return nullptr;

  return GetAccessible();
}
#endif

float MetroWidget::GetDPI()
{
  LogFunction();
  if (!mView) {
    return 96.0;
  }
  return mView->GetDPI();
}

NS_IMETHODIMP
MetroWidget::ConstrainPosition(bool aAllowSlop, int32_t *aX, int32_t *aY)
{
  return NS_OK;
}

void
MetroWidget::SizeModeChanged()
{
  if (mWidgetListener) {
    mWidgetListener->SizeModeChanged(nsSizeMode_Normal);
  }
}

void
MetroWidget::Activated(bool aActiveated)
{
  if (mWidgetListener) {
    aActiveated ?
      mWidgetListener->WindowActivated() :
      mWidgetListener->WindowDeactivated();
  }
}

NS_IMETHODIMP
MetroWidget::Move(double aX, double aY)
{
  if (mWidgetListener) {
    mWidgetListener->WindowMoved(this, aX, aY);
  }
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::Resize(double aWidth, double aHeight, bool aRepaint)
{
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::Resize(double aX, double aY, double aWidth, double aHeight, bool aRepaint)
{
  if (mAttachedWidgetListener) {
    mAttachedWidgetListener->WindowResized(this, aWidth, aHeight);
  }
  if (mWidgetListener) {
    mWidgetListener->WindowResized(this, aWidth, aHeight);
  }
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::SetFocus(bool aRaise)
{
  return NS_OK;
}

nsresult
MetroWidget::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
  return NS_OK;
}

void*
MetroWidget::GetNativeData(uint32_t aDataType)
{
  switch(aDataType) {
    case NS_NATIVE_WINDOW:
      return mWnd;
    case NS_NATIVE_ICOREWINDOW:
      if (mView) {
        return reinterpret_cast<IUnknown*>(mView->GetCoreWindow());
      }
      break;
   case NS_NATIVE_TSF_THREAD_MGR:
   case NS_NATIVE_TSF_CATEGORY_MGR:
   case NS_NATIVE_TSF_DISPLAY_ATTR_MGR:
     return nsTextStore::GetNativeData(aDataType);
  }
  return nullptr;
}

void
MetroWidget::FreeNativeData(void * data, uint32_t aDataType)
{
}

NS_IMETHODIMP
MetroWidget::SetTitle(const nsAString& aTitle)
{
  return NS_OK;
}

nsIntPoint
MetroWidget::WidgetToScreenOffset()
{
  return nsIntPoint(0,0);
}

NS_IMETHODIMP
MetroWidget::CaptureRollupEvents(nsIRollupListener * aListener,
                                 bool aDoCapture)
{
  return NS_OK;
}

NS_IMETHODIMP_(void)
MetroWidget::SetInputContext(const InputContext& aContext,
                             const InputContextAction& aAction)
{
  mInputContext = aContext;
  nsTextStore::SetInputContext(mInputContext);
  bool enable = (mInputContext.mIMEState.mEnabled == IMEState::ENABLED ||
                 mInputContext.mIMEState.mEnabled == IMEState::PLUGIN);
  if (enable &&
      mInputContext.mIMEState.mOpen != IMEState::DONT_CHANGE_OPEN_STATE) {
    bool open = (mInputContext.mIMEState.mOpen == IMEState::OPEN);
    nsTextStore::SetIMEOpenState(open);
  }
}

NS_IMETHODIMP_(nsIWidget::InputContext)
MetroWidget::GetInputContext()
{
  return mInputContext;
}

NS_IMETHODIMP
MetroWidget::ResetInputState()
{
  nsTextStore::CommitComposition(false);
  return NS_OK;
}


NS_IMETHODIMP
MetroWidget::CancelIMEComposition()
{
  nsTextStore::CommitComposition(true);
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState)
{
  NS_ENSURE_ARG_POINTER(aLEDState);
  *aLEDState = (::GetKeyState(aKeyCode) & 1) != 0;
  return NS_OK;
}

NS_IMETHODIMP
MetroWidget::OnIMEFocusChange(bool aFocus)
{
  return nsTextStore::OnFocusChange(aFocus, this,
                                    mInputContext.mIMEState.mEnabled);
}

NS_IMETHODIMP
MetroWidget::OnIMETextChange(uint32_t aStart,
                             uint32_t aOldEnd,
                             uint32_t aNewEnd)
{
  return nsTextStore::OnTextChange(aStart, aOldEnd, aNewEnd);
}

NS_IMETHODIMP
MetroWidget::OnIMESelectionChange(void)
{
  return nsTextStore::OnSelectionChange();
}

NS_IMETHODIMP
MetroWidget::ReparentNativeWidget(nsIWidget* aNewParent)
{
  return NS_OK;
}

void
MetroWidget::SuppressBlurEvents(bool aSuppress)
{
}

bool
MetroWidget::BlurEventsSuppressed()
{
  return false;
}

void
MetroWidget::PickerOpen()
{
}

void
MetroWidget::PickerClosed()
{
}

bool
MetroWidget::HasPendingInputEvent()
{
  if (HIWORD(GetQueueStatus(QS_INPUT)))
    return true;
  return false;
}
