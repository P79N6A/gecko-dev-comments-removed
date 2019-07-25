





































#include "mozilla/Mutex.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"
#include "nsIPipe.h"
#include "nsIEventTarget.h"
#include "nsIRunnable.h"
#include "nsISafeOutputStream.h"
#include "nsString.h"

using namespace mozilla;



class nsInputStreamReadyEvent : public nsIRunnable
                              , public nsIInputStreamCallback
{
public:
    NS_DECL_ISUPPORTS

    nsInputStreamReadyEvent(nsIInputStreamCallback *callback,
                            nsIEventTarget *target)
        : mCallback(callback)
        , mTarget(target)
    {
    }

private:
    ~nsInputStreamReadyEvent()
    {
        if (!mCallback)
            return;
        
        
        
        
        
        
        
        bool val;
        nsresult rv = mTarget->IsOnCurrentThread(&val);
        if (NS_FAILED(rv) || !val) {
            nsCOMPtr<nsIInputStreamCallback> event;
            NS_NewInputStreamReadyEvent(getter_AddRefs(event), mCallback,
                                        mTarget);
            mCallback = 0;
            if (event) {
                rv = event->OnInputStreamReady(nsnull);
                if (NS_FAILED(rv)) {
                    NS_NOTREACHED("leaking stream event");
                    nsISupports *sup = event;
                    NS_ADDREF(sup);
                }
            }
        }
    }

public:
    NS_IMETHOD OnInputStreamReady(nsIAsyncInputStream *stream)
    {
        mStream = stream;

        nsresult rv =
            mTarget->Dispatch(this, NS_DISPATCH_NORMAL);
        if (NS_FAILED(rv)) {
            NS_WARNING("Dispatch failed");
            return NS_ERROR_FAILURE;
        }

        return NS_OK;
    }

    NS_IMETHOD Run()
    {
        if (mCallback) {
            if (mStream)
                mCallback->OnInputStreamReady(mStream);
            mCallback = nsnull;
        }
        return NS_OK;
    }

private:
    nsCOMPtr<nsIAsyncInputStream>    mStream;
    nsCOMPtr<nsIInputStreamCallback> mCallback;
    nsCOMPtr<nsIEventTarget>         mTarget;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsInputStreamReadyEvent, nsIRunnable,
                              nsIInputStreamCallback)



class nsOutputStreamReadyEvent : public nsIRunnable
                               , public nsIOutputStreamCallback
{
public:
    NS_DECL_ISUPPORTS

    nsOutputStreamReadyEvent(nsIOutputStreamCallback *callback,
                             nsIEventTarget *target)
        : mCallback(callback)
        , mTarget(target)
    {
    }

private:
    ~nsOutputStreamReadyEvent()
    {
        if (!mCallback)
            return;
        
        
        
        
        
        
        
        bool val;
        nsresult rv = mTarget->IsOnCurrentThread(&val);
        if (NS_FAILED(rv) || !val) {
            nsCOMPtr<nsIOutputStreamCallback> event;
            NS_NewOutputStreamReadyEvent(getter_AddRefs(event), mCallback,
                                         mTarget);
            mCallback = 0;
            if (event) {
                rv = event->OnOutputStreamReady(nsnull);
                if (NS_FAILED(rv)) {
                    NS_NOTREACHED("leaking stream event");
                    nsISupports *sup = event;
                    NS_ADDREF(sup);
                }
            }
        }
    }

public:
    NS_IMETHOD OnOutputStreamReady(nsIAsyncOutputStream *stream)
    {
        mStream = stream;

        nsresult rv =
            mTarget->Dispatch(this, NS_DISPATCH_NORMAL);
        if (NS_FAILED(rv)) {
            NS_WARNING("PostEvent failed");
            return NS_ERROR_FAILURE;
        }

        return NS_OK;
    }

    NS_IMETHOD Run()
    {
        if (mCallback) {
            if (mStream)
                mCallback->OnOutputStreamReady(mStream);
            mCallback = nsnull;
        }
        return NS_OK;
    }

private:
    nsCOMPtr<nsIAsyncOutputStream>    mStream;
    nsCOMPtr<nsIOutputStreamCallback> mCallback;
    nsCOMPtr<nsIEventTarget>          mTarget;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsOutputStreamReadyEvent, nsIRunnable,
                              nsIOutputStreamCallback)



nsresult
NS_NewInputStreamReadyEvent(nsIInputStreamCallback **event,
                            nsIInputStreamCallback *callback,
                            nsIEventTarget *target)
{
    NS_ASSERTION(callback, "null callback");
    NS_ASSERTION(target, "null target");
    nsInputStreamReadyEvent *ev = new nsInputStreamReadyEvent(callback, target);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*event = ev);
    return NS_OK;
}

nsresult
NS_NewOutputStreamReadyEvent(nsIOutputStreamCallback **event,
                             nsIOutputStreamCallback *callback,
                             nsIEventTarget *target)
{
    NS_ASSERTION(callback, "null callback");
    NS_ASSERTION(target, "null target");
    nsOutputStreamReadyEvent *ev = new nsOutputStreamReadyEvent(callback, target);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*event = ev);
    return NS_OK;
}





class nsAStreamCopier : public nsIInputStreamCallback
                      , public nsIOutputStreamCallback
                      , public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS

    nsAStreamCopier()
        : mLock("nsAStreamCopier.mLock")
        , mCallback(nsnull)
        , mClosure(nsnull)
        , mChunkSize(0)
        , mEventInProcess(false)
        , mEventIsPending(false)
        , mCloseSource(true)
        , mCloseSink(true)
        , mCanceled(false)
        , mCancelStatus(NS_OK)
    {
    }

    
    virtual ~nsAStreamCopier()
    {
    }

    
    nsresult Start(nsIInputStream *source,
                   nsIOutputStream *sink,
                   nsIEventTarget *target,
                   nsAsyncCopyCallbackFun callback,
                   void *closure,
                   PRUint32 chunksize,
                   bool closeSource,
                   bool closeSink)
    {
        mSource = source;
        mSink = sink;
        mTarget = target;
        mCallback = callback;
        mClosure = closure;
        mChunkSize = chunksize;
        mCloseSource = closeSource;
        mCloseSink = closeSink;

        mAsyncSource = do_QueryInterface(mSource);
        mAsyncSink = do_QueryInterface(mSink);

        return PostContinuationEvent();
    }

    
    
    virtual PRUint32 DoCopy(nsresult *sourceCondition, nsresult *sinkCondition) = 0;

    void Process()
    {
        if (!mSource || !mSink)
            return;

        nsresult sourceCondition, sinkCondition;
        nsresult cancelStatus;
        bool canceled;
        {
            MutexAutoLock lock(mLock);
            canceled = mCanceled;
            cancelStatus = mCancelStatus;
        }

        
        
        for (;;) {
            
            
            
            bool copyFailed = false;
            if (!canceled) {
                PRUint32 n = DoCopy(&sourceCondition, &sinkCondition);
                copyFailed = NS_FAILED(sourceCondition) ||
                             NS_FAILED(sinkCondition) || n == 0;

                MutexAutoLock lock(mLock);
                canceled = mCanceled;
                cancelStatus = mCancelStatus;
            }
            if (copyFailed && !canceled) {
                if (sourceCondition == NS_BASE_STREAM_WOULD_BLOCK && mAsyncSource) {
                    
                    
                    mAsyncSource->AsyncWait(this, 0, 0, nsnull);

                    if (mAsyncSink)
                        mAsyncSink->AsyncWait(this,
                                              nsIAsyncOutputStream::WAIT_CLOSURE_ONLY,
                                              0, nsnull);
                    break;
                }
                else if (sinkCondition == NS_BASE_STREAM_WOULD_BLOCK && mAsyncSink) {
                    
                    
                    
                    mAsyncSink->AsyncWait(this, 0, 0, nsnull);

                    if (mAsyncSource)
                        mAsyncSource->AsyncWait(this,
                                                nsIAsyncInputStream::WAIT_CLOSURE_ONLY,
                                                0, nsnull);
                    break;
                }
            }
            if (copyFailed || canceled) {
                if (mCloseSource) {
                    
                    if (mAsyncSource)
                        mAsyncSource->CloseWithStatus(canceled ? cancelStatus :
                                                                 sinkCondition);
                    else
                        mSource->Close();
                }
                mAsyncSource = nsnull;
                mSource = nsnull;

                if (mCloseSink) {
                    
                    if (mAsyncSink)
                        mAsyncSink->CloseWithStatus(canceled ? cancelStatus :
                                                               sourceCondition);
                    else {
                        
                        
                        
                        nsCOMPtr<nsISafeOutputStream> sostream =
                            do_QueryInterface(mSink);
                        if (sostream && NS_SUCCEEDED(sourceCondition) &&
                            NS_SUCCEEDED(sinkCondition))
                            sostream->Finish();
                        else
                            mSink->Close();
                    }
                }
                mAsyncSink = nsnull;
                mSink = nsnull;

                
                if (mCallback) {
                    nsresult status;
                    if (!canceled) {
                        status = sourceCondition;
                        if (NS_SUCCEEDED(status))
                            status = sinkCondition;
                        if (status == NS_BASE_STREAM_CLOSED)
                            status = NS_OK;
                    } else {
                        status = cancelStatus; 
                    }
                    mCallback(mClosure, status);
                }
                break;
            }
        }
    }

    nsresult Cancel(nsresult aReason)
    {
        MutexAutoLock lock(mLock);
        if (mCanceled)
            return NS_ERROR_FAILURE;

        if (NS_SUCCEEDED(aReason)) {
            NS_WARNING("cancel with non-failure status code");
            aReason = NS_BASE_STREAM_CLOSED;
        }

        mCanceled = true;
        mCancelStatus = aReason;
        return NS_OK;
    }

    NS_IMETHOD OnInputStreamReady(nsIAsyncInputStream *source)
    {
        PostContinuationEvent();
        return NS_OK;
    }

    NS_IMETHOD OnOutputStreamReady(nsIAsyncOutputStream *sink)
    {
        PostContinuationEvent();
        return NS_OK;
    }

    
    NS_IMETHOD Run()
    {
        Process();

        
        MutexAutoLock lock(mLock);
        mEventInProcess = false;
        if (mEventIsPending) {
            mEventIsPending = false;
            PostContinuationEvent_Locked();
        }

        return NS_OK;
    }

    nsresult PostContinuationEvent()
    {
        
        
        
        
        
        

        MutexAutoLock lock(mLock);
        return PostContinuationEvent_Locked();
    }

    nsresult PostContinuationEvent_Locked()
    {
        nsresult rv = NS_OK;
        if (mEventInProcess)
            mEventIsPending = true;
        else {
            rv = mTarget->Dispatch(this, NS_DISPATCH_NORMAL);
            if (NS_SUCCEEDED(rv))
                mEventInProcess = true;
            else
                NS_WARNING("unable to post continuation event");
        }
        return rv;
    }

protected:
    nsCOMPtr<nsIInputStream>       mSource;
    nsCOMPtr<nsIOutputStream>      mSink;
    nsCOMPtr<nsIAsyncInputStream>  mAsyncSource;
    nsCOMPtr<nsIAsyncOutputStream> mAsyncSink;
    nsCOMPtr<nsIEventTarget>       mTarget;
    Mutex                          mLock;
    nsAsyncCopyCallbackFun         mCallback;
    void                          *mClosure;
    PRUint32                       mChunkSize;
    bool                           mEventInProcess;
    bool                           mEventIsPending;
    bool                           mCloseSource;
    bool                           mCloseSink;
    bool                           mCanceled;
    nsresult                       mCancelStatus;
};

NS_IMPL_THREADSAFE_ISUPPORTS3(nsAStreamCopier,
                              nsIInputStreamCallback,
                              nsIOutputStreamCallback,
                              nsIRunnable)

class nsStreamCopierIB : public nsAStreamCopier
{
public:
    nsStreamCopierIB() : nsAStreamCopier() {}
    virtual ~nsStreamCopierIB() {}

    struct ReadSegmentsState {
        nsIOutputStream *mSink;
        nsresult         mSinkCondition;
    };

    static NS_METHOD ConsumeInputBuffer(nsIInputStream *inStr,
                                        void *closure,
                                        const char *buffer,
                                        PRUint32 offset,
                                        PRUint32 count,
                                        PRUint32 *countWritten)
    {
        ReadSegmentsState *state = (ReadSegmentsState *) closure;

        nsresult rv = state->mSink->Write(buffer, count, countWritten);
        if (NS_FAILED(rv))
            state->mSinkCondition = rv;
        else if (*countWritten == 0)
            state->mSinkCondition = NS_BASE_STREAM_CLOSED;

        return state->mSinkCondition;
    }

    PRUint32 DoCopy(nsresult *sourceCondition, nsresult *sinkCondition)
    {
        ReadSegmentsState state;
        state.mSink = mSink;
        state.mSinkCondition = NS_OK;

        PRUint32 n;
        *sourceCondition =
            mSource->ReadSegments(ConsumeInputBuffer, &state, mChunkSize, &n);
        *sinkCondition = state.mSinkCondition;
        return n;
    }
};

class nsStreamCopierOB : public nsAStreamCopier
{
public:
    nsStreamCopierOB() : nsAStreamCopier() {}
    virtual ~nsStreamCopierOB() {}

    struct WriteSegmentsState {
        nsIInputStream *mSource;
        nsresult        mSourceCondition;
    };

    static NS_METHOD FillOutputBuffer(nsIOutputStream *outStr,
                                      void *closure,
                                      char *buffer,
                                      PRUint32 offset,
                                      PRUint32 count,
                                      PRUint32 *countRead)
    {
        WriteSegmentsState *state = (WriteSegmentsState *) closure;

        nsresult rv = state->mSource->Read(buffer, count, countRead);
        if (NS_FAILED(rv))
            state->mSourceCondition = rv;
        else if (*countRead == 0)
            state->mSourceCondition = NS_BASE_STREAM_CLOSED;

        return state->mSourceCondition;
    }

    PRUint32 DoCopy(nsresult *sourceCondition, nsresult *sinkCondition)
    {
        WriteSegmentsState state;
        state.mSource = mSource;
        state.mSourceCondition = NS_OK;

        PRUint32 n;
        *sinkCondition =
            mSink->WriteSegments(FillOutputBuffer, &state, mChunkSize, &n);
        *sourceCondition = state.mSourceCondition;
        return n;
    }
};



nsresult
NS_AsyncCopy(nsIInputStream         *source,
             nsIOutputStream        *sink,
             nsIEventTarget         *target,
             nsAsyncCopyMode         mode,
             PRUint32                chunkSize,
             nsAsyncCopyCallbackFun  callback,
             void                   *closure,
             bool                    closeSource,
             bool                    closeSink,
             nsISupports           **aCopierCtx)
{
    NS_ASSERTION(target, "non-null target required");

    nsresult rv;
    nsAStreamCopier *copier;

    if (mode == NS_ASYNCCOPY_VIA_READSEGMENTS)
        copier = new nsStreamCopierIB();
    else
        copier = new nsStreamCopierOB();

    if (!copier)
        return NS_ERROR_OUT_OF_MEMORY;

    
    NS_ADDREF(copier);
    rv = copier->Start(source, sink, target, callback, closure, chunkSize,
                       closeSource, closeSink);

    if (aCopierCtx) {
        *aCopierCtx = static_cast<nsISupports*>(
                      static_cast<nsIRunnable*>(copier));
        NS_ADDREF(*aCopierCtx);
    }
    NS_RELEASE(copier);

    return rv;
}



nsresult
NS_CancelAsyncCopy(nsISupports *aCopierCtx, nsresult aReason)
{
  nsAStreamCopier *copier = static_cast<nsAStreamCopier *>(
                            static_cast<nsIRunnable *>(aCopierCtx));
  return copier->Cancel(aReason);
}



nsresult
NS_ConsumeStream(nsIInputStream *stream, PRUint32 maxCount, nsACString &result)
{
    nsresult rv = NS_OK;
    result.Truncate();

    while (maxCount) {
        PRUint32 avail;
        rv = stream->Available(&avail);
        if (NS_FAILED(rv)) {
            if (rv == NS_BASE_STREAM_CLOSED)
                rv = NS_OK;
            break;
        }
        if (avail == 0)
            break;
        if (avail > maxCount)
            avail = maxCount;

        
        PRUint32 length = result.Length();
        result.SetLength(length + avail);
        if (result.Length() != (length + avail))
            return NS_ERROR_OUT_OF_MEMORY;
        char *buf = result.BeginWriting() + length;
        
        PRUint32 n;
        rv = stream->Read(buf, avail, &n);
        if (NS_FAILED(rv))
            break;
        if (n != avail)
            result.SetLength(length + n);
        if (n == 0)
            break;
        maxCount -= n;
    }

    return rv;
}



static NS_METHOD
TestInputStream(nsIInputStream *inStr,
                void *closure,
                const char *buffer,
                PRUint32 offset,
                PRUint32 count,
                PRUint32 *countWritten)
{
    bool *result = static_cast<bool *>(closure);
    *result = true;
    return NS_ERROR_ABORT;  
}

bool
NS_InputStreamIsBuffered(nsIInputStream *stream)
{
    bool result = false;
    PRUint32 n;
    nsresult rv = stream->ReadSegments(TestInputStream,
                                       &result, 1, &n);
    return result || NS_SUCCEEDED(rv);
}

static NS_METHOD
TestOutputStream(nsIOutputStream *outStr,
                 void *closure,
                 char *buffer,
                 PRUint32 offset,
                 PRUint32 count,
                 PRUint32 *countRead)
{
    bool *result = static_cast<bool *>(closure);
    *result = true;
    return NS_ERROR_ABORT;  
}

bool
NS_OutputStreamIsBuffered(nsIOutputStream *stream)
{
    bool result = false;
    PRUint32 n;
    stream->WriteSegments(TestOutputStream, &result, 1, &n);
    return result;
}



NS_METHOD
NS_CopySegmentToStream(nsIInputStream *inStr,
                       void *closure,
                       const char *buffer,
                       PRUint32 offset,
                       PRUint32 count,
                       PRUint32 *countWritten)
{
    nsIOutputStream *outStr = static_cast<nsIOutputStream *>(closure);
    *countWritten = 0;
    while (count) {
        PRUint32 n;
        nsresult rv = outStr->Write(buffer, count, &n);
        if (NS_FAILED(rv))
            return rv;
        buffer += n;
        count -= n;
        *countWritten += n;
    }
    return NS_OK;
}

NS_METHOD
NS_CopySegmentToBuffer(nsIInputStream *inStr,
                       void *closure,
                       const char *buffer,
                       PRUint32 offset,
                       PRUint32 count,
                       PRUint32 *countWritten)
{
    char *toBuf = static_cast<char *>(closure);
    memcpy(&toBuf[offset], buffer, count);
    *countWritten = count;
    return NS_OK;
}

NS_METHOD
NS_DiscardSegment(nsIInputStream *inStr,
                  void *closure,
                  const char *buffer,
                  PRUint32 offset,
                  PRUint32 count,
                  PRUint32 *countWritten)
{
    *countWritten = count;
    return NS_OK;
}



NS_METHOD
NS_WriteSegmentThunk(nsIInputStream *inStr,
                     void *closure,
                     const char *buffer,
                     PRUint32 offset,
                     PRUint32 count,
                     PRUint32 *countWritten)
{
    nsWriteSegmentThunk *thunk = static_cast<nsWriteSegmentThunk *>(closure);
    return thunk->mFun(thunk->mStream, thunk->mClosure, buffer, offset, count,
                       countWritten);
}
