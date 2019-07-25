





































#include "TabParent.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/DocumentRendererParent.h"
#include "mozilla/layout/RenderFrameParent.h"
#include "mozilla/docshell/OfflineCacheUpdateParent.h"

#include "nsIURI.h"
#include "nsFocusManager.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIDOMElement.h"
#include "nsEventDispatcher.h"
#include "nsIDOMEventTarget.h"
#include "nsIWindowWatcher.h"
#include "nsIDOMWindow.h"
#include "nsIIdentityInfo.h"
#include "nsPIDOMWindow.h"
#include "TabChild.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsFrameLoader.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsContentPermissionHelper.h"
#include "nsIDOMNSHTMLFrameElement.h"
#include "nsIDialogCreator.h"
#include "nsThreadUtils.h"
#include "nsSerializationHelper.h"
#include "nsIPromptFactory.h"
#include "nsIContent.h"
#include "mozilla/unused.h"
#include "nsDebug.h"

using namespace mozilla::dom;
using namespace mozilla::ipc;
using namespace mozilla::layout;



#define NOTIFY_FLAG_SHIFT 16

namespace mozilla {
namespace dom {

TabParent *TabParent::mIMETabParent = nsnull;

NS_IMPL_ISUPPORTS3(TabParent, nsITabParent, nsIAuthPromptProvider, nsISecureBrowserUI)

TabParent::TabParent()
  : mIMEComposing(PR_FALSE)
  , mIMECompositionEnding(PR_FALSE)
  , mIMESeqno(0)
  , mDPI(0)
{
}

TabParent::~TabParent()
{
}

void
TabParent::SetOwnerElement(nsIDOMElement* aElement)
{
  mFrameElement = aElement;

  
  if (aElement) {
    nsCOMPtr<nsIWidget> widget = GetWidget();
    NS_ABORT_IF_FALSE(widget, "Non-null OwnerElement must provide a widget!");
    mDPI = widget->GetDPI();
  }
}

void
TabParent::Destroy()
{
  
  
  
  unused << SendDestroy();

  for (size_t i = 0; i < ManagedPRenderFrameParent().Length(); ++i) {
    RenderFrameParent* rfp =
      static_cast<RenderFrameParent*>(ManagedPRenderFrameParent()[i]);
    rfp->Destroy();
  }
}

void
TabParent::ActorDestroy(ActorDestroyReason why)
{
  if (mIMETabParent == this)
    mIMETabParent = nsnull;
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  if (frameLoader) {
    frameLoader->DestroyChild();
  }
}

bool
TabParent::RecvMoveFocus(const bool& aForward)
{
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMElement> dummy;
    PRUint32 type = aForward ? PRUint32(nsIFocusManager::MOVEFOCUS_FORWARD)
                             : PRUint32(nsIFocusManager::MOVEFOCUS_BACKWARD);
    fm->MoveFocus(nsnull, mFrameElement, type, nsIFocusManager::FLAG_BYKEY, 
                  getter_AddRefs(dummy));
  }
  return true;
}

bool
TabParent::RecvEvent(const RemoteDOMEvent& aEvent)
{
  nsCOMPtr<nsIDOMEvent> event = do_QueryInterface(aEvent.mEvent);
  NS_ENSURE_TRUE(event, true);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mFrameElement);
  NS_ENSURE_TRUE(target, true);

  PRBool dummy;
  target->DispatchEvent(event, &dummy);
  return true;
}

bool
TabParent::AnswerCreateWindow(PBrowserParent** retval)
{
    if (!mBrowserDOMWindow) {
        return false;
    }

    
    
    nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner;
    mBrowserDOMWindow->OpenURIInFrame(nsnull, nsnull,
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
    nsCString spec;
    aURI->GetSpec(spec);

    unused << SendLoadURL(spec);
}

void
TabParent::Show(const nsIntSize& size)
{
    
    unused << SendShow(size);
}

void
TabParent::Move(const nsIntSize& size)
{
    unused << SendMove(size);
}

void
TabParent::Activate()
{
    unused << SendActivate();
}

NS_IMETHODIMP
TabParent::Init(nsIDOMWindow *window)
{
  return NS_OK;
}

NS_IMETHODIMP
TabParent::GetState(PRUint32 *aState)
{
  NS_ENSURE_ARG(aState);
  NS_WARNING("SecurityState not valid here");
  *aState = 0;
  return NS_OK;
}

NS_IMETHODIMP
TabParent::GetTooltipText(nsAString & aTooltipText)
{
  aTooltipText.Truncate();
  return NS_OK;
}

PDocumentRendererParent*
TabParent::AllocPDocumentRenderer(const nsRect& documentRect,
                                  const gfxMatrix& transform,
                                  const nsString& bgcolor,
                                  const PRUint32& renderFlags,
                                  const bool& flushLayout,
                                  const nsIntSize& renderSize)
{
    return new DocumentRendererParent();
}

bool
TabParent::DeallocPDocumentRenderer(PDocumentRendererParent* actor)
{
    delete actor;
    return true;
}

PContentPermissionRequestParent*
TabParent::AllocPContentPermissionRequest(const nsCString& type, const IPC::URI& uri)
{
  return new ContentPermissionRequestParent(type, mFrameElement, uri);
}
  
bool
TabParent::DeallocPContentPermissionRequest(PContentPermissionRequestParent* actor)
{
  delete actor;
  return true;
}

void
TabParent::SendMouseEvent(const nsAString& aType, float aX, float aY,
                          PRInt32 aButton, PRInt32 aClickCount,
                          PRInt32 aModifiers, PRBool aIgnoreRootScrollFrame)
{
  unused << PBrowserParent::SendMouseEvent(nsString(aType), aX, aY,
                                           aButton, aClickCount,
                                           aModifiers, aIgnoreRootScrollFrame);
}

void
TabParent::SendKeyEvent(const nsAString& aType,
                        PRInt32 aKeyCode,
                        PRInt32 aCharCode,
                        PRInt32 aModifiers,
                        PRBool aPreventDefault)
{
  unused << PBrowserParent::SendKeyEvent(nsString(aType), aKeyCode, aCharCode,
                                         aModifiers, aPreventDefault);
}

bool
TabParent::RecvSyncMessage(const nsString& aMessage,
                           const nsString& aJSON,
                           InfallibleTArray<nsString>* aJSONRetVal)
{
  return ReceiveMessage(aMessage, PR_TRUE, aJSON, aJSONRetVal);
}

bool
TabParent::RecvAsyncMessage(const nsString& aMessage,
                            const nsString& aJSON)
{
  return ReceiveMessage(aMessage, PR_FALSE, aJSON, nsnull);
}

bool
TabParent::RecvNotifyIMEFocus(const PRBool& aFocus,
                              nsIMEUpdatePreference* aPreference,
                              PRUint32* aSeqno)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  *aSeqno = mIMESeqno;
  mIMETabParent = aFocus ? this : nsnull;
  mIMESelectionAnchor = 0;
  mIMESelectionFocus = 0;
  nsresult rv = widget->OnIMEFocusChange(aFocus);

  if (aFocus) {
    if (NS_SUCCEEDED(rv) && rv != NS_SUCCESS_IME_NO_UPDATES) {
      *aPreference = widget->GetIMEUpdatePreference();
    } else {
      aPreference->mWantUpdates = PR_FALSE;
      aPreference->mWantHints = PR_FALSE;
    }
  } else {
    mIMECacheText.Truncate(0);
  }
  return true;
}

bool
TabParent::RecvNotifyIMETextChange(const PRUint32& aStart,
                                   const PRUint32& aEnd,
                                   const PRUint32& aNewEnd)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  widget->OnIMETextChange(aStart, aEnd, aNewEnd);
  return true;
}

bool
TabParent::RecvNotifyIMESelection(const PRUint32& aSeqno,
                                  const PRUint32& aAnchor,
                                  const PRUint32& aFocus)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  if (aSeqno == mIMESeqno) {
    mIMESelectionAnchor = aAnchor;
    mIMESelectionFocus = aFocus;
    widget->OnIMESelectionChange();
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
TabParent::HandleQueryContentEvent(nsQueryContentEvent& aEvent)
{
  aEvent.mSucceeded = PR_FALSE;
  aEvent.mWasAsync = PR_FALSE;
  aEvent.mReply.mFocusedWidget = nsCOMPtr<nsIWidget>(GetWidget()).get();

  switch (aEvent.message)
  {
  case NS_QUERY_SELECTED_TEXT:
    {
      aEvent.mReply.mOffset = NS_MIN(mIMESelectionAnchor, mIMESelectionFocus);
      if (mIMESelectionAnchor == mIMESelectionFocus) {
        aEvent.mReply.mString.Truncate(0);
      } else {
        if (mIMESelectionAnchor > mIMECacheText.Length() ||
            mIMESelectionFocus > mIMECacheText.Length()) {
          break;
        }
        PRUint32 selLen = mIMESelectionAnchor > mIMESelectionFocus ?
                          mIMESelectionAnchor - mIMESelectionFocus :
                          mIMESelectionFocus - mIMESelectionAnchor;
        aEvent.mReply.mString = Substring(mIMECacheText,
                                          aEvent.mReply.mOffset,
                                          selLen);
      }
      aEvent.mReply.mReversed = mIMESelectionFocus < mIMESelectionAnchor;
      aEvent.mReply.mHasSelection = PR_TRUE;
      aEvent.mSucceeded = PR_TRUE;
    }
    break;
  case NS_QUERY_TEXT_CONTENT:
    {
      PRUint32 inputOffset = aEvent.mInput.mOffset,
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
      aEvent.mSucceeded = PR_TRUE;
    }
    break;
  }
  return true;
}

bool
TabParent::SendCompositionEvent(nsCompositionEvent& event)
{
  mIMEComposing = event.message == NS_COMPOSITION_START;
  mIMECompositionStart = NS_MIN(mIMESelectionAnchor, mIMESelectionFocus);
  if (mIMECompositionEnding)
    return true;
  event.seqno = ++mIMESeqno;
  return PBrowserParent::SendCompositionEvent(event);
}








bool
TabParent::SendTextEvent(nsTextEvent& event)
{
  if (mIMECompositionEnding) {
    mIMECompositionText = event.theText;
    return true;
  }

  
  
  if (!mIMEComposing) {
    mIMECompositionStart = NS_MIN(mIMESelectionAnchor, mIMESelectionFocus);
  }
  mIMESelectionAnchor = mIMESelectionFocus =
      mIMECompositionStart + event.theText.Length();

  event.seqno = ++mIMESeqno;
  return PBrowserParent::SendTextEvent(event);
}

bool
TabParent::SendSelectionEvent(nsSelectionEvent& event)
{
  mIMESelectionAnchor = event.mOffset + (event.mReversed ? event.mLength : 0);
  mIMESelectionFocus = event.mOffset + (!event.mReversed ? event.mLength : 0);
  event.seqno = ++mIMESeqno;
  return PBrowserParent::SendSelectionEvent(event);
}

bool
TabParent::RecvEndIMEComposition(const PRBool& aCancel,
                                 nsString* aComposition)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget)
    return true;

  mIMECompositionEnding = PR_TRUE;

  if (aCancel) {
    widget->CancelIMEComposition();
  } else {
    widget->ResetInputState();
  }

  mIMECompositionEnding = PR_FALSE;
  *aComposition = mIMECompositionText;
  mIMECompositionText.Truncate(0);  
  return true;
}

bool
TabParent::RecvGetIMEEnabled(PRUint32* aValue)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget) {
    *aValue = nsIWidget::IME_STATUS_DISABLED;
    return true;
  }

  IMEContext context;
  widget->GetInputMode(context);
  *aValue = context.mStatus;
  return true;
}

bool
TabParent::RecvSetInputMode(const PRUint32& aValue, const nsString& aType, const nsString& aAction, const PRUint32& aReason)
{
  
  
  mIMETabParent = aValue & nsIContent::IME_STATUS_MASK_ENABLED ? this : nsnull;
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget || !AllowContentIME())
    return true;

  IMEContext context;
  context.mStatus = aValue;
  context.mHTMLInputType.Assign(aType);
  context.mActionHint.Assign(aAction);
  context.mReason = aReason;
  widget->SetInputMode(context);

  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  if (!observerService)
    return true;

  nsAutoString state;
  state.AppendInt(aValue);
  observerService->NotifyObservers(nsnull, "ime-enabled-state-changed", state.get());

  return true;
}

bool
TabParent::RecvGetIMEOpenState(PRBool* aValue)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (widget)
    widget->GetIMEOpenState(aValue);
  return true;
}

bool
TabParent::RecvSetIMEOpenState(const PRBool& aValue)
{
  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (widget && AllowContentIME())
    widget->SetIMEOpenState(aValue);
  return true;
}

bool
TabParent::RecvGetDPI(float* aValue)
{
  NS_ABORT_IF_FALSE(mDPI > 0, 
                    "Must not ask for DPI before OwnerElement is received!");
  *aValue = mDPI;
  return true;
}

bool
TabParent::ReceiveMessage(const nsString& aMessage,
                          PRBool aSync,
                          const nsString& aJSON,
                          InfallibleTArray<nsString>* aJSONRetVal)
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  if (frameLoader && frameLoader->GetFrameMessageManager()) {
    nsRefPtr<nsFrameMessageManager> manager =
      frameLoader->GetFrameMessageManager();
    JSContext* ctx = manager->GetJSContext();
    JSAutoRequest ar(ctx);
    PRUint32 len = 0; 
    
    
    JSObject* objectsArray = JS_NewArrayObject(ctx, len, NULL);
    if (!objectsArray) {
      return false;
    }

    manager->ReceiveMessage(mFrameElement,
                            aMessage,
                            aSync,
                            aJSON,
                            objectsArray,
                            aJSONRetVal);
  }
  return true;
}




NS_IMETHODIMP
TabParent::GetAuthPrompt(PRUint32 aPromptReason, const nsIID& iid,
                          void** aResult)
{
  
  nsresult rv;
  nsCOMPtr<nsIPromptFactory> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> window;
  nsCOMPtr<nsIContent> frame = do_QueryInterface(mFrameElement);
  if (frame)
    window = do_QueryInterface(frame->GetOwnerDoc()->GetWindow());

  
  
  return wwatch->GetPrompt(window, iid,
                           reinterpret_cast<void**>(aResult));
}

PContentDialogParent*
TabParent::AllocPContentDialog(const PRUint32& aType,
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
  nsCOMPtr<nsIContent> frame = do_QueryInterface(mFrameElement);
  if (frame) {
    window = do_QueryInterface(frame->GetOwnerDoc()->GetWindow());
  }
  nsCOMPtr<nsIDialogCreator> dialogCreator = do_QueryInterface(mBrowserDOMWindow);
  while (!ShouldDelayDialogs() && mDelayedDialogs.Length()) {
    PRUint32 index = mDelayedDialogs.Length() - 1;
    DelayedDialogData* data = mDelayedDialogs[index];
    mDelayedDialogs.RemoveElementAt(index);
    nsCOMPtr<nsIDialogParamBlock> params;
    params.swap(data->mParams);
    PContentDialogParent* dialog = data->mDialog;
    if (dialogCreator) {
      dialogCreator->OpenDialog(data->mType,
                                data->mName, data->mFeatures,
                                params, mFrameElement);
    } else if (ww) {
      nsCAutoString url;
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
      InfallibleTArray<PRInt32> intParams;
      InfallibleTArray<nsString> stringParams;
      TabChild::ParamsToArrays(params, intParams, stringParams);
      unused << PContentDialogParent::Send__delete__(dialog,
                                                     intParams, stringParams);
    }
  }
  if (ShouldDelayDialogs() && mDelayedDialogs.Length()) {
    nsContentUtils::DispatchTrustedEvent(frame->GetOwnerDoc(), frame,
                                         NS_LITERAL_STRING("MozDelayedModalDialog"),
                                         PR_TRUE, PR_TRUE);
  }
}

PRenderFrameParent*
TabParent::AllocPRenderFrame()
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  NS_WARN_IF_FALSE(frameLoader, "'message sent to unknown actor ID' coming up");
  return frameLoader ? new RenderFrameParent(frameLoader) : nsnull;
}

bool
TabParent::DeallocPRenderFrame(PRenderFrameParent* aFrame)
{
  delete aFrame;
  return true;
}

mozilla::docshell::POfflineCacheUpdateParent*
TabParent::AllocPOfflineCacheUpdate(const URI& aManifestURI,
                                    const URI& aDocumentURI,
                                    const nsCString& aClientID,
                                    const bool& stickDocument)
{
  nsRefPtr<mozilla::docshell::OfflineCacheUpdateParent> update =
    new mozilla::docshell::OfflineCacheUpdateParent();

  nsresult rv = update->Schedule(aManifestURI, aDocumentURI, aClientID,
                                 stickDocument);
  if (NS_FAILED(rv))
    return nsnull;

  POfflineCacheUpdateParent* result = update.get();
  update.forget();
  return result;
}

bool
TabParent::DeallocPOfflineCacheUpdate(mozilla::docshell::POfflineCacheUpdateParent* actor)
{
  mozilla::docshell::OfflineCacheUpdateParent* update =
    static_cast<mozilla::docshell::OfflineCacheUpdateParent*>(actor);

  update->Release();
  return true;
}

PRBool
TabParent::ShouldDelayDialogs()
{
  nsRefPtr<nsFrameLoader> frameLoader = GetFrameLoader();
  NS_ENSURE_TRUE(frameLoader, PR_TRUE);
  PRBool delay = PR_FALSE;
  frameLoader->GetDelayRemoteDialogs(&delay);
  return delay;
}

PRBool
TabParent::AllowContentIME()
{
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, PR_FALSE);

  nsCOMPtr<nsIContent> focusedContent = fm->GetFocusedContent();
  if (focusedContent && focusedContent->IsEditable())
    return PR_FALSE;

  return PR_TRUE;
}

already_AddRefed<nsFrameLoader>
TabParent::GetFrameLoader() const
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner = do_QueryInterface(mFrameElement);
  return frameLoaderOwner ? frameLoaderOwner->GetFrameLoader() : nsnull;
}

already_AddRefed<nsIWidget>
TabParent::GetWidget() const
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mFrameElement);
  if (!content)
    return nsnull;

  nsIFrame *frame = content->GetPrimaryFrame();
  if (!frame)
    return nsnull;

  return nsCOMPtr<nsIWidget>(frame->GetNearestWidget()).forget();
}

} 
} 
