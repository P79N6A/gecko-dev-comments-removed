






































#ifndef nsWebSocket_h__
#define nsWebSocket_h__

#include "nsISupportsUtils.h"
#include "nsIWebSocket.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIJSNativeInitializer.h"
#include "nsIPrincipal.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMEventListener.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsIProxiedProtocolHandler.h"

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
                   public nsIWebSocket,
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
  NS_DECL_NSIWEBSOCKET

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObject, PRUint32 aArgc, jsval* aArgv);

  
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture);
  NS_IMETHOD RemoveEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 PRBool aUseCapture);

  
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener *aListener,
                              PRBool aUseCapture,
                              PRBool aWantsUntrusted,
                              PRUint8 optional_argc);

  static void ReleaseGlobals();

  
  static PRBool PrefEnabled();

protected:
  nsresult ParseURL(const nsString& aURL);
  nsresult SetProtocol(const nsString& aProtocol);
  nsresult EstablishConnection();

  nsresult CreateAndDispatchSimpleEvent(const nsString& aName);
  nsresult CreateAndDispatchMessageEvent(nsCString *aData);
  nsresult CreateAndDispatchCloseEvent(PRBool aWasClean);

  
  void SetReadyState(PRUint16 aNewReadyState);

  
  
  
  void UpdateMustKeepAlive();
  
  
  void DontKeepAliveAnyMore();

  nsRefPtr<nsDOMEventListenerWrapper> mOnOpenListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnMessageListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnCloseListener;

  
  nsString mOriginalURL;
  PRPackedBool mSecure; 
                        

  PRPackedBool mKeepingAlive;
  PRPackedBool mCheckMustKeepAlive;
  PRPackedBool mTriggeredCloseEvent;

  nsCString mAsciiHost;  
  PRUint32  mPort;
  nsCString mResource; 
  nsCString mOrigin;
  nsCOMPtr<nsIURI> mURI;
  nsCString mProtocol;

  PRUint16 mReadyState;

  nsCOMPtr<nsIPrincipal> mPrincipal;

  nsRefPtr<nsWebSocketEstablishedConnection> mConnection;
  PRUint32 mOutgoingBufferedAmount; 
                                    
                                    

private:
  nsWebSocket(const nsWebSocket& x);   
  nsWebSocket& operator=(const nsWebSocket& x);
};

#define NS_WSPROTOCOLHANDLER_CONTRACTID \
    NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "ws"

#define NS_WSSPROTOCOLHANDLER_CONTRACTID \
    NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "wss"

#define NS_WSPROTOCOLHANDLER_CID                     \
{ /* a4e6aa3b-b6db-4809-aa11-e292e074cbc4 */         \
    0xa4e6aa3b,                                      \
    0xb6db,                                          \
    0x4809,                                          \
    {0xaa, 0x11, 0xe2, 0x92, 0xe0, 0x74, 0xcb, 0xc4} \
}

#define NS_WSSPROTOCOLHANDLER_CID                    \
{ /* c6531804-b5c8-4a53-80bf-e339b82d3161 */         \
    0xc6531804,                                      \
    0xb5c8,                                          \
    0x4a53,                                          \
    {0x80, 0xbf, 0xe3, 0x39, 0xb8, 0x2d, 0x31, 0x61} \
}

class nsWSProtocolHandler: public nsIProxiedProtocolHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROTOCOLHANDLER
  NS_DECL_NSIPROXIEDPROTOCOLHANDLER

  nsWSProtocolHandler() {};
};

class nsWSSProtocolHandler: public nsWSProtocolHandler
{
public:
  NS_IMETHOD GetScheme(nsACString & aScheme);
  NS_IMETHOD GetDefaultPort(PRInt32 *aDefaultPort);
  nsWSSProtocolHandler() {};
};

#endif
