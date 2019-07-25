












































#ifndef nsEventSource_h__
#define nsEventSource_h__

#include "nsIEventSource.h"
#include "nsIJSNativeInitializer.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsIObserver.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsITimer.h"
#include "nsIHttpChannel.h"
#include "nsWeakReference.h"
#include "nsDeque.h"
#include "nsIUnicodeDecoder.h"

#define NS_EVENTSOURCE_CID                          \
 { /* 755e2d2d-a836-4539-83f4-16b51156341f */       \
  0x755e2d2d, 0xa836, 0x4539,                       \
 {0x83, 0xf4, 0x16, 0xb5, 0x11, 0x56, 0x34, 0x1f} }

#define NS_EVENTSOURCE_CONTRACTID "@mozilla.org/eventsource;1"

class AsyncVerifyRedirectCallbackFwr;
class nsAutoClearFields;

class nsEventSource: public nsDOMEventTargetWrapperCache,
                     public nsIEventSource,
                     public nsIJSNativeInitializer,
                     public nsIObserver,
                     public nsIStreamListener,
                     public nsIChannelEventSink,
                     public nsIInterfaceRequestor,
                     public nsSupportsWeakReference
{
friend class AsyncVerifyRedirectCallbackFwr;

public:
  nsEventSource();
  virtual ~nsEventSource();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsEventSource,
                                           nsDOMEventTargetWrapperCache)

  NS_DECL_NSIEVENTSOURCE

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                        PRUint32 argc, jsval* argv);

  NS_DECL_NSIOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

  
  static PRBool PrefEnabled();

protected:
  nsresult GetBaseURI(nsIURI **aBaseURI);

  nsresult SetupHttpChannel();
  nsresult InitChannelAndRequestEventSource();
  nsresult ResetConnection();
  nsresult DispatchFailConnection();
  nsresult SetReconnectionTimeout();

  void AnnounceConnection();
  void DispatchAllMessageEvents();
  void ReestablishConnection();
  void FailConnection();

  nsresult Thaw();
  nsresult Freeze();

  static void TimerCallback(nsITimer *aTimer, void *aClosure);

  nsresult PrintErrorOnConsole(const char       *aBundleURI,
                               const PRUnichar  *aError,
                               const PRUnichar **aFormatStrings,
                               PRUint32          aFormatStringsLen);
  nsresult ConsoleError();

  static NS_METHOD StreamReaderFunc(nsIInputStream *aInputStream,
                                    void           *aClosure,
                                    const char     *aFromRawSegment,
                                    PRUint32        aToOffset,
                                    PRUint32        aCount,
                                    PRUint32       *aWriteCount);
  nsresult SetFieldAndClear();
  nsresult ClearFields();
  nsresult ResetEvent();
  nsresult DispatchCurrentMessageEvent();
  nsresult ParseCharacter(PRUnichar aChr);
  PRBool CheckCanRequestSrc(nsIURI* aSrc = nsnull);  
  nsresult CheckHealthOfRequestCallback(nsIRequest *aRequestCallback);
  nsresult OnRedirectVerifyCallback(nsresult result);

  nsCOMPtr<nsIURI> mSrc;

  nsString mLastEventID;
  PRUint32 mReconnectionTime;  

  struct Message {
    nsString mEventName;
    nsString mLastEventID;
    nsString mData;
  };
  nsDeque mMessagesToDispatch;
  Message mCurrentMessage;

  











































  enum ParserStatus {
    PARSE_STATE_OFF,
    PARSE_STATE_BEGIN_OF_STREAM,
    PARSE_STATE_BOM_WAS_READ,
    PARSE_STATE_CR_CHAR,
    PARSE_STATE_COMMENT,
    PARSE_STATE_FIELD_NAME,
    PARSE_STATE_FIRST_CHAR_OF_FIELD_VALUE,
    PARSE_STATE_FIELD_VALUE,
    PARSE_STATE_BEGIN_OF_LINE
  };
  ParserStatus mStatus;

  PRPackedBool mFrozen;
  PRPackedBool mErrorLoadOnRedirect;
  PRPackedBool mGoingToDispatchAllMessages;

  
  nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;
  nsresult mLastConvertionResult;
  nsString mLastFieldName;
  nsString mLastFieldValue;

  nsRefPtr<nsDOMEventListenerWrapper> mOnOpenListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnMessageListener;

  nsCOMPtr<nsILoadGroup> mLoadGroup;

  



  nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;
  nsCOMPtr<nsIChannelEventSink>   mChannelEventSink;

  nsCOMPtr<nsIHttpChannel> mHttpChannel;

  nsCOMPtr<nsITimer> mTimer;

  PRInt32 mReadyState;
  nsString mOriginalURL;

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCString mOrigin;

  PRUint32 mRedirectFlags;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  
  
  
  
  
  
  nsString mScriptFile;
  PRUint32 mScriptLine;
  PRUint64 mWindowID;

private:
  nsEventSource(const nsEventSource& x);   
  nsEventSource& operator=(const nsEventSource& x);
};

#endif 
