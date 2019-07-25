






































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
#include "nsDOMEventTargetHelper.h"
#include "nsAutoPtr.h"
#include "nsIDOMDOMStringList.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWebSocketChannel.h"
#include "nsIWebSocketListener.h"
#include "nsIRequest.h"

#define DEFAULT_WS_SCHEME_PORT  80
#define DEFAULT_WSS_SCHEME_PORT 443

#define NS_WEBSOCKET_CID                            \
 { /* 7ca25214-98dc-40a6-bc1f-41ddbe41f46c */       \
  0x7ca25214, 0x98dc, 0x40a6,                       \
 {0xbc, 0x1f, 0x41, 0xdd, 0xbe, 0x41, 0xf4, 0x6c} }

#define NS_WEBSOCKET_CONTRACTID "@mozilla.org/websocket;1"

class nsWSCloseEvent;
class nsAutoCloseWS;

class nsWebSocket: public nsDOMEventTargetHelper,
                   public nsIWebSocket,
                   public nsIJSNativeInitializer,
                   public nsIInterfaceRequestor,
                   public nsIWebSocketListener,
                   public nsIRequest
{
friend class nsWSCloseEvent;
friend class nsAutoCloseWS;

public:
  nsWebSocket();
  virtual ~nsWebSocket();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_INHERITED(nsWebSocket,
                                                                   nsDOMEventTargetHelper)
  NS_DECL_NSIWEBSOCKET
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIWEBSOCKETLISTENER
  NS_DECL_NSIREQUEST

  
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

protected:
  nsresult ParseURL(const nsString& aURL);
  nsresult EstablishConnection();

  
  nsresult FailConnection(PRUint16 reasonCode,
                          const nsACString& aReasonString = EmptyCString());
  nsresult CloseConnection(PRUint16 reasonCode,
                           const nsACString& aReasonString = EmptyCString());
  nsresult Disconnect();

  nsresult ConsoleError();
  nsresult PrintErrorOnConsole(const char       *aBundleURI,
                               const PRUnichar  *aError,
                               const PRUnichar **aFormatStrings,
                               PRUint32          aFormatStringsLen);

  nsresult ConvertTextToUTF8(const nsString& aMessage, nsCString& buf);

  
  nsresult GetSendParams(nsIVariant *aData, nsCString &aStringOut,
                         nsCOMPtr<nsIInputStream> &aStreamOut,
                         bool &aIsBinary, PRUint32 &aOutgoingLength);

  nsresult DoOnMessageAvailable(const nsACString & aMsg, bool isBinary);
  nsresult CreateAndDispatchSimpleEvent(const nsString& aName);
  nsresult CreateAndDispatchMessageEvent(const nsACString& aData,
                                         bool isBinary);
  nsresult CreateAndDispatchCloseEvent(bool aWasClean, PRUint16 aCode,
                                       const nsString &aReason);
  nsresult CreateResponseBlob(const nsACString& aData, JSContext *aCx,
                              jsval &jsData);

  void SetReadyState(PRUint16 aNewReadyState);

  
  
  
  void UpdateMustKeepAlive();
  
  
  void DontKeepAliveAnyMore();

  nsresult UpdateURI();

  nsCOMPtr<nsIWebSocketChannel> mChannel;

  nsRefPtr<nsDOMEventListenerWrapper> mOnOpenListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnMessageListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnCloseListener;

  
  nsString mOriginalURL;
  nsString mEffectiveURL;   
  bool mSecure; 
                        

  bool mKeepingAlive;
  bool mCheckMustKeepAlive;
  bool mTriggeredCloseEvent;
  bool mDisconnected;

  
  bool      mCloseEventWasClean;
  nsString  mCloseEventReason;
  PRUint16  mCloseEventCode;

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

  PRUint32 mOutgoingBufferedAmount;

  enum
  {
    WS_BINARY_TYPE_ARRAYBUFFER,
    WS_BINARY_TYPE_BLOB,
  } mBinaryType;

  
  
  
  
  
  
  nsCString mScriptFile;
  PRUint32 mScriptLine;
  PRUint64 mInnerWindowID;

private:
  nsWebSocket(const nsWebSocket& x);   
  nsWebSocket& operator=(const nsWebSocket& x);
};

#endif
