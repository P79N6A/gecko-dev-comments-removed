





#include "mozilla/MathAlgorithms.h"


#include "nsXULWindow.h"
#include <algorithm>


#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsWidgetsCID.h"
#include "prprf.h"
#include "nsThreadUtils.h"
#include "nsNetCID.h"
#include "nsQueryObject.h"


#include "nsIAppShell.h"
#include "nsIAppShellService.h"
#include "nsIServiceManager.h"
#include "nsIContentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMScreen.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIIOService.h"
#include "nsILoadContext.h"
#include "nsIObserverService.h"
#include "nsIWindowMediator.h"
#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsIScrollable.h"
#include "nsIScriptSecurityManager.h"
#include "nsIWindowWatcher.h"
#include "nsIURI.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsAppShellCID.h"
#include "nsReadableUtils.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsContentUtils.h"
#include "nsWebShellWindow.h" 
#include "nsGlobalWindow.h"

#include "prenv.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/dom/BarProps.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/ScriptSettings.h"

using namespace mozilla;
using dom::AutoNoJSAPI;

#define SIZEMODE_NORMAL     NS_LITERAL_STRING("normal")
#define SIZEMODE_MAXIMIZED  NS_LITERAL_STRING("maximized")
#define SIZEMODE_MINIMIZED  NS_LITERAL_STRING("minimized")
#define SIZEMODE_FULLSCREEN NS_LITERAL_STRING("fullscreen")

#define WINDOWTYPE_ATTRIBUTE NS_LITERAL_STRING("windowtype")

#define PERSIST_ATTRIBUTE  NS_LITERAL_STRING("persist")
#define SCREENX_ATTRIBUTE  NS_LITERAL_STRING("screenX")
#define SCREENY_ATTRIBUTE  NS_LITERAL_STRING("screenY")
#define WIDTH_ATTRIBUTE    NS_LITERAL_STRING("width")
#define HEIGHT_ATTRIBUTE   NS_LITERAL_STRING("height")
#define MODE_ATTRIBUTE     NS_LITERAL_STRING("sizemode")
#define ZLEVEL_ATTRIBUTE   NS_LITERAL_STRING("zlevel")






nsXULWindow::nsXULWindow(uint32_t aChromeFlags)
  : mChromeTreeOwner(nullptr), 
    mContentTreeOwner(nullptr),
    mPrimaryContentTreeOwner(nullptr),
    mModalStatus(NS_OK),
    mContinueModalLoop(false),
    mDebuting(false),
    mChromeLoaded(false), 
    mShowAfterLoad(false),
    mIntrinsicallySized(false),
    mCenterAfterLoad(false),
    mIsHiddenWindow(false),
    mLockedUntilChromeLoad(false),
    mIgnoreXULSize(false),
    mIgnoreXULPosition(false),
    mChromeFlagsFrozen(false),
    mIgnoreXULSizeMode(false),
    mDestroying(false),
    mContextFlags(0),
    mPersistentAttributesDirty(0),
    mPersistentAttributesMask(0),
    mChromeFlags(aChromeFlags)
{
}

nsXULWindow::~nsXULWindow()
{
  Destroy();
}





NS_IMPL_ADDREF(nsXULWindow)
NS_IMPL_RELEASE(nsXULWindow)

NS_INTERFACE_MAP_BEGIN(nsXULWindow)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULWindow)
  NS_INTERFACE_MAP_ENTRY(nsIXULWindow)
  NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  if (aIID.Equals(NS_GET_IID(nsXULWindow)))
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsXULWindow::GetInterface(const nsIID& aIID, void** aSink)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aSink);

  if (aIID.Equals(NS_GET_IID(nsIPrompt))) {
    rv = EnsurePrompter();
    if (NS_FAILED(rv)) return rv;
    return mPrompter->QueryInterface(aIID, aSink);
  }   
  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt))) {
    rv = EnsureAuthPrompter();
    if (NS_FAILED(rv)) return rv;
    return mAuthPrompter->QueryInterface(aIID, aSink);
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMWindow))) {
    return GetWindowDOMWindow(reinterpret_cast<nsIDOMWindow**>(aSink));
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMWindowInternal))) {
    nsIDOMWindow* domWindow = nullptr;
    rv = GetWindowDOMWindow(&domWindow);
    nsIDOMWindowInternal* domWindowInternal =
      static_cast<nsIDOMWindowInternal*>(domWindow);
    *aSink = domWindowInternal;
    return rv;
  }
  if (aIID.Equals(NS_GET_IID(nsIWebBrowserChrome)) && 
    NS_SUCCEEDED(EnsureContentTreeOwner()) &&
    NS_SUCCEEDED(mContentTreeOwner->QueryInterface(aIID, aSink)))
    return NS_OK;

  if (aIID.Equals(NS_GET_IID(nsIEmbeddingSiteWindow)) && 
    NS_SUCCEEDED(EnsureContentTreeOwner()) &&
    NS_SUCCEEDED(mContentTreeOwner->QueryInterface(aIID, aSink)))
    return NS_OK;

  return QueryInterface(aIID, aSink);
}





NS_IMETHODIMP nsXULWindow::GetDocShell(nsIDocShell** aDocShell)
{
  NS_ENSURE_ARG_POINTER(aDocShell);

  *aDocShell = mDocShell;
  NS_IF_ADDREF(*aDocShell);
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetZLevel(uint32_t *outLevel)
{
  nsCOMPtr<nsIWindowMediator> mediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (mediator)
    mediator->GetZLevel(this, outLevel);
  else
    *outLevel = normalZ;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetZLevel(uint32_t aLevel)
{
  nsCOMPtr<nsIWindowMediator> mediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!mediator)
    return NS_ERROR_FAILURE;

  uint32_t zLevel;
  mediator->GetZLevel(this, &zLevel);
  if (zLevel == aLevel)
    return NS_OK;

  

  if (aLevel > nsIXULWindow::normalZ && mWindow) {
    int32_t sizeMode = mWindow->SizeMode();
    if (sizeMode == nsSizeMode_Maximized || sizeMode == nsSizeMode_Fullscreen) {
      return NS_ERROR_FAILURE;
    }
  }

  
  mediator->SetZLevel(this, aLevel);
  PersistentAttributesDirty(PAD_MISC);
  SavePersistentAttributes();

  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    nsCOMPtr<nsIDocument> doc = cv->GetDocument();
    if (doc) {
      ErrorResult rv;
      nsRefPtr<dom::Event> event =
        doc->CreateEvent(NS_LITERAL_STRING("Events"),rv);
      if (event) {
        event->InitEvent(NS_LITERAL_STRING("windowZLevel"), true, false);

        event->SetTrusted(true);

        bool defaultActionEnabled;
        doc->DispatchEvent(event, &defaultActionEnabled);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetContextFlags(uint32_t *aContextFlags)
{
  NS_ENSURE_ARG_POINTER(aContextFlags);
  *aContextFlags = mContextFlags;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetContextFlags(uint32_t aContextFlags)
{
  mContextFlags = aContextFlags;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetChromeFlags(uint32_t *aChromeFlags)
{
  NS_ENSURE_ARG_POINTER(aChromeFlags);
  *aChromeFlags = mChromeFlags;
  




  
  if (!mChromeLoaded)
    return NS_OK;

  if (GetContentScrollbarVisibility())
    *aChromeFlags |= nsIWebBrowserChrome::CHROME_SCROLLBARS;
  else
    *aChromeFlags &= ~nsIWebBrowserChrome::CHROME_SCROLLBARS;

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetChromeFlags(uint32_t aChromeFlags)
{
  NS_ASSERTION(!mChromeFlagsFrozen,
               "SetChromeFlags() after AssumeChromeFlagsAreFrozen()!");

  mChromeFlags = aChromeFlags;
  if (mChromeLoaded)
    NS_ENSURE_SUCCESS(ApplyChromeFlags(), NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::AssumeChromeFlagsAreFrozen()
{
  mChromeFlagsFrozen = true;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetIntrinsicallySized(bool aIntrinsicallySized)
{
  mIntrinsicallySized = aIntrinsicallySized;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetIntrinsicallySized(bool* aIntrinsicallySized)
{
  NS_ENSURE_ARG_POINTER(aIntrinsicallySized);

  *aIntrinsicallySized = mIntrinsicallySized;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetPrimaryContentShell(nsIDocShellTreeItem** 
   aDocShellTreeItem)
{
  NS_ENSURE_ARG_POINTER(aDocShellTreeItem);
  NS_IF_ADDREF(*aDocShellTreeItem = mPrimaryContentShell);
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetContentShellById(const char16_t* aID, 
   nsIDocShellTreeItem** aDocShellTreeItem)
{
  NS_ENSURE_ARG_POINTER(aDocShellTreeItem);
  *aDocShellTreeItem = nullptr;

  uint32_t count = mContentShells.Length();
  for (uint32_t i = 0; i < count; i++) {
    nsContentShellInfo* shellInfo = mContentShells.ElementAt(i);
    if (shellInfo->id.Equals(aID)) {
      *aDocShellTreeItem = nullptr;
      if (shellInfo->child)
        CallQueryReferent(shellInfo->child.get(), aDocShellTreeItem);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULWindow::AddChildWindow(nsIXULWindow *aChild)
{
  
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::RemoveChildWindow(nsIXULWindow *aChild)
{
  
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::ShowModal()
{
  
  nsCOMPtr<nsIWidget> window = mWindow;
  nsCOMPtr<nsIXULWindow> tempRef = this;  

  window->SetModal(true);
  mContinueModalLoop = true;
  EnableParent(false);

  {
    AutoNoJSAPI nojsapi;
    nsIThread *thread = NS_GetCurrentThread();
    while (mContinueModalLoop) {
      if (!NS_ProcessNextEvent(thread))
        break;
    }
  }

  mContinueModalLoop = false;
  window->SetModal(false);
  











  return mModalStatus;
}





NS_IMETHODIMP nsXULWindow::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* parentWidget, int32_t x, int32_t y, int32_t cx, int32_t cy)   
{
  
  NS_ASSERTION(false, "Not Yet Implemented");
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::Create()
{
  
  NS_ASSERTION(false, "Not Yet Implemented");
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::Destroy()
{
  if (!mWindow)
     return NS_OK;

  
  if (mDestroying)
    return NS_OK;

  mozilla::AutoRestore<bool> guard(mDestroying);
  mDestroying = true;

  nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ASSERTION(appShell, "Couldn't get appShell... xpcom shutdown?");
  if (appShell)
    appShell->UnregisterTopLevelWindow(static_cast<nsIXULWindow*>(this));

  nsCOMPtr<nsIXULWindow> parentWindow(do_QueryReferent(mParentWindow));
  if (parentWindow)
    parentWindow->RemoveChildWindow(this);

  
  
  

  
  
  

  nsCOMPtr<nsIXULWindow> placeHolder = this;

  
  
  
  
  ExitModalLoop(NS_OK);
  if (mWindow)
    mWindow->Show(false);

#if defined(XP_WIN)
  
  
  nsCOMPtr<nsIBaseWindow> parent(do_QueryReferent(mParentWindow));
  if (parent) {
    nsCOMPtr<nsIWidget> parentWidget;
    parent->GetMainWidget(getter_AddRefs(parentWidget));
    if (!parentWidget || parentWidget->IsVisible()) {
      nsCOMPtr<nsIBaseWindow> baseHiddenWindow;
      if (appShell) {
        nsCOMPtr<nsIXULWindow> hiddenWindow;
        appShell->GetHiddenWindow(getter_AddRefs(hiddenWindow));
        if (hiddenWindow)
          baseHiddenWindow = do_GetInterface(hiddenWindow);
      }
      
      
      if (baseHiddenWindow != parent) {
        nsCOMPtr<nsIWidget> parentWidget;
        parent->GetMainWidget(getter_AddRefs(parentWidget));
        if (parentWidget)
          parentWidget->PlaceBehind(eZPlacementTop, 0, true);
      }
    }
  }
#endif
   
  mDOMWindow = nullptr;
  if (mDocShell) {
    nsCOMPtr<nsIBaseWindow> shellAsWin(do_QueryInterface(mDocShell));
    shellAsWin->Destroy();
    mDocShell = nullptr; 
  }

  
  uint32_t count = mContentShells.Length();
  for (uint32_t i = 0; i < count; i++) {
    nsContentShellInfo* shellInfo = mContentShells.ElementAt(i);
    delete shellInfo;
  }
  mContentShells.Clear();
  mPrimaryContentShell = nullptr;

  if (mContentTreeOwner) {
    mContentTreeOwner->XULWindow(nullptr);
    NS_RELEASE(mContentTreeOwner);
  }
  if (mPrimaryContentTreeOwner) {
    mPrimaryContentTreeOwner->XULWindow(nullptr);
    NS_RELEASE(mPrimaryContentTreeOwner);
  }
  if (mChromeTreeOwner) {
    mChromeTreeOwner->XULWindow(nullptr);
    NS_RELEASE(mChromeTreeOwner);
  }
  if (mWindow) {
    mWindow->SetWidgetListener(nullptr); 
    mWindow->Destroy();
    mWindow = nullptr;
  }

  if (!mIsHiddenWindow) {
    




    nsCOMPtr<nsIObserverService> obssvc = services::GetObserverService();
    NS_ASSERTION(obssvc, "Couldn't get observer service?");

    if (obssvc)
      obssvc->NotifyObservers(nullptr, "xul-window-destroyed", nullptr);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetUnscaledDevicePixelsPerCSSPixel(double *aScale)
{
  *aScale = mWindow ? mWindow->GetDefaultScale().scale : 1.0;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetPosition(int32_t aX, int32_t aY)
{
  
  
  CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
  double invScale = 1.0 / scale.scale;
  nsresult rv = mWindow->Move(aX * invScale, aY * invScale);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  if (!mChromeLoaded) {
    
    
    mIgnoreXULPosition = true;
    return NS_OK;
  }
  PersistentAttributesDirty(PAD_POSITION);
  SavePersistentAttributes();
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetPosition(int32_t* aX, int32_t* aY)
{
  return GetPositionAndSize(aX, aY, nullptr, nullptr);
}

NS_IMETHODIMP nsXULWindow::SetSize(int32_t aCX, int32_t aCY, bool aRepaint)
{
  


  mWindow->SetSizeMode(nsSizeMode_Normal);

  mIntrinsicallySized = false;

  CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
  double invScale = 1.0 / scale.scale;
  nsresult rv = mWindow->Resize(aCX * invScale, aCY * invScale, aRepaint);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  if (!mChromeLoaded) {
    
    
    
    
    mIgnoreXULSize = true;
    mIgnoreXULSizeMode = true;
    return NS_OK;
  }
  PersistentAttributesDirty(PAD_SIZE);
  SavePersistentAttributes();
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetSize(int32_t* aCX, int32_t* aCY)
{
  return GetPositionAndSize(nullptr, nullptr, aCX, aCY);
}

NS_IMETHODIMP nsXULWindow::SetPositionAndSize(int32_t aX, int32_t aY, 
   int32_t aCX, int32_t aCY, bool aRepaint)
{
  


  mWindow->SetSizeMode(nsSizeMode_Normal);

  mIntrinsicallySized = false;

  CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
  double invScale = 1.0 / scale.scale;
  nsresult rv = mWindow->Resize(aX * invScale, aY * invScale,
                                aCX * invScale, aCY * invScale,
                                aRepaint);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  if (!mChromeLoaded) {
    
    
    mIgnoreXULPosition = true;
    mIgnoreXULSize = true;
    mIgnoreXULSizeMode = true;
    return NS_OK;
  }
  PersistentAttributesDirty(PAD_POSITION | PAD_SIZE);
  SavePersistentAttributes();
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetPositionAndSize(int32_t* x, int32_t* y, int32_t* cx,
   int32_t* cy)
{
  nsIntRect rect;

  if (!mWindow)
    return NS_ERROR_FAILURE;

  mWindow->GetScreenBounds(rect);

  if (x)
    *x = rect.x;
  if (y)
    *y = rect.y;
  if (cx)
    *cx = rect.width;
  if (cy)
    *cy = rect.height;

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::Center(nsIXULWindow *aRelative, bool aScreen, bool aAlert)
{
  int32_t  left, top, width, height,
           ourWidth, ourHeight;
  bool     screenCoordinates =  false,
           windowCoordinates =  false;
  nsresult result;

  if (!mChromeLoaded) {
    
    mCenterAfterLoad = true;
    return NS_OK;
  }

  if (!aScreen && !aRelative)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService("@mozilla.org/gfx/screenmanager;1", &result);
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIScreen> screen;

  if (aRelative) {
    nsCOMPtr<nsIBaseWindow> base(do_QueryInterface(aRelative, &result));
    if (base) {
      
      result = base->GetPositionAndSize(&left, &top, &width, &height);
      if (NS_SUCCEEDED(result)) {
        double scale;
        if (NS_SUCCEEDED(base->GetUnscaledDevicePixelsPerCSSPixel(&scale))) {
          
          left = NSToIntRound(left / scale);
          top = NSToIntRound(top / scale);
          width = NSToIntRound(width / scale);
          height = NSToIntRound(height / scale);
        }
        
        if (aScreen)
          screenmgr->ScreenForRect(left, top, width, height, getter_AddRefs(screen));
        else
          windowCoordinates = true;
      } else {
        
        
        aRelative = 0;
        aScreen = true;
      }
    }
  }
  if (!aRelative) {
    if (!mOpenerScreenRect.IsEmpty()) {
      
      screenmgr->ScreenForRect(mOpenerScreenRect.x, mOpenerScreenRect.y,
                               mOpenerScreenRect.width, mOpenerScreenRect.height,
                               getter_AddRefs(screen));
    } else {
      screenmgr->GetPrimaryScreen(getter_AddRefs(screen));
    }
  }

  if (aScreen && screen) {
    screen->GetAvailRectDisplayPix(&left, &top, &width, &height);
    screenCoordinates = true;
  }

  if (screenCoordinates || windowCoordinates) {
    NS_ASSERTION(mWindow, "what, no window?");
    CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
    GetSize(&ourWidth, &ourHeight);
    ourWidth = NSToIntRound(ourWidth / scale.scale);
    ourHeight = NSToIntRound(ourHeight / scale.scale);
    left += (width - ourWidth) / 2;
    top += (height - ourHeight) / (aAlert ? 3 : 2);
    if (windowCoordinates) {
      mWindow->ConstrainPosition(false, &left, &top);
    }
    SetPosition(left * scale.scale, top * scale.scale);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULWindow::Repaint(bool aForce)
{
  
  NS_ASSERTION(false, "Not Yet Implemented");
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetParentWidget(nsIWidget** aParentWidget)
{
  NS_ENSURE_ARG_POINTER(aParentWidget);
  NS_ENSURE_STATE(mWindow);

  NS_IF_ADDREF(*aParentWidget = mWindow->GetParent());
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetParentWidget(nsIWidget* aParentWidget)
{
  
  NS_ASSERTION(false, "Not Yet Implemented");
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aParentNativeWindow);

  nsCOMPtr<nsIWidget> parentWidget;
  NS_ENSURE_SUCCESS(GetParentWidget(getter_AddRefs(parentWidget)), NS_ERROR_FAILURE);

  if (parentWidget) {
    *aParentNativeWindow = parentWidget->GetNativeData(NS_NATIVE_WIDGET);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
  
  NS_ASSERTION(false, "Not Yet Implemented");
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetNativeHandle(nsAString& aNativeHandle)
{
  nsCOMPtr<nsIWidget> mainWidget;
  NS_ENSURE_SUCCESS(GetMainWidget(getter_AddRefs(mainWidget)), NS_ERROR_FAILURE);

  if (mainWidget) {
    nativeWindow nativeWindowPtr = mainWidget->GetNativeData(NS_NATIVE_WINDOW);
    


    aNativeHandle = NS_ConvertASCIItoUTF16(nsPrintfCString("0x%p", nativeWindowPtr));
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetVisibility(bool* aVisibility)
{
  NS_ENSURE_ARG_POINTER(aVisibility);

  
  

  *aVisibility = true;

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetVisibility(bool aVisibility)
{
  if (!mChromeLoaded) {
    mShowAfterLoad = aVisibility;
    return NS_OK;
  }

  if (mDebuting) {
    return NS_OK;
  }
  mDebuting = true;  

  
  
  nsCOMPtr<nsIBaseWindow> shellAsWin(do_QueryInterface(mDocShell));
  shellAsWin->SetVisibility(aVisibility);
  
  
  
  
  nsCOMPtr<nsIWidget> window = mWindow;
  window->Show(aVisibility);

  nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (windowMediator)
     windowMediator->UpdateWindowTimeStamp(static_cast<nsIXULWindow*>(this));

  
  nsCOMPtr<nsIObserverService> obssvc = services::GetObserverService();
  NS_ASSERTION(obssvc, "Couldn't get observer service.");
  if (obssvc) {
    obssvc->NotifyObservers(nullptr, "xul-window-visible", nullptr); 
  }

  mDebuting = false;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetEnabled(bool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);

  if (mWindow) {
    *aEnabled = mWindow->IsEnabled();
    return NS_OK;
  }

  *aEnabled = true; 
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULWindow::SetEnabled(bool aEnable)
{
  if (mWindow) {
    mWindow->Enable(aEnable);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULWindow::GetMainWidget(nsIWidget** aMainWidget)
{
  NS_ENSURE_ARG_POINTER(aMainWidget);
   
  *aMainWidget = mWindow;
  NS_IF_ADDREF(*aMainWidget);
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetFocus()
{
  
  NS_ASSERTION(false, "Not Yet Implemented");
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetTitle(char16_t** aTitle)
{
  NS_ENSURE_ARG_POINTER(aTitle);

  *aTitle = ToNewUnicode(mTitle);
  if (!*aTitle)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetTitle(const char16_t* aTitle)
{
  NS_ENSURE_STATE(mWindow);
  mTitle.Assign(aTitle);
  mTitle.StripChars("\n\r");
  NS_ENSURE_SUCCESS(mWindow->SetTitle(mTitle), NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!windowMediator)
    return NS_OK;

  windowMediator->UpdateWindowTitle(static_cast<nsIXULWindow*>(this), aTitle);

  return NS_OK;
}






NS_IMETHODIMP nsXULWindow::EnsureChromeTreeOwner()
{
  if (mChromeTreeOwner)
    return NS_OK;

  mChromeTreeOwner = new nsChromeTreeOwner();
  NS_ENSURE_TRUE(mChromeTreeOwner, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(mChromeTreeOwner);
  mChromeTreeOwner->XULWindow(this);

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::EnsureContentTreeOwner()
{
  if (mContentTreeOwner)
    return NS_OK;

  mContentTreeOwner = new nsContentTreeOwner(false);
  NS_ENSURE_TRUE(mContentTreeOwner, NS_ERROR_FAILURE);

  NS_ADDREF(mContentTreeOwner);
  mContentTreeOwner->XULWindow(this);
   
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::EnsurePrimaryContentTreeOwner()
{
  if (mPrimaryContentTreeOwner)
    return NS_OK;

  mPrimaryContentTreeOwner = new nsContentTreeOwner(true);
  NS_ENSURE_TRUE(mPrimaryContentTreeOwner, NS_ERROR_FAILURE);

  NS_ADDREF(mPrimaryContentTreeOwner);
  mPrimaryContentTreeOwner->XULWindow(this);

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::EnsurePrompter()
{
  if (mPrompter)
    return NS_OK;
   
  nsCOMPtr<nsIDOMWindow> ourWindow;
  nsresult rv = GetWindowDOMWindow(getter_AddRefs(ourWindow));
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIWindowWatcher> wwatch = 
        do_GetService(NS_WINDOWWATCHER_CONTRACTID);
    if (wwatch)
      wwatch->GetNewPrompter(ourWindow, getter_AddRefs(mPrompter));
  }
  return mPrompter ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULWindow::EnsureAuthPrompter()
{
  if (mAuthPrompter)
    return NS_OK;
      
  nsCOMPtr<nsIDOMWindow> ourWindow;
  nsresult rv = GetWindowDOMWindow(getter_AddRefs(ourWindow));
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (wwatch)
      wwatch->GetNewAuthPrompter(ourWindow, getter_AddRefs(mAuthPrompter));
  }
  return mAuthPrompter ? NS_OK : NS_ERROR_FAILURE;
}
 
void nsXULWindow::OnChromeLoaded()
{
  nsresult rv = EnsureContentTreeOwner();

  if (NS_SUCCEEDED(rv)) {
    mChromeLoaded = true;
    ApplyChromeFlags();
    SyncAttributesToWidget();
    if (!mIgnoreXULSize)
      LoadSizeFromXUL();
    if (mIntrinsicallySized) {
      
      nsCOMPtr<nsIContentViewer> cv;
      mDocShell->GetContentViewer(getter_AddRefs(cv));
      if (cv) {
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem = do_QueryInterface(mDocShell);
        nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
        docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
        if (treeOwner) {
          int32_t width, height;
          cv->GetContentSize(&width, &height);
          treeOwner->SizeShellTo(docShellAsItem, width, height);
        }
      }
    }

    bool positionSet = !mIgnoreXULPosition;
    nsCOMPtr<nsIXULWindow> parentWindow(do_QueryReferent(mParentWindow));
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    
    
    
    if (!parentWindow)
      positionSet = false;
#endif
    if (positionSet)
      positionSet = LoadPositionFromXUL();
    LoadMiscPersistentAttributesFromXUL();

    if (mCenterAfterLoad && !positionSet)
      Center(parentWindow, parentWindow ? false : true, false);

    if (mShowAfterLoad) {
      SetVisibility(true);
      
      
    }
  }
  mPersistentAttributesMask |= PAD_POSITION | PAD_SIZE | PAD_MISC;
}

bool nsXULWindow::LoadPositionFromXUL()
{
  bool     gotPosition = false;

  
  
  if (mIsHiddenWindow)
    return false;

  nsCOMPtr<dom::Element> windowElement = GetWindowDOMElement();
  NS_ENSURE_TRUE(windowElement, false);

  int32_t currX = 0;
  int32_t currY = 0;
  int32_t currWidth = 0;
  int32_t currHeight = 0;
  nsresult errorCode;
  int32_t temp;

  GetPositionAndSize(&currX, &currY, &currWidth, &currHeight);

  
  
  CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
  currX = NSToIntRound(currX / scale.scale);
  currY = NSToIntRound(currY / scale.scale);
  currWidth = NSToIntRound(currWidth / scale.scale);
  currHeight = NSToIntRound(currHeight / scale.scale);

  
  int32_t specX = currX;
  int32_t specY = currY;
  nsAutoString posString;

  windowElement->GetAttribute(SCREENX_ATTRIBUTE, posString);
  temp = posString.ToInteger(&errorCode);
  if (NS_SUCCEEDED(errorCode)) {
    specX = temp;
    gotPosition = true;
  }
  windowElement->GetAttribute(SCREENY_ATTRIBUTE, posString);
  temp = posString.ToInteger(&errorCode);
  if (NS_SUCCEEDED(errorCode)) {
    specY = temp;
    gotPosition = true;
  }

  if (gotPosition) {
    
    nsCOMPtr<nsIBaseWindow> parent(do_QueryReferent(mParentWindow));
    if (parent) {
      int32_t parentX, parentY;
      if (NS_SUCCEEDED(parent->GetPosition(&parentX, &parentY))) {
        double scale;
        if (NS_SUCCEEDED(parent->GetUnscaledDevicePixelsPerCSSPixel(&scale))) {
          parentX = NSToIntRound(parentX / scale);
          parentY = NSToIntRound(parentY / scale);
        }
        specX += parentX;
        specY += parentY;
      }
    }
    else {
      StaggerPosition(specX, specY, currWidth, currHeight);
    }
  }
  mWindow->ConstrainPosition(false, &specX, &specY);
  if (specX != currX || specY != currY) {
    CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
    SetPosition(specX * scale.scale, specY * scale.scale);
  }

  return gotPosition;
}

bool nsXULWindow::LoadSizeFromXUL()
{
  bool     gotSize = false;

  
  
  if (mIsHiddenWindow)
    return false;

  nsCOMPtr<dom::Element> windowElement = GetWindowDOMElement();
  NS_ENSURE_TRUE(windowElement, false);

  int32_t currWidth = 0;
  int32_t currHeight = 0;
  nsresult errorCode;
  int32_t temp;

  NS_ASSERTION(mWindow, "we expected to have a window already");

  CSSToLayoutDeviceScale scale = mWindow ? mWindow->GetDefaultScale()
                                         : CSSToLayoutDeviceScale(1.0);
  GetSize(&currWidth, &currHeight);
  currWidth = NSToIntRound(currWidth / scale.scale);
  currHeight = NSToIntRound(currHeight / scale.scale);

  
  int32_t specWidth = currWidth;
  int32_t specHeight = currHeight;
  nsAutoString sizeString;

  windowElement->GetAttribute(WIDTH_ATTRIBUTE, sizeString);
  temp = sizeString.ToInteger(&errorCode);
  if (NS_SUCCEEDED(errorCode) && temp > 0) {
    specWidth = std::max(temp, 100);
    gotSize = true;
  }
  windowElement->GetAttribute(HEIGHT_ATTRIBUTE, sizeString);
  temp = sizeString.ToInteger(&errorCode);
  if (NS_SUCCEEDED(errorCode) && temp > 0) {
    specHeight = std::max(temp, 100);
    gotSize = true;
  }

  if (gotSize) {
    
    nsCOMPtr<nsIDOMWindow> domWindow;
    GetWindowDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) {
      nsCOMPtr<nsIDOMScreen> screen;
      domWindow->GetScreen(getter_AddRefs(screen));
      if (screen) {
        int32_t screenWidth;
        int32_t screenHeight;
        screen->GetAvailWidth(&screenWidth);
        screen->GetAvailHeight(&screenHeight);
        if (specWidth > screenWidth)
          specWidth = screenWidth;
        if (specHeight > screenHeight)
          specHeight = screenHeight;
      }
    }

    mIntrinsicallySized = false;
    if (specWidth != currWidth || specHeight != currHeight) {
      CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();
      SetSize(specWidth * scale.scale, specHeight * scale.scale, false);
    }
  }

  return gotSize;
}





bool nsXULWindow::LoadMiscPersistentAttributesFromXUL()
{
  bool     gotState = false;

  



  if (mIsHiddenWindow)
    return false;

  nsCOMPtr<dom::Element> windowElement = GetWindowDOMElement();
  NS_ENSURE_TRUE(windowElement, false);

  nsAutoString stateString;

  
  windowElement->GetAttribute(MODE_ATTRIBUTE, stateString);
  int32_t sizeMode = nsSizeMode_Normal;
  



  if (!mIgnoreXULSizeMode &&
      (stateString.Equals(SIZEMODE_MAXIMIZED) || stateString.Equals(SIZEMODE_FULLSCREEN))) {
    


    if (mChromeFlags & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE) {
      mIntrinsicallySized = false;

      if (stateString.Equals(SIZEMODE_MAXIMIZED))
        sizeMode = nsSizeMode_Maximized;
      else
        sizeMode = nsSizeMode_Fullscreen;
    }
  }

  
  
  if (mIgnoreXULSizeMode) {
    nsAutoString sizeString;
    if (sizeMode == nsSizeMode_Maximized)
      sizeString.Assign(SIZEMODE_MAXIMIZED);
    else if (sizeMode == nsSizeMode_Fullscreen)
      sizeString.Assign(SIZEMODE_FULLSCREEN);
    else if (sizeMode == nsSizeMode_Normal)
      sizeString.Assign(SIZEMODE_NORMAL);
    if (!sizeString.IsEmpty()) {
      ErrorResult rv;
      windowElement->SetAttribute(MODE_ATTRIBUTE, sizeString, rv);
    }
  }

  if (sizeMode == nsSizeMode_Fullscreen) {
    nsCOMPtr<nsIDOMWindow> ourWindow;
    GetWindowDOMWindow(getter_AddRefs(ourWindow));
    ourWindow->SetFullScreen(true);
  } else {
    mWindow->SetSizeMode(sizeMode);
  }
  gotState = true;

  
  windowElement->GetAttribute(ZLEVEL_ATTRIBUTE, stateString);
  if (!stateString.IsEmpty()) {
    nsresult errorCode;
    int32_t zLevel = stateString.ToInteger(&errorCode);
    if (NS_SUCCEEDED(errorCode) && zLevel >= lowestZ && zLevel <= highestZ)
      SetZLevel(zLevel);
  }

  return gotState;
}






void nsXULWindow::StaggerPosition(int32_t &aRequestedX, int32_t &aRequestedY,
                                  int32_t aSpecWidth, int32_t aSpecHeight)
{
  const int32_t kOffset = 22;
  const uint32_t kSlop  = 4;

  bool     keepTrying;
  int      bouncedX = 0, 
           bouncedY = 0; 

  
  nsCOMPtr<nsIWindowMediator> wm(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!wm)
    return;

  nsCOMPtr<dom::Element> windowElement = GetWindowDOMElement();
  if (!windowElement)
    return;

  nsCOMPtr<nsIXULWindow> ourXULWindow(this);

  nsAutoString windowType;
  windowElement->GetAttribute(WINDOWTYPE_ATTRIBUTE, windowType);

  int32_t screenTop = 0,    
          screenRight = 0,  
          screenBottom = 0, 
          screenLeft = 0;   
  bool    gotScreen = false;

  { 
    nsCOMPtr<nsIScreenManager> screenMgr(do_GetService(
                                         "@mozilla.org/gfx/screenmanager;1"));
    if (screenMgr) {
      nsCOMPtr<nsIScreen> ourScreen;
      
      screenMgr->ScreenForRect(aRequestedX, aRequestedY,
                               aSpecWidth, aSpecHeight,
                               getter_AddRefs(ourScreen));
      if (ourScreen) {
        int32_t screenWidth, screenHeight;
        ourScreen->GetAvailRectDisplayPix(&screenLeft, &screenTop,
                                          &screenWidth, &screenHeight);
        screenBottom = screenTop + screenHeight;
        screenRight = screenLeft + screenWidth;
        gotScreen = true;
      }
    }
  }

  
  do {
    keepTrying = false;
    nsCOMPtr<nsISimpleEnumerator> windowList;
    wm->GetXULWindowEnumerator(windowType.get(), getter_AddRefs(windowList));

    if (!windowList)
      break;

    
    do {
      bool more;
      windowList->HasMoreElements(&more);
      if (!more)
        break;

      nsCOMPtr<nsISupports> supportsWindow;
      windowList->GetNext(getter_AddRefs(supportsWindow));

      nsCOMPtr<nsIXULWindow> listXULWindow(do_QueryInterface(supportsWindow));
      if (listXULWindow != ourXULWindow) {
        int32_t listX, listY;
        nsCOMPtr<nsIBaseWindow> listBaseWindow(do_QueryInterface(supportsWindow));
        listBaseWindow->GetPosition(&listX, &listY);
        double scale;
        if (NS_SUCCEEDED(listBaseWindow->GetUnscaledDevicePixelsPerCSSPixel(&scale))) {
          listX = NSToIntRound(listX / scale);
          listY = NSToIntRound(listY / scale);
        }

        if (Abs(listX - aRequestedX) <= kSlop && Abs(listY - aRequestedY) <= kSlop) {
          
          if (bouncedX & 0x1)
            aRequestedX -= kOffset;
          else
            aRequestedX += kOffset;
          aRequestedY += kOffset;

          if (gotScreen) {
            
            if (!(bouncedX & 0x1) && ((aRequestedX + aSpecWidth) > screenRight)) {
              aRequestedX = screenRight - aSpecWidth;
              ++bouncedX;
            }

            
            if ((bouncedX & 0x1) && aRequestedX < screenLeft) {
              aRequestedX = screenLeft;
              ++bouncedX;
            }

            
            if (aRequestedY + aSpecHeight > screenBottom) {
              aRequestedY = screenTop;
              ++bouncedY;
            }
          }

          


          keepTrying = bouncedX < 2 || bouncedY == 0;
          break;
        }
      }
    } while(1);
  } while (keepTrying);
}

void nsXULWindow::SyncAttributesToWidget()
{
  nsCOMPtr<dom::Element> windowElement = GetWindowDOMElement();
  if (!windowElement)
    return;

  nsAutoString attr;

  
  if (windowElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::hidechrome,
                                 nsGkAtoms::_true, eCaseMatters)) {
    mWindow->HideWindowChrome(true);
  }

  
  nsIntMargin margins;
  windowElement->GetAttribute(NS_LITERAL_STRING("chromemargin"), attr);
  if (nsContentUtils::ParseIntMarginValue(attr, margins)) {
    mWindow->SetNonClientMargins(margins);
  }

  
  windowElement->GetAttribute(WINDOWTYPE_ATTRIBUTE, attr);
  if (!attr.IsEmpty()) {
    mWindow->SetWindowClass(attr);
  }

  
  windowElement->GetAttribute(NS_LITERAL_STRING("id"), attr);
  if (attr.IsEmpty()) {
    attr.AssignLiteral("default");
  }
  mWindow->SetIcon(attr);

  
  windowElement->GetAttribute(NS_LITERAL_STRING("drawtitle"), attr);
  mWindow->SetDrawsTitle(attr.LowerCaseEqualsLiteral("true"));

  
  windowElement->GetAttribute(NS_LITERAL_STRING("toggletoolbar"), attr);
  mWindow->SetShowsToolbarButton(attr.LowerCaseEqualsLiteral("true"));

  
  windowElement->GetAttribute(NS_LITERAL_STRING("fullscreenbutton"), attr);
  mWindow->SetShowsFullScreenButton(attr.LowerCaseEqualsLiteral("true"));

  
  windowElement->GetAttribute(NS_LITERAL_STRING("macanimationtype"), attr);
  if (attr.EqualsLiteral("document")) {
    mWindow->SetWindowAnimationType(nsIWidget::eDocumentWindowAnimation);
  }
}

NS_IMETHODIMP nsXULWindow::SavePersistentAttributes()
{
  
  
  if (!mDocShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<dom::Element> docShellElement = GetWindowDOMElement();
  if (!docShellElement)
    return NS_ERROR_FAILURE;

  nsAutoString   persistString;
  docShellElement->GetAttribute(PERSIST_ATTRIBUTE, persistString);
  if (persistString.IsEmpty()) { 
    mPersistentAttributesDirty = 0;
    return NS_OK;
  }

  
  nsIntRect rect;
  bool gotRestoredBounds = NS_SUCCEEDED(mWindow->GetRestoredBounds(rect));

  CSSToLayoutDeviceScale scale = mWindow->GetDefaultScale();

  
  nsCOMPtr<nsIBaseWindow> parent(do_QueryReferent(mParentWindow));
  if (parent && gotRestoredBounds) {
    int32_t parentX, parentY;
    if (NS_SUCCEEDED(parent->GetPosition(&parentX, &parentY))) {
      rect.x -= parentX;
      rect.y -= parentY;
    }
  }

  char                        sizeBuf[10];
  nsAutoString                sizeString;
  nsAutoString                windowElementId;
  nsCOMPtr<nsIDOMXULDocument> ownerXULDoc;

  
  ownerXULDoc = do_QueryInterface(docShellElement->OwnerDoc());
  if (docShellElement->IsXULElement()) {
    docShellElement->GetId(windowElementId);
  }

  ErrorResult rv;
  
  if ((mPersistentAttributesDirty & PAD_POSITION) && gotRestoredBounds) {
    if (persistString.Find("screenX") >= 0) {
      PR_snprintf(sizeBuf, sizeof(sizeBuf), "%d", NSToIntRound(rect.x / scale.scale));
      sizeString.AssignWithConversion(sizeBuf);
      docShellElement->SetAttribute(SCREENX_ATTRIBUTE, sizeString, rv);
      if (ownerXULDoc) 
        ownerXULDoc->Persist(windowElementId, SCREENX_ATTRIBUTE);
    }
    if (persistString.Find("screenY") >= 0) {
      PR_snprintf(sizeBuf, sizeof(sizeBuf), "%d", NSToIntRound(rect.y / scale.scale));
      sizeString.AssignWithConversion(sizeBuf);
      docShellElement->SetAttribute(SCREENY_ATTRIBUTE, sizeString, rv);
      if (ownerXULDoc)
        ownerXULDoc->Persist(windowElementId, SCREENY_ATTRIBUTE);
    }
  }

  if ((mPersistentAttributesDirty & PAD_SIZE) && gotRestoredBounds) {
    if (persistString.Find("width") >= 0) {
      PR_snprintf(sizeBuf, sizeof(sizeBuf), "%d", NSToIntRound(rect.width / scale.scale));
      sizeString.AssignWithConversion(sizeBuf);
      docShellElement->SetAttribute(WIDTH_ATTRIBUTE, sizeString, rv);
      if (ownerXULDoc)
        ownerXULDoc->Persist(windowElementId, WIDTH_ATTRIBUTE);
    }
    if (persistString.Find("height") >= 0) {
      PR_snprintf(sizeBuf, sizeof(sizeBuf), "%d", NSToIntRound(rect.height / scale.scale));
      sizeString.AssignWithConversion(sizeBuf);
      docShellElement->SetAttribute(HEIGHT_ATTRIBUTE, sizeString, rv);
      if (ownerXULDoc)
        ownerXULDoc->Persist(windowElementId, HEIGHT_ATTRIBUTE);
    }
  }

  if (mPersistentAttributesDirty & PAD_MISC) {
    int32_t sizeMode = mWindow->SizeMode();

    if (sizeMode != nsSizeMode_Minimized) {
      if (sizeMode == nsSizeMode_Maximized)
        sizeString.Assign(SIZEMODE_MAXIMIZED);
      else if (sizeMode == nsSizeMode_Fullscreen)
        sizeString.Assign(SIZEMODE_FULLSCREEN);
      else
        sizeString.Assign(SIZEMODE_NORMAL);
      docShellElement->SetAttribute(MODE_ATTRIBUTE, sizeString, rv);
      if (ownerXULDoc && persistString.Find("sizemode") >= 0)
        ownerXULDoc->Persist(windowElementId, MODE_ATTRIBUTE);
    }
    if (persistString.Find("zlevel") >= 0) {
      uint32_t zLevel;
      nsCOMPtr<nsIWindowMediator> mediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
      if (mediator) {
        mediator->GetZLevel(this, &zLevel);
        PR_snprintf(sizeBuf, sizeof(sizeBuf), "%lu", (unsigned long)zLevel);
        sizeString.AssignWithConversion(sizeBuf);
        docShellElement->SetAttribute(ZLEVEL_ATTRIBUTE, sizeString, rv);
        ownerXULDoc->Persist(windowElementId, ZLEVEL_ATTRIBUTE);
      }
    }
  }

  mPersistentAttributesDirty = 0;
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetWindowDOMWindow(nsIDOMWindow** aDOMWindow)
{
  NS_ENSURE_STATE(mDocShell);

  if (!mDOMWindow)
    mDOMWindow = mDocShell->GetWindow();
  NS_ENSURE_TRUE(mDOMWindow, NS_ERROR_FAILURE);

  *aDOMWindow = mDOMWindow;
  NS_ADDREF(*aDOMWindow);
  return NS_OK;
}

dom::Element*
nsXULWindow::GetWindowDOMElement() const
{
  NS_ENSURE_TRUE(mDocShell, nullptr);

  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  NS_ENSURE_TRUE(cv, nullptr);

  const nsIDocument* document = cv->GetDocument();
  NS_ENSURE_TRUE(document, nullptr);

  return document->GetRootElement();
}

nsresult nsXULWindow::ContentShellAdded(nsIDocShellTreeItem* aContentShell,
   bool aPrimary, bool aTargetable, const nsAString& aID)
{
  nsContentShellInfo* shellInfo = nullptr;

  uint32_t i, count = mContentShells.Length();
  nsWeakPtr contentShellWeak = do_GetWeakReference(aContentShell);
  for (i = 0; i < count; i++) {
    nsContentShellInfo* info = mContentShells.ElementAt(i);
    if (info->id.Equals(aID)) {
      
      info->child = contentShellWeak;
      shellInfo = info;
    }
    else if (info->child == contentShellWeak)
      info->child = nullptr;
  }

  if (!shellInfo) {
    shellInfo = new nsContentShellInfo(aID, contentShellWeak);
    mContentShells.AppendElement(shellInfo);
  }
    
  
  if (aPrimary) {
    NS_ENSURE_SUCCESS(EnsurePrimaryContentTreeOwner(), NS_ERROR_FAILURE);
    aContentShell->SetTreeOwner(mPrimaryContentTreeOwner);
    mPrimaryContentShell = aContentShell;
  }
  else {
    NS_ENSURE_SUCCESS(EnsureContentTreeOwner(), NS_ERROR_FAILURE);
    aContentShell->SetTreeOwner(mContentTreeOwner);
    if (mPrimaryContentShell == aContentShell)
      mPrimaryContentShell = nullptr;
  }

  if (aTargetable) {
#ifdef DEBUG
    int32_t debugCount = mTargetableShells.Count();
    int32_t debugCounter;
    for (debugCounter = debugCount - 1; debugCounter >= 0; --debugCounter) {
      nsCOMPtr<nsIDocShellTreeItem> curItem =
        do_QueryReferent(mTargetableShells[debugCounter]);
      NS_ASSERTION(!SameCOMIdentity(curItem, aContentShell),
                   "Adding already existing item to mTargetableShells");
    }
#endif
    
    
    
    
    
    
    
    
    
    
    
    bool inserted;
    if (aPrimary || !mPrimaryContentShell) {
      inserted = mTargetableShells.InsertObjectAt(contentShellWeak, 0);
    } else {
      inserted = mTargetableShells.AppendObject(contentShellWeak);
    }
    NS_ENSURE_TRUE(inserted, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult nsXULWindow::ContentShellRemoved(nsIDocShellTreeItem* aContentShell)
{
  if (mPrimaryContentShell == aContentShell) {
    mPrimaryContentShell = nullptr;
  }

  int32_t i, count = mContentShells.Length();
  for (i = count - 1; i >= 0; --i) {
    nsContentShellInfo* info = mContentShells.ElementAt(i);
    nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryReferent(info->child);
    if (!curItem || SameCOMIdentity(curItem, aContentShell)) {
      mContentShells.RemoveElementAt(i);
      delete info;
    }
  }

  count = mTargetableShells.Count();
  for (i = count - 1; i >= 0; --i) {
    nsCOMPtr<nsIDocShellTreeItem> curItem =
      do_QueryReferent(mTargetableShells[i]);
    if (!curItem || SameCOMIdentity(curItem, aContentShell)) {
      mTargetableShells.RemoveObjectAt(i);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SizeShellTo(nsIDocShellTreeItem* aShellItem,
   int32_t aCX, int32_t aCY)
{
  
  
  
  

  nsCOMPtr<nsIBaseWindow> shellAsWin(do_QueryInterface(aShellItem));
  NS_ENSURE_TRUE(shellAsWin, NS_ERROR_FAILURE);

  int32_t width = 0;
  int32_t height = 0;
  shellAsWin->GetSize(&width, &height);

  int32_t widthDelta = aCX - width;
  int32_t heightDelta = aCY - height;

  if (widthDelta || heightDelta) {
    int32_t winCX = 0;
    int32_t winCY = 0;

    GetSize(&winCX, &winCY);
    
    
    
    
    winCX = std::max(winCX + widthDelta, aCX);
    winCY = std::max(winCY + heightDelta, aCY);
    SetSize(winCX, winCY, true);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::ExitModalLoop(nsresult aStatus)
{
  if (mContinueModalLoop)
    EnableParent(true);
  mContinueModalLoop = false;
  mModalStatus = aStatus;
  return NS_OK;
}


NS_IMETHODIMP nsXULWindow::CreateNewWindow(int32_t aChromeFlags,
                                           nsITabParent *aOpeningTab,
                                           nsIXULWindow **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
    return CreateNewChromeWindow(aChromeFlags, aOpeningTab, _retval);
  return CreateNewContentWindow(aChromeFlags, aOpeningTab, _retval);
}

NS_IMETHODIMP nsXULWindow::CreateNewChromeWindow(int32_t aChromeFlags,
                                                 nsITabParent *aOpeningTab,
                                                 nsIXULWindow **_retval)
{
  nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShell, NS_ERROR_FAILURE);

  

  nsCOMPtr<nsIXULWindow> newWindow;
  appShell->CreateTopLevelWindow(this, nullptr, aChromeFlags,
                                 nsIAppShellService::SIZE_TO_CONTENT,
                                 nsIAppShellService::SIZE_TO_CONTENT,
                                 aOpeningTab,
                                 getter_AddRefs(newWindow));

  NS_ENSURE_TRUE(newWindow, NS_ERROR_FAILURE);

  *_retval = newWindow;
  NS_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::CreateNewContentWindow(int32_t aChromeFlags,
                                                  nsITabParent *aOpeningTab,
                                                  nsIXULWindow **_retval)
{
  nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShell, NS_ERROR_FAILURE);

  
  
  
  

  nsCOMPtr<nsIURI> uri;

  nsAdoptingCString urlStr = Preferences::GetCString("browser.chromeURL");
  if (urlStr.IsEmpty()) {
    urlStr.AssignLiteral("chrome://navigator/content/navigator.xul");
  }

  nsCOMPtr<nsIIOService> service(do_GetService(NS_IOSERVICE_CONTRACTID));
  if (service) {
    service->NewURI(urlStr, nullptr, nullptr, getter_AddRefs(uri));
  }
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  
  
  
  
  nsCOMPtr<nsIXULWindow> newWindow;
  {
    AutoNoJSAPI nojsapi;
    appShell->CreateTopLevelWindow(this, uri,
                                   aChromeFlags, 615, 480,
                                   aOpeningTab,
                                   getter_AddRefs(newWindow));
    NS_ENSURE_TRUE(newWindow, NS_ERROR_FAILURE);
  }

  
  nsXULWindow *xulWin = static_cast<nsXULWindow*>
                                   (static_cast<nsIXULWindow*>
                                               (newWindow));

  xulWin->LockUntilChromeLoad();

  {
    AutoNoJSAPI nojsapi;
    nsIThread *thread = NS_GetCurrentThread();
    while (xulWin->IsLocked()) {
      if (!NS_ProcessNextEvent(thread))
        break;
    }
  }

  NS_ENSURE_STATE(xulWin->mPrimaryContentShell);

  *_retval = newWindow;
  NS_ADDREF(*_retval);

  return NS_OK;
}

void nsXULWindow::EnableParent(bool aEnable)
{
  nsCOMPtr<nsIBaseWindow> parentWindow;
  nsCOMPtr<nsIWidget>     parentWidget;

  parentWindow = do_QueryReferent(mParentWindow);
  if (parentWindow)
    parentWindow->GetMainWidget(getter_AddRefs(parentWidget));
  if (parentWidget)
    parentWidget->Enable(aEnable);
}


bool nsXULWindow::ConstrainToZLevel(bool        aImmediate,
                                      nsWindowZ  *aPlacement,
                                      nsIWidget  *aReqBelow,
                                      nsIWidget **aActualBelow)
{
#if 0
  



  nsCOMPtr<nsIBaseWindow> parentWindow = do_QueryReferent(mParentWindow);
  if (parentWindow)
    return false;
#endif

  nsCOMPtr<nsIWindowMediator> mediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!mediator)
    return false;

  bool           altered;
  uint32_t       position,
                 newPosition,
                 zLevel;
  nsIXULWindow  *us = this;

  altered = false;
  mediator->GetZLevel(this, &zLevel);

  
  position = nsIWindowMediator::zLevelTop;
  if (*aPlacement == nsWindowZBottom || zLevel == nsIXULWindow::lowestZ)
    position = nsIWindowMediator::zLevelBottom;
  else if (*aPlacement == nsWindowZRelative)
    position = nsIWindowMediator::zLevelBelow;

  if (NS_SUCCEEDED(mediator->CalculateZPosition(us, position, aReqBelow,
                               &newPosition, aActualBelow, &altered))) {
    




    if (altered &&
        (position == nsIWindowMediator::zLevelTop ||
         (position == nsIWindowMediator::zLevelBelow && aReqBelow == 0)))
      PlaceWindowLayersBehind(zLevel + 1, nsIXULWindow::highestZ, 0);

    if (*aPlacement != nsWindowZBottom &&
        position == nsIWindowMediator::zLevelBottom)
      altered = true;
    if (altered || aImmediate) {
      if (newPosition == nsIWindowMediator::zLevelTop)
        *aPlacement = nsWindowZTop;
      else if (newPosition == nsIWindowMediator::zLevelBottom)
        *aPlacement = nsWindowZBottom;
      else
        *aPlacement = nsWindowZRelative;

      if (aImmediate) {
        nsCOMPtr<nsIBaseWindow> ourBase = do_QueryObject(this);
        if (ourBase) {
          nsCOMPtr<nsIWidget> ourWidget;
          ourBase->GetMainWidget(getter_AddRefs(ourWidget));
          ourWidget->PlaceBehind(*aPlacement == nsWindowZBottom ?
                                   eZPlacementBottom : eZPlacementBelow,
                                 *aActualBelow, false);
        }
      }
    }

    



    nsCOMPtr<nsIXULWindow> windowAbove;
    if (newPosition == nsIWindowMediator::zLevelBelow && *aActualBelow) {
      windowAbove = (*aActualBelow)->GetWidgetListener()->GetXULWindow();
    }

    mediator->SetZPosition(us, newPosition, windowAbove);
  }

  return altered;
}









void nsXULWindow::PlaceWindowLayersBehind(uint32_t aLowLevel,
                                          uint32_t aHighLevel,
                                          nsIXULWindow *aBehind)
{
  

  nsCOMPtr<nsIWindowMediator> mediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!mediator)
    return;

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  mediator->GetZOrderXULWindowEnumerator(0, true,
              getter_AddRefs(windowEnumerator));
  if (!windowEnumerator)
    return;

  
  
  nsCOMPtr<nsIWidget> previousHighWidget;
  if (aBehind) {
    nsCOMPtr<nsIBaseWindow> highBase(do_QueryInterface(aBehind));
    if (highBase)
      highBase->GetMainWidget(getter_AddRefs(previousHighWidget));
  }

  
  bool more;
  while (windowEnumerator->HasMoreElements(&more), more) {
    uint32_t nextZ; 
    nsCOMPtr<nsISupports> nextWindow;
    windowEnumerator->GetNext(getter_AddRefs(nextWindow));
    nsCOMPtr<nsIXULWindow> nextXULWindow(do_QueryInterface(nextWindow));
    nextXULWindow->GetZLevel(&nextZ);
    if (nextZ < aLowLevel)
      break; 
    
    
    nsCOMPtr<nsIBaseWindow> nextBase(do_QueryInterface(nextXULWindow));
    if (nextBase) {
      nsCOMPtr<nsIWidget> nextWidget;
      nextBase->GetMainWidget(getter_AddRefs(nextWidget));
      if (nextZ <= aHighLevel)
        nextWidget->PlaceBehind(eZPlacementBelow, previousHighWidget, false);
      previousHighWidget = nextWidget;
    }
  }
}

void nsXULWindow::SetContentScrollbarVisibility(bool aVisible)
{
  nsCOMPtr<nsPIDOMWindow> contentWin(do_GetInterface(mPrimaryContentShell));
  if (contentWin) {
    mozilla::ErrorResult rv;
    nsRefPtr<nsGlobalWindow> window = static_cast<nsGlobalWindow*>(contentWin.get());
    nsRefPtr<mozilla::dom::BarProp> scrollbars = window->GetScrollbars(rv);
    if (scrollbars) {
      scrollbars->SetVisible(aVisible, rv);
    }
  }
}

bool nsXULWindow::GetContentScrollbarVisibility()
{
  
  
  
  
  nsCOMPtr<nsIScrollable> scroller(do_QueryInterface(mPrimaryContentShell));

  if (scroller) {
    int32_t prefValue;
    scroller->GetDefaultScrollbarPreferences(
                  nsIScrollable::ScrollOrientation_Y, &prefValue);
    if (prefValue == nsIScrollable::Scrollbar_Never) 
      scroller->GetDefaultScrollbarPreferences(
                  nsIScrollable::ScrollOrientation_X, &prefValue);

    if (prefValue == nsIScrollable::Scrollbar_Never)
      return false;
  }

  return true;
}


void nsXULWindow::PersistentAttributesDirty(uint32_t aDirtyFlags)
{
  mPersistentAttributesDirty |= aDirtyFlags & mPersistentAttributesMask;
}

NS_IMETHODIMP nsXULWindow::ApplyChromeFlags()
{
  nsCOMPtr<dom::Element> window = GetWindowDOMElement();
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

  if (mChromeLoaded) {
    
    
    
    

    
    SetContentScrollbarVisibility(mChromeFlags &
                                  nsIWebBrowserChrome::CHROME_SCROLLBARS ?
                                    true : false);
  }

  


  nsAutoString newvalue;

  if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_MENUBAR))
    newvalue.AppendLiteral("menubar ");

  if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_TOOLBAR))
    newvalue.AppendLiteral("toolbar ");

  if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_LOCATIONBAR))
    newvalue.AppendLiteral("location ");

  if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR))
    newvalue.AppendLiteral("directories ");

  if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_STATUSBAR))
    newvalue.AppendLiteral("status ");

  if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_EXTRA))
    newvalue.AppendLiteral("extrachrome ");

  
  
  ErrorResult rv;
  window->SetAttribute(NS_LITERAL_STRING("chromehidden"), newvalue, rv);

  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::GetXULBrowserWindow(nsIXULBrowserWindow * *aXULBrowserWindow)
{
  NS_IF_ADDREF(*aXULBrowserWindow = mXULBrowserWindow);
  return NS_OK;
}

NS_IMETHODIMP nsXULWindow::SetXULBrowserWindow(nsIXULBrowserWindow * aXULBrowserWindow)
{
  mXULBrowserWindow = aXULBrowserWindow;
  return NS_OK;
}





nsContentShellInfo::nsContentShellInfo(const nsAString& aID,
                                       nsIWeakReference* aContentShell)
  : id(aID),
    child(aContentShell)
{
  MOZ_COUNT_CTOR(nsContentShellInfo);
}

nsContentShellInfo::~nsContentShellInfo()
{
  MOZ_COUNT_DTOR(nsContentShellInfo);
   
} 
