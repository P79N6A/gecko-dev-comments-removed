









#ifndef mozilla_image_src_sourcebuffer_h
#define mozilla_image_src_sourcebuffer_h

#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/Move.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "nsRefPtr.h"
#include "nsTArray.h"

namespace mozilla {
namespace image {

class SourceBuffer;






struct IResumable
{
  MOZ_DECLARE_REFCOUNTED_TYPENAME(IResumable)

  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) = 0;

  virtual void Resume() = 0;

protected:
  virtual ~IResumable() { }
};



















class SourceBufferIterator final
{
public:
  enum State {
    START,    
    READY,    
    WAITING,  
    COMPLETE  
  };

  explicit SourceBufferIterator(SourceBuffer* aOwner)
    : mOwner(aOwner)
    , mState(START)
  {
    MOZ_ASSERT(aOwner);
    mData.mIterating.mChunk = 0;
    mData.mIterating.mData = nullptr;
    mData.mIterating.mOffset = 0;
    mData.mIterating.mLength = 0;
  }

  SourceBufferIterator(SourceBufferIterator&& aOther)
    : mOwner(Move(aOther.mOwner))
    , mState(aOther.mState)
    , mData(aOther.mData)
  { }

  ~SourceBufferIterator();

  SourceBufferIterator& operator=(SourceBufferIterator&& aOther)
  {
    mOwner = Move(aOther.mOwner);
    mState = aOther.mState;
    mData = aOther.mData;
    return *this;
  }

  



  bool RemainingBytesIsNoMoreThan(size_t aBytes) const;

  




  State AdvanceOrScheduleResume(IResumable* aConsumer);

  
  nsresult CompletionStatus() const
  {
    MOZ_ASSERT(mState == COMPLETE,
               "Calling CompletionStatus() in the wrong state");
    return mState == COMPLETE ? mData.mAtEnd.mStatus : NS_OK;
  }

  
  const char* Data() const
  {
    MOZ_ASSERT(mState == READY, "Calling Data() in the wrong state");
    return mState == READY ? mData.mIterating.mData + mData.mIterating.mOffset
                           : nullptr;
  }

  
  size_t Length() const
  {
    MOZ_ASSERT(mState == READY, "Calling Length() in the wrong state");
    return mState == READY ? mData.mIterating.mLength : 0;
  }

private:
  friend class SourceBuffer;

  SourceBufferIterator(const SourceBufferIterator&) = delete;
  SourceBufferIterator& operator=(const SourceBufferIterator&) = delete;

  bool HasMore() const { return mState != COMPLETE; }

  State SetReady(uint32_t aChunk, const char* aData,
                size_t aOffset, size_t aLength)
  {
    MOZ_ASSERT(mState != COMPLETE);
    mData.mIterating.mChunk = aChunk;
    mData.mIterating.mData = aData;
    mData.mIterating.mOffset = aOffset;
    mData.mIterating.mLength = aLength;
    return mState = READY;
  }

  State SetWaiting()
  {
    MOZ_ASSERT(mState != COMPLETE);
    MOZ_ASSERT(mState != WAITING, "Did we get a spurious wakeup somehow?");
    return mState = WAITING;
  }

  State SetComplete(nsresult aStatus)
  {
    mData.mAtEnd.mStatus = aStatus;
    return mState = COMPLETE;
  }

  nsRefPtr<SourceBuffer> mOwner;

  State mState;

  




  union {
    struct {
      uint32_t mChunk;
      const char* mData;
      size_t mOffset;
      size_t mLength;
    } mIterating;
    struct {
      nsresult mStatus;
    } mAtEnd;
  } mData;
};
























class SourceBuffer final
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(image::SourceBuffer)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(image::SourceBuffer)

  SourceBuffer();

  
  
  

  



  nsresult ExpectLength(size_t aExpectedLength);

  
  nsresult Append(const char* aData, size_t aLength);

  



  void Complete(nsresult aStatus);

  
  bool IsComplete();

  
  size_t SizeOfIncludingThisWithComputedFallback(MallocSizeOf) const;


  
  
  

  
  SourceBufferIterator Iterator();


private:
  friend class SourceBufferIterator;

  ~SourceBuffer();

  
  
  

  class Chunk
  {
  public:
    explicit Chunk(size_t aCapacity)
      : mCapacity(aCapacity)
      , mLength(0)
    {
      MOZ_ASSERT(aCapacity > 0, "Creating zero-capacity chunk");
      mData = new (fallible) char[mCapacity];
    }

    ~Chunk() { delete[] mData; }

    Chunk(Chunk&& aOther)
      : mCapacity(aOther.mCapacity)
      , mLength(aOther.mLength)
      , mData(aOther.mData)
    {
      aOther.mCapacity = aOther.mLength = 0;
      aOther.mData = nullptr;
    }

    Chunk& operator=(Chunk&& aOther)
    {
      mCapacity = aOther.mCapacity;
      mLength = aOther.mLength;
      mData = aOther.mData;
      aOther.mCapacity = aOther.mLength = 0;
      aOther.mData = nullptr;
      return *this;
    }

    bool AllocationFailed() const { return !mData; }
    size_t Capacity() const { return mCapacity; }
    size_t Length() const { return mLength; }

    char* Data() const
    {
      MOZ_ASSERT(mData, "Allocation failed but nobody checked for it");
      return mData;
    }

    void AddLength(size_t aAdditionalLength)
    {
      MOZ_ASSERT(mLength + aAdditionalLength <= mCapacity);
      mLength += aAdditionalLength;
    }

  private:
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    size_t mCapacity;
    size_t mLength;
    char* mData;
  };

  nsresult AppendChunk(Maybe<Chunk>&& aChunk);
  Maybe<Chunk> CreateChunk(size_t aCapacity, bool aRoundUp = true);
  nsresult Compact();
  static size_t RoundedUpCapacity(size_t aCapacity);
  size_t FibonacciCapacityWithMinimum(size_t aMinCapacity);


  
  
  

  void AddWaitingConsumer(IResumable* aConsumer);
  void ResumeWaitingConsumers();

  typedef SourceBufferIterator::State State;

  State AdvanceIteratorOrScheduleResume(SourceBufferIterator& aIterator,
                                        IResumable* aConsumer);
  bool RemainingBytesIsNoMoreThan(const SourceBufferIterator& aIterator,
                                  size_t aBytes) const;

  void OnIteratorRelease();

  
  
  

  nsresult HandleError(nsresult aError);
  bool IsEmpty();
  bool IsLastChunk(uint32_t aChunk);


  
  
  

  static const size_t MIN_CHUNK_CAPACITY = 4096;

  
  mutable Mutex mMutex;

  
  FallibleTArray<Chunk> mChunks;

  
  nsTArray<nsRefPtr<IResumable>> mWaitingConsumers;

  
  Maybe<nsresult> mStatus;

  
  uint32_t mConsumerCount;
};

} 
} 

#endif 
