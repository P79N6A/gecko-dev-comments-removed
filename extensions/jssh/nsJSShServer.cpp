





































#include "nsJSShServer.h"
#include "nsNetCID.h"
#include "nsISocketTransport.h"
#include "nsIComponentManager.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsThreadUtils.h"
#include "nsJSSh.h"
#include "nsComponentManagerUtils.h"




class ConnectionListener : public nsIServerSocketListener
{
public:
  ConnectionListener(const nsACString &startupURI);
  ~ConnectionListener();
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERVERSOCKETLISTENER

private:
  nsCString mStartupURI;
};




ConnectionListener::ConnectionListener(const nsACString &startupURI)
    : mStartupURI(startupURI)
{
}

ConnectionListener::~ConnectionListener()
{ 

}

already_AddRefed<nsIServerSocketListener> CreateConnectionListener(const nsACString &startupURI)
{
  nsIServerSocketListener* obj = new ConnectionListener(startupURI);
  NS_IF_ADDREF(obj);
  return obj;
}




NS_IMPL_THREADSAFE_ADDREF(ConnectionListener)
NS_IMPL_THREADSAFE_RELEASE(ConnectionListener)

NS_INTERFACE_MAP_BEGIN(ConnectionListener)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIServerSocketListener)
NS_INTERFACE_MAP_END





NS_IMETHODIMP ConnectionListener::OnSocketAccepted(nsIServerSocket *aServ, nsISocketTransport *aTransport)
{
  nsCOMPtr<nsIInputStream> input;
  nsCOMPtr<nsIOutputStream> output;
  aTransport->OpenInputStream(nsITransport::OPEN_BLOCKING, 0, 0, getter_AddRefs(input));
  aTransport->OpenOutputStream(nsITransport::OPEN_BLOCKING, 0, 0, getter_AddRefs(output));
  
#ifdef DEBUG
  printf("JSSh server: new connection!\n");
#endif

  nsCOMPtr<nsIRunnable> shell = CreateJSSh(input, output, mStartupURI);

  nsCOMPtr<nsIThread> thread;
  NS_NewThread(getter_AddRefs(thread), shell);
  return NS_OK;
}


NS_IMETHODIMP ConnectionListener::OnStopListening(nsIServerSocket *aServ, nsresult aStatus)
{
#ifdef DEBUG
  printf("JSSh server: stopped listening!\n");
#endif
  return NS_OK;
}




nsJSShServer::nsJSShServer()
{
#ifdef DEBUG
  printf("nsJSShServer ctor\n");
#endif
}

nsJSShServer::~nsJSShServer()
{
#ifdef DEBUG
  printf("nsJSShServer dtor\n");
#endif
  
  StopServerSocket();
}




NS_IMPL_ADDREF(nsJSShServer)
NS_IMPL_RELEASE(nsJSShServer)

NS_INTERFACE_MAP_BEGIN(nsJSShServer)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIJSShServer)
NS_INTERFACE_MAP_END






NS_IMETHODIMP
nsJSShServer::StartServerSocket(PRUint32 port, const nsACString & startupURI,
                                PRBool loopbackOnly)
{
  if (mServerSocket) {
    NS_ERROR("server socket already running");
    return NS_ERROR_FAILURE;
  }
  
  mServerSocket = do_CreateInstance(NS_SERVERSOCKET_CONTRACTID);
  if (!mServerSocket) {
    NS_ERROR("could not create server socket");
    return NS_ERROR_FAILURE;
  }

  if (NS_FAILED(mServerSocket->Init(port, loopbackOnly, -1))) {
    mServerSocket = nsnull;
    NS_ERROR("server socket initialization error");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIServerSocketListener> listener = CreateConnectionListener(startupURI);
  mServerSocket->AsyncListen(listener);

  mServerPort = port;
  mServerStartupURI = startupURI;
  mServerLoopbackOnly = loopbackOnly;
    
  return NS_OK;
}


NS_IMETHODIMP nsJSShServer::StopServerSocket()
{
  if (mServerSocket)
    mServerSocket->Close();
  mServerSocket = nsnull;
  mServerPort = 0;
  mServerStartupURI.Truncate();
  mServerLoopbackOnly = PR_FALSE;
  return NS_OK;
}



NS_IMETHODIMP
nsJSShServer::RunShell(nsIInputStream *input, nsIOutputStream *output,
                       const char *startupURI, PRBool blocking)
{
  nsCOMPtr<nsIRunnable> shell = CreateJSSh(input, output, nsCString(startupURI));
  if (!blocking) {
    nsCOMPtr<nsIThread> thread;
    NS_NewThread(getter_AddRefs(thread), shell);
  }
  else
    shell->Run();

  return NS_OK;
}


NS_IMETHODIMP
nsJSShServer::GetServerListening(PRBool *aServerListening)
{
  if (mServerSocket)
    *aServerListening = PR_TRUE;
  else
    *aServerListening = PR_FALSE;
  
  return NS_OK;
}


NS_IMETHODIMP
nsJSShServer::GetServerPort(PRUint32 *aServerPort)
{
  *aServerPort = mServerPort;
  return NS_OK;
}


NS_IMETHODIMP
nsJSShServer::GetServerStartupURI(nsACString & aServerStartupURI)
{
  aServerStartupURI = mServerStartupURI;
  return NS_OK;
}


NS_IMETHODIMP
nsJSShServer::GetServerLoopbackOnly(PRBool *aServerLoopbackOnly)
{
  *aServerLoopbackOnly = mServerLoopbackOnly;
  return NS_OK;
}
