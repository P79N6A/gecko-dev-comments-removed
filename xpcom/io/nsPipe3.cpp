





#include <algorithm>
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsICloneableInputStream.h"
#include "nsIPipe.h"
#include "nsIEventTarget.h"
#include "nsISeekableStream.h"
#include "nsRefPtr.h"
#include "nsSegmentedBuffer.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "mozilla/Logging.h"
#include "nsIClassInfoImpl.h"
#include "nsAlgorithm.h"
#include "nsMemory.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"

using namespace mozilla;

#ifdef LOG
#undef LOG
#endif



static PRLogModuleInfo*
GetPipeLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("nsPipe");
  }
  return sLog;
}
#define LOG(args) MOZ_LOG(GetPipeLog(), mozilla::LogLevel::Debug, args)

#define DEFAULT_SEGMENT_SIZE  4096
#define DEFAULT_SEGMENT_COUNT 16

class nsPipe;
class nsPipeEvents;
class nsPipeInputStream;
class nsPipeOutputStream;
class AutoReadSegment;

namespace {

enum MonitorAction
{
  DoNotNotifyMonitor,
  NotifyMonitor
};

enum SegmentChangeResult
{
  SegmentNotChanged,
  SegmentDeleted
};

} 






class nsPipeEvents
{
public:
  nsPipeEvents() { }
  ~nsPipeEvents();

  inline void NotifyInputReady(nsIAsyncInputStream* aStream,
                               nsIInputStreamCallback* aCallback)
  {
    NS_ASSERTION(!mInputCallback, "already have an input event");
    mInputStream = aStream;
    mInputCallback = aCallback;
  }

  inline void NotifyOutputReady(nsIAsyncOutputStream* aStream,
                                nsIOutputStreamCallback* aCallback)
  {
    NS_ASSERTION(!mOutputCallback, "already have an output event");
    mOutputStream = aStream;
    mOutputCallback = aCallback;
  }

private:
  nsCOMPtr<nsIAsyncInputStream>     mInputStream;
  nsCOMPtr<nsIInputStreamCallback>  mInputCallback;
  nsCOMPtr<nsIAsyncOutputStream>    mOutputStream;
  nsCOMPtr<nsIOutputStreamCallback> mOutputCallback;
};






struct nsPipeReadState
{
  nsPipeReadState()
    : mReadCursor(nullptr)
    , mReadLimit(nullptr)
    , mSegment(0)
    , mAvailable(0)
    , mActiveRead(false)
    , mNeedDrain(false)
  { }

  char*    mReadCursor;
  char*    mReadLimit;
  int32_t  mSegment;
  uint32_t mAvailable;

  
  bool     mActiveRead;

  
  
  
  bool     mNeedDrain;
};




class nsPipeInputStream final
  : public nsIAsyncInputStream
  , public nsISeekableStream
  , public nsISearchableInputStream
  , public nsICloneableInputStream
  , public nsIClassInfo
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM
  NS_DECL_NSISEEKABLESTREAM
  NS_DECL_NSISEARCHABLEINPUTSTREAM
  NS_DECL_NSICLONEABLEINPUTSTREAM
  NS_DECL_NSICLASSINFO

  explicit nsPipeInputStream(nsPipe* aPipe)
    : mPipe(aPipe)
    , mLogicalOffset(0)
    , mInputStatus(NS_OK)
    , mBlocking(true)
    , mBlocked(false)
    , mCallbackFlags(0)
  { }

  explicit nsPipeInputStream(const nsPipeInputStream& aOther)
    : mPipe(aOther.mPipe)
    , mLogicalOffset(aOther.mLogicalOffset)
    , mInputStatus(aOther.mInputStatus)
    , mBlocking(aOther.mBlocking)
    , mBlocked(false)
    , mCallbackFlags(0)
    , mReadState(aOther.mReadState)
  { }

  nsresult Fill();
  void SetNonBlocking(bool aNonBlocking)
  {
    mBlocking = !aNonBlocking;
  }

  uint32_t Available();

  
  nsresult Wait();

  MonitorAction OnInputReadable(uint32_t aBytesWritten, nsPipeEvents&);
  MonitorAction OnInputException(nsresult, nsPipeEvents&);

  nsPipeReadState& ReadState()
  {
    return mReadState;
  }

  const nsPipeReadState& ReadState() const
  {
    return mReadState;
  }

  nsresult Status() const;

private:
  virtual ~nsPipeInputStream();

  nsRefPtr<nsPipe>               mPipe;

  int64_t                        mLogicalOffset;
  
  
  nsresult                       mInputStatus;
  bool                           mBlocking;

  
  bool                           mBlocked;
  nsCOMPtr<nsIInputStreamCallback> mCallback;
  uint32_t                       mCallbackFlags;

  
  nsPipeReadState                mReadState;
};




class nsPipeOutputStream
  : public nsIAsyncOutputStream
  , public nsIClassInfo
{
public:
  
  
  
  
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIOUTPUTSTREAM
  NS_DECL_NSIASYNCOUTPUTSTREAM
  NS_DECL_NSICLASSINFO

  explicit nsPipeOutputStream(nsPipe* aPipe)
    : mPipe(aPipe)
    , mWriterRefCnt(0)
    , mLogicalOffset(0)
    , mBlocking(true)
    , mBlocked(false)
    , mWritable(true)
    , mCallbackFlags(0)
  { }

  void SetNonBlocking(bool aNonBlocking)
  {
    mBlocking = !aNonBlocking;
  }
  void SetWritable(bool aWritable)
  {
    mWritable = aWritable;
  }

  
  nsresult Wait();

  MonitorAction OnOutputWritable(nsPipeEvents&);
  MonitorAction OnOutputException(nsresult, nsPipeEvents&);

private:
  nsPipe*                         mPipe;

  
  mozilla::ThreadSafeAutoRefCnt   mWriterRefCnt;
  int64_t                         mLogicalOffset;
  bool                            mBlocking;

  
  bool                            mBlocked;
  bool                            mWritable;
  nsCOMPtr<nsIOutputStreamCallback> mCallback;
  uint32_t                        mCallbackFlags;
};



class nsPipe final : public nsIPipe
{
public:
  friend class nsPipeInputStream;
  friend class nsPipeOutputStream;
  friend class AutoReadSegment;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPIPE

  
  nsPipe();

private:
  ~nsPipe();

  
  
  

  void PeekSegment(const nsPipeReadState& aReadState, uint32_t aIndex,
                   char*& aCursor, char*& aLimit);
  SegmentChangeResult AdvanceReadSegment(nsPipeReadState& aReadState);
  bool ReadSegmentBeingWritten(nsPipeReadState& aReadState);
  uint32_t CountSegmentReferences(int32_t aSegment);
  void SetAllNullReadCursors();
  bool AllReadCursorsMatchWriteCursor();
  void RollBackAllReadCursors(char* aWriteCursor);
  void UpdateAllReadCursors(char* aWriteCursor);
  void ValidateAllReadCursors();

  
  
  

  void     DrainInputStream(nsPipeReadState& aReadState, nsPipeEvents& aEvents);

  nsresult GetWriteSegment(char*& aSegment, uint32_t& aSegmentLen);
  void     AdvanceWriteCursor(uint32_t aCount);

  void     OnInputStreamException(nsPipeInputStream* aStream, nsresult aReason);
  void     OnPipeException(nsresult aReason, bool aOutputOnly = false);

  nsresult CloneInputStream(nsPipeInputStream* aOriginal,
                            nsIInputStream** aCloneOut);

  
  nsresult GetReadSegment(nsPipeReadState& aReadState, const char*& aSegment,
                          uint32_t& aLength);
  void     ReleaseReadSegment(nsPipeReadState& aReadState,
                              nsPipeEvents& aEvents);
  void     AdvanceReadCursor(nsPipeReadState& aReadState, uint32_t aCount);

  
  
  
  nsPipeOutputStream  mOutput;

  
  
  
  
  nsTArray<nsPipeInputStream*> mInputList;

  
  
  
  
  nsRefPtr<nsPipeInputStream> mOriginalInput;

  ReentrantMonitor    mReentrantMonitor;
  nsSegmentedBuffer   mBuffer;

  int32_t             mWriteSegment;
  char*               mWriteCursor;
  char*               mWriteLimit;

  nsresult            mStatus;
  bool                mInited;
};





class MOZ_STACK_CLASS AutoReadSegment final
{
public:
  AutoReadSegment(nsPipe* aPipe, nsPipeReadState& aReadState,
                  uint32_t aMaxLength)
    : mPipe(aPipe)
    , mReadState(aReadState)
    , mStatus(NS_ERROR_FAILURE)
    , mSegment(nullptr)
    , mLength(0)
    , mOffset(0)
  {
    MOZ_ASSERT(mPipe);
    MOZ_ASSERT(!mReadState.mActiveRead);
    mStatus = mPipe->GetReadSegment(mReadState, mSegment, mLength);
    if (NS_SUCCEEDED(mStatus)) {
      MOZ_ASSERT(mReadState.mActiveRead);
      MOZ_ASSERT(mSegment);
      mLength = std::min(mLength, aMaxLength);
      MOZ_ASSERT(mLength);
    }
  }

  ~AutoReadSegment()
  {
    if (NS_SUCCEEDED(mStatus)) {
      if (mOffset) {
        mPipe->AdvanceReadCursor(mReadState, mOffset);
      } else {
        nsPipeEvents events;
        mPipe->ReleaseReadSegment(mReadState, events);
      }
    }
    MOZ_ASSERT(!mReadState.mActiveRead);
  }

  nsresult Status() const
  {
    return mStatus;
  }

  const char* Data() const
  {
    MOZ_ASSERT(NS_SUCCEEDED(mStatus));
    MOZ_ASSERT(mSegment);
    return mSegment + mOffset;
  }

  uint32_t Length() const
  {
    MOZ_ASSERT(NS_SUCCEEDED(mStatus));
    MOZ_ASSERT(mLength >= mOffset);
    return mLength - mOffset;
  }

  void
  Advance(uint32_t aCount)
  {
    MOZ_ASSERT(NS_SUCCEEDED(mStatus));
    MOZ_ASSERT(aCount <= (mLength - mOffset));
    mOffset += aCount;
  }

  nsPipeReadState&
  ReadState() const
  {
    return mReadState;
  }

private:
  
  nsPipe* mPipe;
  nsPipeReadState& mReadState;
  nsresult mStatus;
  const char* mSegment;
  uint32_t mLength;
  uint32_t mOffset;
};

















































nsPipe::nsPipe()
  : mOutput(this)
  , mOriginalInput(new nsPipeInputStream(this))
  , mReentrantMonitor("nsPipe.mReentrantMonitor")
  , mWriteSegment(-1)
  , mWriteCursor(nullptr)
  , mWriteLimit(nullptr)
  , mStatus(NS_OK)
  , mInited(false)
{
  mInputList.AppendElement(mOriginalInput);
}

nsPipe::~nsPipe()
{
}

NS_IMPL_ADDREF(nsPipe)
NS_IMPL_QUERY_INTERFACE(nsPipe, nsIPipe)

NS_IMETHODIMP_(MozExternalRefCountType)
nsPipe::Release()
{
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");
  nsrefcnt count = --mRefCnt;
  NS_LOG_RELEASE(this, count, "nsPipe");
  if (count == 0) {
    delete (this);
    return 0;
  }
  if (mOriginalInput && count == 1) {
    mOriginalInput = nullptr;
    return 1;
  }
  return count;
}

NS_IMETHODIMP
nsPipe::Init(bool aNonBlockingIn,
             bool aNonBlockingOut,
             uint32_t aSegmentSize,
             uint32_t aSegmentCount)
{
  mInited = true;

  if (aSegmentSize == 0) {
    aSegmentSize = DEFAULT_SEGMENT_SIZE;
  }
  if (aSegmentCount == 0) {
    aSegmentCount = DEFAULT_SEGMENT_COUNT;
  }

  
  uint32_t maxCount = uint32_t(-1) / aSegmentSize;
  if (aSegmentCount > maxCount) {
    aSegmentCount = maxCount;
  }

  nsresult rv = mBuffer.Init(aSegmentSize, aSegmentSize * aSegmentCount);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mOutput.SetNonBlocking(aNonBlockingOut);
  mOriginalInput->SetNonBlocking(aNonBlockingIn);

  return NS_OK;
}

NS_IMETHODIMP
nsPipe::GetInputStream(nsIAsyncInputStream** aInputStream)
{
  nsRefPtr<nsPipeInputStream> ref = mOriginalInput;
  ref.forget(aInputStream);
  return NS_OK;
}

NS_IMETHODIMP
nsPipe::GetOutputStream(nsIAsyncOutputStream** aOutputStream)
{
  if (NS_WARN_IF(!mInited)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  NS_ADDREF(*aOutputStream = &mOutput);
  return NS_OK;
}

void
nsPipe::PeekSegment(const nsPipeReadState& aReadState, uint32_t aIndex,
                    char*& aCursor, char*& aLimit)
{
  if (aIndex == 0) {
    NS_ASSERTION(!aReadState.mReadCursor || mBuffer.GetSegmentCount(),
                 "unexpected state");
    aCursor = aReadState.mReadCursor;
    aLimit = aReadState.mReadLimit;
  } else {
    uint32_t absoluteIndex = aReadState.mSegment + aIndex;
    uint32_t numSegments = mBuffer.GetSegmentCount();
    if (absoluteIndex >= numSegments) {
      aCursor = aLimit = nullptr;
    } else {
      aCursor = mBuffer.GetSegment(absoluteIndex);
      if (mWriteSegment == (int32_t)absoluteIndex) {
        aLimit = mWriteCursor;
      } else {
        aLimit = aCursor + mBuffer.GetSegmentSize();
      }
    }
  }
}

nsresult
nsPipe::GetReadSegment(nsPipeReadState& aReadState, const char*& aSegment,
                       uint32_t& aLength)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (aReadState.mReadCursor == aReadState.mReadLimit) {
    return NS_FAILED(mStatus) ? mStatus : NS_BASE_STREAM_WOULD_BLOCK;
  }

  
  
  
  
  
  MOZ_ASSERT(!aReadState.mActiveRead);
  aReadState.mActiveRead = true;

  aSegment = aReadState.mReadCursor;
  aLength = aReadState.mReadLimit - aReadState.mReadCursor;

  return NS_OK;
}

void
nsPipe::ReleaseReadSegment(nsPipeReadState& aReadState, nsPipeEvents& aEvents)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  MOZ_ASSERT(aReadState.mActiveRead);
  aReadState.mActiveRead = false;

  
  
  
  
  if (aReadState.mNeedDrain) {
    aReadState.mNeedDrain = false;
    DrainInputStream(aReadState, aEvents);
  }
}

void
nsPipe::AdvanceReadCursor(nsPipeReadState& aReadState, uint32_t aBytesRead)
{
  NS_ASSERTION(aBytesRead, "don't call if no bytes read");

  nsPipeEvents events;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    LOG(("III advancing read cursor by %u\n", aBytesRead));
    NS_ASSERTION(aBytesRead <= mBuffer.GetSegmentSize(), "read too much");

    aReadState.mReadCursor += aBytesRead;
    NS_ASSERTION(aReadState.mReadCursor <= aReadState.mReadLimit,
                 "read cursor exceeds limit");

    MOZ_ASSERT(aReadState.mAvailable >= aBytesRead);
    aReadState.mAvailable -= aBytesRead;

    
    
    
    if (aReadState.mReadCursor == aReadState.mReadLimit &&
        !ReadSegmentBeingWritten(aReadState)) {

      
      
      if (AdvanceReadSegment(aReadState) == SegmentDeleted &&
          mOutput.OnOutputWritable(events) == NotifyMonitor) {
        mon.NotifyAll();
      }
    }

    ReleaseReadSegment(aReadState, events);
  }
}

SegmentChangeResult
nsPipe::AdvanceReadSegment(nsPipeReadState& aReadState)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  int32_t currentSegment = aReadState.mSegment;

  
  aReadState.mSegment += 1;

  SegmentChangeResult result = SegmentNotChanged;

  
  if (currentSegment == 0 && CountSegmentReferences(currentSegment) == 0) {

    
    mWriteSegment -= 1;

    
    
    
    aReadState.mSegment -= 1;

    for (uint32_t i = 0; i < mInputList.Length(); ++i) {
      
      
      if (&mInputList[i]->ReadState() == &aReadState) {
        continue;
      }
      mInputList[i]->ReadState().mSegment -= 1;
    }

    
    mBuffer.DeleteFirstSegment();
    LOG(("III deleting first segment\n"));

    result = SegmentDeleted;
  }

  if (mWriteSegment < aReadState.mSegment) {
    
    MOZ_ASSERT(mWriteSegment == (aReadState.mSegment - 1));
    aReadState.mReadCursor = nullptr;
    aReadState.mReadLimit = nullptr;
    
    if (mWriteSegment == -1) {
      mWriteCursor = nullptr;
      mWriteLimit = nullptr;
    }
  } else {
    
    aReadState.mReadCursor = mBuffer.GetSegment(aReadState.mSegment);
    if (mWriteSegment == aReadState.mSegment) {
      aReadState.mReadLimit = mWriteCursor;
    } else {
      aReadState.mReadLimit = aReadState.mReadCursor + mBuffer.GetSegmentSize();
    }
  }

  return result;
}

void
nsPipe::DrainInputStream(nsPipeReadState& aReadState, nsPipeEvents& aEvents)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  
  
  
  
  
  if (aReadState.mActiveRead) {
    MOZ_ASSERT(!aReadState.mNeedDrain);
    aReadState.mNeedDrain = true;
    return;
  }

  aReadState.mAvailable = 0;

  SegmentChangeResult result = SegmentNotChanged;
  while(mWriteSegment >= aReadState.mSegment) {

    
    
    if (ReadSegmentBeingWritten(aReadState)) {
      break;
    }

    if (AdvanceReadSegment(aReadState) == SegmentDeleted) {
      result = SegmentDeleted;
    }
  }

  
  
  if (result == SegmentDeleted &&
      mOutput.OnOutputWritable(aEvents) == NotifyMonitor) {
    mon.NotifyAll();
  }
}

bool
nsPipe::ReadSegmentBeingWritten(nsPipeReadState& aReadState)
{
  mReentrantMonitor.AssertCurrentThreadIn();
  bool beingWritten = mWriteSegment == aReadState.mSegment &&
                      mWriteLimit > mWriteCursor;
  NS_ASSERTION(!beingWritten || aReadState.mReadLimit == mWriteCursor,
               "unexpected state");
  return beingWritten;
}

nsresult
nsPipe::GetWriteSegment(char*& aSegment, uint32_t& aSegmentLen)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  
  if (mWriteCursor == mWriteLimit) {
    char* seg = mBuffer.AppendNewSegment();
    
    if (!seg) {
      return NS_BASE_STREAM_WOULD_BLOCK;
    }
    LOG(("OOO appended new segment\n"));
    mWriteCursor = seg;
    mWriteLimit = mWriteCursor + mBuffer.GetSegmentSize();
    ++mWriteSegment;
  }

  
  SetAllNullReadCursors();

  
  
  if (mWriteSegment == 0 && AllReadCursorsMatchWriteCursor()) {
    char* head = mBuffer.GetSegment(0);
    LOG(("OOO rolling back write cursor %u bytes\n", mWriteCursor - head));
    RollBackAllReadCursors(head);
    mWriteCursor = head;
  }

  aSegment    = mWriteCursor;
  aSegmentLen = mWriteLimit - mWriteCursor;
  return NS_OK;
}

void
nsPipe::AdvanceWriteCursor(uint32_t aBytesWritten)
{
  NS_ASSERTION(aBytesWritten, "don't call if no bytes written");

  nsPipeEvents events;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    LOG(("OOO advancing write cursor by %u\n", aBytesWritten));

    char* newWriteCursor = mWriteCursor + aBytesWritten;
    NS_ASSERTION(newWriteCursor <= mWriteLimit, "write cursor exceeds limit");

    
    UpdateAllReadCursors(newWriteCursor);

    mWriteCursor = newWriteCursor;

    ValidateAllReadCursors();

    
    if (mWriteCursor == mWriteLimit) {
      if (mBuffer.GetSize() >= mBuffer.GetMaxSize()) {
        mOutput.SetWritable(false);
      }
    }

    
    bool needNotify = false;
    for (uint32_t i = 0; i < mInputList.Length(); ++i) {
      if (mInputList[i]->OnInputReadable(aBytesWritten, events) == NotifyMonitor) {
        needNotify = true;
      }
    }

    if (needNotify) {
      mon.NotifyAll();
    }
  }
}

void
nsPipe::OnInputStreamException(nsPipeInputStream* aStream, nsresult aReason)
{
  MOZ_ASSERT(NS_FAILED(aReason));

  nsPipeEvents events;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    
    
    
    

    
    
    if (mInputList.Length() == 1) {
      if (mInputList[0] == aStream) {
        OnPipeException(aReason);
      }
      return;
    }

    
    for (uint32_t i = 0; i < mInputList.Length(); ++i) {
      if (mInputList[i] != aStream) {
        continue;
      }

      MonitorAction action = mInputList[i]->OnInputException(aReason, events);
      mInputList.RemoveElementAt(i);

      
      if (action == NotifyMonitor) {
        mon.NotifyAll();
      }

      return;
    }
  }
}

void
nsPipe::OnPipeException(nsresult aReason, bool aOutputOnly)
{
  LOG(("PPP nsPipe::OnPipeException [reason=%x output-only=%d]\n",
       aReason, aOutputOnly));

  nsPipeEvents events;
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    if (NS_FAILED(mStatus)) {
      return;
    }

    mStatus = aReason;

    bool needNotify = false;

    nsTArray<nsPipeInputStream*> tmpInputList;
    for (uint32_t i = 0; i < mInputList.Length(); ++i) {
      
      
      if (aOutputOnly && mInputList[i]->Available()) {
        tmpInputList.AppendElement(mInputList[i]);
        continue;
      }

      if (mInputList[i]->OnInputException(aReason, events) == NotifyMonitor) {
        needNotify = true;
      }
    }
    mInputList = tmpInputList;

    if (mOutput.OnOutputException(aReason, events) == NotifyMonitor) {
      needNotify = true;
    }

    
    if (needNotify) {
      mon.NotifyAll();
    }
  }
}

nsresult
nsPipe::CloneInputStream(nsPipeInputStream* aOriginal,
                         nsIInputStream** aCloneOut)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  nsRefPtr<nsPipeInputStream> ref = new nsPipeInputStream(*aOriginal);
  mInputList.AppendElement(ref);
  ref.forget(aCloneOut);
  return NS_OK;
}

uint32_t
nsPipe::CountSegmentReferences(int32_t aSegment)
{
  mReentrantMonitor.AssertCurrentThreadIn();
  uint32_t count = 0;
  for (uint32_t i = 0; i < mInputList.Length(); ++i) {
    if (aSegment >= mInputList[i]->ReadState().mSegment) {
      count += 1;
    }
  }
  return count;
}

void
nsPipe::SetAllNullReadCursors()
{
  mReentrantMonitor.AssertCurrentThreadIn();
  for (uint32_t i = 0; i < mInputList.Length(); ++i) {
    nsPipeReadState& readState = mInputList[i]->ReadState();
    if (!readState.mReadCursor) {
      NS_ASSERTION(mWriteSegment == readState.mSegment,
                   "unexpected null read cursor");
      readState.mReadCursor = readState.mReadLimit = mWriteCursor;
    }
  }
}

bool
nsPipe::AllReadCursorsMatchWriteCursor()
{
  mReentrantMonitor.AssertCurrentThreadIn();
  for (uint32_t i = 0; i < mInputList.Length(); ++i) {
    const nsPipeReadState& readState = mInputList[i]->ReadState();
    if (readState.mSegment != mWriteSegment ||
        readState.mReadCursor != mWriteCursor) {
      return false;
    }
  }
  return true;
}

void
nsPipe::RollBackAllReadCursors(char* aWriteCursor)
{
  mReentrantMonitor.AssertCurrentThreadIn();
  for (uint32_t i = 0; i < mInputList.Length(); ++i) {
    nsPipeReadState& readState = mInputList[i]->ReadState();
    MOZ_ASSERT(mWriteSegment == readState.mSegment);
    MOZ_ASSERT(mWriteCursor == readState.mReadCursor);
    MOZ_ASSERT(mWriteCursor == readState.mReadLimit);
    readState.mReadCursor = aWriteCursor;
    readState.mReadLimit = aWriteCursor;
  }
}

void
nsPipe::UpdateAllReadCursors(char* aWriteCursor)
{
  mReentrantMonitor.AssertCurrentThreadIn();
  for (uint32_t i = 0; i < mInputList.Length(); ++i) {
    nsPipeReadState& readState = mInputList[i]->ReadState();
    if (mWriteSegment == readState.mSegment &&
        readState.mReadLimit == mWriteCursor) {
      readState.mReadLimit = aWriteCursor;
    }
  }
}

void
nsPipe::ValidateAllReadCursors()
{
  mReentrantMonitor.AssertCurrentThreadIn();
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#ifdef DEBUG
  for (uint32_t i = 0; i < mInputList.Length(); ++i) {
    const nsPipeReadState& state = mInputList[i]->ReadState();
    NS_ASSERTION(state.mReadCursor != mWriteCursor ||
                 (mBuffer.GetSegment(state.mSegment) == state.mReadCursor &&
                  mWriteCursor == mWriteLimit),
                 "read cursor is bad");
  }
#endif
}





nsPipeEvents::~nsPipeEvents()
{
  

  if (mInputCallback) {
    mInputCallback->OnInputStreamReady(mInputStream);
    mInputCallback = 0;
    mInputStream = 0;
  }
  if (mOutputCallback) {
    mOutputCallback->OnOutputStreamReady(mOutputStream);
    mOutputCallback = 0;
    mOutputStream = 0;
  }
}





NS_IMPL_ADDREF(nsPipeInputStream);
NS_IMPL_RELEASE(nsPipeInputStream);

NS_IMPL_QUERY_INTERFACE(nsPipeInputStream,
                        nsIInputStream,
                        nsIAsyncInputStream,
                        nsISeekableStream,
                        nsISearchableInputStream,
                        nsICloneableInputStream,
                        nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER(nsPipeInputStream,
                            nsIInputStream,
                            nsIAsyncInputStream,
                            nsISeekableStream,
                            nsISearchableInputStream,
                            nsICloneableInputStream)

NS_IMPL_THREADSAFE_CI(nsPipeInputStream)

uint32_t
nsPipeInputStream::Available()
{
  mPipe->mReentrantMonitor.AssertCurrentThreadIn();
  return mReadState.mAvailable;
}

nsresult
nsPipeInputStream::Wait()
{
  NS_ASSERTION(mBlocking, "wait on non-blocking pipe input stream");

  ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

  while (NS_SUCCEEDED(Status()) && (mReadState.mAvailable == 0)) {
    LOG(("III pipe input: waiting for data\n"));

    mBlocked = true;
    mon.Wait();
    mBlocked = false;

    LOG(("III pipe input: woke up [status=%x available=%u]\n",
         Status(), mReadState.mAvailable));
  }

  return Status() == NS_BASE_STREAM_CLOSED ? NS_OK : Status();
}

MonitorAction
nsPipeInputStream::OnInputReadable(uint32_t aBytesWritten, nsPipeEvents& aEvents)
{
  MonitorAction result = DoNotNotifyMonitor;

  mPipe->mReentrantMonitor.AssertCurrentThreadIn();
  mReadState.mAvailable += aBytesWritten;

  if (mCallback && !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
    aEvents.NotifyInputReady(this, mCallback);
    mCallback = 0;
    mCallbackFlags = 0;
  } else if (mBlocked) {
    result = NotifyMonitor;
  }

  return result;
}

MonitorAction
nsPipeInputStream::OnInputException(nsresult aReason, nsPipeEvents& aEvents)
{
  LOG(("nsPipeInputStream::OnInputException [this=%x reason=%x]\n",
       this, aReason));

  MonitorAction result = DoNotNotifyMonitor;

  NS_ASSERTION(NS_FAILED(aReason), "huh? successful exception");

  if (NS_SUCCEEDED(mInputStatus)) {
    mInputStatus = aReason;
  }

  
  mPipe->DrainInputStream(mReadState, aEvents);

  if (mCallback) {
    aEvents.NotifyInputReady(this, mCallback);
    mCallback = 0;
    mCallbackFlags = 0;
  } else if (mBlocked) {
    result = NotifyMonitor;
  }

  return result;
}

NS_IMETHODIMP
nsPipeInputStream::CloseWithStatus(nsresult aReason)
{
  LOG(("III CloseWithStatus [this=%x reason=%x]\n", this, aReason));

  if (NS_FAILED(mInputStatus)) {
    return NS_OK;
  }

  if (NS_SUCCEEDED(aReason)) {
    aReason = NS_BASE_STREAM_CLOSED;
  }

  mPipe->OnInputStreamException(this, aReason);
  return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::Close()
{
  return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsPipeInputStream::Available(uint64_t* aResult)
{
  
  ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

  
  if (!mReadState.mAvailable && NS_FAILED(Status())) {
    return Status();
  }

  *aResult = (uint64_t)mReadState.mAvailable;
  return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::ReadSegments(nsWriteSegmentFun aWriter,
                                void* aClosure,
                                uint32_t aCount,
                                uint32_t* aReadCount)
{
  LOG(("III ReadSegments [this=%x count=%u]\n", this, aCount));

  nsresult rv = NS_OK;

  *aReadCount = 0;
  while (aCount) {
    AutoReadSegment segment(mPipe, mReadState, aCount);
    rv = segment.Status();
    if (NS_FAILED(rv)) {
      
      if (*aReadCount > 0) {
        rv = NS_OK;
        break;
      }
      if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        
        if (!mBlocking) {
          break;
        }
        
        rv = Wait();
        if (NS_SUCCEEDED(rv)) {
          continue;
        }
      }
      
      if (rv == NS_BASE_STREAM_CLOSED) {
        rv = NS_OK;
        break;
      }
      mPipe->OnInputStreamException(this, rv);
      break;
    }

    uint32_t writeCount;
    while (segment.Length()) {
      writeCount = 0;

      rv = aWriter(this, aClosure, segment.Data(), *aReadCount,
                   segment.Length(), &writeCount);

      if (NS_FAILED(rv) || writeCount == 0) {
        aCount = 0;
        
        
        rv = NS_OK;
        break;
      }

      NS_ASSERTION(writeCount <= segment.Length(), "wrote more than expected");
      segment.Advance(writeCount);
      aCount -= writeCount;
      *aReadCount += writeCount;
      mLogicalOffset += writeCount;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsPipeInputStream::Read(char* aToBuf, uint32_t aBufLen, uint32_t* aReadCount)
{
  return ReadSegments(NS_CopySegmentToBuffer, aToBuf, aBufLen, aReadCount);
}

NS_IMETHODIMP
nsPipeInputStream::IsNonBlocking(bool* aNonBlocking)
{
  *aNonBlocking = !mBlocking;
  return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::AsyncWait(nsIInputStreamCallback* aCallback,
                             uint32_t aFlags,
                             uint32_t aRequestedCount,
                             nsIEventTarget* aTarget)
{
  LOG(("III AsyncWait [this=%x]\n", this));

  nsPipeEvents pipeEvents;
  {
    ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

    
    mCallback = 0;
    mCallbackFlags = 0;

    if (!aCallback) {
      return NS_OK;
    }

    nsCOMPtr<nsIInputStreamCallback> proxy;
    if (aTarget) {
      proxy = NS_NewInputStreamReadyEvent(aCallback, aTarget);
      aCallback = proxy;
    }

    if (NS_FAILED(Status()) ||
       (mReadState.mAvailable && !(aFlags & WAIT_CLOSURE_ONLY))) {
      
      pipeEvents.NotifyInputReady(this, aCallback);
    } else {
      
      mCallback = aCallback;
      mCallbackFlags = aFlags;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::Seek(int32_t aWhence, int64_t aOffset)
{
  NS_NOTREACHED("nsPipeInputStream::Seek");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPipeInputStream::Tell(int64_t* aOffset)
{
  ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

  
  if (!mReadState.mAvailable && NS_FAILED(Status())) {
    return Status();
  }

  *aOffset = mLogicalOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::SetEOF()
{
  NS_NOTREACHED("nsPipeInputStream::SetEOF");
  return NS_ERROR_NOT_IMPLEMENTED;
}

static bool strings_equal(bool aIgnoreCase,
                          const char* aS1, const char* aS2, uint32_t aLen)
{
  return aIgnoreCase
    ? !nsCRT::strncasecmp(aS1, aS2, aLen) : !nsCRT::strncmp(aS1, aS2, aLen);
}

NS_IMETHODIMP
nsPipeInputStream::Search(const char* aForString,
                          bool aIgnoreCase,
                          bool* aFound,
                          uint32_t* aOffsetSearchedTo)
{
  LOG(("III Search [for=%s ic=%u]\n", aForString, aIgnoreCase));

  ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

  char* cursor1;
  char* limit1;
  uint32_t index = 0, offset = 0;
  uint32_t strLen = strlen(aForString);

  mPipe->PeekSegment(mReadState, 0, cursor1, limit1);
  if (cursor1 == limit1) {
    *aFound = false;
    *aOffsetSearchedTo = 0;
    LOG(("  result [aFound=%u offset=%u]\n", *aFound, *aOffsetSearchedTo));
    return NS_OK;
  }

  while (true) {
    uint32_t i, len1 = limit1 - cursor1;

    
    for (i = 0; i < len1 - strLen + 1; i++) {
      if (strings_equal(aIgnoreCase, &cursor1[i], aForString, strLen)) {
        *aFound = true;
        *aOffsetSearchedTo = offset + i;
        LOG(("  result [aFound=%u offset=%u]\n", *aFound, *aOffsetSearchedTo));
        return NS_OK;
      }
    }

    
    char* cursor2;
    char* limit2;
    uint32_t len2;

    index++;
    offset += len1;

    mPipe->PeekSegment(mReadState, index, cursor2, limit2);
    if (cursor2 == limit2) {
      *aFound = false;
      *aOffsetSearchedTo = offset - strLen + 1;
      LOG(("  result [aFound=%u offset=%u]\n", *aFound, *aOffsetSearchedTo));
      return NS_OK;
    }
    len2 = limit2 - cursor2;

    
    uint32_t lim = XPCOM_MIN(strLen, len2 + 1);
    for (i = 0; i < lim; ++i) {
      uint32_t strPart1Len = strLen - i - 1;
      uint32_t strPart2Len = strLen - strPart1Len;
      const char* strPart2 = &aForString[strLen - strPart2Len];
      uint32_t bufSeg1Offset = len1 - strPart1Len;
      if (strings_equal(aIgnoreCase, &cursor1[bufSeg1Offset], aForString, strPart1Len) &&
          strings_equal(aIgnoreCase, cursor2, strPart2, strPart2Len)) {
        *aFound = true;
        *aOffsetSearchedTo = offset - strPart1Len;
        LOG(("  result [aFound=%u offset=%u]\n", *aFound, *aOffsetSearchedTo));
        return NS_OK;
      }
    }

    
    cursor1 = cursor2;
    limit1 = limit2;
  }

  NS_NOTREACHED("can't get here");
  return NS_ERROR_UNEXPECTED;    
}

NS_IMETHODIMP
nsPipeInputStream::GetCloneable(bool* aCloneableOut)
{
  *aCloneableOut = true;
  return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::Clone(nsIInputStream** aCloneOut)
{
  return mPipe->CloneInputStream(this, aCloneOut);
}

nsresult
nsPipeInputStream::Status() const
{
  return NS_FAILED(mInputStatus) ? mInputStatus : mPipe->mStatus;
}

nsPipeInputStream::~nsPipeInputStream()
{
  Close();
}





NS_IMPL_QUERY_INTERFACE(nsPipeOutputStream,
                        nsIOutputStream,
                        nsIAsyncOutputStream,
                        nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER(nsPipeOutputStream,
                            nsIOutputStream,
                            nsIAsyncOutputStream)

NS_IMPL_THREADSAFE_CI(nsPipeOutputStream)

nsresult
nsPipeOutputStream::Wait()
{
  NS_ASSERTION(mBlocking, "wait on non-blocking pipe output stream");

  ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

  if (NS_SUCCEEDED(mPipe->mStatus) && !mWritable) {
    LOG(("OOO pipe output: waiting for space\n"));
    mBlocked = true;
    mon.Wait();
    mBlocked = false;
    LOG(("OOO pipe output: woke up [pipe-status=%x writable=%u]\n",
         mPipe->mStatus, mWritable));
  }

  return mPipe->mStatus == NS_BASE_STREAM_CLOSED ? NS_OK : mPipe->mStatus;
}

MonitorAction
nsPipeOutputStream::OnOutputWritable(nsPipeEvents& aEvents)
{
  MonitorAction result = DoNotNotifyMonitor;

  mWritable = true;

  if (mCallback && !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
    aEvents.NotifyOutputReady(this, mCallback);
    mCallback = 0;
    mCallbackFlags = 0;
  } else if (mBlocked) {
    result = NotifyMonitor;
  }

  return result;
}

MonitorAction
nsPipeOutputStream::OnOutputException(nsresult aReason, nsPipeEvents& aEvents)
{
  LOG(("nsPipeOutputStream::OnOutputException [this=%x reason=%x]\n",
       this, aReason));

  MonitorAction result = DoNotNotifyMonitor;

  NS_ASSERTION(NS_FAILED(aReason), "huh? successful exception");
  mWritable = false;

  if (mCallback) {
    aEvents.NotifyOutputReady(this, mCallback);
    mCallback = 0;
    mCallbackFlags = 0;
  } else if (mBlocked) {
    result = NotifyMonitor;
  }

  return result;
}


NS_IMETHODIMP_(MozExternalRefCountType)
nsPipeOutputStream::AddRef()
{
  ++mWriterRefCnt;
  return mPipe->AddRef();
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsPipeOutputStream::Release()
{
  if (--mWriterRefCnt == 0) {
    Close();
  }
  return mPipe->Release();
}

NS_IMETHODIMP
nsPipeOutputStream::CloseWithStatus(nsresult aReason)
{
  LOG(("OOO CloseWithStatus [this=%x reason=%x]\n", this, aReason));

  if (NS_SUCCEEDED(aReason)) {
    aReason = NS_BASE_STREAM_CLOSED;
  }

  
  mPipe->OnPipeException(aReason, true);
  return NS_OK;
}

NS_IMETHODIMP
nsPipeOutputStream::Close()
{
  return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsPipeOutputStream::WriteSegments(nsReadSegmentFun aReader,
                                  void* aClosure,
                                  uint32_t aCount,
                                  uint32_t* aWriteCount)
{
  LOG(("OOO WriteSegments [this=%x count=%u]\n", this, aCount));

  nsresult rv = NS_OK;

  char* segment;
  uint32_t segmentLen;

  *aWriteCount = 0;
  while (aCount) {
    rv = mPipe->GetWriteSegment(segment, segmentLen);
    if (NS_FAILED(rv)) {
      if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        
        if (!mBlocking) {
          
          if (*aWriteCount > 0) {
            rv = NS_OK;
          }
          break;
        }
        
        rv = Wait();
        if (NS_SUCCEEDED(rv)) {
          continue;
        }
      }
      mPipe->OnPipeException(rv);
      break;
    }

    
    if (segmentLen > aCount) {
      segmentLen = aCount;
    }

    uint32_t readCount, originalLen = segmentLen;
    while (segmentLen) {
      readCount = 0;

      rv = aReader(this, aClosure, segment, *aWriteCount, segmentLen, &readCount);

      if (NS_FAILED(rv) || readCount == 0) {
        aCount = 0;
        
        
        rv = NS_OK;
        break;
      }

      NS_ASSERTION(readCount <= segmentLen, "read more than expected");
      segment += readCount;
      segmentLen -= readCount;
      aCount -= readCount;
      *aWriteCount += readCount;
      mLogicalOffset += readCount;
    }

    if (segmentLen < originalLen) {
      mPipe->AdvanceWriteCursor(originalLen - segmentLen);
    }
  }

  return rv;
}

static NS_METHOD
nsReadFromRawBuffer(nsIOutputStream* aOutStr,
                    void* aClosure,
                    char* aToRawSegment,
                    uint32_t aOffset,
                    uint32_t aCount,
                    uint32_t* aReadCount)
{
  const char* fromBuf = (const char*)aClosure;
  memcpy(aToRawSegment, &fromBuf[aOffset], aCount);
  *aReadCount = aCount;
  return NS_OK;
}

NS_IMETHODIMP
nsPipeOutputStream::Write(const char* aFromBuf,
                          uint32_t aBufLen,
                          uint32_t* aWriteCount)
{
  return WriteSegments(nsReadFromRawBuffer, (void*)aFromBuf, aBufLen, aWriteCount);
}

NS_IMETHODIMP
nsPipeOutputStream::Flush(void)
{
  
  return NS_OK;
}

static NS_METHOD
nsReadFromInputStream(nsIOutputStream* aOutStr,
                      void* aClosure,
                      char* aToRawSegment,
                      uint32_t aOffset,
                      uint32_t aCount,
                      uint32_t* aReadCount)
{
  nsIInputStream* fromStream = (nsIInputStream*)aClosure;
  return fromStream->Read(aToRawSegment, aCount, aReadCount);
}

NS_IMETHODIMP
nsPipeOutputStream::WriteFrom(nsIInputStream* aFromStream,
                              uint32_t aCount,
                              uint32_t* aWriteCount)
{
  return WriteSegments(nsReadFromInputStream, aFromStream, aCount, aWriteCount);
}

NS_IMETHODIMP
nsPipeOutputStream::IsNonBlocking(bool* aNonBlocking)
{
  *aNonBlocking = !mBlocking;
  return NS_OK;
}

NS_IMETHODIMP
nsPipeOutputStream::AsyncWait(nsIOutputStreamCallback* aCallback,
                              uint32_t aFlags,
                              uint32_t aRequestedCount,
                              nsIEventTarget* aTarget)
{
  LOG(("OOO AsyncWait [this=%x]\n", this));

  nsPipeEvents pipeEvents;
  {
    ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

    
    mCallback = 0;
    mCallbackFlags = 0;

    if (!aCallback) {
      return NS_OK;
    }

    nsCOMPtr<nsIOutputStreamCallback> proxy;
    if (aTarget) {
      proxy = NS_NewOutputStreamReadyEvent(aCallback, aTarget);
      aCallback = proxy;
    }

    if (NS_FAILED(mPipe->mStatus) ||
        (mWritable && !(aFlags & WAIT_CLOSURE_ONLY))) {
      
      pipeEvents.NotifyOutputReady(this, aCallback);
    } else {
      
      mCallback = aCallback;
      mCallbackFlags = aFlags;
    }
  }
  return NS_OK;
}



nsresult
NS_NewPipe(nsIInputStream** aPipeIn,
           nsIOutputStream** aPipeOut,
           uint32_t aSegmentSize,
           uint32_t aMaxSize,
           bool aNonBlockingInput,
           bool aNonBlockingOutput)
{
  if (aSegmentSize == 0) {
    aSegmentSize = DEFAULT_SEGMENT_SIZE;
  }

  
  uint32_t segmentCount;
  if (aMaxSize == UINT32_MAX) {
    segmentCount = UINT32_MAX;
  } else {
    segmentCount = aMaxSize / aSegmentSize;
  }

  nsIAsyncInputStream* in;
  nsIAsyncOutputStream* out;
  nsresult rv = NS_NewPipe2(&in, &out, aNonBlockingInput, aNonBlockingOutput,
                            aSegmentSize, segmentCount);
  if (NS_FAILED(rv)) {
    return rv;
  }

  *aPipeIn = in;
  *aPipeOut = out;
  return NS_OK;
}

nsresult
NS_NewPipe2(nsIAsyncInputStream** aPipeIn,
            nsIAsyncOutputStream** aPipeOut,
            bool aNonBlockingInput,
            bool aNonBlockingOutput,
            uint32_t aSegmentSize,
            uint32_t aSegmentCount)
{
  nsresult rv;

  nsPipe* pipe = new nsPipe();
  rv = pipe->Init(aNonBlockingInput,
                  aNonBlockingOutput,
                  aSegmentSize,
                  aSegmentCount);
  if (NS_FAILED(rv)) {
    NS_ADDREF(pipe);
    NS_RELEASE(pipe);
    return rv;
  }

  pipe->GetInputStream(aPipeIn);
  pipe->GetOutputStream(aPipeOut);
  return NS_OK;
}

nsresult
nsPipeConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }
  nsPipe* pipe = new nsPipe();
  NS_ADDREF(pipe);
  nsresult rv = pipe->QueryInterface(aIID, aResult);
  NS_RELEASE(pipe);
  return rv;
}


