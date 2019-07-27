




#include "SourceBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include "mozilla/Likely.h"
#include "MainThreadUtils.h"
#include "SurfaceCache.h"

using std::max;
using std::min;

namespace mozilla {
namespace image {





SourceBufferIterator::State
SourceBufferIterator::AdvanceOrScheduleResume(IResumable* aConsumer)
{
  MOZ_ASSERT(mOwner);
  return mOwner->AdvanceIteratorOrScheduleResume(*this, aConsumer);
}

bool
SourceBufferIterator::RemainingBytesIsNoMoreThan(size_t aBytes) const
{
  MOZ_ASSERT(mOwner);
  return mOwner->RemainingBytesIsNoMoreThan(*this, aBytes);
}






SourceBuffer::SourceBuffer()
  : mMutex("image::SourceBuffer")
{ }

nsresult
SourceBuffer::AppendChunk(Maybe<Chunk>&& aChunk)
{
  mMutex.AssertCurrentThreadOwns();

#ifdef DEBUG
  if (mChunks.Length() > 0) {
    NS_WARNING("Appending an extra chunk for SourceBuffer");
  }
#endif

  if (MOZ_UNLIKELY(!aChunk)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (MOZ_UNLIKELY(aChunk->AllocationFailed())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (MOZ_UNLIKELY(!mChunks.AppendElement(Move(*aChunk)))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

Maybe<SourceBuffer::Chunk>
SourceBuffer::CreateChunk(size_t aCapacity)
{
  if (MOZ_UNLIKELY(aCapacity == 0)) {
    MOZ_ASSERT_UNREACHABLE("Appending a chunk of zero size?");
    return Nothing();
  }

  
  if (MOZ_UNLIKELY(SIZE_MAX - aCapacity < MIN_CHUNK_CAPACITY)) {
    return Nothing();
  }

  
  
  size_t roundedCapacity =
    (aCapacity + MIN_CHUNK_CAPACITY - 1) & ~(MIN_CHUNK_CAPACITY - 1);
  MOZ_ASSERT(roundedCapacity >= aCapacity, "Bad math?");
  MOZ_ASSERT(roundedCapacity - aCapacity < MIN_CHUNK_CAPACITY, "Bad math?");

  
  
  
  
  if (MOZ_UNLIKELY(!SurfaceCache::CanHold(roundedCapacity))) {
    return Nothing();
  }

  return Some(Chunk(roundedCapacity));
}

size_t
SourceBuffer::FibonacciCapacityWithMinimum(size_t aMinCapacity)
{
  mMutex.AssertCurrentThreadOwns();

  size_t length = mChunks.Length();

#if defined(MOZILLA_IMAGELIB_SOURCEBUFFER_USE_FIBONACCI)
  

  if (length == 0) {
    return aMinCapacity;
  }

  if (length == 1) {
    return max(2 * mChunks[0].Capacity(), aMinCapacity);
  }

  return max(mChunks[length - 1].Capacity() + mChunks[length - 2].Capacity(),
             aMinCapacity);
#else
  

  if (length == 0) {
    return aMinCapacity;
  }

  return max(size_t(mChunks[length - 1].Capacity() * 1.25), aMinCapacity);
#endif
}

void
SourceBuffer::AddWaitingConsumer(IResumable* aConsumer)
{
  mMutex.AssertCurrentThreadOwns();

  MOZ_ASSERT(!mStatus, "Waiting when we're complete?");

  if (MOZ_UNLIKELY(NS_IsMainThread())) {
    NS_WARNING("SourceBuffer consumer on the main thread needed to wait");
  }

  mWaitingConsumers.AppendElement(aConsumer);
}

void
SourceBuffer::ResumeWaitingConsumers()
{
  mMutex.AssertCurrentThreadOwns();

  if (mWaitingConsumers.Length() == 0) {
    return;
  }

  for (uint32_t i = 0 ; i < mWaitingConsumers.Length() ; ++i) {
    mWaitingConsumers[i]->Resume();
  }

  mWaitingConsumers.Clear();
}

nsresult
SourceBuffer::ExpectLength(size_t aExpectedLength)
{
  MOZ_ASSERT(aExpectedLength > 0, "Zero expected size?");

  MutexAutoLock lock(mMutex);

  if (MOZ_UNLIKELY(mStatus)) {
    MOZ_ASSERT_UNREACHABLE("ExpectLength after SourceBuffer is complete");
    return NS_OK;
  }

  if (MOZ_UNLIKELY(mChunks.Length() > 0)) {
    MOZ_ASSERT_UNREACHABLE("Duplicate or post-Append call to ExpectLength");
    return NS_OK;
  }

  if (MOZ_UNLIKELY(NS_FAILED(AppendChunk(CreateChunk(aExpectedLength))))) {
    return HandleError(NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
SourceBuffer::Append(const char* aData, size_t aLength)
{
  MOZ_ASSERT(aData, "Should have a buffer");
  MOZ_ASSERT(aLength > 0, "Writing a zero-sized chunk");

  size_t currentChunkCapacity = 0;
  size_t currentChunkLength = 0;
  char* currentChunkData = nullptr;
  size_t currentChunkRemaining = 0;
  size_t forCurrentChunk = 0;
  size_t forNextChunk = 0;
  size_t nextChunkCapacity = 0;

  {
    MutexAutoLock lock(mMutex);

    if (MOZ_UNLIKELY(mStatus)) {
      
      return NS_ERROR_FAILURE;
    }

    if (MOZ_UNLIKELY(mChunks.Length() == 0)) {
      if (MOZ_UNLIKELY(NS_FAILED(AppendChunk(CreateChunk(aLength))))) {
        return HandleError(NS_ERROR_OUT_OF_MEMORY);
      }
    }

    
    
    Chunk& currentChunk = mChunks.LastElement();
    currentChunkCapacity = currentChunk.Capacity();
    currentChunkLength = currentChunk.Length();
    currentChunkData = currentChunk.Data();

    
    
    
    
    currentChunkRemaining = currentChunkCapacity - currentChunkLength;
    forCurrentChunk = min(aLength, currentChunkRemaining);
    forNextChunk = aLength - forCurrentChunk;

    
    
    nextChunkCapacity = forNextChunk > 0
                      ? FibonacciCapacityWithMinimum(forNextChunk)
                      : 0;
  }

  
  MOZ_ASSERT(currentChunkLength + forCurrentChunk <= currentChunkCapacity);
  memcpy(currentChunkData + currentChunkLength, aData, forCurrentChunk);

  
  Maybe<Chunk> nextChunk;
  if (forNextChunk > 0) {
    MOZ_ASSERT(nextChunkCapacity >= forNextChunk, "Next chunk too small?");
    nextChunk = CreateChunk(nextChunkCapacity);
    if (MOZ_LIKELY(nextChunk && !nextChunk->AllocationFailed())) {
      memcpy(nextChunk->Data(), aData + forCurrentChunk, forNextChunk);
      nextChunk->AddLength(forNextChunk);
    }
  }

  
  {
    MutexAutoLock lock(mMutex);

    
    Chunk& currentChunk = mChunks.LastElement();
    MOZ_ASSERT(currentChunk.Data() == currentChunkData, "Multiple producers?");
    MOZ_ASSERT(currentChunk.Length() == currentChunkLength,
               "Multiple producers?");

    currentChunk.AddLength(forCurrentChunk);

    
    if (forNextChunk > 0) {
      if (MOZ_UNLIKELY(!nextChunk)) {
        return HandleError(NS_ERROR_OUT_OF_MEMORY);
      }

      if (MOZ_UNLIKELY(NS_FAILED(AppendChunk(Move(nextChunk))))) {
        return HandleError(NS_ERROR_OUT_OF_MEMORY);
      }
    }

    
    ResumeWaitingConsumers();
  }

  return NS_OK;
}

void
SourceBuffer::Complete(nsresult aStatus)
{
  MutexAutoLock lock(mMutex);

  if (MOZ_UNLIKELY(mStatus)) {
    MOZ_ASSERT_UNREACHABLE("Called Complete more than once");
    return;
  }

  if (MOZ_UNLIKELY(NS_SUCCEEDED(aStatus) && IsEmpty())) {
    
    aStatus = NS_ERROR_FAILURE;
  }

  mStatus = Some(aStatus);

  
  ResumeWaitingConsumers();
}

bool
SourceBuffer::IsComplete()
{
  MutexAutoLock lock(mMutex);
  return bool(mStatus);
}

size_t
SourceBuffer::SizeOfIncludingThisWithComputedFallback(MallocSizeOf
                                                        aMallocSizeOf) const
{
  MutexAutoLock lock(mMutex);

  size_t n = aMallocSizeOf(this);
  n += mChunks.SizeOfExcludingThis(aMallocSizeOf);

  for (uint32_t i = 0 ; i < mChunks.Length() ; ++i) {
    n += mChunks[i].Capacity();
  }

  return n;
}

bool
SourceBuffer::RemainingBytesIsNoMoreThan(const SourceBufferIterator& aIterator,
                                         size_t aBytes) const
{
  MutexAutoLock lock(mMutex);

  
  if (!mStatus) {
    return false;
  }

  
  if (!aIterator.HasMore()) {
    return true;
  }

  uint32_t iteratorChunk = aIterator.mData.mIterating.mChunk;
  size_t iteratorOffset = aIterator.mData.mIterating.mOffset;
  size_t iteratorLength = aIterator.mData.mIterating.mLength;

  
  
  size_t bytes = aBytes + iteratorOffset + iteratorLength;

  
  
  
  size_t lengthSoFar = 0;
  for (uint32_t i = iteratorChunk ; i < mChunks.Length() ; ++i) {
    lengthSoFar += mChunks[i].Length();
    if (lengthSoFar > bytes) {
      return false;
    }
  }

  return true;
}

SourceBufferIterator::State
SourceBuffer::AdvanceIteratorOrScheduleResume(SourceBufferIterator& aIterator,
                                              IResumable* aConsumer)
{
  MutexAutoLock lock(mMutex);

  if (MOZ_UNLIKELY(!aIterator.HasMore())) {
    MOZ_ASSERT_UNREACHABLE("Should not advance a completed iterator");
    return SourceBufferIterator::COMPLETE;
  }

  if (MOZ_UNLIKELY(mStatus && NS_FAILED(*mStatus))) {
    
    return aIterator.SetComplete(*mStatus);
  }

  if (MOZ_UNLIKELY(mChunks.Length() == 0)) {
    
    AddWaitingConsumer(aConsumer);
    return aIterator.SetWaiting();
  }

  uint32_t iteratorChunkIdx = aIterator.mData.mIterating.mChunk;
  MOZ_ASSERT(iteratorChunkIdx < mChunks.Length());

  const Chunk& currentChunk = mChunks[iteratorChunkIdx];
  size_t iteratorEnd = aIterator.mData.mIterating.mOffset +
                       aIterator.mData.mIterating.mLength;
  MOZ_ASSERT(iteratorEnd <= currentChunk.Length());
  MOZ_ASSERT(iteratorEnd <= currentChunk.Capacity());

  if (iteratorEnd < currentChunk.Length()) {
    
    return aIterator.SetReady(iteratorChunkIdx, currentChunk.Data(),
                              iteratorEnd, currentChunk.Length() - iteratorEnd);
  }

  if (iteratorEnd == currentChunk.Capacity() &&
      !IsLastChunk(iteratorChunkIdx)) {
    
    const Chunk& nextChunk = mChunks[iteratorChunkIdx + 1];
    return aIterator.SetReady(iteratorChunkIdx + 1, nextChunk.Data(), 0,
                              nextChunk.Length());
  }

  MOZ_ASSERT(IsLastChunk(iteratorChunkIdx), "Should've advanced");

  if (mStatus) {
    
    MOZ_ASSERT(NS_SUCCEEDED(*mStatus), "Handled failures earlier");
    return aIterator.SetComplete(*mStatus);
  }

  
  
  AddWaitingConsumer(aConsumer);
  return aIterator.SetWaiting();
}

nsresult
SourceBuffer::HandleError(nsresult aError)
{
  MOZ_ASSERT(NS_FAILED(aError), "Should have an error here");
  MOZ_ASSERT(aError == NS_ERROR_OUT_OF_MEMORY,
             "Unexpected error; may want to notify waiting readers, which "
             "HandleError currently doesn't do");

  mMutex.AssertCurrentThreadOwns();

  NS_WARNING("SourceBuffer encountered an unrecoverable error");

  
  mStatus = Some(aError);

  
  mWaitingConsumers.Clear();

  return *mStatus;
}

bool
SourceBuffer::IsEmpty()
{
  mMutex.AssertCurrentThreadOwns();
  return mChunks.Length() == 0 ||
         mChunks[0].Length() == 0;
}

bool
SourceBuffer::IsLastChunk(uint32_t aChunk)
{
  mMutex.AssertCurrentThreadOwns();
  return aChunk + 1 == mChunks.Length();
}

} 
} 
