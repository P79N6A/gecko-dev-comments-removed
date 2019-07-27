




#include "ipc/IPCMessageUtils.h"

#include "nsBufferedStreams.h"
#include "nsStreamUtils.h"
#include "nsNetCID.h"
#include "nsIClassInfoImpl.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include <algorithm>

#ifdef DEBUG_brendan
# define METERING
#endif

#ifdef METERING
# include <stdio.h>
# define METER(x)       x
# define MAX_BIG_SEEKS  20

static struct {
    uint32_t            mSeeksWithinBuffer;
    uint32_t            mSeeksOutsideBuffer;
    uint32_t            mBufferReadUponSeek;
    uint32_t            mBufferUnreadUponSeek;
    uint32_t            mBytesReadFromBuffer;
    uint32_t            mBigSeekIndex;
    struct {
        int64_t         mOldOffset;
        int64_t         mNewOffset;
    } mBigSeek[MAX_BIG_SEEKS];
} bufstats;
#else
# define METER(x)
#endif

using namespace mozilla::ipc;




nsBufferedStream::nsBufferedStream()
    : mBuffer(nullptr),
      mBufferStartOffset(0),
      mCursor(0), 
      mFillPoint(0),
      mStream(nullptr),
      mBufferDisabled(false),
      mEOF(false),
      mGetBufferCount(0)
{
}

nsBufferedStream::~nsBufferedStream()
{
    Close();
}

NS_IMPL_ISUPPORTS(nsBufferedStream, nsISeekableStream)

nsresult
nsBufferedStream::Init(nsISupports* stream, uint32_t bufferSize)
{
    NS_ASSERTION(stream, "need to supply a stream");
    NS_ASSERTION(mStream == nullptr, "already inited");
    mStream = stream;
    NS_IF_ADDREF(mStream);
    mBufferSize = bufferSize;
    mBufferStartOffset = 0;
    mCursor = 0;
    const mozilla::fallible_t fallible = mozilla::fallible_t();
    mBuffer = new (fallible) char[bufferSize];
    if (mBuffer == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

nsresult
nsBufferedStream::Close()
{
    NS_IF_RELEASE(mStream);
    if (mBuffer) {
        delete[] mBuffer;
        mBuffer = nullptr;
        mBufferSize = 0;
        mBufferStartOffset = 0;
        mCursor = 0;
        mFillPoint = 0;
    }
#ifdef METERING
    {
        static FILE *tfp;
        if (!tfp) {
            tfp = fopen("/tmp/bufstats", "w");
            if (tfp)
                setvbuf(tfp, nullptr, _IOLBF, 0);
        }
        if (tfp) {
            fprintf(tfp, "seeks within buffer:    %u\n",
                    bufstats.mSeeksWithinBuffer);
            fprintf(tfp, "seeks outside buffer:   %u\n",
                    bufstats.mSeeksOutsideBuffer);
            fprintf(tfp, "buffer read on seek:    %u\n",
                    bufstats.mBufferReadUponSeek);
            fprintf(tfp, "buffer unread on seek:  %u\n",
                    bufstats.mBufferUnreadUponSeek);
            fprintf(tfp, "bytes read from buffer: %u\n",
                    bufstats.mBytesReadFromBuffer);
            for (uint32_t i = 0; i < bufstats.mBigSeekIndex; i++) {
                fprintf(tfp, "bigseek[%u] = {old: %u, new: %u}\n",
                        i,
                        bufstats.mBigSeek[i].mOldOffset,
                        bufstats.mBigSeek[i].mNewOffset);
            }
        }
    }
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedStream::Seek(int32_t whence, int64_t offset)
{
    if (mStream == nullptr)
        return NS_BASE_STREAM_CLOSED;
    
    
    
    
    
    nsresult rv;
    nsCOMPtr<nsISeekableStream> ras = do_QueryInterface(mStream, &rv);
    if (NS_FAILED(rv)) return rv;

    int64_t absPos = 0;
    switch (whence) {
      case nsISeekableStream::NS_SEEK_SET:
        absPos = offset;
        break;
      case nsISeekableStream::NS_SEEK_CUR:
        absPos = mBufferStartOffset;
        absPos += mCursor;
        absPos += offset;
        break;
      case nsISeekableStream::NS_SEEK_END:
        absPos = -1;
        break;
      default:
        NS_NOTREACHED("bogus seek whence parameter");
        return NS_ERROR_UNEXPECTED;
    }

    
    
    
    
    
    
    
    uint32_t offsetInBuffer = uint32_t(absPos - mBufferStartOffset);
    if (offsetInBuffer <= mFillPoint && !mEOF) {
        METER(bufstats.mSeeksWithinBuffer++);
        mCursor = offsetInBuffer;
        return NS_OK;
    }

    METER(bufstats.mSeeksOutsideBuffer++);
    METER(bufstats.mBufferReadUponSeek += mCursor);
    METER(bufstats.mBufferUnreadUponSeek += mFillPoint - mCursor);
    rv = Flush();
    if (NS_FAILED(rv)) return rv;

    rv = ras->Seek(whence, offset);
    if (NS_FAILED(rv)) return rv;

    mEOF = false;

    
    
    
    offsetInBuffer = uint32_t(absPos - mBufferStartOffset);
    if (offsetInBuffer <= mFillPoint) {
        
        
        
        
        mCursor = offsetInBuffer;
        return NS_OK;
    }

    METER(if (bufstats.mBigSeekIndex < MAX_BIG_SEEKS)
              bufstats.mBigSeek[bufstats.mBigSeekIndex].mOldOffset =
                  mBufferStartOffset + int64_t(mCursor));
    const int64_t minus1 = -1;
    if (absPos == minus1) {
        
        int64_t tellPos;
        rv = ras->Tell(&tellPos);
        mBufferStartOffset = tellPos;
        if (NS_FAILED(rv)) return rv;
    }
    else {
        mBufferStartOffset = absPos;
    }
    METER(if (bufstats.mBigSeekIndex < MAX_BIG_SEEKS)
              bufstats.mBigSeek[bufstats.mBigSeekIndex++].mNewOffset =
                  mBufferStartOffset);

    mFillPoint = mCursor = 0;
    return Fill();
}

NS_IMETHODIMP
nsBufferedStream::Tell(int64_t *result)
{
    if (mStream == nullptr)
        return NS_BASE_STREAM_CLOSED;
    
    int64_t result64 = mBufferStartOffset;
    result64 += mCursor;
    *result = result64;
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedStream::SetEOF()
{
    if (mStream == nullptr)
        return NS_BASE_STREAM_CLOSED;
    
    nsresult rv;
    nsCOMPtr<nsISeekableStream> ras = do_QueryInterface(mStream, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = ras->SetEOF();
    if (NS_SUCCEEDED(rv))
        mEOF = true;
    return rv;
}




NS_IMPL_ADDREF_INHERITED(nsBufferedInputStream, nsBufferedStream)
NS_IMPL_RELEASE_INHERITED(nsBufferedInputStream, nsBufferedStream)

NS_IMPL_CLASSINFO(nsBufferedInputStream, nullptr, nsIClassInfo::THREADSAFE,
                  NS_BUFFEREDINPUTSTREAM_CID)

NS_INTERFACE_MAP_BEGIN(nsBufferedInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIBufferedInputStream)
    NS_INTERFACE_MAP_ENTRY(nsIStreamBufferAccess)
    NS_INTERFACE_MAP_ENTRY(nsIIPCSerializableInputStream)
    NS_IMPL_QUERY_CLASSINFO(nsBufferedInputStream)
NS_INTERFACE_MAP_END_INHERITING(nsBufferedStream)

NS_IMPL_CI_INTERFACE_GETTER(nsBufferedInputStream,
                            nsIInputStream,
                            nsIBufferedInputStream,
                            nsISeekableStream,
                            nsIStreamBufferAccess)

nsresult
nsBufferedInputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsBufferedInputStream* stream = new nsBufferedInputStream();
    if (stream == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

NS_IMETHODIMP
nsBufferedInputStream::Init(nsIInputStream* stream, uint32_t bufferSize)
{
    return nsBufferedStream::Init(stream, bufferSize);
}

NS_IMETHODIMP
nsBufferedInputStream::Close()
{
    nsresult rv1 = NS_OK, rv2;
    if (mStream) {
        rv1 = Source()->Close();
        NS_RELEASE(mStream);
    }
    rv2 = nsBufferedStream::Close();
    if (NS_FAILED(rv1)) return rv1;
    return rv2;
}

NS_IMETHODIMP
nsBufferedInputStream::Available(uint64_t *result)
{
    nsresult rv = NS_OK;
    *result = 0;
    if (mStream) {
        rv = Source()->Available(result);
    }
    *result += (mFillPoint - mCursor);
    return rv;
}

NS_IMETHODIMP
nsBufferedInputStream::Read(char * buf, uint32_t count, uint32_t *result)
{
    if (mBufferDisabled) {
        if (!mStream) {
            *result = 0;
            return NS_OK;
        }
        nsresult rv = Source()->Read(buf, count, result);
        if (NS_SUCCEEDED(rv)) {
            mBufferStartOffset += *result;  
            if (*result == 0) {
                mEOF = true;
            }
        }
        return rv;
    }

    return ReadSegments(NS_CopySegmentToBuffer, buf, count, result);
}

NS_IMETHODIMP
nsBufferedInputStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                    uint32_t count, uint32_t *result)
{
    *result = 0;

    if (!mStream)
        return NS_OK;

    nsresult rv = NS_OK;
    while (count > 0) {
        uint32_t amt = std::min(count, mFillPoint - mCursor);
        if (amt > 0) {
            uint32_t read = 0;
            rv = writer(this, closure, mBuffer + mCursor, *result, amt, &read);
            if (NS_FAILED(rv)) {
                
                rv = NS_OK;
                break;
            }
            *result += read;
            count -= read;
            mCursor += read;
        }
        else {
            rv = Fill();
            if (NS_FAILED(rv) || mFillPoint == mCursor)
                break;
        }
    }
    return (*result > 0) ? NS_OK : rv;
}

NS_IMETHODIMP
nsBufferedInputStream::IsNonBlocking(bool *aNonBlocking)
{
    if (mStream)
        return Source()->IsNonBlocking(aNonBlocking);
    return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
nsBufferedInputStream::Fill()
{
    if (mBufferDisabled)
        return NS_OK;
    NS_ENSURE_TRUE(mStream, NS_ERROR_NOT_INITIALIZED);

    nsresult rv;
    int32_t rem = int32_t(mFillPoint - mCursor);
    if (rem > 0) {
        
        
        
        memcpy(mBuffer, mBuffer + mCursor, rem);
    }
    mBufferStartOffset += mCursor;
    mFillPoint = rem;
    mCursor = 0;

    uint32_t amt;
    rv = Source()->Read(mBuffer + mFillPoint, mBufferSize - mFillPoint, &amt);
    if (NS_FAILED(rv)) return rv;

    if (amt == 0)
        mEOF = true;
    
    mFillPoint += amt;
    return NS_OK;
}

NS_IMETHODIMP_(char*)
nsBufferedInputStream::GetBuffer(uint32_t aLength, uint32_t aAlignMask)
{
    NS_ASSERTION(mGetBufferCount == 0, "nested GetBuffer!");
    if (mGetBufferCount != 0)
        return nullptr;

    if (mBufferDisabled)
        return nullptr;

    char* buf = mBuffer + mCursor;
    uint32_t rem = mFillPoint - mCursor;
    if (rem == 0) {
        if (NS_FAILED(Fill()))
            return nullptr;
        buf = mBuffer + mCursor;
        rem = mFillPoint - mCursor;
    }

    uint32_t mod = (NS_PTR_TO_INT32(buf) & aAlignMask);
    if (mod) {
        uint32_t pad = aAlignMask + 1 - mod;
        if (pad > rem)
            return nullptr;

        memset(buf, 0, pad);
        mCursor += pad;
        buf += pad;
        rem -= pad;
    }

    if (aLength > rem)
        return nullptr;
    mGetBufferCount++;
    return buf;
}

NS_IMETHODIMP_(void)
nsBufferedInputStream::PutBuffer(char* aBuffer, uint32_t aLength)
{
    NS_ASSERTION(mGetBufferCount == 1, "stray PutBuffer!");
    if (--mGetBufferCount != 0)
        return;

    NS_ASSERTION(mCursor + aLength <= mFillPoint, "PutBuffer botch");
    mCursor += aLength;
}

NS_IMETHODIMP
nsBufferedInputStream::DisableBuffering()
{
    NS_ASSERTION(!mBufferDisabled, "redundant call to DisableBuffering!");
    NS_ASSERTION(mGetBufferCount == 0,
                 "DisableBuffer call between GetBuffer and PutBuffer!");
    if (mGetBufferCount != 0)
        return NS_ERROR_UNEXPECTED;

    
    mBufferStartOffset += mCursor;
    mFillPoint = mCursor = 0;
    mBufferDisabled = true;
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedInputStream::EnableBuffering()
{
    NS_ASSERTION(mBufferDisabled, "gratuitous call to EnableBuffering!");
    mBufferDisabled = false;
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedInputStream::GetUnbufferedStream(nsISupports* *aStream)
{
    
    mBufferStartOffset += mCursor;
    mFillPoint = mCursor = 0;

    *aStream = mStream;
    NS_IF_ADDREF(*aStream);
    return NS_OK;
}

void
nsBufferedInputStream::Serialize(InputStreamParams& aParams,
                                 FileDescriptorArray& aFileDescriptors)
{
    BufferedInputStreamParams params;

    if (mStream) {
        nsCOMPtr<nsIInputStream> stream = do_QueryInterface(mStream);
        MOZ_ASSERT(stream);

        InputStreamParams wrappedParams;
        SerializeInputStream(stream, wrappedParams, aFileDescriptors);

        params.optionalStream() = wrappedParams;
    }
    else {
        params.optionalStream() = mozilla::void_t();
    }

    params.bufferSize() = mBufferSize;

    aParams = params;
}

bool
nsBufferedInputStream::Deserialize(const InputStreamParams& aParams,
                                   const FileDescriptorArray& aFileDescriptors)
{
    if (aParams.type() != InputStreamParams::TBufferedInputStreamParams) {
        NS_ERROR("Received unknown parameters from the other process!");
        return false;
    }

    const BufferedInputStreamParams& params =
        aParams.get_BufferedInputStreamParams();
    const OptionalInputStreamParams& wrappedParams = params.optionalStream();

    nsCOMPtr<nsIInputStream> stream;
    if (wrappedParams.type() == OptionalInputStreamParams::TInputStreamParams) {
        stream = DeserializeInputStream(wrappedParams.get_InputStreamParams(),
                                        aFileDescriptors);
        if (!stream) {
            NS_WARNING("Failed to deserialize wrapped stream!");
            return false;
        }
    }
    else {
        NS_ASSERTION(wrappedParams.type() == OptionalInputStreamParams::Tvoid_t,
                     "Unknown type for OptionalInputStreamParams!");
    }

    nsresult rv = Init(stream, params.bufferSize());
    NS_ENSURE_SUCCESS(rv, false);

    return true;
}




NS_IMPL_ADDREF_INHERITED(nsBufferedOutputStream, nsBufferedStream)
NS_IMPL_RELEASE_INHERITED(nsBufferedOutputStream, nsBufferedStream)


NS_INTERFACE_MAP_BEGIN(nsBufferedOutputStream)
    NS_INTERFACE_MAP_ENTRY(nsIOutputStream)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsISafeOutputStream, mSafeStream)
    NS_INTERFACE_MAP_ENTRY(nsIBufferedOutputStream)
    NS_INTERFACE_MAP_ENTRY(nsIStreamBufferAccess)
NS_INTERFACE_MAP_END_INHERITING(nsBufferedStream)

nsresult
nsBufferedOutputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsBufferedOutputStream* stream = new nsBufferedOutputStream();
    if (stream == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

NS_IMETHODIMP
nsBufferedOutputStream::Init(nsIOutputStream* stream, uint32_t bufferSize)
{
    
    mSafeStream = do_QueryInterface(stream);

    return nsBufferedStream::Init(stream, bufferSize);
}

NS_IMETHODIMP
nsBufferedOutputStream::Close()
{
    nsresult rv1, rv2 = NS_OK, rv3;
    rv1 = Flush();
    
    
    
    if (mStream) {
        rv2 = Sink()->Close();
        NS_RELEASE(mStream);
    }
    rv3 = nsBufferedStream::Close();
    if (NS_FAILED(rv1)) return rv1;
    if (NS_FAILED(rv2)) return rv2;
    return rv3;
}

NS_IMETHODIMP
nsBufferedOutputStream::Write(const char *buf, uint32_t count, uint32_t *result)
{
    nsresult rv = NS_OK;
    uint32_t written = 0;
    while (count > 0) {
        uint32_t amt = std::min(count, mBufferSize - mCursor);
        if (amt > 0) {
            memcpy(mBuffer + mCursor, buf + written, amt);
            written += amt;
            count -= amt;
            mCursor += amt;
            if (mFillPoint < mCursor)
                mFillPoint = mCursor;
        }
        else {
            NS_ASSERTION(mFillPoint, "iloop in nsBufferedOutputStream::Write!");
            rv = Flush();
            if (NS_FAILED(rv)) break;
        }
    }
    *result = written;
    return (written > 0) ? NS_OK : rv;
}

NS_IMETHODIMP
nsBufferedOutputStream::Flush()
{
    nsresult rv;
    uint32_t amt;
    if (!mStream) {
        
        return NS_OK;
    }
    rv = Sink()->Write(mBuffer, mFillPoint, &amt);
    if (NS_FAILED(rv)) return rv;
    mBufferStartOffset += amt;
    if (amt == mFillPoint) {
        mFillPoint = mCursor = 0;
        return NS_OK;   
    }

    
    
    
    uint32_t rem = mFillPoint - amt;
    memmove(mBuffer, mBuffer + amt, rem);
    mFillPoint = mCursor = rem;
    return NS_ERROR_FAILURE;        
}


NS_IMETHODIMP
nsBufferedOutputStream::Finish()
{
    
    nsresult rv = nsBufferedOutputStream::Flush();
    if (NS_FAILED(rv))
        NS_WARNING("failed to flush buffered data! possible dataloss");

    
    if (NS_SUCCEEDED(rv))
        rv = mSafeStream->Finish();
    else
        Sink()->Close();

    
    
    nsBufferedStream::Close();

    return rv;
}

static NS_METHOD
nsReadFromInputStream(nsIOutputStream* outStr,
                      void* closure,
                      char* toRawSegment, 
                      uint32_t offset,
                      uint32_t count,
                      uint32_t *readCount)
{
    nsIInputStream* fromStream = (nsIInputStream*)closure;
    return fromStream->Read(toRawSegment, count, readCount);
}

NS_IMETHODIMP
nsBufferedOutputStream::WriteFrom(nsIInputStream *inStr, uint32_t count, uint32_t *_retval)
{
    return WriteSegments(nsReadFromInputStream, inStr, count, _retval);
}

NS_IMETHODIMP
nsBufferedOutputStream::WriteSegments(nsReadSegmentFun reader, void * closure, uint32_t count, uint32_t *_retval)
{
    *_retval = 0;
    nsresult rv;
    while (count > 0) {
        uint32_t left = std::min(count, mBufferSize - mCursor);
        if (left == 0) {
            rv = Flush();
            if (NS_FAILED(rv))
                return (*_retval > 0) ? NS_OK : rv;

            continue;
        }

        uint32_t read = 0;
        rv = reader(this, closure, mBuffer + mCursor, *_retval, left, &read);

        if (NS_FAILED(rv)) 
            return (*_retval > 0) ? NS_OK : rv;
        mCursor += read;
        *_retval += read;
        count -= read;
        mFillPoint = std::max(mFillPoint, mCursor);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedOutputStream::IsNonBlocking(bool *aNonBlocking)
{
    if (mStream)
        return Sink()->IsNonBlocking(aNonBlocking);
    return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP_(char*)
nsBufferedOutputStream::GetBuffer(uint32_t aLength, uint32_t aAlignMask)
{
    NS_ASSERTION(mGetBufferCount == 0, "nested GetBuffer!");
    if (mGetBufferCount != 0)
        return nullptr;

    if (mBufferDisabled)
        return nullptr;

    char* buf = mBuffer + mCursor;
    uint32_t rem = mBufferSize - mCursor;
    if (rem == 0) {
        if (NS_FAILED(Flush()))
            return nullptr;
        buf = mBuffer + mCursor;
        rem = mBufferSize - mCursor;
    }

    uint32_t mod = (NS_PTR_TO_INT32(buf) & aAlignMask);
    if (mod) {
        uint32_t pad = aAlignMask + 1 - mod;
        if (pad > rem)
            return nullptr;

        memset(buf, 0, pad);
        mCursor += pad;
        buf += pad;
        rem -= pad;
    }

    if (aLength > rem)
        return nullptr;
    mGetBufferCount++;
    return buf;
}

NS_IMETHODIMP_(void)
nsBufferedOutputStream::PutBuffer(char* aBuffer, uint32_t aLength)
{
    NS_ASSERTION(mGetBufferCount == 1, "stray PutBuffer!");
    if (--mGetBufferCount != 0)
        return;

    NS_ASSERTION(mCursor + aLength <= mBufferSize, "PutBuffer botch");
    mCursor += aLength;
    if (mFillPoint < mCursor)
        mFillPoint = mCursor;
}

NS_IMETHODIMP
nsBufferedOutputStream::DisableBuffering()
{
    NS_ASSERTION(!mBufferDisabled, "redundant call to DisableBuffering!");
    NS_ASSERTION(mGetBufferCount == 0,
                 "DisableBuffer call between GetBuffer and PutBuffer!");
    if (mGetBufferCount != 0)
        return NS_ERROR_UNEXPECTED;

    
    nsresult rv = Flush();
    if (NS_FAILED(rv))
        return rv;

    mBufferDisabled = true;
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedOutputStream::EnableBuffering()
{
    NS_ASSERTION(mBufferDisabled, "gratuitous call to EnableBuffering!");
    mBufferDisabled = false;
    return NS_OK;
}

NS_IMETHODIMP
nsBufferedOutputStream::GetUnbufferedStream(nsISupports* *aStream)
{
    
    if (mFillPoint) {
        nsresult rv = Flush();
        if (NS_FAILED(rv))
            return rv;
    }

    *aStream = mStream;
    NS_IF_ADDREF(*aStream);
    return NS_OK;
}

#undef METER


