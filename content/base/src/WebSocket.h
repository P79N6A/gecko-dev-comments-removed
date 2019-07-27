





#ifndef WebSocket_h__
#define WebSocket_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/WebSocketBinding.h" 
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIRequest.h"
#include "nsISupports.h"
#include "nsISupportsUtils.h"
#include "nsIWebSocketChannel.h"
#include "nsIWebSocketListener.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsWrapperCache.h"

#define DEFAULT_WS_SCHEME_PORT  80
#define DEFAULT_WSS_SCHEME_PORT 443

namespace mozilla {
namespace dom {

class WebSocket MOZ_FINAL : public DOMEventTargetHelper,
                            public nsIInterfaceRequestor,
                            public nsIWebSocketListener,
                            public nsIObserver,
                            public nsSupportsWeakReference,
                            public nsIRequest
{
friend class CallDispatchConnectionCloseEvents;
friend class nsAutoCloseWS;

public:
  enum {
    CONNECTING = 0,
    OPEN       = 1,
    CLOSING    = 2,
    CLOSED     = 3
  };

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_INHERITED(
    WebSocket, DOMEventTargetHelper)
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIWEBSOCKETLISTENER
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIREQUEST

  
  virtual void EventListenerAdded(nsIAtom* aType) MOZ_OVERRIDE;
  virtual void EventListenerRemoved(nsIAtom* aType) MOZ_OVERRIDE;

  virtual void DisconnectFromOwner() MOZ_OVERRIDE;

  
  nsPIDOMWindow* GetParentObject() { return GetOwner(); }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

public: 

  
  static bool PrefEnabled(JSContext* aCx = nullptr, JSObject* aGlobal = nullptr);

public: 

  
  static already_AddRefed<WebSocket> Constructor(const GlobalObject& aGlobal,
                                                 const nsAString& aUrl,
                                                 ErrorResult& rv);

  static already_AddRefed<WebSocket> Constructor(const GlobalObject& aGlobal,
                                                 const nsAString& aUrl,
                                                 const nsAString& aProtocol,
                                                 ErrorResult& rv);

  static already_AddRefed<WebSocket> Constructor(const GlobalObject& aGlobal,
                                                 const nsAString& aUrl,
                                                 const Sequence<nsString>& aProtocols,
                                                 ErrorResult& rv);

  
  void GetUrl(nsAString& aResult);

  
  uint16_t ReadyState() const { return mReadyState; }

  
  uint32_t BufferedAmount() const { return mOutgoingBufferedAmount; }

  
  IMPL_EVENT_HANDLER(open)

  
  IMPL_EVENT_HANDLER(error)

  
  IMPL_EVENT_HANDLER(close)

  
  void GetExtensions(nsAString& aResult);

  
  void GetProtocol(nsAString& aResult);

  
  void Close(const Optional<uint16_t>& aCode,
             const Optional<nsAString>& aReason,
             ErrorResult& aRv);

  
  IMPL_EVENT_HANDLER(message)

  
  dom::BinaryType BinaryType() const { return mBinaryType; }
  void SetBinaryType(dom::BinaryType aData) { mBinaryType = aData; }

  
  void Send(const nsAString& aData,
            ErrorResult& aRv);
  void Send(nsIDOMBlob* aData,
            ErrorResult& aRv);
  void Send(const ArrayBuffer& aData,
            ErrorResult& aRv);
  void Send(const ArrayBufferView& aData,
            ErrorResult& aRv);

private: 
  WebSocket(nsPIDOMWindow* aOwnerWindow);
  virtual ~WebSocket();

protected:
  nsresult Init(JSContext* aCx,
                nsIPrincipal* aPrincipal,
                const nsAString& aURL,
                nsTArray<nsString>& aProtocolArray);

  void Send(nsIInputStream* aMsgStream,
            const nsACString& aMsgString,
            uint32_t aMsgLength,
            bool aIsBinary,
            ErrorResult& aRv);

  nsresult ParseURL(const nsString& aURL);
  nsresult EstablishConnection();

  
  void FailConnection(uint16_t reasonCode,
                      const nsACString& aReasonString = EmptyCString());
  nsresult CloseConnection(uint16_t reasonCode,
                           const nsACString& aReasonString = EmptyCString());
  nsresult Disconnect();

  nsresult ConsoleError();
  nsresult PrintErrorOnConsole(const char* aBundleURI,
                               const char16_t* aError,
                               const char16_t** aFormatStrings,
                               uint32_t aFormatStringsLen);

  nsresult DoOnMessageAvailable(const nsACString& aMsg,
                                bool isBinary);

  
  
  
  
  nsresult ScheduleConnectionCloseEvents(nsISupports* aContext,
                                         nsresult aStatusCode,
                                         bool sync);
  
  void DispatchConnectionCloseEvents();

  
  nsresult CreateAndDispatchSimpleEvent(const nsString& aName);
  nsresult CreateAndDispatchMessageEvent(const nsACString& aData,
                                         bool isBinary);
  nsresult CreateAndDispatchCloseEvent(bool aWasClean,
                                       uint16_t aCode,
                                       const nsString& aReason);

  
  
  
  void UpdateMustKeepAlive();
  
  
  void DontKeepAliveAnyMore();

  nsresult UpdateURI();

protected: 

  nsCOMPtr<nsIWebSocketChannel> mChannel;

  
  nsString mOriginalURL;
  nsString mEffectiveURL;   
  bool mSecure; 
                

  bool mKeepingAlive;
  bool mCheckMustKeepAlive;
  bool mOnCloseScheduled;
  bool mFailed;
  bool mDisconnected;

  
  bool      mCloseEventWasClean;
  nsString  mCloseEventReason;
  uint16_t  mCloseEventCode;

  nsCString mAsciiHost;  
  uint32_t  mPort;
  nsCString mResource; 
  nsString  mUTF16Origin;

  nsCOMPtr<nsIURI> mURI;
  nsCString mRequestedProtocolList;
  nsCString mEstablishedProtocol;
  nsCString mEstablishedExtensions;

  uint16_t mReadyState;

  nsCOMPtr<nsIPrincipal> mPrincipal;

  uint32_t mOutgoingBufferedAmount;

  dom::BinaryType mBinaryType;

  
  
  
  
  
  
  nsCString mScriptFile;
  uint32_t mScriptLine;
  uint64_t mInnerWindowID;

private:
  WebSocket(const WebSocket& x) MOZ_DELETE;   
  WebSocket& operator=(const WebSocket& x) MOZ_DELETE;
};

} 
} 

#endif
