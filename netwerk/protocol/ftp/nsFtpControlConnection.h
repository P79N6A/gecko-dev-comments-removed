





#ifndef nsFtpControlConnection_h___
#define nsFtpControlConnection_h___

#include "nsCOMPtr.h"

#include "nsISocketTransport.h"
#include "nsIAsyncInputStream.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsIOutputStream;
class nsIProxyInfo;
class nsITransportEventSink;

class nsFtpControlConnectionListener : public nsISupports {
public:
    






    virtual void OnControlDataAvailable(const char *data, uint32_t dataLen) = 0;

    




    virtual void OnControlError(nsresult status) = 0;
};

class nsFtpControlConnection MOZ_FINAL : public nsIInputStreamCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAMCALLBACK

    nsFtpControlConnection(const nsCSubstring& host, uint32_t port);
    ~nsFtpControlConnection();

    nsresult Connect(nsIProxyInfo* proxyInfo, nsITransportEventSink* eventSink);
    nsresult Disconnect(nsresult status);
    nsresult Write(const nsCSubstring& command);

    bool IsAlive();

    nsITransport *Transport()   { return mSocket; }

    





    nsresult WaitData(nsFtpControlConnectionListener *listener);

    uint32_t         mServerType;           
    nsString         mPassword;
    int32_t          mSuspendedWrite;
    nsCString        mPwd;
    uint32_t         mSessionId;

private:
    nsCString mHost;
    uint32_t  mPort;

    nsCOMPtr<nsISocketTransport>     mSocket;
    nsCOMPtr<nsIOutputStream>        mSocketOutput;
    nsCOMPtr<nsIAsyncInputStream>    mSocketInput;

    nsRefPtr<nsFtpControlConnectionListener> mListener;
};

#endif
