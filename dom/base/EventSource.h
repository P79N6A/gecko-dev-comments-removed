











#ifndef mozilla_dom_EventSource_h
#define mozilla_dom_EventSource_h

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsIObserver.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsITimer.h"
#include "nsIHttpChannel.h"
#include "nsWeakReference.h"
#include "nsDeque.h"
#include "nsIUnicodeDecoder.h"

class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

class AsyncVerifyRedirectCallbackFwr;
struct EventSourceInit;

class EventSource : public DOMEventTargetHelper
                  , public nsIObserver
                  , public nsIStreamListener
                  , public nsIChannelEventSink
                  , public nsIInterfaceRequestor
                  , public nsSupportsWeakReference
{
friend class AsyncVerifyRedirectCallbackFwr;

public:
  explicit EventSource(nsPIDOMWindow* aOwnerWindow);
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_INHERITED(
    EventSource, DOMEventTargetHelper)

  NS_DECL_NSIOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

  
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }
  static already_AddRefed<EventSource>
  Constructor(const GlobalObject& aGlobal, const nsAString& aURL,
              const EventSourceInit& aEventSourceInitDict,
              ErrorResult& aRv);

  void GetUrl(nsAString& aURL) const
  {
    aURL = mOriginalURL;
  }
  bool WithCredentials() const
  {
    return mWithCredentials;
  }

  enum {
    CONNECTING = 0U,
    OPEN = 1U,
    CLOSED = 2U
  };
  uint16_t ReadyState() const
  {
    return mReadyState;
  }

  IMPL_EVENT_HANDLER(open)
  IMPL_EVENT_HANDLER(message)
  IMPL_EVENT_HANDLER(error)
  void Close();

  
  static bool PrefEnabled(JSContext* aCx = nullptr, JSObject* aGlobal = nullptr);

  virtual void DisconnectFromOwner() MOZ_OVERRIDE;

protected:
  virtual ~EventSource();

  nsresult Init(nsISupports* aOwner,
                const nsAString& aURL,
                bool aWithCredentials);

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
                               const char16_t  *aError,
                               const char16_t **aFormatStrings,
                               uint32_t          aFormatStringsLen);
  nsresult ConsoleError();

  static NS_METHOD StreamReaderFunc(nsIInputStream *aInputStream,
                                    void           *aClosure,
                                    const char     *aFromRawSegment,
                                    uint32_t        aToOffset,
                                    uint32_t        aCount,
                                    uint32_t       *aWriteCount);
  nsresult SetFieldAndClear();
  nsresult ClearFields();
  nsresult ResetEvent();
  nsresult DispatchCurrentMessageEvent();
  nsresult ParseCharacter(char16_t aChr);
  bool CheckCanRequestSrc(nsIURI* aSrc = nullptr);  
  nsresult CheckHealthOfRequestCallback(nsIRequest *aRequestCallback);
  nsresult OnRedirectVerifyCallback(nsresult result);

  nsCOMPtr<nsIURI> mSrc;

  nsString mLastEventID;
  uint32_t mReconnectionTime;  

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

  bool mFrozen;
  bool mErrorLoadOnRedirect;
  bool mGoingToDispatchAllMessages;
  bool mWithCredentials;
  bool mWaitingForOnStopRequest;
  bool mInterrupted;

  
  nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;
  nsresult mLastConvertionResult;
  nsString mLastFieldName;
  nsString mLastFieldValue;

  nsCOMPtr<nsILoadGroup> mLoadGroup;

  



  nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;
  nsCOMPtr<nsIChannelEventSink>   mChannelEventSink;

  nsCOMPtr<nsIHttpChannel> mHttpChannel;

  nsCOMPtr<nsITimer> mTimer;

  uint16_t mReadyState;
  nsString mOriginalURL;

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsString mOrigin;

  uint32_t mRedirectFlags;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  
  
  
  
  
  
  nsString mScriptFile;
  uint32_t mScriptLine;
  uint64_t mInnerWindowID;

private:
  EventSource(const EventSource& x);   
  EventSource& operator=(const EventSource& x);
};

} 
} 

#endif 
