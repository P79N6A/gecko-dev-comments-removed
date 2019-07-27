





#include "nsIServiceManager.h"
#include "UDPSocketParent.h"
#include "nsComponentManagerUtils.h"
#include "nsIUDPSocket.h"
#include "nsINetAddr.h"
#include "mozilla/unused.h"
#include "mozilla/net/DNS.h"

namespace mozilla {
namespace dom {

static void
FireInternalError(mozilla::net::PUDPSocketParent *aActor, uint32_t aLineNo)
{
  mozilla::unused <<
      aActor->SendCallback(NS_LITERAL_CSTRING("onerror"),
                          UDPError(NS_LITERAL_CSTRING("Internal error"),
                                   NS_LITERAL_CSTRING(__FILE__), aLineNo, 0),
                          NS_LITERAL_CSTRING("connecting"));
}

static nsresult
ConvertNetAddrToString(mozilla::net::NetAddr &netAddr, nsACString *address, uint16_t *port)
{
  NS_ENSURE_ARG_POINTER(address);
  NS_ENSURE_ARG_POINTER(port);

  *port = 0;
  uint32_t bufSize = 0;

  switch(netAddr.raw.family) {
  case AF_INET:
    *port = PR_ntohs(netAddr.inet.port);
    bufSize = mozilla::net::kIPv4CStrBufSize;
    break;
  case AF_INET6:
    *port = PR_ntohs(netAddr.inet6.port);
    bufSize = mozilla::net::kIPv6CStrBufSize;
    break;
  default:
    
    MOZ_ASSERT(false, "Unexpected address family");
    return NS_ERROR_INVALID_ARG;
  }

  address->SetCapacity(bufSize);
  NetAddrToString(&netAddr, address->BeginWriting(), bufSize);
  address->SetLength(strlen(address->BeginReading()));

  return NS_OK;
}

NS_IMPL_ISUPPORTS(UDPSocketParent, nsIUDPSocketListener)

UDPSocketParent::~UDPSocketParent()
{
}



bool
UDPSocketParent::Init(const nsCString &aHost, const uint16_t aPort)
{
  nsresult rv;
  NS_ASSERTION(mFilter, "No packet filter");

  nsCOMPtr<nsIUDPSocket> sock =
      do_CreateInstance("@mozilla.org/network/udp-socket;1", &rv);
  if (NS_FAILED(rv)) {
    FireInternalError(this, __LINE__);
    return true;
  }

  if (aHost.IsEmpty()) {
    rv = sock->Init(aPort, false);
  } else {
    PRNetAddr prAddr;
    PR_InitializeNetAddr(PR_IpAddrAny, aPort, &prAddr);
    PRStatus status = PR_StringToNetAddr(aHost.BeginReading(), &prAddr);
    if (status != PR_SUCCESS) {
      FireInternalError(this, __LINE__);
      return true;
    }

    mozilla::net::NetAddr addr;
    PRNetAddrToNetAddr(&prAddr, &addr);
    rv = sock->InitWithAddress(&addr);
  }

  if (NS_FAILED(rv)) {
    FireInternalError(this, __LINE__);
    return true;
  }

  mSocket = sock;

  net::NetAddr localAddr;
  mSocket->GetAddress(&localAddr);

  uint16_t port;
  nsCString addr;
  rv = ConvertNetAddrToString(localAddr, &addr, &port);

  if (NS_FAILED(rv)) {
    FireInternalError(this, __LINE__);
    return true;
  }

  
  mSocket->AsyncListen(this);
  mozilla::unused <<
      PUDPSocketParent::SendCallback(NS_LITERAL_CSTRING("onopen"),
                                     UDPAddressInfo(addr, port),
                                     NS_LITERAL_CSTRING("connected"));

  return true;
}

bool
UDPSocketParent::RecvData(const InfallibleTArray<uint8_t> &aData,
                          const nsCString& aRemoteAddress,
                          const uint16_t& aPort)
{
  NS_ENSURE_TRUE(mSocket, true);
  NS_ASSERTION(mFilter, "No packet filter");
  
  
  return true;

#if 0
  
  uint32_t count;
  nsresult rv = mSocket->Send(aRemoteAddress,
                              aPort, aData.Elements(),
                              aData.Length(), &count);
  mozilla::unused <<
      PUDPSocketParent::SendCallback(NS_LITERAL_CSTRING("onsent"),
                                     UDPSendResult(rv),
                                     NS_LITERAL_CSTRING("connected"));
  NS_ENSURE_SUCCESS(rv, true);
  NS_ENSURE_TRUE(count > 0, true);
  return true;
#endif
}

bool
UDPSocketParent::RecvDataWithAddress(const InfallibleTArray<uint8_t>& aData,
                                     const mozilla::net::NetAddr& aAddr)
{
  NS_ENSURE_TRUE(mSocket, true);
  NS_ASSERTION(mFilter, "No packet filter");

  uint32_t count;
  nsresult rv;
  bool allowed;
  rv = mFilter->FilterPacket(&aAddr, aData.Elements(),
                             aData.Length(), nsIUDPSocketFilter::SF_OUTGOING,
                             &allowed);
  
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(allowed, false);

  rv = mSocket->SendWithAddress(&aAddr, aData.Elements(),
                                aData.Length(), &count);
  mozilla::unused <<
      PUDPSocketParent::SendCallback(NS_LITERAL_CSTRING("onsent"),
                                     UDPSendResult(rv),
                                     NS_LITERAL_CSTRING("connected"));
  NS_ENSURE_SUCCESS(rv, true);
  NS_ENSURE_TRUE(count > 0, true);
  return true;
}

bool
UDPSocketParent::RecvClose()
{
  NS_ENSURE_TRUE(mSocket, true);
  nsresult rv = mSocket->Close();
  mSocket = nullptr;
  NS_ENSURE_SUCCESS(rv, true);
  return true;
}

bool
UDPSocketParent::RecvRequestDelete()
{
  mozilla::unused << Send__delete__(this);
  return true;
}

void
UDPSocketParent::ActorDestroy(ActorDestroyReason why)
{
  MOZ_ASSERT(mIPCOpen);
  mIPCOpen = false;
  if (mSocket) {
    mSocket->Close();
  }
  mSocket = nullptr;
}



NS_IMETHODIMP
UDPSocketParent::OnPacketReceived(nsIUDPSocket* aSocket, nsIUDPMessage* aMessage)
{
  
  if (!mIPCOpen) {
    return NS_OK;
  }
  NS_ASSERTION(mFilter, "No packet filter");

  uint16_t port;
  nsCString ip;
  nsCOMPtr<nsINetAddr> fromAddr;
  aMessage->GetFromAddr(getter_AddRefs(fromAddr));
  fromAddr->GetPort(&port);
  fromAddr->GetAddress(ip);

  nsCString data;
  aMessage->GetData(data);

  const char* buffer = data.get();
  uint32_t len = data.Length();

  bool allowed;
  mozilla::net::NetAddr addr;
  fromAddr->GetNetAddr(&addr);
  nsresult rv = mFilter->FilterPacket(&addr,
                                      (const uint8_t*)buffer, len,
                                      nsIUDPSocketFilter::SF_INCOMING,
                                      &allowed);
  
  NS_ENSURE_SUCCESS(rv, NS_OK);
  NS_ENSURE_TRUE(allowed, NS_OK);

  FallibleTArray<uint8_t> fallibleArray;
  if (!fallibleArray.InsertElementsAt(0, buffer, len)) {
    FireInternalError(this, __LINE__);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InfallibleTArray<uint8_t> infallibleArray;
  infallibleArray.SwapElements(fallibleArray);

  
  mozilla::unused <<
      PUDPSocketParent::SendCallback(NS_LITERAL_CSTRING("ondata"),
                                     UDPMessage(ip, port, infallibleArray),
                                     NS_LITERAL_CSTRING("connected"));

  return NS_OK;
}

NS_IMETHODIMP
UDPSocketParent::OnStopListening(nsIUDPSocket* aSocket, nsresult aStatus)
{
  
  if (mIPCOpen) {
    mozilla::unused <<
        PUDPSocketParent::SendCallback(NS_LITERAL_CSTRING("onclose"),
                                       mozilla::void_t(),
                                       NS_LITERAL_CSTRING("closed"));
  }
  return NS_OK;
}

} 
} 
