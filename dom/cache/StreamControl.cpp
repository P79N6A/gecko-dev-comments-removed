





#include "mozilla/dom/cache/StreamControl.h"

namespace mozilla {
namespace dom {
namespace cache {

void
StreamControl::AddReadStream(ReadStream::Controllable* aReadStream)
{
  AssertOwningThread();
  MOZ_ASSERT(aReadStream);
  MOZ_ASSERT(!mReadStreamList.Contains(aReadStream));
  mReadStreamList.AppendElement(aReadStream);
}

void
StreamControl::ForgetReadStream(ReadStream::Controllable* aReadStream)
{
  AssertOwningThread();
  MOZ_ALWAYS_TRUE(mReadStreamList.RemoveElement(aReadStream));
}

void
StreamControl::NoteClosed(ReadStream::Controllable* aReadStream,
                          const nsID& aId)
{
  AssertOwningThread();
  ForgetReadStream(aReadStream);
  NoteClosedAfterForget(aId);
}

StreamControl::~StreamControl()
{
  
  MOZ_ASSERT(mReadStreamList.IsEmpty());
}

void
StreamControl::CloseReadStreams(const nsID& aId)
{
  AssertOwningThread();
  DebugOnly<uint32_t> closedCount = 0;

  ReadStreamList::ForwardIterator iter(mReadStreamList);
  while (iter.HasMore()) {
    nsRefPtr<ReadStream::Controllable> stream = iter.GetNext();
    if (stream->MatchId(aId)) {
      stream->CloseStream();
      closedCount += 1;
    }
  }

  MOZ_ASSERT(closedCount > 0);
}

void
StreamControl::CloseAllReadStreams()
{
  AssertOwningThread();

  ReadStreamList::ForwardIterator iter(mReadStreamList);
  while (iter.HasMore()) {
    iter.GetNext()->CloseStream();
  }
}

void
StreamControl::CloseAllReadStreamsWithoutReporting()
{
  AssertOwningThread();

  ReadStreamList::ForwardIterator iter(mReadStreamList);
  while (iter.HasMore()) {
    nsRefPtr<ReadStream::Controllable> stream = iter.GetNext();
    
    
    stream->CloseStreamWithoutReporting();
  }
}

} 
} 
} 
