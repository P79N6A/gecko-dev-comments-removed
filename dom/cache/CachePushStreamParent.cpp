





#include "mozilla/dom/cache/CachePushStreamParent.h"

#include "mozilla/unused.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIPipe.h"

namespace mozilla {
namespace dom {
namespace cache {


CachePushStreamParent*
CachePushStreamParent::Create()
{
  
  
  
  nsCOMPtr<nsIAsyncInputStream> reader;
  nsCOMPtr<nsIAsyncOutputStream> writer;

  
  
  
  nsresult rv = NS_NewPipe2(getter_AddRefs(reader),
                            getter_AddRefs(writer),
                            true, true,   
                            0,            
                            UINT32_MAX);  
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  return new CachePushStreamParent(reader, writer);
}

CachePushStreamParent::~CachePushStreamParent()
{
}

already_AddRefed<nsIInputStream>
CachePushStreamParent::TakeReader()
{
  MOZ_ASSERT(mReader);
  return mReader.forget();
}

void
CachePushStreamParent::ActorDestroy(ActorDestroyReason aReason)
{
  
  
  
  
  mWriter->CloseWithStatus(NS_ERROR_ABORT);
}

bool
CachePushStreamParent::RecvBuffer(const nsCString& aBuffer)
{
  uint32_t numWritten = 0;

  
  nsresult rv = mWriter->Write(aBuffer.get(), aBuffer.Length(), &numWritten);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    RecvClose(rv);
  }

  return true;
}

bool
CachePushStreamParent::RecvClose(const nsresult& aRv)
{
  mWriter->CloseWithStatus(aRv);
  unused << Send__delete__(this);
  return true;
}

CachePushStreamParent::CachePushStreamParent(nsIAsyncInputStream* aReader,
                                             nsIAsyncOutputStream* aWriter)
  : mReader(aReader)
  , mWriter(aWriter)
{
  MOZ_ASSERT(mReader);
  MOZ_ASSERT(mWriter);
}

} 
} 
} 
