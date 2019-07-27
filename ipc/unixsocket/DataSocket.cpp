







#include "mozilla/ipc/DataSocket.h"

namespace mozilla {
namespace ipc {





DataSocketIO::~DataSocketIO()
{ }

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

DataSocketIO::DataSocketIO(size_t aMaxReadSize)
  : mMaxReadSize(aMaxReadSize)
{ }





DataSocket::~DataSocket()
{ }

}
}
