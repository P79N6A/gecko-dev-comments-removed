





#include "base/basictypes.h"

#include "TabParent.h"

#include "IDBFactory.h"
#include "IndexedDBParent.h"
#include "mozIApplication.h"
#include "mozilla/BrowserElementParent.h"
#include "mozilla/docshell/OfflineCacheUpdateParent.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/Hal.h"
#include "mozilla/ipc/DocumentRendererParent.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layout/RenderFrameParent.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/unused.h"
#include "nsCOMPtr.h"
#include "nsContentPermissionHelper.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsEventStateManager.h"
#include "nsFocusManager.h"
#include "nsFrameLoader.h"
#include "nsIContent.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDialogCreator.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPromptFactory.h"
#include "nsIURI.h"
#include "nsIWebBrowserChrome.h"
#include "nsIXULBrowserWindow.h"
#include "nsIXULWindow.h"
#include "nsViewManager.h"
#include "nsIWidget.h"
#include "nsIWindowWatcher.h"
#include "nsPIDOMWindow.h"
#include "nsPrintfCString.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "private/pprio.h"
#include "StructuredCloneUtils.h"
#include "JavaScriptParent.h"
#include "TabChild.h"
#include "nsNetCID.h"
#include <algorithm>

using namespace mozilla::dom;
using namespace mozilla::ipc;
using namespace mozilla::layers;
using namespace mozilla::layout;
using namespace mozilla::services;
using namespace mozilla::widget;
using namespace mozilla::dom::indexedDB;
using namespace mozilla::jsipc;



#define NOTIFY_FLAG_SHIFT 16

class OpenFileAndSendFDRunnable : public nsRunnable
{
    const nsString mPath;
    nsRefPtr<TabParent> mTabParent;
    nsCOMPtr<nsIEventTarget> mEventTarget;
    PRFileDesc* mFD;

public:
    OpenFileAndSendFDRunnable(const nsAString& aPath, TabParent* aTabParent)
      : mPath(aPath), mTabParent(aTabParent), mFD(nullptr)
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(!aPath.IsEmpty());
        MOZ_ASSERT(aTabParent);
    }

    void Dispatch()
    {
        MOZ_ASSERT(NS_IsMainThread());

        mEventTarget = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
        NS_ENSURE_TRUE_VOID(mEventTarget);

        nsresult rv = mEventTarget->Dispatch(this, NS_DISPATCH_NORMAL);
        NS_ENSURE_SUCCESS_VOID(rv);
    }

private:
    ~OpenFileAndSendFDRunnable()
    {
        MOZ_ASSERT(!mFD);
    }

    
    
    NS_IMETHOD Run()
    {
        if (NS_IsMainThread()) {
            SendResponse();
        } else if (mFD) {
            CloseFile();
        } else {
            OpenFile();
        }

        return NS_OK;
    }

    void SendResponse()
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(mTabParent);
        MOZ_ASSERT(mEventTarget);
        MOZ_ASSERT(mFD);

        nsRefPtr<TabParent> tabParent;
        mTabParent.swap(tabParent);

        FileDescriptor::PlatformHandleType handle =
            FileDescriptor::PlatformHandleType(PR_FileDesc2NativeHandle(mFD));

        mozilla::unused << tabParent->SendCacheFileDescriptor(mPath, handle);

        nsCOMPtr<nsIEventTarget> eventTarget;
        mEventTarget.swap(eventTarget);

        if (NS_FAILED(eventTarget->Dispatch(this, NS_DISPATCH_NORMAL))) {
            NS_WARNING("Failed to dispatch to stream transport service!");

            
            
            CloseFile();
        }
    }

    void OpenFile()
    {
        MOZ_ASSERT(!NS_IsMainThread());
        MOZ_ASSERT(!mFD);

        nsCOMPtr<nsIFile> file;
        nsresult rv = NS_NewLocalFile(mPath, false, getter_AddRefs(file));
        NS_ENSURE_SUCCESS_VOID(rv);

        PRFileDesc* fd;
        rv = file->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);
        NS_ENSURE_SUCCESS_VOID(rv);

        mFD = fd;

        if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
            NS_WARNING("Failed to dispatch to main thread!");

            CloseFile();
        }
    }

    void CloseFile()
    {
        
        
        

        MOZ_ASSERT(mFD);

        PR_Close(mFD);
        mFD = nullptr;
    }
};

namespace mozilla {
namespace dom {

TabParent* sEventCapturer;

TabParent *TabParent::mIMETabParent = nullptr;

NS_IMPL_ISUPPORTS3(TabParent, nsITabParent, nsIAuthPromptProvider, nsISecureBrowserUI)

TabParent::TabParent(ContentParent* aManager, const TabContext& aContext)
  : TabContext(aContext)
  , mFrameElement(NULL)
  , mIMESelectionAnchor(0)
  , mIMESelectionFocus(0)
  , mIMEComposing(false)
  , mIMECompositionEnding(false)
  , mIMECompositionStart(0)
  , mIMESeqno(0)
  , mEventCaptureDepth(0)
  , mRect(0, 0, 0, 0)
  , mDimensions(0, 0)
  , mOrientation(0)
  , mDPI(0)
  , mDefaultScale(0)
  , mShown(false)
  , mUpdatedDimensions(false)
  , mManager(aManager)
  , mMarkedDestroying(false)
  , mIsDestroyed(false)
  , mAppPackageFileDescriptorSent(false)
{
}

TabParent::~TabParent()
{
}

void
TabParent::SetOwnerElement(Element* aElement)
{
  mFrameElement = aElement;
  TryCacheDPIAndScale();
}

void
TabParent::GetAppType(nsAString& aOut)
{
  aOut.Truncate();
  nsCOMPtr<Element> elem = do_QueryInterface(mFrameElement);
  if (!elem) {
    return;
  }

  elem->GetAttr(kNameSpaceID_None, nsGkAtoms::mozapptype, aOut);
}

bool
TabParent::IsVisible()
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  if (!frameLoader) {
    return false;
  }

  bool visible = false;
  frameLoader->GetVisible(&visible);
  return visible;
}

void
TabParent::Destroy()
{
  if (mIsDestroyed) {
    return;
  }

  
  
  
  unused << SendDestroy();

  const InfallibleTArray<PIndexedDBParent*>& idbParents =
    ManagedPIndexedDBParent();
  for (uint32_t i = 0; i < idbParents.Length(); ++i) {
    static_cast<IndexedDBParent*>(idbParents[i])->Disconnect();
  }

  if (RenderFrameParent* frame = GetRenderFrame()) {
    frame->Destroy();
  }
  mIsDestroyed = true;

  Manager()->NotifyTabDestroying(this);
  mMarkedDestroying = true;
}

bool
TabParent::Recv__delete__()
{
  Manager()->NotifyTabDestroyed(this, mMarkedDestroying);
  return true;
}

void
TabParent::ActorDestroy(ActorDestroyReason why)
{
  if (sEventCapturer == this) {
    sEventCapturer = nullptr;
  }
  if (mIMETabParent == this) {
    mIMETabParent = nullptr;
  }
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (frameLoader) {
    nsCOMPtr<Element> frameElement(mFrameElement);
    ReceiveMessage(CHILD_PROCESS_SHUTDOWN_MESSAGE, false, nullptr, nullptr);
    frameLoader->DestroyChild();

    if (why == AbnormalShutdown && os) {
      os->NotifyObservers(NS_ISUPPORTS_CAST(nsIFrameLoader*, frameLoader),
                          "oop-frameloader-crashed", nullptr);
      nsContentUtils::DispatchTrustedEvent(frameElement->OwnerDoc(), frameElement,
                                           NS_LITERAL_STRING("oop-browser-crashed"),
                                           true, true);
    }
  }

  if (os) {
    os->NotifyObservers(NS_ISUPPORTS_CAST(nsITabParent*, this), "ipc:browser-destroyed", nullptr);
  }
}

bool
TabParent::RecvMoveFocus(const bool& aForward)
{
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMElement> dummy;
    uint32_t type = aForward ? uint32_t(nsIFocusManager::MOVEFOCUS_FORWARD)
                             : uint32_t(nsIFocusManager::MOVEFOCUS_BACKWARD);
    nsCOMPtr<nsIDOMElement> frame = do_QueryInterface(mFrameElement);
    fm->MoveFocus(nullptr, frame, type, nsIFocusManager::FLAG_BYKEY,
                  getter_AddRefs(dummy));
  }
  return true;
}

bool
TabParent::RecvEvent(const RemoteDOMEvent& aEvent)
{
  nsCOMPtr<nsIDOMEvent> event = do_QueryInterface(aEvent.mEvent);
  NS_ENSURE_TRUE(event, true);

  nsCOMPtr<mozilla::dom::EventTarget> target = do_QueryInterface(mFrameElement);
  NS_ENSURE_TRUE(target, true);

  event->SetOwner(target);

  bool dummy;
  target->DispatchEvent(event, &dummy);
  return true;
}

bool
TabParent::AnswerCreateWindow(PBrowserParent** retval)
{
    if (!mBrowserDOMWindow) {
        return false;
    }

    
    if (IsBrowserOrApp()) {
        return false;
    }

    
    
    nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner;
    mBrowserDOMWindow->OpenURIInFrame(nullptr, nullptr,
                                      nsIBrowserDOMWindow::OPEN_NEWTAB,
                                      nsIBrowserDOMWindow::OPEN_NEW,
                                      getter_AddRefs(frameLoaderOwner));
    if (!frameLoaderOwner) {
        return false;
    }

    nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
    if (!frameLoader) {
        return false;
    }

    *retval = frameLoader->GetRemoteBrowser();
    return true;
}

void
TabParent::LoadURL(nsIURI* aURI)
{
    MOZ_ASSERT(aURI);

    if (mIsDestroyed) {
        return;
    }

    nsCString spec;
    aURI->GetSpec(spec);

    if (!mShown) {
        NS_WARNING(nsPrintfCString("TabParent::LoadURL(%s) called before "
                                   "Show(). Ignoring LoadURL.\n",
                                   spec.get()).get());
        return;
    }

    unused << SendLoadURL(spec);

    
    
    
    if (!mAppPackageFileDescriptorSent) {
        mAppPackageFileDescriptorSent = true;

        nsCOMPtr<mozIApplication> app = GetOwnOrContainingApp();
        if (app) {
            nsString manifestURL;
            nsresult rv = app->GetManifestURL(manifestURL);
            NS_ENSURE_SUCCESS_VOID(rv);

            if (StringBeginsWith(manifestURL, NS_LITERAL_STRING("app:"))) {
                nsString basePath;
                rv = app->GetBasePath(basePath);
                NS_ENSURE_SUCCESS_VOID(rv);

                nsString appId;
                rv = app->GetId(appId);
                NS_ENSURE_SUCCESS_VOID(rv);

                nsCOMPtr<nsIFile> packageFile;
                rv = NS_NewLocalFile(basePath, false,
                                     getter_AddRefs(packageFile));
                NS_ENSURE_SUCCESS_VOID(rv);

                rv = packageFile->Append(appId);
                NS_ENSURE_SUCCESS_VOID(rv);

                rv = packageFile->Append(NS_LITERAL_STRING("application.zip"));
                NS_ENSURE_SUCCESS_VOID(rv);

                nsString path;
                rv = packageFile->GetPath(path);
                NS_ENSURE_SUCCESS_VOID(rv);

                nsRefPtr<OpenFileAndSendFDRunnable> openFileRunnable =
                    new OpenFileAndSendFDRunnable(path, this);
                openFileRunnable->Dispatch();
            }
        }
    }
}

void
TabParent::Show(const nsIntSize& size)
{
    
    mShown = true;
    mDimensions = size;
    if (!mIsDestroyed) {
      unused << SendShow(size);
    }
}

void
TabParent::UpdateDimensions(const nsRect& rect, const nsIntSize& size)
{
  if (mIsDestroyed) {
    return;
  }
  hal::ScreenConfiguration config;
  hal::GetCurrentScreenConfiguration(&config);
  ScreenOrientation orientation = config.orientation();

  if (!mUpdatedDimensions || mOrientation != orientation ||
      mDimensions != size || !mRect.IsEqualEdges(rect)) {
    mUpdatedDimensions = true;
    mRect = rect;
    mDimensions = size;
    mOrientation = orientation;

    unused << SendUpdateDimensions(mRect, mDimensions, mOrientation);
    if (RenderFrameParent* rfp = GetRenderFrame()) {
      rfp->NotifyDimensionsChanged(ScreenIntSize::FromUnknownSize(
        gfx::IntSize(mDimensions.width, mDimensions.height)));
    }
  }
}

void
TabParent::UpdateFrame(const FrameMetrics& aFrameMetrics)
{
  if (!mIsDestroyed) {
    unused << SendUpdateFrame(aFrameMetrics);
  }
}

void TabParent::HandleDoubleTap(const CSSIntPoint& aPoint)
{
  if (!mIsDestroyed) {
    unused << SendHandleDoubleTap(aPoint);
  }
}

void TabParent::HandleSingleTap(const CSSIntPoint& aPoint)
{
  if (!mIsDestroyed) {
    unused << SendHandleSingleTap(aPoint);
  }
}

void TabParent::HandleLongTap(const CSSIntPoint& aPoint)
{
  if (!mIsDestroyed) {
    unused << SendHandleLongTap(aPoint);
  }
}

void
TabParent::Activate()
{
  if (!mIsDestroyed) {
    unused << SendActivate();
  }
}

void
TabParent::Deactivate()
{
  if (!mIsDestroyed) {
    unused << SendDeactivate();
  }
}

NS_IMETHODIMP
TabParent::Init(nsIDOMWindow *window)
{
  return NS_OK;
}

NS_IMETHODIMP
TabParent::GetState(uint32_t *aState)
{
  NS_ENSURE_ARG(aState);
  NS_WARNING("SecurityState not valid here");
  *aState = 0;
  return NS_OK;
}

NS_IMETHODIMP
TabParent::SetDocShell(nsIDocShell *aDocShell)
{
  NS_ENSURE_ARG(aDocShell);
  NS_WARNING("No mDocShell member in TabParent so there is no docShell to set");
  return NS_OK;
}

PDocumentRendererParent*
TabParent::AllocPDocumentRendererParent(const nsRect& documentRect,
                                        const gfxMatrix& transform,
                                        const nsString& bgcolor,
                                        const uint32_t& renderFlags,
                                        const bool& flushLayout,
                                        const nsIntSize& renderSize)
{
    return new DocumentRendererParent();
}

bool
TabParent::DeallocPDocumentRendererParent(PDocumentRendererParent* actor)
{
    delete actor;
    return true;
}

PContentPermissionRequestParent*
TabParent::AllocPContentPermissionRequestParent(const nsCString& type, const nsCString& access, const IPC::Principal& principal)
{
  return new ContentPermissionRequestParent(type, access, mFrameElement, principal);
}

bool
TabParent::DeallocPContentPermissionRequestParent(PContentPermissionRequestParent* actor)
{
  delete actor;
  return true;
}

void
TabParent::SendMouseEvent(const nsAString& aType, float aX, float aY,
                          int32_t aButton, int32_t aClickCount,
                          int32_t aModifiers, bool aIgnoreRootScrollFrame)
{
  if (!mIsDestroyed) {
    unused << PBrowserParent::SendMouseEvent(nsString(aType), aX, aY,
                                             aButton, aClickCount,
                                             aModifiers, aIgnoreRootScrollFrame);
  }
}

void
TabParent::SendKeyEvent(const nsAString& aType,
                        int32_t aKeyCode,
                        int32_t aCharCode,
                        int32_t aModifiers,
                        bool aPreventDefault)
{
  if (!mIsDestroyed) {
    unused << PBrowserParent::SendKeyEvent(nsString(aType), aKeyCode, aCharCode,
                                           aModifiers, aPreventDefault);
  }
}

bool
TabParent::MapEventCoordinatesForChildProcess(nsEvent* aEvent)
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  if (!frameLoader) {
    return false;
  }
  LayoutDeviceIntPoint offset = nsEventStateManager::GetChildProcessOffset(frameLoader, *aEvent);
  MapEventCoordinatesForChildProcess(offset, aEvent);
  return true;
}

void
TabParent::MapEventCoordinatesForChildProcess(
  const LayoutDeviceIntPoint& aOffset, nsEvent* aEvent)
{
  if (aEvent->eventStructType != NS_TOUCH_EVENT) {
    aEvent->refPoint = aOffset;
  } else {
    aEvent->refPoint = LayoutDeviceIntPoint();
    nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
    
    
    const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      Touch* touch = touches[i];
      if (touch) {
        touch->mRefPoint += LayoutDeviceIntPoint::ToUntyped(aOffset);
      }
    }
  }
}

bool TabParent::SendRealMouseEvent(nsMouseEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  nsMouseEvent e(event);
  MaybeForwardEventToRenderFrame(event, &e);
  if (!MapEventCoordinatesForChildProcess(&e)) {
    return false;
  }
  return PBrowserParent::SendRealMouseEvent(e);
}

bool TabParent::SendMouseWheelEvent(WheelEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  WheelEvent e(event);
  MaybeForwardEventToRenderFrame(event, &e);
  if (!MapEventCoordinatesForChildProcess(&e)) {
    return false;
  }
  return PBrowserParent::SendMouseWheelEvent(event);
}

bool TabParent::SendRealKeyEvent(nsKeyEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  nsKeyEvent e(event);
  MaybeForwardEventToRenderFrame(event, &e);
  if (!MapEventCoordinatesForChildProcess(&e)) {
    return false;
  }
  return PBrowserParent::SendRealKeyEvent(e);
}

bool TabParent::SendRealTouchEvent(nsTouchEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  if (event.message == NS_TOUCH_START) {
    
    nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
    if (!frameLoader) {
      
      sEventCapturer = nullptr;
      return false;
    }

    mChildProcessOffsetAtTouchStart =
        nsEventStateManager::GetChildProcessOffset(frameLoader,
                                                   event);

    MOZ_ASSERT((!sEventCapturer && mEventCaptureDepth == 0) ||
               (sEventCapturer == this && mEventCaptureDepth > 0));
    
    
    sEventCapturer = this;
    ++mEventCaptureDepth;
  }

  nsTouchEvent e(event);
  
  
  
  if (event.message == NS_TOUCH_END || event.message == NS_TOUCH_CANCEL) {
    for (int i = e.touches.Length() - 1; i >= 0; i--) {
      if (!e.touches[i]->mChanged) {
        e.touches.RemoveElementAt(i);
      }
    }
  }

  MaybeForwardEventToRenderFrame(event, &e);

  MapEventCoordinatesForChildProcess(mChildProcessOffsetAtTouchStart, &e);

  return (e.message == NS_TOUCH_MOVE) ?
    PBrowserParent::SendRealTouchMoveEvent(e) :
    PBrowserParent::SendRealTouchEvent(e);
}

 TabParent*
TabParent::GetEventCapturer()
{
  return sEventCapturer;
}

bool
TabParent::TryCapture(const nsGUIEvent& aEvent)
{
  MOZ_ASSERT(sEventCapturer == this && mEventCaptureDepth > 0);

  if (aEvent.eventStructType != NS_TOUCH_EVENT) {
    
    return false;
  }

  nsTouchEvent event(static_cast<const nsTouchEvent&>(aEvent));

  bool isTouchPointUp = (event.message == NS_TOUCH_END ||
                         event.message == NS_TOUCH_CANCEL);
  if (event.message == NS_TOUCH_START || isTouchPointUp) {
    
    
    if (isTouchPointUp && 0 == --mEventCaptureDepth) {
      
      
      sEventCapturer = nullptr;
    }
    return false;
  }

  SendRealTouchEvent(event);
  return true;
}

bool
TabParent::RecvSyncMessage(const nsString& aMessage,
                           const ClonedMessageData& aData,
                           const InfallibleTArray<CpowEntry>& aCpows,
                           InfallibleTArray<nsString>* aJSONRetVal)
{
  StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
  CpowIdHolder cpows(static_cast<ContentParent*>(Manager())->GetCPOWManager(), aCpows);
  return ReceiveMessage(aMessage, true, &cloneData, &cpows, aJSONRetVal);
}

bool
TabParent::RecvAsyncMessage(const nsString& aMessage,
                            const ClonedMessageData& aData,
                            const InfallibleTArray<CpowEntry>& aCpows)
{
  StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
  CpowIdHolder cpows(static_cast<ContentParent*>(Manager())->GetCPOWManager(), aCpows);
  return ReceiveMessage(aMessage, false, &cloneData, &cpows, nullptr);
}

bool
TabParent::RecvSetCursor(const uint32_t& aCursor)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (widget) {
    widget->SetCursor((nsCursor) aCursor);
  }
  return true;
}

bool
TabParent::RecvSetBackgroundColor(const nscolor& aColor)
{
  if (RenderFrameParent* frame = GetRenderFrame()) {
    frame->SetBackgroundColor(aColor);
  }
  return true;
}

bool
TabParent::RecvSetStatus(const uint32_t& aType, const nsString& aStatus)
{
  nsCOMPtr<nsIContent> frame = do_QueryInterface(mFrameElement);
  if (frame) {
    nsCOMPtr<nsISupports> container = frame->OwnerDoc()->GetContainer();
    if (!container)
      return true;
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
    if (!docShell)
      return true;
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    docShell->GetTreeOwner(getter_AddRefs(treeOwner));
    if (!treeOwner)
      return true;

    nsCOMPtr<nsIXULWindow> window = do_GetInterface(treeOwner);
    if (window) {
      nsCOMPtr<nsIXULBrowserWindow> xulBrowserWindow;
      window->GetXULBrowserWindow(getter_AddRefs(xulBrowserWindow));
      if (xulBrowserWindow) {
        switch (aType)
        {
        case nsIWebBrowserChrome::STATUS_SCRIPT:
          xulBrowserWindow->SetJSStatus(aStatus);
          break;
        case nsIWebBrowserChrome::STATUS_LINK:
          xulBrowserWindow->SetOverLink(aStatus, nullptr);
          break;
        }
      }
    }
  }
  return true;
}

bool
TabParent::RecvNotifyIMEFocus(const bool& aFocus,
                              nsIMEUpdatePreference* aPreference,
                              uint32_t* aSeqno)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget) {
    aPreference->mWantUpdates = nsIMEUpdatePreference::NOTIFY_NOTHING;
    aPreference->mWantHints = false;
    return true;
  }

  *aSeqno = mIMESeqno;
  mIMETabParent = aFocus ? this : nullptr;
  mIMESelectionAnchor = 0;
  mIMESelectionFocus = 0;
  widget->NotifyIME(aFocus ? NOTIFY_IME_OF_FOCUS : NOTIFY_IME_OF_BLUR);

  if (aFocus) {
    *aPreference = widget->GetIMEUpdatePreference();
  } else {
    mIMECacheText.Truncate(0);
  }
  return true;
}

bool
TabParent::RecvNotifyIMETextChange(const uint32_t& aStart,
                                   const uint32_t& aEnd,
                                   const uint32_t& aNewEnd)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  widget->NotifyIMEOfTextChange(aStart, aEnd, aNewEnd);
  return true;
}

bool
TabParent::RecvNotifyIMESelection(const uint32_t& aSeqno,
                                  const uint32_t& aAnchor,
                                  const uint32_t& aFocus)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  if (aSeqno == mIMESeqno) {
    mIMESelectionAnchor = aAnchor;
    mIMESelectionFocus = aFocus;
    widget->NotifyIME(NOTIFY_IME_OF_SELECTION_CHANGE);
  }
  return true;
}

bool
TabParent::RecvNotifyIMETextHint(const nsString& aText)
{
  
  mIMECacheText = aText;
  return true;
}

bool
TabParent::RecvRequestFocus(const bool& aCanRaise)
{
  nsCOMPtr<nsIFocusManager> fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    return true;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(mFrameElement);
  if (!content || !content->OwnerDoc()) {
    return true;
  }

  uint32_t flags = nsIFocusManager::FLAG_NOSCROLL;
  if (aCanRaise)
    flags |= nsIFocusManager::FLAG_RAISE;

  nsCOMPtr<nsIDOMElement> node = do_QueryInterface(mFrameElement);
  fm->SetFocus(node, flags);
  return true;
}















bool
TabParent::HandleQueryContentEvent(nsQueryContentEvent& aEvent)
{
  aEvent.mSucceeded = false;
  aEvent.mWasAsync = false;
  aEvent.mReply.mFocusedWidget = nsCOMPtr<nsIWidget>(GetWidget()).get();

  switch (aEvent.message)
  {
  case NS_QUERY_SELECTED_TEXT:
    {
      aEvent.mReply.mOffset = std::min(mIMESelectionAnchor, mIMESelectionFocus);
      if (mIMESelectionAnchor == mIMESelectionFocus) {
        aEvent.mReply.mString.Truncate(0);
      } else {
        if (mIMESelectionAnchor > mIMECacheText.Length() ||
            mIMESelectionFocus > mIMECacheText.Length()) {
          break;
        }
        uint32_t selLen = mIMESelectionAnchor > mIMESelectionFocus ?
                          mIMESelectionAnchor - mIMESelectionFocus :
                          mIMESelectionFocus - mIMESelectionAnchor;
        aEvent.mReply.mString = Substring(mIMECacheText,
                                          aEvent.mReply.mOffset,
                                          selLen);
      }
      aEvent.mReply.mReversed = mIMESelectionFocus < mIMESelectionAnchor;
      aEvent.mReply.mHasSelection = true;
      aEvent.mSucceeded = true;
    }
    break;
  case NS_QUERY_TEXT_CONTENT:
    {
      uint32_t inputOffset = aEvent.mInput.mOffset,
               inputEnd = inputOffset + aEvent.mInput.mLength;

      if (inputEnd > mIMECacheText.Length()) {
        inputEnd = mIMECacheText.Length();
      }
      if (inputEnd < inputOffset) {
        break;
      }
      aEvent.mReply.mOffset = inputOffset;
      aEvent.mReply.mString = Substring(mIMECacheText,
                                        inputOffset,
                                        inputEnd - inputOffset);
      aEvent.mSucceeded = true;
    }
    break;
  }
  return true;
}

bool
TabParent::SendCompositionEvent(nsCompositionEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  mIMEComposing = event.message != NS_COMPOSITION_END;
  mIMECompositionStart = std::min(mIMESelectionAnchor, mIMESelectionFocus);
  if (mIMECompositionEnding)
    return true;
  event.seqno = ++mIMESeqno;
  return PBrowserParent::SendCompositionEvent(event);
}









bool
TabParent::SendTextEvent(nsTextEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  if (mIMECompositionEnding) {
    mIMECompositionText = event.theText;
    return true;
  }

  
  
  if (!mIMEComposing) {
    mIMECompositionStart = std::min(mIMESelectionAnchor, mIMESelectionFocus);
  }
  mIMESelectionAnchor = mIMESelectionFocus =
      mIMECompositionStart + event.theText.Length();

  event.seqno = ++mIMESeqno;
  return PBrowserParent::SendTextEvent(event);
}

bool
TabParent::SendSelectionEvent(nsSelectionEvent& event)
{
  if (mIsDestroyed) {
    return false;
  }
  mIMESelectionAnchor = event.mOffset + (event.mReversed ? event.mLength : 0);
  mIMESelectionFocus = event.mOffset + (!event.mReversed ? event.mLength : 0);
  event.seqno = ++mIMESeqno;
  return PBrowserParent::SendSelectionEvent(event);
}

 TabParent*
TabParent::GetFrom(nsFrameLoader* aFrameLoader)
{
  if (!aFrameLoader) {
    return nullptr;
  }
  PBrowserParent* remoteBrowser = aFrameLoader->GetRemoteBrowser();
  return static_cast<TabParent*>(remoteBrowser);
}

 TabParent*
TabParent::GetFrom(nsIContent* aContent)
{
  nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(aContent);
  if (!loaderOwner) {
    return nullptr;
  }
  nsRefPtr<nsFrameLoader> frameLoader = loaderOwner->GetFrameLoader();
  return GetFrom(frameLoader);
}

RenderFrameParent*
TabParent::GetRenderFrame()
{
  if (ManagedPRenderFrameParent().IsEmpty()) {
    return nullptr;
  }
  return static_cast<RenderFrameParent*>(ManagedPRenderFrameParent()[0]);
}

bool
TabParent::RecvEndIMEComposition(const bool& aCancel,
                                 nsString* aComposition)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  mIMECompositionEnding = true;

  widget->NotifyIME(aCancel ? REQUEST_TO_CANCEL_COMPOSITION :
                              REQUEST_TO_COMMIT_COMPOSITION);

  mIMECompositionEnding = false;
  *aComposition = mIMECompositionText;
  mIMECompositionText.Truncate(0);  
  return true;
}

bool
TabParent::RecvGetInputContext(int32_t* aIMEEnabled,
                               int32_t* aIMEOpen,
                               intptr_t* aNativeIMEContext)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget) {
    *aIMEEnabled = IMEState::DISABLED;
    *aIMEOpen = IMEState::OPEN_STATE_NOT_SUPPORTED;
    *aNativeIMEContext = 0;
    return true;
  }

  InputContext context = widget->GetInputContext();
  *aIMEEnabled = static_cast<int32_t>(context.mIMEState.mEnabled);
  *aIMEOpen = static_cast<int32_t>(context.mIMEState.mOpen);
  *aNativeIMEContext = reinterpret_cast<intptr_t>(context.mNativeIMEContext);
  return true;
}

bool
TabParent::RecvSetInputContext(const int32_t& aIMEEnabled,
                               const int32_t& aIMEOpen,
                               const nsString& aType,
                               const nsString& aInputmode,
                               const nsString& aActionHint,
                               const int32_t& aCause,
                               const int32_t& aFocusChange)
{
  
  
  
  mIMETabParent =
    aIMEEnabled != static_cast<int32_t>(IMEState::DISABLED) ? this : nullptr;
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget || !AllowContentIME())
    return true;

  InputContext context;
  context.mIMEState.mEnabled = static_cast<IMEState::Enabled>(aIMEEnabled);
  context.mIMEState.mOpen = static_cast<IMEState::Open>(aIMEOpen);
  context.mHTMLInputType.Assign(aType);
  context.mHTMLInputInputmode.Assign(aInputmode);
  context.mActionHint.Assign(aActionHint);
  InputContextAction action(
    static_cast<InputContextAction::Cause>(aCause),
    static_cast<InputContextAction::FocusChange>(aFocusChange));
  widget->SetInputContext(context, action);

  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  if (!observerService)
    return true;

  nsAutoString state;
  state.AppendInt(aIMEEnabled);
  observerService->NotifyObservers(nullptr, "ime-enabled-state-changed", state.get());

  return true;
}

bool
TabParent::RecvGetDPI(float* aValue)
{
  TryCacheDPIAndScale();

  NS_ABORT_IF_FALSE(mDPI > 0,
                    "Must not ask for DPI before OwnerElement is received!");
  *aValue = mDPI;
  return true;
}

bool
TabParent::RecvGetDefaultScale(double* aValue)
{
  TryCacheDPIAndScale();

  NS_ABORT_IF_FALSE(mDefaultScale.scale > 0,
                    "Must not ask for scale before OwnerElement is received!");
  *aValue = mDefaultScale.scale;
  return true;
}

bool
TabParent::RecvGetWidgetNativeData(WindowsHandle* aValue)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mFrameElement);
  if (content) {
    nsIPresShell* shell = content->OwnerDoc()->GetShell();
    if (shell) {
      nsViewManager* vm = shell->GetViewManager();
      nsCOMPtr<nsIWidget> widget;
      vm->GetRootWidget(getter_AddRefs(widget));
      if (widget) {
        *aValue = reinterpret_cast<WindowsHandle>(
          widget->GetNativeData(NS_NATIVE_SHAREABLE_WINDOW));
        return true;
      }
    }
  }
  return false;
}

bool
TabParent::ReceiveMessage(const nsString& aMessage,
                          bool aSync,
                          const StructuredCloneData* aCloneData,
                          CpowHolder* aCpows,
                          InfallibleTArray<nsString>* aJSONRetVal)
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  if (frameLoader && frameLoader->GetFrameMessageManager()) {
    nsRefPtr<nsFrameMessageManager> manager =
      frameLoader->GetFrameMessageManager();

    manager->ReceiveMessage(mFrameElement,
                            aMessage,
                            aSync,
                            aCloneData,
                            aCpows,
                            aJSONRetVal);
  }
  return true;
}

PIndexedDBParent*
TabParent::AllocPIndexedDBParent(
                            const nsCString& aGroup,
                            const nsCString& aASCIIOrigin, bool* )
{
  return new IndexedDBParent(this);
}

bool
TabParent::DeallocPIndexedDBParent(PIndexedDBParent* aActor)
{
  delete aActor;
  return true;
}

bool
TabParent::RecvPIndexedDBConstructor(PIndexedDBParent* aActor,
                                     const nsCString& aGroup,
                                     const nsCString& aASCIIOrigin,
                                     bool* aAllowed)
{
  nsRefPtr<IndexedDatabaseManager> mgr = IndexedDatabaseManager::GetOrCreate();
  NS_ENSURE_TRUE(mgr, false);

  if (!IndexedDatabaseManager::IsMainProcess()) {
    NS_RUNTIMEABORT("Not supported yet!");
  }

  nsresult rv;

  

  
  
  
  
  
  
  
  
  
  
  
  if (!aASCIIOrigin.EqualsLiteral("chrome") && IsBrowserOrApp() &&
      !IndexedDatabaseManager::TabContextMayAccessOrigin(*this, aASCIIOrigin)) {

    NS_WARNING("App attempted to open databases that it does not have "
               "permission to access!");
    return false;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(GetOwnerElement());
  NS_ENSURE_TRUE(node, false);

  nsIDocument* doc = node->GetOwnerDocument();
  NS_ENSURE_TRUE(doc, false);

  nsCOMPtr<nsPIDOMWindow> window = doc->GetInnerWindow();
  NS_ENSURE_TRUE(window, false);

  
  
  nsCOMPtr<nsPIDOMWindow> outer = doc->GetWindow();
  if (!outer || outer->GetCurrentInnerWindow() != window) {
    *aAllowed = false;
    return true;
  }

  ContentParent* contentParent = Manager();
  NS_ASSERTION(contentParent, "Null manager?!");

  nsRefPtr<IDBFactory> factory;
  rv = IDBFactory::Create(window, aGroup, aASCIIOrigin, contentParent,
                          getter_AddRefs(factory));
  NS_ENSURE_SUCCESS(rv, false);

  if (!factory) {
    *aAllowed = false;
    return true;
  }

  IndexedDBParent* actor = static_cast<IndexedDBParent*>(aActor);
  actor->mFactory = factory;
  actor->mASCIIOrigin = aASCIIOrigin;

  *aAllowed = true;
  return true;
}




NS_IMETHODIMP
TabParent::GetAuthPrompt(uint32_t aPromptReason, const nsIID& iid,
                          void** aResult)
{
  
  nsresult rv;
  nsCOMPtr<nsIPromptFactory> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> window;
  nsCOMPtr<nsIContent> frame = do_QueryInterface(mFrameElement);
  if (frame)
    window = do_QueryInterface(frame->OwnerDoc()->GetWindow());

  
  
  return wwatch->GetPrompt(window, iid,
                           reinterpret_cast<void**>(aResult));
}

PContentDialogParent*
TabParent::AllocPContentDialogParent(const uint32_t& aType,
                                     const nsCString& aName,
                                     const nsCString& aFeatures,
                                     const InfallibleTArray<int>& aIntParams,
                                     const InfallibleTArray<nsString>& aStringParams)
{
  ContentDialogParent* parent = new ContentDialogParent();
  nsCOMPtr<nsIDialogParamBlock> params =
    do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID);
  TabChild::ArraysToParams(aIntParams, aStringParams, params);
  mDelayedDialogs.AppendElement(new DelayedDialogData(parent, aType, aName,
                                                      aFeatures, params));
  nsRefPtr<nsIRunnable> ev =
    NS_NewRunnableMethod(this, &TabParent::HandleDelayedDialogs);
  NS_DispatchToCurrentThread(ev);
  return parent;
}

void
TabParent::HandleDelayedDialogs()
{
  nsCOMPtr<nsIWindowWatcher> ww = do_GetService(NS_WINDOWWATCHER_CONTRACTID);
  nsCOMPtr<nsIDOMWindow> window;
  if (mFrameElement) {
    window = do_QueryInterface(mFrameElement->OwnerDoc()->GetWindow());
  }
  nsCOMPtr<nsIDialogCreator> dialogCreator = do_QueryInterface(mBrowserDOMWindow);
  while (!ShouldDelayDialogs() && mDelayedDialogs.Length()) {
    uint32_t index = mDelayedDialogs.Length() - 1;
    DelayedDialogData* data = mDelayedDialogs[index];
    mDelayedDialogs.RemoveElementAt(index);
    nsCOMPtr<nsIDialogParamBlock> params;
    params.swap(data->mParams);
    PContentDialogParent* dialog = data->mDialog;
    if (dialogCreator) {
      nsCOMPtr<nsIDOMElement> frame = do_QueryInterface(mFrameElement);
      dialogCreator->OpenDialog(data->mType,
                                data->mName, data->mFeatures,
                                params, frame);
    } else if (ww) {
      nsAutoCString url;
      if (data->mType) {
        if (data->mType == nsIDialogCreator::SELECT_DIALOG) {
          url.Assign("chrome://global/content/selectDialog.xul");
        } else if (data->mType == nsIDialogCreator::GENERIC_DIALOG) {
          url.Assign("chrome://global/content/commonDialog.xul");
        }

        nsCOMPtr<nsISupports> arguments(do_QueryInterface(params));
        nsCOMPtr<nsIDOMWindow> dialog;
        ww->OpenWindow(window, url.get(), data->mName.get(),
                       data->mFeatures.get(), arguments, getter_AddRefs(dialog));
      } else {
        NS_WARNING("unknown dialog types aren't automatically supported in E10s yet!");
      }
    }

    delete data;
    if (dialog) {
      InfallibleTArray<int32_t> intParams;
      InfallibleTArray<nsString> stringParams;
      TabChild::ParamsToArrays(params, intParams, stringParams);
      unused << PContentDialogParent::Send__delete__(dialog,
                                                     intParams, stringParams);
    }
  }
  if (ShouldDelayDialogs() && mDelayedDialogs.Length()) {
    nsContentUtils::DispatchTrustedEvent(mFrameElement->OwnerDoc(), mFrameElement,
                                         NS_LITERAL_STRING("MozDelayedModalDialog"),
                                         true, true);
  }
}

PRenderFrameParent*
TabParent::AllocPRenderFrameParent(ScrollingBehavior* aScrolling,
                                   TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                   uint64_t* aLayersId)
{
  MOZ_ASSERT(ManagedPRenderFrameParent().IsEmpty());

  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  if (!frameLoader) {
    NS_WARNING("Can't allocate graphics resources, aborting subprocess");
    return nullptr;
  }

  *aScrolling = UseAsyncPanZoom() ? ASYNC_PAN_ZOOM : DEFAULT_SCROLLING;
  return new RenderFrameParent(frameLoader,
                               *aScrolling,
                               aTextureFactoryIdentifier, aLayersId);
}

bool
TabParent::DeallocPRenderFrameParent(PRenderFrameParent* aFrame)
{
  delete aFrame;
  return true;
}

mozilla::docshell::POfflineCacheUpdateParent*
TabParent::AllocPOfflineCacheUpdateParent(const URIParams& aManifestURI,
                                          const URIParams& aDocumentURI,
                                          const bool& stickDocument)
{
  nsRefPtr<mozilla::docshell::OfflineCacheUpdateParent> update =
    new mozilla::docshell::OfflineCacheUpdateParent(OwnOrContainingAppId(),
                                                    IsBrowserElement());

  nsresult rv = update->Schedule(aManifestURI, aDocumentURI, stickDocument);
  if (NS_FAILED(rv))
    return nullptr;

  POfflineCacheUpdateParent* result = update.get();
  update.forget();
  return result;
}

bool
TabParent::DeallocPOfflineCacheUpdateParent(mozilla::docshell::POfflineCacheUpdateParent* actor)
{
  mozilla::docshell::OfflineCacheUpdateParent* update =
    static_cast<mozilla::docshell::OfflineCacheUpdateParent*>(actor);

  update->Release();
  return true;
}

bool
TabParent::ShouldDelayDialogs()
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  NS_ENSURE_TRUE(frameLoader, true);
  bool delay = false;
  frameLoader->GetDelayRemoteDialogs(&delay);
  return delay;
}

bool
TabParent::AllowContentIME()
{
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, false);

  nsCOMPtr<nsIContent> focusedContent = fm->GetFocusedContent();
  if (focusedContent && focusedContent->IsEditable())
    return false;

  return true;
}

already_AddRefed<nsFrameLoader>
TabParent::GetFrameLoader() const
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner = do_QueryInterface(mFrameElement);
  return frameLoaderOwner ? frameLoaderOwner->GetFrameLoader() : nullptr;
}

void
TabParent::TryCacheDPIAndScale()
{
  if (mDPI > 0) {
    return;
  }

  nsCOMPtr<nsIWidget> widget = GetWidget();

  if (!widget && mFrameElement) {
    
    
    widget = nsContentUtils::WidgetForDocument(mFrameElement->OwnerDoc());
  }

  if (widget) {
    mDPI = widget->GetDPI();
    mDefaultScale = widget->GetDefaultScale();
  }
}

already_AddRefed<nsIWidget>
TabParent::GetWidget() const
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mFrameElement);
  if (!content)
    return nullptr;

  nsIFrame *frame = content->GetPrimaryFrame();
  if (!frame)
    return nullptr;

  nsCOMPtr<nsIWidget> widget = frame->GetNearestWidget();
  return widget.forget();
}

bool
TabParent::UseAsyncPanZoom()
{
  bool usingOffMainThreadCompositing = !!CompositorParent::CompositorLoop();
  bool asyncPanZoomEnabled =
    Preferences::GetBool("layers.async-pan-zoom.enabled", false);
  return (usingOffMainThreadCompositing && asyncPanZoomEnabled &&
          GetScrollingBehavior() == ASYNC_PAN_ZOOM);
}

void
TabParent::MaybeForwardEventToRenderFrame(const nsInputEvent& aEvent,
                                          nsInputEvent* aOutEvent)
{
  if (RenderFrameParent* rfp = GetRenderFrame()) {
    rfp->NotifyInputEvent(aEvent, aOutEvent);
  }
}

bool
TabParent::RecvBrowserFrameOpenWindow(PBrowserParent* aOpener,
                                      const nsString& aURL,
                                      const nsString& aName,
                                      const nsString& aFeatures,
                                      bool* aOutWindowOpened)
{
  BrowserElementParent::OpenWindowResult opened =
    BrowserElementParent::OpenWindowOOP(static_cast<TabParent*>(aOpener),
                                        this, aURL, aName, aFeatures);
  *aOutWindowOpened = (opened != BrowserElementParent::OPEN_WINDOW_CANCELLED);
  return true;
}

bool
TabParent::RecvPRenderFrameConstructor(PRenderFrameParent* actor,
                                       ScrollingBehavior* scrolling,
                                       TextureFactoryIdentifier* factoryIdentifier,
                                       uint64_t* layersId)
{
  RenderFrameParent* rfp = GetRenderFrame();
  if (mDimensions != nsIntSize() && rfp) {
    rfp->NotifyDimensionsChanged(ScreenIntSize::FromUnknownSize(
      gfx::IntSize(mDimensions.width, mDimensions.height)));
  }

  return true;
}

bool
TabParent::RecvZoomToRect(const CSSRect& aRect)
{
  if (RenderFrameParent* rfp = GetRenderFrame()) {
    rfp->ZoomToRect(aRect);
  }
  return true;
}

bool
TabParent::RecvUpdateZoomConstraints(const bool& aAllowZoom,
                                     const CSSToScreenScale& aMinZoom,
                                     const CSSToScreenScale& aMaxZoom)
{
  if (RenderFrameParent* rfp = GetRenderFrame()) {
    rfp->UpdateZoomConstraints(aAllowZoom, aMinZoom, aMaxZoom);
  }
  return true;
}

bool
TabParent::RecvUpdateScrollOffset(const uint32_t& aPresShellId,
                                  const ViewID& aViewId,
                                  const CSSIntPoint& aScrollOffset)
{
  if (RenderFrameParent* rfp = GetRenderFrame()) {
    rfp->UpdateScrollOffset(aPresShellId, aViewId, aScrollOffset);
  }
  return true;
}

bool
TabParent::RecvContentReceivedTouch(const bool& aPreventDefault)
{
  if (RenderFrameParent* rfp = GetRenderFrame()) {
    rfp->ContentReceivedTouch(aPreventDefault);
  }
  return true;
}

} 
} 
