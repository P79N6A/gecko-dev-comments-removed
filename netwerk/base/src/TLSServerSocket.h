




#ifndef mozilla_net_TLSServerSocket_h
#define mozilla_net_TLSServerSocket_h

#include "nsAutoPtr.h"
#include "nsITLSServerSocket.h"
#include "nsServerSocket.h"
#include "mozilla/Mutex.h"
#include "seccomon.h"

namespace mozilla {
namespace net {

class TLSServerSocket : public nsServerSocket
                      , public nsITLSServerSocket
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSISERVERSOCKET(nsServerSocket::)
  NS_DECL_NSITLSSERVERSOCKET

  
  virtual void CreateClientTransport(PRFileDesc* clientFD,
                                     const NetAddr& clientAddr) MOZ_OVERRIDE;
  virtual nsresult SetSocketDefaults() MOZ_OVERRIDE;
  virtual nsresult OnSocketListen() MOZ_OVERRIDE;

  TLSServerSocket();

private:
  virtual ~TLSServerSocket();

  static SECStatus AuthCertificateHook(void* arg, PRFileDesc* fd,
                                       PRBool checksig, PRBool isServer);

  nsCOMPtr<nsIX509Cert>                  mServerCert;
};

class TLSServerConnectionInfo : public nsITLSServerConnectionInfo
                              , public nsITLSClientStatus
{
  friend class TLSServerSocket;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITLSSERVERCONNECTIONINFO
  NS_DECL_NSITLSCLIENTSTATUS

  TLSServerConnectionInfo();

private:
  virtual ~TLSServerConnectionInfo();

  static void HandshakeCallback(PRFileDesc* aFD, void* aArg);
  nsresult HandshakeCallback(PRFileDesc* aFD);

  nsRefPtr<TLSServerSocket>              mServerSocket;
  
  
  
  
  nsISocketTransport*                    mTransport;
  nsCOMPtr<nsIX509Cert>                  mPeerCert;
  int16_t                                mTlsVersionUsed;
  nsCString                              mCipherName;
  uint32_t                               mKeyLength;
  uint32_t                               mMacLength;
  
  mozilla::Mutex                         mLock;
  nsCOMPtr<nsITLSServerSecurityObserver> mSecurityObserver;
};

} 
} 

#endif 
