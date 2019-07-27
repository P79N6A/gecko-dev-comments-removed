







#include "DataSocket.h"
#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
#endif
#include "nsISupportsImpl.h" 

#ifdef MOZ_TASK_TRACER
using namespace mozilla::tasktracer;
#endif

namespace mozilla {
namespace ipc {





DataSocketIO::~DataSocketIO()
{
  MOZ_COUNT_DTOR_INHERITED(DataSocketIO, SocketIOBase);
}

void
DataSocketIO::EnqueueData(UnixSocketIOBuffer* aBuffer)
{
  if (!aBuffer->GetSize()) {
    delete aBuffer; 
    return;
  }
  mOutgoingQ.AppendElement(aBuffer);
}

bool
DataSocketIO::HasPendingData() const
{
  return !mOutgoingQ.IsEmpty();
}

ssize_t
DataSocketIO::ReceiveData(int aFd)
{
  MOZ_ASSERT(aFd >= 0);

  UnixSocketIOBuffer* incoming;
  nsresult rv = QueryReceiveBuffer(&incoming);
  if (NS_FAILED(rv)) {
    
    GetConsumerThread()->PostTask(FROM_HERE,
                                  new SocketRequestClosingTask(this));
    return -1;
  }

  ssize_t res = incoming->Receive(aFd);
  if (res < 0) {
    
    DiscardBuffer();
    GetConsumerThread()->PostTask(FROM_HERE,
                                  new SocketRequestClosingTask(this));
    return -1;
  } else if (!res) {
    
    DiscardBuffer();
    GetConsumerThread()->PostTask(FROM_HERE,
                                  new SocketRequestClosingTask(this));
    return 0;
  }

#ifdef MOZ_TASK_TRACER
  


  AutoSourceEvent taskTracerEvent(SourceEventType::Unixsocket);
#endif

  ConsumeBuffer();

  return res;
}

nsresult
DataSocketIO::SendPendingData(int aFd)
{
  MOZ_ASSERT(aFd >= 0);

  while (HasPendingData()) {
    UnixSocketIOBuffer* outgoing = mOutgoingQ.ElementAt(0);

    ssize_t res = outgoing->Send(aFd);
    if (res < 0) {
      
      GetConsumerThread()->PostTask(FROM_HERE,
                                    new SocketRequestClosingTask(this));
      return NS_ERROR_FAILURE;
    } else if (!res && outgoing->GetSize()) {
      
      return NS_OK;
    }
    if (!outgoing->GetSize()) {
      mOutgoingQ.RemoveElementAt(0);
      delete outgoing;
    }
  }

  return NS_OK;
}

DataSocketIO::DataSocketIO(MessageLoop* aConsumerLoop)
  : SocketIOBase(aConsumerLoop)
{
  MOZ_COUNT_CTOR_INHERITED(DataSocketIO, SocketIOBase);
}





DataSocket::DataSocket()
{
  MOZ_COUNT_CTOR_INHERITED(DataSocket, SocketBase);
}

DataSocket::~DataSocket()
{
  MOZ_COUNT_DTOR_INHERITED(DataSocket, SocketBase);
}

}
}
