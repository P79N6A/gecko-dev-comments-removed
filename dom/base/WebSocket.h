





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
#include "nsISupports.h"
#include "nsISupportsUtils.h"
#include "nsString.h"
#include "nsWrapperCache.h"

#define DEFAULT_WS_SCHEME_PORT  80
#define DEFAULT_WSS_SCHEME_PORT 443

class nsIInputStream;

namespace mozilla {
namespace dom {

class File;

class WebSocketImpl;

class WebSocket final : public DOMEventTargetHelper
{
  friend class WebSocketImpl;

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

  
  virtual void EventListenerAdded(nsIAtom* aType) override;
  virtual void EventListenerRemoved(nsIAtom* aType) override;

  virtual void DisconnectFromOwner() override;

  
  nsPIDOMWindow* GetParentObject() { return GetOwner(); }

  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

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

  
  uint16_t ReadyState();

  
  uint32_t BufferedAmount() const;

  
  IMPL_EVENT_HANDLER(open)

  
  IMPL_EVENT_HANDLER(error)

  
  IMPL_EVENT_HANDLER(close)

  
  void GetExtensions(nsAString& aResult);

  
  void GetProtocol(nsAString& aResult);

  
  void Close(const Optional<uint16_t>& aCode,
             const Optional<nsAString>& aReason,
             ErrorResult& aRv);

  
  IMPL_EVENT_HANDLER(message)

  
  dom::BinaryType BinaryType() const;
  void SetBinaryType(dom::BinaryType aData);

  
  void Send(const nsAString& aData,
            ErrorResult& aRv);
  void Send(File& aData,
            ErrorResult& aRv);
  void Send(const ArrayBuffer& aData,
            ErrorResult& aRv);
  void Send(const ArrayBufferView& aData,
            ErrorResult& aRv);

private: 
  explicit WebSocket(nsPIDOMWindow* aOwnerWindow);
  virtual ~WebSocket();

  void SetReadyState(uint16_t aReadyState);

  
  nsresult CreateAndDispatchSimpleEvent(const nsAString& aName);
  nsresult CreateAndDispatchMessageEvent(const nsACString& aData,
                                         bool aIsBinary);
  nsresult CreateAndDispatchMessageEvent(JSContext* aCx,
                                         const nsACString& aData,
                                         bool aIsBinary);
  nsresult CreateAndDispatchCloseEvent(bool aWasClean,
                                       uint16_t aCode,
                                       const nsAString& aReason);

  
  
  
  void UpdateMustKeepAlive();
  
  
  void DontKeepAliveAnyMore();

private:
  WebSocket(const WebSocket& x) = delete;   
  WebSocket& operator=(const WebSocket& x) = delete;

  void Send(nsIInputStream* aMsgStream,
            const nsACString& aMsgString,
            uint32_t aMsgLength,
            bool aIsBinary,
            ErrorResult& aRv);

  void AssertIsOnTargetThread() const;

  
  
  WebSocketImpl* mImpl;

  bool mIsMainThread;

  bool mKeepingAlive;
  bool mCheckMustKeepAlive;

  uint32_t mOutgoingBufferedAmount;

  
  nsString mURI;
  nsString mEffectiveURL;   
  nsCString mEstablishedExtensions;
  nsCString mEstablishedProtocol;

  dom::BinaryType mBinaryType;

  
  
  mozilla::Mutex mMutex;

  
  uint16_t mReadyState;
};

} 
} 

#endif
