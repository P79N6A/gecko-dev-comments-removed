





































#include "TabParent.h"

#include "mozilla/ipc/DocumentRendererParent.h"
#include "mozilla/ipc/DocumentRendererShmemParent.h"
#include "mozilla/ipc/DocumentRendererNativeIDParent.h"
#include "mozilla/dom/ContentProcessParent.h"

#include "nsIURI.h"
#include "nsFocusManager.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIDOMElement.h"
#include "nsEventDispatcher.h"
#include "nsIDOMEventTarget.h"
#include "nsIWindowWatcher.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIIdentityInfo.h"
#include "TabChild.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIWebProgressListener2.h"
#include "nsFrameLoader.h"
#include "nsNetUtil.h"
#include "jsarray.h"
#include "nsContentUtils.h"
#include "nsGeolocationOOP.h"
#include "nsIDOMNSHTMLFrameElement.h"
#include "nsIDialogCreator.h"
#include "nsThreadUtils.h"
#include "nsIPromptFactory.h"
#include "nsIContent.h"
#include "nsSerializationHelper.h"

using mozilla::ipc::DocumentRendererParent;
using mozilla::ipc::DocumentRendererShmemParent;
using mozilla::ipc::DocumentRendererNativeIDParent;
using mozilla::dom::ContentProcessParent;



#define NOTIFY_FLAG_SHIFT 16

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS5(TabParent, nsITabParent, nsIWebProgress, nsIAuthPromptProvider, 
                   nsISecureBrowserUI, nsISSLStatusProvider)

TabParent::TabParent()
  : mSecurityState(nsIWebProgressListener::STATE_IS_INSECURE)
{
}

TabParent::~TabParent()
{
}

void
TabParent::ActorDestroy(ActorDestroyReason why)
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner = do_QueryInterface(mFrameElement);
  if (frameLoaderOwner) {
    nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
    if (frameLoader) {
      frameLoader->DestroyChild();
    }
  }
}

bool
TabParent::RecvmoveFocus(const bool& aForward)
{
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMElement> dummy;
    PRUint32 type = aForward ? nsIFocusManager::MOVEFOCUS_FORWARD
                             : nsIFocusManager::MOVEFOCUS_BACKWARD;
    fm->MoveFocus(nsnull, mFrameElement, type, nsIFocusManager::FLAG_BYKEY, 
                  getter_AddRefs(dummy));
  }
  return true;
}

bool
TabParent::RecvsendEvent(const RemoteDOMEvent& aEvent)
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
TabParent::RecvnotifyProgressChange(const PRInt64& aProgress,
                                    const PRInt64& aProgressMax,
                                    const PRInt64& aTotalProgress,
                                    const PRInt64& aMaxTotalProgress)
{
  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRUint32 count = mListenerInfoList.Length();

  while (count-- > 0) {
    TabParentListenerInfo *info = &mListenerInfoList[count];
    if (!(info->mNotifyMask & nsIWebProgress::NOTIFY_PROGRESS)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      continue;
    }

    nsCOMPtr<nsIWebProgressListener2> listener2 =
      do_QueryReferent(info->mWeakListener);
    if (listener2) {
      listener2->OnProgressChange64(this, nsnull, aProgress, aProgressMax,
                                    aTotalProgress, aMaxTotalProgress);
    } else {
      listener->OnProgressChange(this, nsnull, PRInt32(aProgress),
                                 PRInt32(aProgressMax),
                                 PRInt32(aTotalProgress), 
                                 PRInt32(aMaxTotalProgress));
    }
  }

  return true;
}

bool
TabParent::RecvnotifyStateChange(const PRUint32& aStateFlags,
                                 const nsresult& aStatus)
{
  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRUint32 count = mListenerInfoList.Length();
  
  while (count-- > 0) {
    TabParentListenerInfo *info = &mListenerInfoList[count];

    
    
    
    
    
    
    
    if (!(info->mNotifyMask & (aStateFlags >> NOTIFY_FLAG_SHIFT))) {
        continue;
    }
    
    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      continue;
    }
    
    listener->OnStateChange(this, nsnull, aStateFlags, aStatus); 
  }   

  return true;
 }

bool
TabParent::RecvnotifyLocationChange(const nsCString& aUri)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUri);
  if (NS_FAILED(rv)) {
    return false;
  }

  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRUint32 count = mListenerInfoList.Length();

  while (count-- > 0) {
    TabParentListenerInfo *info = &mListenerInfoList[count];
    if (!(info->mNotifyMask & nsIWebProgress::NOTIFY_LOCATION)) {
      continue;
    }
    
    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      continue;
    }
    
    listener->OnLocationChange(this, nsnull, uri);
  }

  return true;
}

bool
TabParent::RecvnotifyStatusChange(const nsresult& status,
                                  const nsString& message)
{
  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRUint32 count = mListenerInfoList.Length();

  while (count-- > 0) {
    TabParentListenerInfo *info = &mListenerInfoList[count];
    if (!(info->mNotifyMask & nsIWebProgress::NOTIFY_STATUS)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      continue;
    }

    listener->OnStatusChange(this, nsnull, status, message.BeginReading());
  }

  return true;
}

bool
TabParent::RecvnotifySecurityChange(const PRUint32& aState,
                                    const PRBool& aUseSSLStatusObject,
                                    const nsString& aTooltip,
                                    const nsCString& aSecInfoAsString)
{
  






  mSecurityState = aState;
  mSecurityTooltipText = aTooltip;

  if (!aSecInfoAsString.IsEmpty()) {
    nsCOMPtr<nsISupports> secInfoSupports;
    nsresult rv = NS_DeserializeObject(aSecInfoAsString, getter_AddRefs(secInfoSupports));

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIIdentityInfo> idInfo = do_QueryInterface(secInfoSupports);
      if (idInfo) {
        PRBool isEV;
        if (NS_SUCCEEDED(idInfo->GetIsExtendedValidation(&isEV)) && isEV)
          mSecurityState |= nsIWebProgressListener::STATE_IDENTITY_EV_TOPLEVEL;
      }
    }

    mSecurityStatusObject = nsnull;
    if (aUseSSLStatusObject)
    {
      nsCOMPtr<nsISSLStatusProvider> sslStatusProvider =
        do_QueryInterface(secInfoSupports);
      if (sslStatusProvider)
        sslStatusProvider->GetSSLStatus(getter_AddRefs(mSecurityStatusObject));
    }
  }

  nsCOMPtr<nsIWebProgressListener> listener;
  PRUint32 count = mListenerInfoList.Length();

  while (count-- > 0) {
    TabParentListenerInfo *info = &mListenerInfoList[count];
    if (!(info->mNotifyMask & nsIWebProgress::NOTIFY_SECURITY)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      continue;
    }

    listener->OnSecurityChange(this, nsnull, mSecurityState);
  }

  return true;
}

bool
TabParent::RecvrefreshAttempted(const nsCString& aURI, const PRInt32& aMillis, 
                                const bool& aSameURI, bool* refreshAllowed)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI);
  if (NS_FAILED(rv)) {
    return false;
  }
  






  nsCOMPtr<nsIWebProgressListener> listener;
  PRUint32 count = mListenerInfoList.Length();

  *refreshAllowed = true;
  while (count-- > 0) {
    TabParentListenerInfo *info = &mListenerInfoList[count];
    if (!(info->mNotifyMask & nsIWebProgress::NOTIFY_REFRESH)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      continue;
    }

    nsCOMPtr<nsIWebProgressListener2> listener2 =
      do_QueryReferent(info->mWeakListener);
    if (!listener2) {
      continue;
    }

    
    PRBool allowed = true;
    listener2->OnRefreshAttempted(this, uri, 
                                  aMillis, aSameURI, &allowed);
    *refreshAllowed = allowed && *refreshAllowed;
  }

  return true;
}

bool
TabParent::AnswercreateWindow(PIFrameEmbeddingParent** retval)
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

    *retval = frameLoader->GetChildProcess();
    return true;
}

void
TabParent::LoadURL(nsIURI* aURI)
{
    nsCString spec;
    aURI->GetSpec(spec);

    SendloadURL(spec);
}

void
TabParent::Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height)
{
    Sendmove(x, y, width, height);
}

void
TabParent::Activate()
{
    Sendactivate();
}

mozilla::ipc::PDocumentRendererParent*
TabParent::AllocPDocumentRenderer(const PRInt32& x,
        const PRInt32& y, const PRInt32& w, const PRInt32& h, const nsString& bgcolor,
        const PRUint32& flags, const bool& flush)
{
    return new DocumentRendererParent();
}

bool
TabParent::DeallocPDocumentRenderer(PDocumentRendererParent* actor)
{
    delete actor;
    return true;
}

mozilla::ipc::PDocumentRendererShmemParent*
TabParent::AllocPDocumentRendererShmem(const PRInt32& x,
        const PRInt32& y, const PRInt32& w, const PRInt32& h, const nsString& bgcolor,
        const PRUint32& flags, const bool& flush, const gfxMatrix& aMatrix,
        Shmem& buf)
{
    return new DocumentRendererShmemParent();
}

bool
TabParent::DeallocPDocumentRendererShmem(PDocumentRendererShmemParent* actor)
{
    delete actor;
    return true;
}

mozilla::ipc::PDocumentRendererNativeIDParent*
TabParent::AllocPDocumentRendererNativeID(const PRInt32& x,
        const PRInt32& y, const PRInt32& w, const PRInt32& h, const nsString& bgcolor,
        const PRUint32& flags, const bool& flush, const gfxMatrix& aMatrix,
        const PRUint32& nativeID)
{
    return new DocumentRendererNativeIDParent();
}

bool
TabParent::DeallocPDocumentRendererNativeID(PDocumentRendererNativeIDParent* actor)
{
    delete actor;
    return true;
}

PGeolocationRequestParent*
TabParent::AllocPGeolocationRequest(const IPC::URI& uri)
{
  return new GeolocationRequestParent(mFrameElement, uri);
}
  
bool
TabParent::DeallocPGeolocationRequest(PGeolocationRequestParent* actor)
{
  delete actor;
  return true;
}

void
TabParent::SendMouseEvent(const nsAString& aType, float aX, float aY,
                          PRInt32 aButton, PRInt32 aClickCount,
                          PRInt32 aModifiers, PRBool aIgnoreRootScrollFrame)
{
  SendsendMouseEvent(nsString(aType), aX, aY, aButton, aClickCount,
                     aModifiers, aIgnoreRootScrollFrame);
}

void
TabParent::SendKeyEvent(const nsAString& aType,
                        PRInt32 aKeyCode,
                        PRInt32 aCharCode,
                        PRInt32 aModifiers,
                        PRBool aPreventDefault)
{
    SendsendKeyEvent(nsString(aType), aKeyCode, aCharCode, aModifiers,
                     aPreventDefault);
}

bool
TabParent::RecvsendSyncMessageToParent(const nsString& aMessage,
                                       const nsString& aJSON,
                                       nsTArray<nsString>* aJSONRetVal)
{
  return ReceiveMessage(aMessage, PR_TRUE, aJSON, aJSONRetVal);
}

bool
TabParent::RecvsendAsyncMessageToParent(const nsString& aMessage,
                                        const nsString& aJSON)
{
  return ReceiveMessage(aMessage, PR_FALSE, aJSON, nsnull);
}

bool
TabParent::ReceiveMessage(const nsString& aMessage,
                          PRBool aSync,
                          const nsString& aJSON,
                          nsTArray<nsString>* aJSONRetVal)
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner =
    do_QueryInterface(mFrameElement);
  if (frameLoaderOwner) {
    nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
    if (frameLoader && frameLoader->GetFrameMessageManager()) {
      nsFrameMessageManager* manager = frameLoader->GetFrameMessageManager();
      JSContext* ctx = manager->GetJSContext();
      JSAutoRequest ar(ctx);
      jsval* dest;
      PRUint32 len = 0; 
      
      
      JSObject* objectsArray =
        js_NewArrayObjectWithCapacity(ctx, len, &dest);
      if (!objectsArray) {
        return false;
      }

      nsresult rv = NS_OK;
      nsAutoGCRoot arrayGCRoot(&objectsArray, &rv);
      NS_ENSURE_SUCCESS(rv, false);
      
      manager->ReceiveMessage(mFrameElement,
                              aMessage,
                              aSync,
                              aJSON,
                              objectsArray,
                              aJSONRetVal);
    }
  }
  return true;
}


nsresult
TabParent::AddProgressListener(nsIWebProgressListener* aListener,
                               PRUint32 aNotifyMask)
{
  nsresult rv;

  TabParentListenerInfo* info = GetListenerInfo(aListener);
  if (info) {
    
    return NS_ERROR_FAILURE;
  }

  nsWeakPtr listener = do_GetWeakReference(aListener);
  if (!listener) {
    return NS_ERROR_INVALID_ARG;
  }

  info = new TabParentListenerInfo(listener, aNotifyMask);
  if (!info) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = mListenerInfoList.AppendElement(*info) ? NS_OK : NS_ERROR_FAILURE;
  if (NS_FAILED(rv)) {
    delete info;
  }
  return rv;
}

NS_IMETHODIMP
TabParent::RemoveProgressListener(nsIWebProgressListener *aListener)
{
  nsAutoPtr<TabParentListenerInfo> info(GetListenerInfo(aListener));
  
  return info && mListenerInfoList.RemoveElement(*info) ?
    NS_OK : NS_ERROR_FAILURE;
}

TabParentListenerInfo * 
TabParent::GetListenerInfo(nsIWebProgressListener *aListener)
{
  PRUint32 i, count;
  TabParentListenerInfo *info;

  nsCOMPtr<nsISupports> listener1 = do_QueryInterface(aListener);
  count = mListenerInfoList.Length();
  for (i = 0; i < count; ++i) {
    info = &mListenerInfoList[i];

    if (info) {
      nsCOMPtr<nsISupports> listener2 = do_QueryReferent(info->mWeakListener);
      if (listener1 == listener2) {
        return info;
      }
    }
  }
  return nsnull;
}

NS_IMETHODIMP
TabParent::GetDOMWindow(nsIDOMWindow **aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabParent::GetIsLoadingDocument(PRBool *aIsLoadingDocument)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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
  *aState = mSecurityState;
  return NS_OK;
}

NS_IMETHODIMP
TabParent::GetTooltipText(nsAString & aTooltipText)
{
  aTooltipText = mSecurityTooltipText;
  return NS_OK;
}

NS_IMETHODIMP
TabParent::GetSSLStatus(nsISupports ** aStatus)
{
  NS_IF_ADDREF(*aStatus = mSecurityStatusObject);
  return NS_OK;
}




NS_IMETHODIMP
TabParent::GetAuthPrompt(PRUint32 aPromptReason, const nsIID& iid,
                          void** aResult)
{
  
  PRBool priorityPrompt = (aPromptReason == PROMPT_PROXY);

  
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
                               const nsTArray<int>& aIntParams,
                               const nsTArray<nsString>& aStringParams)
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
      nsTArray<PRInt32> intParams;
      nsTArray<nsString> stringParams;
      TabChild::ParamsToArrays(params, intParams, stringParams);
      PContentDialogParent::Send__delete__(dialog, intParams, stringParams);
    }
  }
  if (ShouldDelayDialogs() && mDelayedDialogs.Length()) {
    nsContentUtils::DispatchTrustedEvent(frame->GetOwnerDoc(), frame,
                                         NS_LITERAL_STRING("MozDelayedModalDialog"),
                                         PR_TRUE, PR_TRUE);
  }
}

PRBool
TabParent::ShouldDelayDialogs()
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner = do_QueryInterface(mFrameElement);
  NS_ENSURE_TRUE(frameLoaderOwner, PR_TRUE);
  nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
  NS_ENSURE_TRUE(frameLoader, PR_TRUE);
  PRBool delay = PR_FALSE;
  frameLoader->GetDelayRemoteDialogs(&delay);
  return delay;
}

} 
} 
