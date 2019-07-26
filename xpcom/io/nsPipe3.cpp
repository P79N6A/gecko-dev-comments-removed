



#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsIPipe.h"
#include "nsIEventTarget.h"
#include "nsISeekableStream.h"
#include "nsIProgrammingLanguage.h"
#include "nsSegmentedBuffer.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "prlog.h"
#include "nsIClassInfoImpl.h"
#include "nsAtomicRefcnt.h"
#include "nsAlgorithm.h"

using namespace mozilla;

#if defined(PR_LOGGING)



static PRLogModuleInfo *gPipeLog = PR_NewLogModule("nsPipe");
#define LOG(args) PR_LOG(gPipeLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

#define DEFAULT_SEGMENT_SIZE  4096
#define DEFAULT_SEGMENT_COUNT 16

class nsPipe;
class nsPipeEvents;
class nsPipeInputStream;
class nsPipeOutputStream;






class nsPipeEvents
{
public:
    nsPipeEvents() { }
   ~nsPipeEvents();

    inline void NotifyInputReady(nsIAsyncInputStream *stream,
                                 nsIInputStreamCallback *callback)
    {
        NS_ASSERTION(!mInputCallback, "already have an input event");
        mInputStream = stream;
        mInputCallback = callback;
    }

    inline void NotifyOutputReady(nsIAsyncOutputStream *stream,
                                  nsIOutputStreamCallback *callback)
    {
        NS_ASSERTION(!mOutputCallback, "already have an output event");
        mOutputStream = stream;
        mOutputCallback = callback;
    }

private:
    nsCOMPtr<nsIAsyncInputStream>     mInputStream;
    nsCOMPtr<nsIInputStreamCallback>  mInputCallback;
    nsCOMPtr<nsIAsyncOutputStream>    mOutputStream;
    nsCOMPtr<nsIOutputStreamCallback> mOutputCallback;
};




class nsPipeInputStream : public nsIAsyncInputStream
                        , public nsISeekableStream
                        , public nsISearchableInputStream
                        , public nsIClassInfo
{
public:
    
    
    
    
    
    NS_DECL_ISUPPORTS_INHERITED

    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIASYNCINPUTSTREAM
    NS_DECL_NSISEEKABLESTREAM
    NS_DECL_NSISEARCHABLEINPUTSTREAM
    NS_DECL_NSICLASSINFO

    nsPipeInputStream(nsPipe *pipe)
        : mPipe(pipe)
        , mReaderRefCnt(0)
        , mLogicalOffset(0)
        , mBlocking(true)
        , mBlocked(false)
        , mAvailable(0)
        , mCallbackFlags(0)
        { }

    nsresult Fill();
    void SetNonBlocking(bool aNonBlocking) { mBlocking = !aNonBlocking; }

    PRUint32 Available() { return mAvailable; }
    void     ReduceAvailable(PRUint32 avail) { mAvailable -= avail; }

    
    nsresult Wait();

    
    
    bool     OnInputReadable(PRUint32 bytesWritten, nsPipeEvents &);
    bool     OnInputException(nsresult, nsPipeEvents &);

private:
    nsPipe                        *mPipe;

    
    nsrefcnt                       mReaderRefCnt;
    PRInt64                        mLogicalOffset;
    bool                           mBlocking;

    
    bool                           mBlocked;
    PRUint32                       mAvailable;
    nsCOMPtr<nsIInputStreamCallback> mCallback;
    PRUint32                       mCallbackFlags;
};




class nsPipeOutputStream : public nsIAsyncOutputStream
                         , public nsIClassInfo
{
public:
    
    
    
    
    
    NS_DECL_ISUPPORTS_INHERITED

    NS_DECL_NSIOUTPUTSTREAM
    NS_DECL_NSIASYNCOUTPUTSTREAM
    NS_DECL_NSICLASSINFO

    nsPipeOutputStream(nsPipe *pipe)
        : mPipe(pipe)
        , mWriterRefCnt(0)
        , mLogicalOffset(0)
        , mBlocking(true)
        , mBlocked(false)
        , mWritable(true)
        , mCallbackFlags(0)
        { }

    void SetNonBlocking(bool aNonBlocking) { mBlocking = !aNonBlocking; }
    void SetWritable(bool writable) { mWritable = writable; }

    
    nsresult Wait();

    
    
    bool     OnOutputWritable(nsPipeEvents &);
    bool     OnOutputException(nsresult, nsPipeEvents &);

private:
    nsPipe                         *mPipe;

    
    nsrefcnt                        mWriterRefCnt;
    PRInt64                         mLogicalOffset;
    bool                            mBlocking;

    
    bool                            mBlocked;
    bool                            mWritable;
    nsCOMPtr<nsIOutputStreamCallback> mCallback;
    PRUint32                        mCallbackFlags;
};



class nsPipe MOZ_FINAL : public nsIPipe
{
public:
    friend class nsPipeInputStream;
    friend class nsPipeOutputStream;

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPIPE

    
    nsPipe();

private:
    ~nsPipe();

public:
    
    
    

    void PeekSegment(PRUint32 n, char *&cursor, char *&limit);

    
    
    
 
    nsresult GetReadSegment(const char *&segment, PRUint32 &segmentLen);
    void     AdvanceReadCursor(PRUint32 count);

    nsresult GetWriteSegment(char *&segment, PRUint32 &segmentLen);
    void     AdvanceWriteCursor(PRUint32 count);

    void     OnPipeException(nsresult reason, bool outputOnly = false);

protected:
    
    
    
    nsPipeInputStream   mInput;
    nsPipeOutputStream  mOutput;

    ReentrantMonitor    mReentrantMonitor;
    nsSegmentedBuffer   mBuffer;

    char*               mReadCursor;
    char*               mReadLimit;

    PRInt32             mWriteSegment;
    char*               mWriteCursor;
    char*               mWriteLimit;

    nsresult            mStatus;
    bool                mInited;
};












































nsPipe::nsPipe()
    : mInput(this)
    , mOutput(this)
    , mReentrantMonitor("nsPipe.mReentrantMonitor")
    , mReadCursor(nullptr)
    , mReadLimit(nullptr)
    , mWriteSegment(-1)
    , mWriteCursor(nullptr)
    , mWriteLimit(nullptr)
    , mStatus(NS_OK)
    , mInited(false)
{
}

nsPipe::~nsPipe()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsPipe, nsIPipe)

NS_IMETHODIMP
nsPipe::Init(bool nonBlockingIn,
             bool nonBlockingOut,
             PRUint32 segmentSize,
             PRUint32 segmentCount,
             nsIMemory *segmentAlloc)
{
    mInited = true;

    if (segmentSize == 0)
        segmentSize = DEFAULT_SEGMENT_SIZE;
    if (segmentCount == 0)
        segmentCount = DEFAULT_SEGMENT_COUNT;

    
    PRUint32 maxCount = PRUint32(-1) / segmentSize;
    if (segmentCount > maxCount)
        segmentCount = maxCount;

    nsresult rv = mBuffer.Init(segmentSize, segmentSize * segmentCount, segmentAlloc);
    if (NS_FAILED(rv))
        return rv;

    mInput.SetNonBlocking(nonBlockingIn);
    mOutput.SetNonBlocking(nonBlockingOut);
    return NS_OK;
}

NS_IMETHODIMP
nsPipe::GetInputStream(nsIAsyncInputStream **aInputStream)
{
    NS_ADDREF(*aInputStream = &mInput);
    return NS_OK;
}

NS_IMETHODIMP
nsPipe::GetOutputStream(nsIAsyncOutputStream **aOutputStream)
{
    NS_ENSURE_TRUE(mInited, NS_ERROR_NOT_INITIALIZED);
    NS_ADDREF(*aOutputStream = &mOutput);
    return NS_OK;
}

void
nsPipe::PeekSegment(PRUint32 index, char *&cursor, char *&limit)
{
    if (index == 0) {
        NS_ASSERTION(!mReadCursor || mBuffer.GetSegmentCount(), "unexpected state");
        cursor = mReadCursor;
        limit = mReadLimit;
    }
    else {
        PRUint32 numSegments = mBuffer.GetSegmentCount();
        if (index >= numSegments)
            cursor = limit = nullptr;
        else {
            cursor = mBuffer.GetSegment(index);
            if (mWriteSegment == (PRInt32) index)
                limit = mWriteCursor;
            else
                limit = cursor + mBuffer.GetSegmentSize();
        }
    }
}

nsresult
nsPipe::GetReadSegment(const char *&segment, PRUint32 &segmentLen)
{
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    if (mReadCursor == mReadLimit)
        return NS_FAILED(mStatus) ? mStatus : NS_BASE_STREAM_WOULD_BLOCK;

    segment    = mReadCursor;
    segmentLen = mReadLimit - mReadCursor;
    return NS_OK;
}

void
nsPipe::AdvanceReadCursor(PRUint32 bytesRead)
{
    NS_ASSERTION(bytesRead, "don't call if no bytes read");

    nsPipeEvents events;
    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        LOG(("III advancing read cursor by %u\n", bytesRead));
        NS_ASSERTION(bytesRead <= mBuffer.GetSegmentSize(), "read too much");

        mReadCursor += bytesRead;
        NS_ASSERTION(mReadCursor <= mReadLimit, "read cursor exceeds limit");

        mInput.ReduceAvailable(bytesRead);

        if (mReadCursor == mReadLimit) {
            
            

            
            
            if (mWriteSegment == 0 && mWriteLimit > mWriteCursor) {
                NS_ASSERTION(mReadLimit == mWriteCursor, "unexpected state");
                return;
            }

            
            --mWriteSegment;

            
            mBuffer.DeleteFirstSegment();
            LOG(("III deleting first segment\n"));

            if (mWriteSegment == -1) {
                
                mReadCursor = nullptr;
                mReadLimit = nullptr;
                mWriteCursor = nullptr;
                mWriteLimit = nullptr;
            }
            else {
                
                mReadCursor = mBuffer.GetSegment(0);
                if (mWriteSegment == 0)
                    mReadLimit = mWriteCursor;
                else
                    mReadLimit = mReadCursor + mBuffer.GetSegmentSize();
            }

            
            
            if (mOutput.OnOutputWritable(events))
                mon.Notify();
        }
    }
}

nsresult
nsPipe::GetWriteSegment(char *&segment, PRUint32 &segmentLen)
{
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    if (NS_FAILED(mStatus))
        return mStatus;

    
    if (mWriteCursor == mWriteLimit) {
        char *seg = mBuffer.AppendNewSegment();
        
        if (seg == nullptr)
            return NS_BASE_STREAM_WOULD_BLOCK;
        LOG(("OOO appended new segment\n"));
        mWriteCursor = seg;
        mWriteLimit = mWriteCursor + mBuffer.GetSegmentSize();
        ++mWriteSegment;
    }

    
    if (mReadCursor == nullptr) {
        NS_ASSERTION(mWriteSegment == 0, "unexpected null read cursor");
        mReadCursor = mReadLimit = mWriteCursor;
    }

    
    
    if (mReadCursor == mWriteCursor && mWriteSegment == 0) {
        char *head = mBuffer.GetSegment(0);
        LOG(("OOO rolling back write cursor %u bytes\n", mWriteCursor - head));
        mWriteCursor = mReadCursor = mReadLimit = head;
    }

    segment    = mWriteCursor;
    segmentLen = mWriteLimit - mWriteCursor;
    return NS_OK;
}

void
nsPipe::AdvanceWriteCursor(PRUint32 bytesWritten)
{
    NS_ASSERTION(bytesWritten, "don't call if no bytes written");

    nsPipeEvents events;
    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        LOG(("OOO advancing write cursor by %u\n", bytesWritten));

        char *newWriteCursor = mWriteCursor + bytesWritten;
        NS_ASSERTION(newWriteCursor <= mWriteLimit, "write cursor exceeds limit");

        
        if (mWriteSegment == 0 && mReadLimit == mWriteCursor)
            mReadLimit = newWriteCursor;

        mWriteCursor = newWriteCursor;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        NS_ASSERTION(mReadCursor != mWriteCursor ||
                     (mBuffer.GetSegment(0) == mReadCursor &&
                      mWriteCursor == mWriteLimit),
                     "read cursor is bad");

        
        if (mWriteCursor == mWriteLimit) {
            if (mBuffer.GetSize() >= mBuffer.GetMaxSize())
                mOutput.SetWritable(false);
        }

        
        if (mInput.OnInputReadable(bytesWritten, events))
            mon.Notify();
    }
}

void
nsPipe::OnPipeException(nsresult reason, bool outputOnly)
{
    LOG(("PPP nsPipe::OnPipeException [reason=%x output-only=%d]\n",
        reason, outputOnly));

    nsPipeEvents events;
    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        
        if (NS_FAILED(mStatus))
            return;

        mStatus = reason;

        
        
        if (outputOnly && !mInput.Available())
            outputOnly = false;

        if (!outputOnly)
            if (mInput.OnInputException(reason, events))
                mon.Notify();

        if (mOutput.OnOutputException(reason, events))
            mon.Notify();
    }
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





NS_IMPL_QUERY_INTERFACE5(nsPipeInputStream,
                         nsIInputStream,
                         nsIAsyncInputStream,
                         nsISeekableStream,
                         nsISearchableInputStream,
                         nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER4(nsPipeInputStream,
                             nsIInputStream,
                             nsIAsyncInputStream,
                             nsISeekableStream,
                             nsISearchableInputStream)

NS_IMPL_THREADSAFE_CI(nsPipeInputStream)

nsresult
nsPipeInputStream::Wait()
{
    NS_ASSERTION(mBlocking, "wait on non-blocking pipe input stream");

    ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

    while (NS_SUCCEEDED(mPipe->mStatus) && (mAvailable == 0)) {
        LOG(("III pipe input: waiting for data\n"));

        mBlocked = true;
        mon.Wait();
        mBlocked = false;

        LOG(("III pipe input: woke up [pipe-status=%x available=%u]\n",
            mPipe->mStatus, mAvailable));
    }

    return mPipe->mStatus == NS_BASE_STREAM_CLOSED ? NS_OK : mPipe->mStatus;
}

bool
nsPipeInputStream::OnInputReadable(PRUint32 bytesWritten, nsPipeEvents &events)
{
    bool result = false;

    mAvailable += bytesWritten;

    if (mCallback && !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
        events.NotifyInputReady(this, mCallback);
        mCallback = 0;
        mCallbackFlags = 0;
    }
    else if (mBlocked)
        result = true;

    return result;
}

bool
nsPipeInputStream::OnInputException(nsresult reason, nsPipeEvents &events)
{
    LOG(("nsPipeInputStream::OnInputException [this=%x reason=%x]\n",
        this, reason));

    bool result = false;

    NS_ASSERTION(NS_FAILED(reason), "huh? successful exception");

    
    mAvailable = 0;

    if (mCallback) {
        events.NotifyInputReady(this, mCallback);
        mCallback = 0;
        mCallbackFlags = 0;
    }
    else if (mBlocked)
        result = true;

    return result;
}

NS_IMETHODIMP_(nsrefcnt)
nsPipeInputStream::AddRef(void)
{
    NS_AtomicIncrementRefcnt(mReaderRefCnt);
    return mPipe->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
nsPipeInputStream::Release(void)
{
    if (NS_AtomicDecrementRefcnt(mReaderRefCnt) == 0)
        Close();
    return mPipe->Release();
}

NS_IMETHODIMP
nsPipeInputStream::CloseWithStatus(nsresult reason)
{
    LOG(("III CloseWithStatus [this=%x reason=%x]\n", this, reason));

    if (NS_SUCCEEDED(reason))
        reason = NS_BASE_STREAM_CLOSED;

    mPipe->OnPipeException(reason);
    return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::Close()
{
    return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsPipeInputStream::Available(PRUint64 *result)
{
    
    ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

    
    if (!mAvailable && NS_FAILED(mPipe->mStatus))
        return mPipe->mStatus;

    *result = (PRUint64)mAvailable;
    return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::ReadSegments(nsWriteSegmentFun writer, 
                                void *closure,  
                                PRUint32 count,
                                PRUint32 *readCount)
{
    LOG(("III ReadSegments [this=%x count=%u]\n", this, count));

    nsresult rv = NS_OK;

    const char *segment;
    PRUint32 segmentLen;

    *readCount = 0;
    while (count) {
        rv = mPipe->GetReadSegment(segment, segmentLen);
        if (NS_FAILED(rv)) {
            
            if (*readCount > 0) {
                rv = NS_OK;
                break;
            }
            if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
                
                if (!mBlocking)
                    break;
                
                rv = Wait();
                if (NS_SUCCEEDED(rv))
                    continue;
            }
            
            if (rv == NS_BASE_STREAM_CLOSED) {
                rv = NS_OK;
                break;
            }
            mPipe->OnPipeException(rv);
            break;
        }

        
        if (segmentLen > count)
            segmentLen = count;

        PRUint32 writeCount, originalLen = segmentLen;
        while (segmentLen) {
            writeCount = 0;

            rv = writer(this, closure, segment, *readCount, segmentLen, &writeCount);

            if (NS_FAILED(rv) || writeCount == 0) {
                count = 0;
                
                
                rv = NS_OK;
                break;
            }

            NS_ASSERTION(writeCount <= segmentLen, "wrote more than expected");
            segment += writeCount;
            segmentLen -= writeCount;
            count -= writeCount;
            *readCount += writeCount;
            mLogicalOffset += writeCount;
        }

        if (segmentLen < originalLen)
            mPipe->AdvanceReadCursor(originalLen - segmentLen);
    }

    return rv;
}

NS_IMETHODIMP
nsPipeInputStream::Read(char* toBuf, PRUint32 bufLen, PRUint32 *readCount)
{
    return ReadSegments(NS_CopySegmentToBuffer, toBuf, bufLen, readCount);
}

NS_IMETHODIMP
nsPipeInputStream::IsNonBlocking(bool *aNonBlocking)
{
    *aNonBlocking = !mBlocking;
    return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::AsyncWait(nsIInputStreamCallback *callback,
                             PRUint32 flags,
                             PRUint32 requestedCount,
                             nsIEventTarget *target)
{
    LOG(("III AsyncWait [this=%x]\n", this));

    nsPipeEvents pipeEvents;
    {
        ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

        
        mCallback = 0;
        mCallbackFlags = 0;

        if (!callback)
            return NS_OK;

        nsCOMPtr<nsIInputStreamCallback> proxy;
        if (target) {
            nsresult rv = NS_NewInputStreamReadyEvent(getter_AddRefs(proxy),
                                                      callback, target);
            if (NS_FAILED(rv)) return rv;
            callback = proxy;
        }

        if (NS_FAILED(mPipe->mStatus) ||
                (mAvailable && !(flags & WAIT_CLOSURE_ONLY))) {
            
            pipeEvents.NotifyInputReady(this, callback);
        }
        else {
            
            mCallback = callback;
            mCallbackFlags = flags;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::Seek(PRInt32 whence, PRInt64 offset)
{
    NS_NOTREACHED("nsPipeInputStream::Seek");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPipeInputStream::Tell(PRInt64 *offset)
{
    ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

    
    if (!mAvailable && NS_FAILED(mPipe->mStatus))
        return mPipe->mStatus;

    *offset = mLogicalOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsPipeInputStream::SetEOF()
{
    NS_NOTREACHED("nsPipeInputStream::SetEOF");
    return NS_ERROR_NOT_IMPLEMENTED;
}

#define COMPARE(s1, s2, i)                                                 \
    (ignoreCase                                                            \
     ? nsCRT::strncasecmp((const char *)s1, (const char *)s2, (PRUint32)i) \
     : nsCRT::strncmp((const char *)s1, (const char *)s2, (PRUint32)i))

NS_IMETHODIMP
nsPipeInputStream::Search(const char *forString, 
                          bool ignoreCase,
                          bool *found,
                          PRUint32 *offsetSearchedTo)
{
    LOG(("III Search [for=%s ic=%u]\n", forString, ignoreCase));

    ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

    char *cursor1, *limit1;
    PRUint32 index = 0, offset = 0;
    PRUint32 strLen = strlen(forString);

    mPipe->PeekSegment(0, cursor1, limit1);
    if (cursor1 == limit1) {
        *found = false;
        *offsetSearchedTo = 0;
        LOG(("  result [found=%u offset=%u]\n", *found, *offsetSearchedTo));
        return NS_OK;
    }

    while (true) {
        PRUint32 i, len1 = limit1 - cursor1;

        
        for (i = 0; i < len1 - strLen + 1; i++) {
            if (COMPARE(&cursor1[i], forString, strLen) == 0) {
                *found = true;
                *offsetSearchedTo = offset + i;
                LOG(("  result [found=%u offset=%u]\n", *found, *offsetSearchedTo));
                return NS_OK;
            }
        }

        
        char *cursor2, *limit2;
        PRUint32 len2;

        index++;
        offset += len1;

        mPipe->PeekSegment(index, cursor2, limit2);
        if (cursor2 == limit2) {
            *found = false;
            *offsetSearchedTo = offset - strLen + 1;
            LOG(("  result [found=%u offset=%u]\n", *found, *offsetSearchedTo));
            return NS_OK;
        }
        len2 = limit2 - cursor2;

        
        PRUint32 lim = NS_MIN(strLen, len2 + 1);
        for (i = 0; i < lim; ++i) {
            PRUint32 strPart1Len = strLen - i - 1;
            PRUint32 strPart2Len = strLen - strPart1Len;
            const char* strPart2 = &forString[strLen - strPart2Len];
            PRUint32 bufSeg1Offset = len1 - strPart1Len;
            if (COMPARE(&cursor1[bufSeg1Offset], forString, strPart1Len) == 0 &&
                COMPARE(cursor2, strPart2, strPart2Len) == 0) {
                *found = true;
                *offsetSearchedTo = offset - strPart1Len;
                LOG(("  result [found=%u offset=%u]\n", *found, *offsetSearchedTo));
                return NS_OK;
            }
        }

        
        cursor1 = cursor2;
        limit1 = limit2;
    }

    NS_NOTREACHED("can't get here");
    return NS_ERROR_UNEXPECTED;    
}





NS_IMPL_QUERY_INTERFACE3(nsPipeOutputStream,
                         nsIOutputStream,
                         nsIAsyncOutputStream,
                         nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(nsPipeOutputStream,
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

bool
nsPipeOutputStream::OnOutputWritable(nsPipeEvents &events)
{
    bool result = false;

    mWritable = true;

    if (mCallback && !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
        events.NotifyOutputReady(this, mCallback);
        mCallback = 0;
        mCallbackFlags = 0;
    }
    else if (mBlocked)
        result = true;

    return result;
}

bool
nsPipeOutputStream::OnOutputException(nsresult reason, nsPipeEvents &events)
{
    LOG(("nsPipeOutputStream::OnOutputException [this=%x reason=%x]\n",
        this, reason));

    bool result = false;

    NS_ASSERTION(NS_FAILED(reason), "huh? successful exception");
    mWritable = false;

    if (mCallback) {
        events.NotifyOutputReady(this, mCallback);
        mCallback = 0;
        mCallbackFlags = 0;
    }
    else if (mBlocked)
        result = true;

    return result;
}


NS_IMETHODIMP_(nsrefcnt)
nsPipeOutputStream::AddRef()
{
    NS_AtomicIncrementRefcnt(mWriterRefCnt);
    return mPipe->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
nsPipeOutputStream::Release()
{
    if (NS_AtomicDecrementRefcnt(mWriterRefCnt) == 0)
        Close();
    return mPipe->Release();
}

NS_IMETHODIMP
nsPipeOutputStream::CloseWithStatus(nsresult reason)
{
    LOG(("OOO CloseWithStatus [this=%x reason=%x]\n", this, reason));

    if (NS_SUCCEEDED(reason))
        reason = NS_BASE_STREAM_CLOSED;

    
    mPipe->OnPipeException(reason, true);
    return NS_OK;
}

NS_IMETHODIMP
nsPipeOutputStream::Close()
{
    return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsPipeOutputStream::WriteSegments(nsReadSegmentFun reader,
                                  void* closure,
                                  PRUint32 count,
                                  PRUint32 *writeCount)
{
    LOG(("OOO WriteSegments [this=%x count=%u]\n", this, count));

    nsresult rv = NS_OK;

    char *segment;
    PRUint32 segmentLen;

    *writeCount = 0;
    while (count) {
        rv = mPipe->GetWriteSegment(segment, segmentLen);
        if (NS_FAILED(rv)) {
            if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
                
                if (!mBlocking) {
                    
                    if (*writeCount > 0)
                        rv = NS_OK;
                    break;
                }
                
                rv = Wait();
                if (NS_SUCCEEDED(rv))
                    continue;
            }
            mPipe->OnPipeException(rv);
            break;
        }

        
        if (segmentLen > count)
            segmentLen = count;

        PRUint32 readCount, originalLen = segmentLen;
        while (segmentLen) {
            readCount = 0;

            rv = reader(this, closure, segment, *writeCount, segmentLen, &readCount);

            if (NS_FAILED(rv) || readCount == 0) {
                count = 0;
                
                
                rv = NS_OK;
                break;
            }

            NS_ASSERTION(readCount <= segmentLen, "read more than expected");
            segment += readCount;
            segmentLen -= readCount;
            count -= readCount;
            *writeCount += readCount;
            mLogicalOffset += readCount;
        }

        if (segmentLen < originalLen)
            mPipe->AdvanceWriteCursor(originalLen - segmentLen);
    }

    return rv;
}

static NS_METHOD
nsReadFromRawBuffer(nsIOutputStream* outStr,
                    void* closure,
                    char* toRawSegment,
                    PRUint32 offset,
                    PRUint32 count,
                    PRUint32 *readCount)
{
    const char* fromBuf = (const char*)closure;
    memcpy(toRawSegment, &fromBuf[offset], count);
    *readCount = count;
    return NS_OK;
}

NS_IMETHODIMP
nsPipeOutputStream::Write(const char* fromBuf,
                          PRUint32 bufLen, 
                          PRUint32 *writeCount)
{
    return WriteSegments(nsReadFromRawBuffer, (void*)fromBuf, bufLen, writeCount);
}

NS_IMETHODIMP
nsPipeOutputStream::Flush(void)
{
    
    return NS_OK;
}

static NS_METHOD
nsReadFromInputStream(nsIOutputStream* outStr,
                      void* closure,
                      char* toRawSegment, 
                      PRUint32 offset,
                      PRUint32 count,
                      PRUint32 *readCount)
{
    nsIInputStream* fromStream = (nsIInputStream*)closure;
    return fromStream->Read(toRawSegment, count, readCount);
}

NS_IMETHODIMP
nsPipeOutputStream::WriteFrom(nsIInputStream* fromStream,
                              PRUint32 count,
                              PRUint32 *writeCount)
{
    return WriteSegments(nsReadFromInputStream, fromStream, count, writeCount);
}

NS_IMETHODIMP
nsPipeOutputStream::IsNonBlocking(bool *aNonBlocking)
{
    *aNonBlocking = !mBlocking;
    return NS_OK;
}

NS_IMETHODIMP
nsPipeOutputStream::AsyncWait(nsIOutputStreamCallback *callback,
                              PRUint32 flags,
                              PRUint32 requestedCount,
                              nsIEventTarget *target)
{
    LOG(("OOO AsyncWait [this=%x]\n", this));

    nsPipeEvents pipeEvents;
    {
        ReentrantMonitorAutoEnter mon(mPipe->mReentrantMonitor);

        
        mCallback = 0;
        mCallbackFlags = 0;

        if (!callback)
            return NS_OK;

        nsCOMPtr<nsIOutputStreamCallback> proxy;
        if (target) {
            nsresult rv = NS_NewOutputStreamReadyEvent(getter_AddRefs(proxy),
                                                       callback, target);
            if (NS_FAILED(rv)) return rv;
            callback = proxy;
        }

        if (NS_FAILED(mPipe->mStatus) ||
                (mWritable && !(flags & WAIT_CLOSURE_ONLY))) {
            
            pipeEvents.NotifyOutputReady(this, callback);
        }
        else {
            
            mCallback = callback;
            mCallbackFlags = flags;
        }
    }
    return NS_OK;
}



nsresult
NS_NewPipe(nsIInputStream **pipeIn,
           nsIOutputStream **pipeOut,
           PRUint32 segmentSize,
           PRUint32 maxSize,
           bool nonBlockingInput,
           bool nonBlockingOutput,
           nsIMemory *segmentAlloc)
{
    if (segmentSize == 0)
        segmentSize = DEFAULT_SEGMENT_SIZE;

    
    PRUint32 segmentCount;
    if (maxSize == PR_UINT32_MAX)
        segmentCount = PR_UINT32_MAX;
    else
        segmentCount = maxSize / segmentSize;

    nsIAsyncInputStream *in;
    nsIAsyncOutputStream *out;
    nsresult rv = NS_NewPipe2(&in, &out, nonBlockingInput, nonBlockingOutput,
                              segmentSize, segmentCount, segmentAlloc);
    if (NS_FAILED(rv)) return rv;

    *pipeIn = in;
    *pipeOut = out;
    return NS_OK;
}

nsresult
NS_NewPipe2(nsIAsyncInputStream **pipeIn,
            nsIAsyncOutputStream **pipeOut,
            bool nonBlockingInput,
            bool nonBlockingOutput,
            PRUint32 segmentSize,
            PRUint32 segmentCount,
            nsIMemory *segmentAlloc)
{
    nsresult rv;

    nsPipe *pipe = new nsPipe();
    if (!pipe)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = pipe->Init(nonBlockingInput,
                    nonBlockingOutput,
                    segmentSize,
                    segmentCount,
                    segmentAlloc);
    if (NS_FAILED(rv)) {
        NS_ADDREF(pipe);
        NS_RELEASE(pipe);
        return rv;
    }

    pipe->GetInputStream(pipeIn);
    pipe->GetOutputStream(pipeOut);
    return NS_OK;
}

nsresult
nsPipeConstructor(nsISupports *outer, REFNSIID iid, void **result)
{
    if (outer)
        return NS_ERROR_NO_AGGREGATION;
    nsPipe *pipe = new nsPipe();
    if (!pipe)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(pipe);
    nsresult rv = pipe->QueryInterface(iid, result);
    NS_RELEASE(pipe);
    return rv;
}


