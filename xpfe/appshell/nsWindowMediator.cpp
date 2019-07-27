




#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsTArray.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsAppShellWindowEnumerator.h"
#include "nsWindowMediator.h"
#include "nsIWindowMediatorListener.h"
#include "nsXPIDLString.h"
#include "nsGlobalWindow.h"

#include "nsIDocShell.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIXULWindow.h"

using namespace mozilla;

static bool notifyOpenWindow(nsIWindowMediatorListener *aElement, void* aData);
static bool notifyCloseWindow(nsIWindowMediatorListener *aElement, void* aData);
static bool notifyWindowTitleChange(nsIWindowMediatorListener *aElement, void* aData);


struct WindowTitleData {
  nsIXULWindow* mWindow;
  const char16_t *mTitle;
};

nsresult
nsWindowMediator::GetDOMWindow(nsIXULWindow* inWindow,
                               nsCOMPtr<nsIDOMWindow>& outDOMWindow)
{
  nsCOMPtr<nsIDocShell> docShell;

  outDOMWindow = nullptr;
  inWindow->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  outDOMWindow = docShell->GetWindow();
  return outDOMWindow ? NS_OK : NS_ERROR_FAILURE;
}

nsWindowMediator::nsWindowMediator() :
  mEnumeratorList(), mOldestWindow(nullptr), mTopmostWindow(nullptr),
  mTimeStamp(0), mSortingZOrder(false), mReady(false)
{
}

nsWindowMediator::~nsWindowMediator()
{
  while (mOldestWindow)
    UnregisterWindow(mOldestWindow);
}

nsresult nsWindowMediator::Init()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this, "xpcom-shutdown", true);
  NS_ENSURE_SUCCESS(rv, rv);

  mReady = true;
  return NS_OK;
}

NS_IMETHODIMP nsWindowMediator::RegisterWindow(nsIXULWindow* inWindow)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_STATE(mReady);

  if (GetInfoFor(inWindow)) {
    NS_ERROR("multiple window registration");
    return NS_ERROR_FAILURE;
  }

  mTimeStamp++;

  
  nsWindowInfo* windowInfo = new nsWindowInfo(inWindow, mTimeStamp);
  if (!windowInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  WindowTitleData winData = { inWindow, nullptr };
  mListeners.EnumerateForwards(notifyOpenWindow, &winData);
  
  if (mOldestWindow)
    windowInfo->InsertAfter(mOldestWindow->mOlder, nullptr);
  else
    mOldestWindow = windowInfo;

  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::UnregisterWindow(nsIXULWindow* inWindow)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_STATE(mReady);
  nsWindowInfo *info = GetInfoFor(inWindow);
  if (info)
    return UnregisterWindow(info);
  return NS_ERROR_INVALID_ARG;
}

nsresult
nsWindowMediator::UnregisterWindow(nsWindowInfo *inInfo)
{
  
  uint32_t index = 0;
  while (index < mEnumeratorList.Length()) {
    mEnumeratorList[index]->WindowRemoved(inInfo);
    index++;
  }
  
  WindowTitleData winData = { inInfo->mWindow.get(), nullptr };
  mListeners.EnumerateForwards(notifyCloseWindow, &winData);

  
  if (inInfo == mOldestWindow)
    mOldestWindow = inInfo->mYounger;
  if (inInfo == mTopmostWindow)
    mTopmostWindow = inInfo->mLower;
  inInfo->Unlink(true, true);
  if (inInfo == mOldestWindow)
    mOldestWindow = nullptr;
  if (inInfo == mTopmostWindow)
    mTopmostWindow = nullptr;
  delete inInfo;  

  return NS_OK;
}

nsWindowInfo*
nsWindowMediator::GetInfoFor(nsIXULWindow *aWindow)
{
  nsWindowInfo *info,
               *listEnd;

  if (!aWindow)
    return nullptr;

  info = mOldestWindow;
  listEnd = nullptr;
  while (info != listEnd) {
    if (info->mWindow.get() == aWindow)
      return info;
    info = info->mYounger;
    listEnd = mOldestWindow;
  }
  return nullptr;
}

nsWindowInfo*
nsWindowMediator::GetInfoFor(nsIWidget *aWindow)
{
  nsWindowInfo *info,
               *listEnd;

  if (!aWindow)
    return nullptr;

  info = mOldestWindow;
  listEnd = nullptr;

  nsCOMPtr<nsIWidget> scanWidget;
  while (info != listEnd) {
    nsCOMPtr<nsIBaseWindow> base(do_QueryInterface(info->mWindow));
    if (base)
      base->GetMainWidget(getter_AddRefs(scanWidget));
    if (aWindow == scanWidget.get())
      return info;
    info = info->mYounger;
    listEnd = mOldestWindow;
  }
  return nullptr;
}

NS_IMETHODIMP
nsWindowMediator::GetEnumerator(const char16_t* inType, nsISimpleEnumerator** outEnumerator)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG_POINTER(outEnumerator);
  NS_ENSURE_STATE(mReady);

  nsRefPtr<nsAppShellWindowEnumerator> enumerator = new nsASDOMWindowEarlyToLateEnumerator(inType, *this);
  enumerator.forget(outEnumerator);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::GetXULWindowEnumerator(const char16_t* inType, nsISimpleEnumerator** outEnumerator)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG_POINTER(outEnumerator);
  NS_ENSURE_STATE(mReady);

  nsRefPtr<nsAppShellWindowEnumerator> enumerator = new nsASXULWindowEarlyToLateEnumerator(inType, *this);
  enumerator.forget(outEnumerator);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::GetZOrderDOMWindowEnumerator(
            const char16_t *aWindowType, bool aFrontToBack,
            nsISimpleEnumerator **_retval)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mReady);

  nsRefPtr<nsAppShellWindowEnumerator> enumerator;
  if (aFrontToBack)
    enumerator = new nsASDOMWindowFrontToBackEnumerator(aWindowType, *this);
  else
    enumerator = new nsASDOMWindowBackToFrontEnumerator(aWindowType, *this);

  enumerator.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::GetZOrderXULWindowEnumerator(
            const char16_t *aWindowType, bool aFrontToBack,
            nsISimpleEnumerator **_retval)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mReady);

  nsRefPtr<nsAppShellWindowEnumerator> enumerator;
  if (aFrontToBack)
    enumerator = new nsASXULWindowFrontToBackEnumerator(aWindowType, *this);
  else
    enumerator = new nsASXULWindowBackToFrontEnumerator(aWindowType, *this);

  enumerator.forget(_retval);
  return NS_OK;
}

int32_t
nsWindowMediator::AddEnumerator(nsAppShellWindowEnumerator * inEnumerator)
{
  return mEnumeratorList.AppendElement(inEnumerator) != nullptr;
}

int32_t
nsWindowMediator::RemoveEnumerator(nsAppShellWindowEnumerator * inEnumerator)
{
  return mEnumeratorList.RemoveElement(inEnumerator);
}



NS_IMETHODIMP
nsWindowMediator::GetMostRecentWindow(const char16_t* inType, nsIDOMWindow** outWindow)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG_POINTER(outWindow);
  *outWindow = nullptr;
  if (!mReady)
    return NS_OK;

  
  
  nsWindowInfo *info = MostRecentWindowInfo(inType);
  if (info && info->mWindow) {
    nsCOMPtr<nsIDOMWindow> DOMWindow;
    if (NS_SUCCEEDED(GetDOMWindow(info->mWindow, DOMWindow))) {  
      *outWindow = DOMWindow;
      NS_ADDREF(*outWindow);
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsWindowInfo*
nsWindowMediator::MostRecentWindowInfo(const char16_t* inType)
{
  int32_t       lastTimeStamp = -1;
  nsAutoString  typeString(inType);
  bool          allWindows = !inType || typeString.IsEmpty();

  
  
  nsWindowInfo *searchInfo,
               *listEnd,
               *foundInfo = nullptr;

  searchInfo = mOldestWindow;
  listEnd = nullptr;
  while (searchInfo != listEnd) {
    if ((allWindows || searchInfo->TypeEquals(typeString)) &&
        searchInfo->mTimeStamp >= lastTimeStamp) {

      foundInfo = searchInfo;
      lastTimeStamp = searchInfo->mTimeStamp;
    }
    searchInfo = searchInfo->mYounger;
    listEnd = mOldestWindow;
  }
  return foundInfo;
}

NS_IMETHODIMP
nsWindowMediator::GetOuterWindowWithId(uint64_t aWindowID,
                                       nsIDOMWindow** aWindow)
{
  *aWindow = nsGlobalWindow::GetOuterWindowWithId(aWindowID);
  NS_IF_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::GetCurrentInnerWindowWithId(uint64_t aWindowID,
                                              nsIDOMWindow** aWindow)
{
  nsCOMPtr<nsPIDOMWindow> inner = nsGlobalWindow::GetInnerWindowWithId(aWindowID);

  
  if (!inner)
    return NS_OK;

  nsCOMPtr<nsPIDOMWindow> outer = inner->GetOuterWindow();
  NS_ENSURE_TRUE(outer, NS_ERROR_UNEXPECTED);

  
  if (outer->GetCurrentInnerWindow() != inner)
    return NS_OK;

  nsCOMPtr<nsIDOMWindow> ret = do_QueryInterface(outer);
  ret.forget(aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::UpdateWindowTimeStamp(nsIXULWindow* inWindow)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_STATE(mReady);
  nsWindowInfo *info = GetInfoFor(inWindow);
  if (info) {
    
    info->mTimeStamp = ++mTimeStamp;
    return NS_OK;
  }
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
nsWindowMediator::UpdateWindowTitle(nsIXULWindow* inWindow,
                                    const char16_t* inTitle)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_STATE(mReady);
  if (GetInfoFor(inWindow)) {
    WindowTitleData winData = { inWindow, inTitle };
    mListeners.EnumerateForwards(notifyWindowTitleChange, &winData);
  }

  return NS_OK;
}







NS_IMETHODIMP
nsWindowMediator::CalculateZPosition(
                nsIXULWindow   *inWindow,
                uint32_t        inPosition,
                nsIWidget      *inBelow,
                uint32_t       *outPosition,
                nsIWidget     **outBelow,
                bool           *outAltered)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG_POINTER(outBelow);
  NS_ENSURE_STATE(mReady);

  *outBelow = nullptr;

  if (!inWindow || !outPosition || !outAltered)
    return NS_ERROR_NULL_POINTER;

  if (inPosition != nsIWindowMediator::zLevelTop &&
      inPosition != nsIWindowMediator::zLevelBottom &&
      inPosition != nsIWindowMediator::zLevelBelow)
    return NS_ERROR_INVALID_ARG;

  nsWindowInfo *info = mTopmostWindow;
  nsIXULWindow *belowWindow = nullptr;
  bool          found = false;
  nsresult      result = NS_OK;

  *outPosition = inPosition;
  *outAltered = false;

  if (mSortingZOrder) { 
    *outBelow = inBelow;
    NS_IF_ADDREF(*outBelow);
    return NS_OK;
  }

  uint32_t inZ;
  GetZLevel(inWindow, &inZ);

  if (inPosition == nsIWindowMediator::zLevelBelow) {
    
    
    info = GetInfoFor(inBelow);
    if (!info || (info->mYounger != info && info->mLower == info))
      info = mTopmostWindow;
    else
      found = true;

    if (!found) {
      




      inPosition = nsIWindowMediator::zLevelTop;
    }
  }

  if (inPosition == nsIWindowMediator::zLevelTop) {
    if (mTopmostWindow && mTopmostWindow->mZLevel > inZ) {
      
      do {
        if (info->mZLevel <= inZ)
          break;
        info = info->mLower;
      } while (info != mTopmostWindow);

      *outPosition = nsIWindowMediator::zLevelBelow;
      belowWindow = info->mHigher->mWindow;
      *outAltered = true;
    }
  } else if (inPosition == nsIWindowMediator::zLevelBottom) {
    if (mTopmostWindow && mTopmostWindow->mHigher->mZLevel < inZ) {
      
      do {
        info = info->mHigher;
        if (info->mZLevel >= inZ)
          break;
      } while (info != mTopmostWindow);

      *outPosition = nsIWindowMediator::zLevelBelow;
      belowWindow = info->mWindow;
      *outAltered = true;
    }
  } else {
    unsigned long relativeZ;

    
    if (found) {
      belowWindow = info->mWindow;
      relativeZ = info->mZLevel;
      if (relativeZ > inZ) {
        
        if (info->mLower != info && info->mLower->mZLevel > inZ) {
          do {
            if (info->mZLevel <= inZ)
              break;
            info = info->mLower;
          } while (info != mTopmostWindow);

          belowWindow = info->mHigher->mWindow;
          *outAltered = true;
        }
      } else if (relativeZ < inZ) {
        
        do {
          info = info->mHigher;
          if (info->mZLevel >= inZ)
            break;
        } while (info != mTopmostWindow);

        if (info->mZLevel >= inZ)
          belowWindow = info->mWindow;
        else
          *outPosition = nsIWindowMediator::zLevelTop;
        *outAltered = true;
      } 
    }
  }

  if (NS_SUCCEEDED(result) && belowWindow) {
    nsCOMPtr<nsIBaseWindow> base(do_QueryInterface(belowWindow));
    if (base)
      base->GetMainWidget(outBelow);
    else
      result = NS_ERROR_NO_INTERFACE;
  }

  return result;
}

NS_IMETHODIMP
nsWindowMediator::SetZPosition(
                nsIXULWindow *inWindow,
                uint32_t      inPosition,
                nsIXULWindow *inBelow)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  nsWindowInfo *inInfo,
               *belowInfo;

  if ((inPosition != nsIWindowMediator::zLevelTop &&
       inPosition != nsIWindowMediator::zLevelBottom &&
       inPosition != nsIWindowMediator::zLevelBelow) ||
      !inWindow) {
    return NS_ERROR_INVALID_ARG;
  }

  if (mSortingZOrder) 
    return NS_OK;

  NS_ENSURE_STATE(mReady);

  



  inInfo = GetInfoFor(inWindow);
  if (!inInfo)
    return NS_ERROR_INVALID_ARG;

  
  if (inPosition == nsIWindowMediator::zLevelBelow) {
    belowInfo = GetInfoFor(inBelow);
    
    if (belowInfo &&
        belowInfo->mYounger != belowInfo && belowInfo->mLower == belowInfo) {
      belowInfo = nullptr;
    }
    if (!belowInfo) {
      if (inBelow)
        return NS_ERROR_INVALID_ARG;
      else
        inPosition = nsIWindowMediator::zLevelTop;
    }
  }
  if (inPosition == nsIWindowMediator::zLevelTop ||
      inPosition == nsIWindowMediator::zLevelBottom)
    belowInfo = mTopmostWindow ? mTopmostWindow->mHigher : nullptr;

  if (inInfo != belowInfo) {
    inInfo->Unlink(false, true);
    inInfo->InsertAfter(nullptr, belowInfo);
  }
  if (inPosition == nsIWindowMediator::zLevelTop)
    mTopmostWindow = inInfo;

  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::GetZLevel(nsIXULWindow *aWindow, uint32_t *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsIXULWindow::normalZ;
  nsWindowInfo *info = GetInfoFor(aWindow);
  if (info) {
    *_retval = info->mZLevel;
  } else {
    NS_WARNING("getting z level of unregistered window");
    
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::SetZLevel(nsIXULWindow *aWindow, uint32_t aZLevel)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  NS_ENSURE_STATE(mReady);

  nsWindowInfo *info = GetInfoFor(aWindow);
  NS_ASSERTION(info, "setting z level of unregistered window");
  if (!info)
    return NS_ERROR_FAILURE;

  if (info->mZLevel != aZLevel) {
    bool lowered = info->mZLevel > aZLevel;
    info->mZLevel = aZLevel;
    if (lowered)
      SortZOrderFrontToBack();
    else
      SortZOrderBackToFront();
  }
  return NS_OK;
}













void
nsWindowMediator::SortZOrderFrontToBack()
{
  nsWindowInfo *scan,   
               *search, 
               *prev,   
               *lowest; 
  bool         finished;

  if (!mTopmostWindow) 
    return;            

  mSortingZOrder = true;

  

  do {
    finished = true;
    lowest = mTopmostWindow->mHigher;
    scan = mTopmostWindow;
    while (scan != lowest) {
      uint32_t scanZ = scan->mZLevel;
      if (scanZ < scan->mLower->mZLevel) { 
        search = scan->mLower;
        do {
          prev = search;
          search = search->mLower;
        } while (prev != lowest && scanZ < search->mZLevel);

        
        if (scan == mTopmostWindow)
          mTopmostWindow = scan->mLower;
        scan->Unlink(false, true);
        scan->InsertAfter(nullptr, prev);

        
        nsCOMPtr<nsIBaseWindow> base;
        nsCOMPtr<nsIWidget> scanWidget;
        nsCOMPtr<nsIWidget> prevWidget;
        base = do_QueryInterface(scan->mWindow);
        if (base)
          base->GetMainWidget(getter_AddRefs(scanWidget));
        base = do_QueryInterface(prev->mWindow);
        if (base)
          base->GetMainWidget(getter_AddRefs(prevWidget));
        if (scanWidget)
          scanWidget->PlaceBehind(eZPlacementBelow, prevWidget, false);

        finished = false;
        break;
      }
      scan = scan->mLower;
    }
  } while (!finished);

  mSortingZOrder = false;
}


void
nsWindowMediator::SortZOrderBackToFront()
{
  nsWindowInfo *scan,   
               *search, 
               *lowest; 
  bool         finished;

  if (!mTopmostWindow) 
    return;            

  mSortingZOrder = true;

  

  do {
    finished = true;
    lowest = mTopmostWindow->mHigher;
    scan = lowest;
    while (scan != mTopmostWindow) {
      uint32_t scanZ = scan->mZLevel;
      if (scanZ > scan->mHigher->mZLevel) { 
        search = scan;
        do {
          search = search->mHigher;
        } while (search != lowest && scanZ > search->mZLevel);

        
        if (scan != search && scan != search->mLower) {
          scan->Unlink(false, true);
          scan->InsertAfter(nullptr, search);
        }
        if (search == lowest)
          mTopmostWindow = scan;

        
        nsCOMPtr<nsIBaseWindow> base;
        nsCOMPtr<nsIWidget> scanWidget;
        nsCOMPtr<nsIWidget> searchWidget;
        base = do_QueryInterface(scan->mWindow);
        if (base)
          base->GetMainWidget(getter_AddRefs(scanWidget));
        if (mTopmostWindow != scan) {
          base = do_QueryInterface(search->mWindow);
          if (base)
            base->GetMainWidget(getter_AddRefs(searchWidget));
        }
        if (scanWidget)
          scanWidget->PlaceBehind(eZPlacementBelow, searchWidget, false);
        finished = false;
        break;
      }
      scan = scan->mHigher;
    }
  } while (!finished);

  mSortingZOrder = false;
}

NS_IMPL_ISUPPORTS(nsWindowMediator,
  nsIWindowMediator,
  nsIObserver,
  nsISupportsWeakReference)

NS_IMETHODIMP
nsWindowMediator::AddListener(nsIWindowMediatorListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  
  mListeners.AppendObject(aListener);
  
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::RemoveListener(nsIWindowMediatorListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  mListeners.RemoveObject(aListener);
  
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMediator::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const char16_t* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown") && mReady) {
    MOZ_RELEASE_ASSERT(NS_IsMainThread());
    while (mOldestWindow)
      UnregisterWindow(mOldestWindow);
    mReady = false;
  }
  return NS_OK;
}

bool
notifyOpenWindow(nsIWindowMediatorListener *aListener, void* aData)
{
  WindowTitleData* winData = static_cast<WindowTitleData*>(aData);
  aListener->OnOpenWindow(winData->mWindow);

  return true;
}

bool
notifyCloseWindow(nsIWindowMediatorListener *aListener, void* aData)
{
  WindowTitleData* winData = static_cast<WindowTitleData*>(aData);
  aListener->OnCloseWindow(winData->mWindow);

  return true;
}

bool 
notifyWindowTitleChange(nsIWindowMediatorListener *aListener, void* aData)
{
  WindowTitleData* titleData = reinterpret_cast<WindowTitleData*>(aData);
  aListener->OnWindowTitleChange(titleData->mWindow, titleData->mTitle);

  return true;
}
