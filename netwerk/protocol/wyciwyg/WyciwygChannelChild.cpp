




































#include "nsWyciwyg.h"

#include "mozilla/net/NeckoChild.h"
#include "WyciwygChannelChild.h"

#include "nsIParser.h"
#include "nsStringStream.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsISerializable.h"
#include "nsSerializationHelper.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS3(WyciwygChannelChild,
                   nsIRequest,
                   nsIChannel,
                   nsIWyciwygChannel);


WyciwygChannelChild::WyciwygChannelChild()
  : mStatus(NS_OK)
  , mIsPending(PR_FALSE)
  , mLoadFlags(LOAD_NORMAL)
  , mContentLength(-1)
  , mCharsetSource(kCharsetUninitialized)
  , mState(WCC_NEW)
  , mIPCOpen(false)
{
  LOG(("Creating WyciwygChannelChild @%x\n", this));
}

WyciwygChannelChild::~WyciwygChannelChild()
{
  LOG(("Destroying WyciwygChannelChild @%x\n", this));
}

void
WyciwygChannelChild::AddIPDLReference()
{
  NS_ABORT_IF_FALSE(!mIPCOpen, "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
WyciwygChannelChild::ReleaseIPDLReference()
{
  NS_ABORT_IF_FALSE(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}

nsresult
WyciwygChannelChild::Init(nsIURI* uri)
{
  NS_ENSURE_ARG_POINTER(uri);

  mState = WCC_INIT;

  mURI = uri;
  mOriginalURI = uri;

  SendInit(IPC::URI(mURI));
  return NS_OK;
}





bool
WyciwygChannelChild::RecvOnStartRequest(const nsresult& statusCode,
                                        const PRInt32& contentLength,
                                        const PRInt32& source,
                                        const nsCString& charset,
                                        const nsCString& securityInfo)
{
  LOG(("WyciwygChannelChild::RecvOnStartRequest [this=%x]\n", this));

  mState = WCC_ONSTART;

  mStatus = statusCode;
  mContentLength = contentLength;
  mCharsetSource = source;
  mCharset = charset;

  if (!securityInfo.IsEmpty()) {
    NS_DeserializeObject(securityInfo, getter_AddRefs(mSecurityInfo));
  }

  nsresult rv = mListener->OnStartRequest(this, mListenerContext);
  if (NS_FAILED(rv)) {
    
    
    
    
    
    return false;
  }
  return true;
}

bool
WyciwygChannelChild::RecvOnDataAvailable(const nsCString& data,
                                         const PRUint32& offset)
{
  LOG(("WyciwygChannelChild::RecvOnDataAvailable [this=%x]\n", this));

  mState = WCC_ONDATA;

  
  
  
  
  
  nsCOMPtr<nsIInputStream> stringStream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stringStream),
                                      data.get(),
                                      data.Length(),
                                      NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv)) {
    
    return false;
  }
  rv = mListener->OnDataAvailable(this, mListenerContext,
                                  stringStream, offset, data.Length());
  if (NS_FAILED(rv)) {
    
    return false;
  }

  if (mProgressSink && NS_SUCCEEDED(rv) && !(mLoadFlags & LOAD_BACKGROUND))
    mProgressSink->OnProgress(this, nsnull, PRUint64(offset + data.Length()),
                              PRUint64(mContentLength));

  return true;
}

bool
WyciwygChannelChild::RecvOnStopRequest(const nsresult& statusCode)
{
  LOG(("WyciwygChannelChild::RecvOnStopRequest [this=%x status=%u]\n",
           this, statusCode));

  mState = WCC_ONSTOP;

  mIsPending = PR_FALSE;
  mStatus = statusCode;

  mListener->OnStopRequest(this, mListenerContext, statusCode);

  mListener = 0;
  mListenerContext = 0;

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, mStatus);

  mCallbacks = 0;
  mProgressSink = 0;

  if (mIPCOpen)
    PWyciwygChannelChild::Send__delete__(this);

  return true;
}







NS_IMETHODIMP
WyciwygChannelChild::GetName(nsACString & aName)
{
  return mURI->GetSpec(aName);
}


NS_IMETHODIMP
WyciwygChannelChild::IsPending(PRBool *aIsPending)
{
  *aIsPending = mIsPending;
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetStatus(nsresult *aStatus)
{
  *aStatus = mStatus;
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::Cancel(nsresult aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::Suspend()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::Resume()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
  *aLoadGroup = mLoadGroup;
  NS_IF_ADDREF(*aLoadGroup);
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  NS_QueryNotificationCallbacks(mCallbacks,
                                mLoadGroup,
                                NS_GET_IID(nsIProgressEventSink),
                                getter_AddRefs(mProgressSink));
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}







NS_IMETHODIMP
WyciwygChannelChild::GetOriginalURI(nsIURI * *aOriginalURI)
{
  *aOriginalURI = mOriginalURI;
  NS_ADDREF(*aOriginalURI);
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetOriginalURI(nsIURI * aOriginalURI)
{
  NS_ENSURE_TRUE(mState == WCC_INIT, NS_ERROR_UNEXPECTED);

  NS_ENSURE_ARG_POINTER(aOriginalURI);
  mOriginalURI = aOriginalURI;
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetURI(nsIURI * *aURI)
{
  *aURI = mURI;
  NS_IF_ADDREF(*aURI);
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetOwner(nsISupports * *aOwner)
{
  NS_PRECONDITION(mOwner, "Must have a principal!");
  NS_ENSURE_STATE(mOwner);

  NS_ADDREF(*aOwner = mOwner);
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetOwner(nsISupports * aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetNotificationCallbacks(nsIInterfaceRequestor * *aCallbacks)
{
  *aCallbacks = mCallbacks;
  NS_IF_ADDREF(*aCallbacks);
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetNotificationCallbacks(nsIInterfaceRequestor * aCallbacks)
{
  mCallbacks = aCallbacks;
  NS_QueryNotificationCallbacks(mCallbacks,
                                mLoadGroup,
                                NS_GET_IID(nsIProgressEventSink),
                                getter_AddRefs(mProgressSink));
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);

  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetContentType(nsACString & aContentType)
{
  aContentType.AssignLiteral(WYCIWYG_TYPE);
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetContentType(const nsACString & aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::GetContentCharset(nsACString & aContentCharset)
{
  aContentCharset.Assign("UTF-16");
  return NS_OK;
}
NS_IMETHODIMP
WyciwygChannelChild::SetContentCharset(const nsACString & aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::GetContentLength(PRInt32 *aContentLength)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
WyciwygChannelChild::SetContentLength(PRInt32 aContentLength)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::Open(nsIInputStream **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WyciwygChannelChild::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
  LOG(("WyciwygChannelChild::AsyncOpen [this=%x]\n", this));

  
  
  
  NS_PRECONDITION(mOwner, "Must have a principal");
  NS_ENSURE_STATE(mOwner);

  NS_ENSURE_ARG_POINTER(aListener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

  mListener = aListener;
  mListenerContext = aContext;
  mIsPending = PR_TRUE;

  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  SendAsyncOpen(IPC::URI(mOriginalURI), mLoadFlags);

  mState = WCC_OPENED;

  return NS_OK;
}







NS_IMETHODIMP
WyciwygChannelChild::WriteToCacheEntry(const nsAString & aData)
{
  NS_ENSURE_TRUE((mState == WCC_INIT) ||
                 (mState == WCC_ONWRITE), NS_ERROR_UNEXPECTED);

  SendWriteToCacheEntry(PromiseFlatString(aData));
  mState = WCC_ONWRITE;
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::CloseCacheEntry(nsresult reason)
{
  NS_ENSURE_TRUE(mState == WCC_ONWRITE, NS_ERROR_UNEXPECTED);

  SendCloseCacheEntry(reason);
  mState = WCC_ONCLOSED;

  if (mIPCOpen)
    PWyciwygChannelChild::Send__delete__(this);

  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::SetSecurityInfo(nsISupports *aSecurityInfo)
{
  mSecurityInfo = aSecurityInfo;

  if (mSecurityInfo) {
    nsCOMPtr<nsISerializable> serializable = do_QueryInterface(mSecurityInfo);
    if (serializable) {
      nsCString secInfoStr;
      NS_SerializeToString(serializable, secInfoStr);
      SendSetSecurityInfo(secInfoStr);
    }
    else {
      NS_WARNING("Can't serialize security info");
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::SetCharsetAndSource(PRInt32 aSource, const nsACString & aCharset)
{
  
  
  NS_ENSURE_TRUE((mState == WCC_ONSTART) ||
                 (mState == WCC_INIT), NS_ERROR_UNEXPECTED);

  mCharsetSource = aSource;
  mCharset = aCharset;

  
  SendSetCharsetAndSource(mCharsetSource, mCharset);
  return NS_OK;
}


NS_IMETHODIMP
WyciwygChannelChild::GetCharsetAndSource(PRInt32 *aSource NS_OUTPARAM, nsACString & _retval)
{
  NS_ENSURE_TRUE((mState == WCC_ONSTART) ||
                 (mState == WCC_ONDATA) ||
                 (mState == WCC_ONSTOP), NS_ERROR_NOT_AVAILABLE);

  if (mCharsetSource == kCharsetUninitialized)
    return NS_ERROR_NOT_AVAILABLE;

  *aSource = mCharsetSource;
  _retval = mCharset;
  return NS_OK;
}


}} 
