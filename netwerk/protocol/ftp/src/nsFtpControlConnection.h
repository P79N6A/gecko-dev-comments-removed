





































#ifndef nsFtpControlConnection_h___
#define nsFtpControlConnection_h___

#include "nsCOMPtr.h"

#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "nsIRequest.h"
#include "nsISocketTransport.h"
#include "nsIOutputStream.h"
#include "nsIAsyncInputStream.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsIProxyInfo;
class nsITransportEventSink;

class nsFtpControlConnectionListener : public nsISupports {
public:
    






    virtual void OnControlDataAvailable(const char *data, PRUint32 dataLen) = 0;

    




    virtual void OnControlError(nsresult status) = 0;
};

class nsFtpControlConnection : public nsIInputStreamCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAMCALLBACK

    nsFtpControlConnection(const nsCSubstring& host, PRUint32 port);
    ~nsFtpControlConnection();

    nsresult Connect(nsIProxyInfo* proxyInfo, nsITransportEventSink* eventSink);
    nsresult Disconnect(nsresult status);
    nsresult Write(const nsCSubstring& command);

    PRBool IsAlive();

    nsITransport *Transport()   { return mSocket; }

    





    nsresult WaitData(nsFtpControlConnectionListener *listener);

    PRUint32         mServerType;           
    nsString         mPassword;
    PRInt32          mSuspendedWrite;
    nsCString        mPwd;

private:
    nsCString mHost;
    PRUint32  mPort;

    nsCOMPtr<nsISocketTransport>     mSocket;
    nsCOMPtr<nsIOutputStream>        mSocketOutput;
    nsCOMPtr<nsIAsyncInputStream>    mSocketInput;

    nsRefPtr<nsFtpControlConnectionListener> mListener;
};

#endif
