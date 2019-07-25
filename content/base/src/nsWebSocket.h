






































#ifndef nsWebSocket_h__
#define nsWebSocket_h__

#include "nsISupportsUtils.h"
#include "nsIMozWebSocket.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIJSNativeInitializer.h"
#include "nsIPrincipal.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMEventListener.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsIDOMDOMStringList.h"

#define DEFAULT_WS_SCHEME_PORT  80
#define DEFAULT_WSS_SCHEME_PORT 443

#define NS_WEBSOCKET_CID                            \
 { /* 7ca25214-98dc-40a6-bc1f-41ddbe41f46c */       \
  0x7ca25214, 0x98dc, 0x40a6,                       \
 {0xbc, 0x1f, 0x41, 0xdd, 0xbe, 0x41, 0xf4, 0x6c} }

#define NS_WEBSOCKET_CONTRACTID "@mozilla.org/websocket;1"

class nsWSNetAddressComparator;
class nsWebSocketEstablishedConnection;
class nsWSCloseEvent;

class nsWebSocket: public nsDOMEventTargetWrapperCache,
                   public nsIMozWebSocket,
                   public nsIJSNativeInitializer
{
friend class nsWSNetAddressComparator;
friend class nsWebSocketEstablishedConnection;
friend class nsWSCloseEvent;

public:
  nsWebSocket();
  virtual ~nsWebSocket();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsWebSocket,
                                           nsDOMEventTargetWrapperCache)
  NS_DECL_NSIMOZWEBSOCKET

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObject, PRUint32 aArgc, jsval* aArgv);

  
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener *aListener,
                              bool aUseCapture,
                              bool aWantsUntrusted,
                              PRUint8 optional_argc);
  NS_IMETHOD RemoveEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 bool aUseCapture);

  
  static bool PrefEnabled();

  const PRUint64 InnerWindowID() const { return mInnerWindowID; }
  const nsCString& GetScriptFile() const { return mScriptFile; }
  const PRUint32 GetScriptLine() const { return mScriptLine; }

protected:
  nsresult ParseURL(const nsString& aURL);
  nsresult EstablishConnection();

  nsresult CreateAndDispatchSimpleEvent(const nsString& aName);
  nsresult CreateAndDispatchMessageEvent(const nsACString& aData);
  nsresult CreateAndDispatchCloseEvent(bool aWasClean, PRUint16 aCode,
                                       const nsString &aReason);

  
  void SetReadyState(PRUint16 aNewReadyState);

  
  
  
  void UpdateMustKeepAlive();
  
  
  void DontKeepAliveAnyMore();

  nsRefPtr<nsDOMEventListenerWrapper> mOnOpenListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnMessageListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnCloseListener;

  
  nsString mOriginalURL;
  bool mSecure; 
                        

  bool mKeepingAlive;
  bool mCheckMustKeepAlive;
  bool mTriggeredCloseEvent;

  nsCString mClientReason;
  PRUint16  mClientReasonCode;
  nsString  mServerReason;
  PRUint16  mServerReasonCode;

  nsCString mAsciiHost;  
  PRUint32  mPort;
  nsCString mResource; 
  nsString  mUTF16Origin;
  
  nsCOMPtr<nsIURI> mURI;
  nsCString mRequestedProtocolList;
  nsCString mEstablishedProtocol;
  nsCString mEstablishedExtensions;

  PRUint16 mReadyState;

  nsCOMPtr<nsIPrincipal> mPrincipal;

  nsRefPtr<nsWebSocketEstablishedConnection> mConnection;
  PRUint32 mOutgoingBufferedAmount; 
                                    
                                    

  
  
  
  
  
  
  nsCString mScriptFile;
  PRUint32 mScriptLine;
  PRUint64 mInnerWindowID;

private:
  nsWebSocket(const nsWebSocket& x);   
  nsWebSocket& operator=(const nsWebSocket& x);
};

#endif
