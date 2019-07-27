










#include "mozilla/Attributes.h"
#include "mozilla/MathAlgorithms.h"

#include "base/basictypes.h"

#include "nsMultiplexInputStream.h"
#include "nsIMultiplexInputStream.h"
#include "nsISeekableStream.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIClassInfoImpl.h"
#include "nsIIPCSerializableInputStream.h"
#include "mozilla/ipc/InputStreamUtils.h"

using namespace mozilla::ipc;

using mozilla::DeprecatedAbs;

class nsMultiplexInputStream MOZ_FINAL
  : public nsIMultiplexInputStream
  , public nsISeekableStream
  , public nsIIPCSerializableInputStream
{
public:
  nsMultiplexInputStream();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIMULTIPLEXINPUTSTREAM
  NS_DECL_NSISEEKABLESTREAM
  NS_DECL_NSIIPCSERIALIZABLEINPUTSTREAM

private:
  ~nsMultiplexInputStream()
  {
  }

  struct ReadSegmentsState
  {
    nsIInputStream* mThisStream;
    uint32_t mOffset;
    nsWriteSegmentFun mWriter;
    void* mClosure;
    bool mDone;
  };

  static NS_METHOD ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment, uint32_t aToOffset,
                             uint32_t aCount, uint32_t* aWriteCount);

  nsTArray<nsCOMPtr<nsIInputStream>> mStreams;
  uint32_t mCurrentStream;
  bool mStartedReadingCurrent;
  nsresult mStatus;
};

NS_IMPL_ADDREF(nsMultiplexInputStream)
NS_IMPL_RELEASE(nsMultiplexInputStream)

NS_IMPL_CLASSINFO(nsMultiplexInputStream, nullptr, nsIClassInfo::THREADSAFE,
                  NS_MULTIPLEXINPUTSTREAM_CID)

NS_IMPL_QUERY_INTERFACE_CI(nsMultiplexInputStream,
                           nsIMultiplexInputStream,
                           nsIInputStream,
                           nsISeekableStream,
                           nsIIPCSerializableInputStream)
NS_IMPL_CI_INTERFACE_GETTER(nsMultiplexInputStream,
                            nsIMultiplexInputStream,
                            nsIInputStream,
                            nsISeekableStream)

static nsresult
AvailableMaybeSeek(nsIInputStream* aStream, uint64_t* aResult)
{
  nsresult rv = aStream->Available(aResult);
  if (rv == NS_BASE_STREAM_CLOSED) {
    
    
    
    
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(aStream);
    if (seekable) {
      nsresult rv = seekable->Seek(nsISeekableStream::NS_SEEK_CUR, 0);
      if (NS_SUCCEEDED(rv)) {
        rv = aStream->Available(aResult);
      }
    }
  }
  return rv;
}

static nsresult
TellMaybeSeek(nsISeekableStream* aSeekable, int64_t* aResult)
{
  nsresult rv = aSeekable->Tell(aResult);
  if (rv == NS_BASE_STREAM_CLOSED) {
    
    
    
    
    nsresult rv = aSeekable->Seek(nsISeekableStream::NS_SEEK_CUR, 0);
    if (NS_SUCCEEDED(rv)) {
      rv = aSeekable->Tell(aResult);
    }
  }
  return rv;
}

nsMultiplexInputStream::nsMultiplexInputStream()
  : mCurrentStream(0),
    mStartedReadingCurrent(false),
    mStatus(NS_OK)
{
}


NS_IMETHODIMP
nsMultiplexInputStream::GetCount(uint32_t* aCount)
{
  *aCount = mStreams.Length();
  return NS_OK;
}

#ifdef DEBUG
static bool
SeekableStreamAtBeginning(nsIInputStream* aStream)
{
  int64_t streamPos;
  nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(aStream);
  if (stream && NS_SUCCEEDED(stream->Tell(&streamPos)) && streamPos != 0) {
    return false;
  }
  return true;
}
#endif


NS_IMETHODIMP
nsMultiplexInputStream::AppendStream(nsIInputStream* aStream)
{
  NS_ASSERTION(SeekableStreamAtBeginning(aStream),
               "Appended stream not at beginning.");
  return mStreams.AppendElement(aStream) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsMultiplexInputStream::InsertStream(nsIInputStream* aStream, uint32_t aIndex)
{
  NS_ASSERTION(SeekableStreamAtBeginning(aStream),
               "Inserted stream not at beginning.");
  mStreams.InsertElementAt(aIndex, aStream);
  if (mCurrentStream > aIndex ||
      (mCurrentStream == aIndex && mStartedReadingCurrent)) {
    ++mCurrentStream;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::RemoveStream(uint32_t aIndex)
{
  mStreams.RemoveElementAt(aIndex);
  if (mCurrentStream > aIndex) {
    --mCurrentStream;
  } else if (mCurrentStream == aIndex) {
    mStartedReadingCurrent = false;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::GetStream(uint32_t aIndex, nsIInputStream** aResult)
{
  *aResult = mStreams.SafeElementAt(aIndex, nullptr);
  if (NS_WARN_IF(!*aResult)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ADDREF(*aResult);
  return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::Close()
{
  mStatus = NS_BASE_STREAM_CLOSED;

  nsresult rv = NS_OK;

  uint32_t len = mStreams.Length();
  for (uint32_t i = 0; i < len; ++i) {
    nsresult rv2 = mStreams[i]->Close();
    
    if (NS_FAILED(rv2)) {
      rv = rv2;
    }
  }
  return rv;
}


NS_IMETHODIMP
nsMultiplexInputStream::Available(uint64_t* aResult)
{
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  nsresult rv;
  uint64_t avail = 0;

  uint32_t len = mStreams.Length();
  for (uint32_t i = mCurrentStream; i < len; i++) {
    uint64_t streamAvail;
    rv = AvailableMaybeSeek(mStreams[i], &streamAvail);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    avail += streamAvail;
  }
  *aResult = avail;
  return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::Read(char* aBuf, uint32_t aCount, uint32_t* aResult)
{
  
  
  

  *aResult = 0;

  if (mStatus == NS_BASE_STREAM_CLOSED) {
    return NS_OK;
  }
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  nsresult rv = NS_OK;

  uint32_t len = mStreams.Length();
  while (mCurrentStream < len && aCount) {
    uint32_t read;
    rv = mStreams[mCurrentStream]->Read(aBuf, aCount, &read);

    
    
    if (rv == NS_BASE_STREAM_CLOSED) {
      NS_NOTREACHED("Input stream's Read method returned NS_BASE_STREAM_CLOSED");
      rv = NS_OK;
      read = 0;
    } else if (NS_FAILED(rv)) {
      break;
    }

    if (read == 0) {
      ++mCurrentStream;
      mStartedReadingCurrent = false;
    } else {
      NS_ASSERTION(aCount >= read, "Read more than requested");
      *aResult += read;
      aCount -= read;
      aBuf += read;
      mStartedReadingCurrent = true;
    }
  }
  return *aResult ? NS_OK : rv;
}




NS_IMETHODIMP
nsMultiplexInputStream::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                                     uint32_t aCount, uint32_t* aResult)
{
  if (mStatus == NS_BASE_STREAM_CLOSED) {
    *aResult = 0;
    return NS_OK;
  }
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  NS_ASSERTION(aWriter, "missing aWriter");

  nsresult rv = NS_OK;
  ReadSegmentsState state;
  state.mThisStream = this;
  state.mOffset = 0;
  state.mWriter = aWriter;
  state.mClosure = aClosure;
  state.mDone = false;

  uint32_t len = mStreams.Length();
  while (mCurrentStream < len && aCount) {
    uint32_t read;
    rv = mStreams[mCurrentStream]->ReadSegments(ReadSegCb, &state, aCount, &read);

    
    
    if (rv == NS_BASE_STREAM_CLOSED) {
      NS_NOTREACHED("Input stream's Read method returned NS_BASE_STREAM_CLOSED");
      rv = NS_OK;
      read = 0;
    }

    
    if (state.mDone || NS_FAILED(rv)) {
      break;
    }

    
    if (read == 0) {
      ++mCurrentStream;
      mStartedReadingCurrent = false;
    } else {
      NS_ASSERTION(aCount >= read, "Read more than requested");
      state.mOffset += read;
      aCount -= read;
      mStartedReadingCurrent = true;
    }
  }

  
  *aResult = state.mOffset;
  return state.mOffset ? NS_OK : rv;
}

NS_METHOD
nsMultiplexInputStream::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                                  const char* aFromRawSegment,
                                  uint32_t aToOffset, uint32_t aCount,
                                  uint32_t* aWriteCount)
{
  nsresult rv;
  ReadSegmentsState* state = (ReadSegmentsState*)aClosure;
  rv = (state->mWriter)(state->mThisStream,
                        state->mClosure,
                        aFromRawSegment,
                        aToOffset + state->mOffset,
                        aCount,
                        aWriteCount);
  if (NS_FAILED(rv)) {
    state->mDone = true;
  }
  return rv;
}


NS_IMETHODIMP
nsMultiplexInputStream::IsNonBlocking(bool* aNonBlocking)
{
  uint32_t len = mStreams.Length();
  if (len == 0) {
    
    
    
    
    *aNonBlocking = true;
    return NS_OK;
  }
  for (uint32_t i = 0; i < len; ++i) {
    nsresult rv = mStreams[i]->IsNonBlocking(aNonBlocking);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    
    
    
    if (*aNonBlocking) {
      return NS_OK;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::Seek(int32_t aWhence, int64_t aOffset)
{
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  nsresult rv;

  uint32_t oldCurrentStream = mCurrentStream;
  bool oldStartedReadingCurrent = mStartedReadingCurrent;

  if (aWhence == NS_SEEK_SET) {
    int64_t remaining = aOffset;
    if (aOffset == 0) {
      mCurrentStream = 0;
    }
    for (uint32_t i = 0; i < mStreams.Length(); ++i) {
      nsCOMPtr<nsISeekableStream> stream =
        do_QueryInterface(mStreams[i]);
      if (!stream) {
        return NS_ERROR_FAILURE;
      }

      
      if (remaining == 0) {
        if (i < oldCurrentStream ||
            (i == oldCurrentStream && oldStartedReadingCurrent)) {
          rv = stream->Seek(NS_SEEK_SET, 0);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }
          continue;
        } else {
          break;
        }
      }

      
      int64_t streamPos;
      if (i > oldCurrentStream ||
          (i == oldCurrentStream && !oldStartedReadingCurrent)) {
        streamPos = 0;
      } else {
        rv = TellMaybeSeek(stream, &streamPos);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }

      
      if (remaining < streamPos) {
        rv = stream->Seek(NS_SEEK_SET, remaining);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        mCurrentStream = i;
        mStartedReadingCurrent = remaining != 0;

        remaining = 0;
      } else if (remaining > streamPos) {
        if (i < oldCurrentStream) {
          
          remaining -= streamPos;
          NS_ASSERTION(remaining >= 0, "Remaining invalid");
        } else {
          uint64_t avail;
          rv = AvailableMaybeSeek(mStreams[i], &avail);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          int64_t newPos = XPCOM_MIN(remaining, streamPos + (int64_t)avail);

          rv = stream->Seek(NS_SEEK_SET, newPos);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          mCurrentStream = i;
          mStartedReadingCurrent = true;

          remaining -= newPos;
          NS_ASSERTION(remaining >= 0, "Remaining invalid");
        }
      } else {
        NS_ASSERTION(remaining == streamPos, "Huh?");
        remaining = 0;
      }
    }

    return NS_OK;
  }

  if (aWhence == NS_SEEK_CUR && aOffset > 0) {
    int64_t remaining = aOffset;
    for (uint32_t i = mCurrentStream; remaining && i < mStreams.Length(); ++i) {
      nsCOMPtr<nsISeekableStream> stream =
        do_QueryInterface(mStreams[i]);

      uint64_t avail;
      rv = AvailableMaybeSeek(mStreams[i], &avail);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      int64_t seek = XPCOM_MIN((int64_t)avail, remaining);

      rv = stream->Seek(NS_SEEK_CUR, seek);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      mCurrentStream = i;
      mStartedReadingCurrent = true;

      remaining -= seek;
    }

    return NS_OK;
  }

  if (aWhence == NS_SEEK_CUR && aOffset < 0) {
    int64_t remaining = -aOffset;
    for (uint32_t i = mCurrentStream; remaining && i != (uint32_t)-1; --i) {
      nsCOMPtr<nsISeekableStream> stream =
        do_QueryInterface(mStreams[i]);

      int64_t pos;
      rv = TellMaybeSeek(stream, &pos);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      int64_t seek = XPCOM_MIN(pos, remaining);

      rv = stream->Seek(NS_SEEK_CUR, -seek);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      mCurrentStream = i;
      mStartedReadingCurrent = seek != -pos;

      remaining -= seek;
    }

    return NS_OK;
  }

  if (aWhence == NS_SEEK_CUR) {
    NS_ASSERTION(aOffset == 0, "Should have handled all non-zero values");

    return NS_OK;
  }

  if (aWhence == NS_SEEK_END) {
    if (aOffset > 0) {
      return NS_ERROR_INVALID_ARG;
    }
    int64_t remaining = aOffset;
    for (uint32_t i = mStreams.Length() - 1; i != (uint32_t)-1; --i) {
      nsCOMPtr<nsISeekableStream> stream =
        do_QueryInterface(mStreams[i]);

      
      if (remaining == 0) {
        if (i >= oldCurrentStream) {
          rv = stream->Seek(NS_SEEK_END, 0);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }
        } else {
          break;
        }
      }

      
      int64_t streamPos;
      if (i < oldCurrentStream) {
        streamPos = 0;
      } else {
        uint64_t avail;
        rv = AvailableMaybeSeek(mStreams[i], &avail);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        streamPos = avail;
      }

      
      if (DeprecatedAbs(remaining) < streamPos) {
        rv = stream->Seek(NS_SEEK_END, remaining);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        mCurrentStream = i;
        mStartedReadingCurrent = true;

        remaining = 0;
      } else if (DeprecatedAbs(remaining) > streamPos) {
        if (i > oldCurrentStream ||
            (i == oldCurrentStream && !oldStartedReadingCurrent)) {
          
          remaining += streamPos;
        } else {
          int64_t avail;
          rv = TellMaybeSeek(stream, &avail);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          int64_t newPos = streamPos + XPCOM_MIN(avail, DeprecatedAbs(remaining));

          rv = stream->Seek(NS_SEEK_END, -newPos);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          mCurrentStream = i;
          mStartedReadingCurrent = true;

          remaining += newPos;
        }
      } else {
        NS_ASSERTION(remaining == streamPos, "Huh?");
        remaining = 0;
      }
    }

    return NS_OK;
  }

  
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMultiplexInputStream::Tell(int64_t* aResult)
{
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  nsresult rv;
  int64_t ret64 = 0;
  uint32_t i, last;
  last = mStartedReadingCurrent ? mCurrentStream + 1 : mCurrentStream;
  for (i = 0; i < last; ++i) {
    nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(mStreams[i]);
    if (NS_WARN_IF(!stream)) {
      return NS_ERROR_NO_INTERFACE;
    }

    int64_t pos;
    rv = TellMaybeSeek(stream, &pos);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    ret64 += pos;
  }
  *aResult =  ret64;

  return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::SetEOF()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsMultiplexInputStreamConstructor(nsISupports* aOuter,
                                  REFNSIID aIID,
                                  void** aResult)
{
  *aResult = nullptr;

  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsMultiplexInputStream* inst = new nsMultiplexInputStream();
  if (!inst) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(inst);
  nsresult rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

void
nsMultiplexInputStream::Serialize(InputStreamParams& aParams,
                                  FileDescriptorArray& aFileDescriptors)
{
  MultiplexInputStreamParams params;

  uint32_t streamCount = mStreams.Length();

  if (streamCount) {
    InfallibleTArray<InputStreamParams>& streams = params.streams();

    streams.SetCapacity(streamCount);
    for (uint32_t index = 0; index < streamCount; index++) {
      InputStreamParams childStreamParams;
      SerializeInputStream(mStreams[index], childStreamParams,
                           aFileDescriptors);

      streams.AppendElement(childStreamParams);
    }
  }

  params.currentStream() = mCurrentStream;
  params.status() = mStatus;
  params.startedReadingCurrent() = mStartedReadingCurrent;

  aParams = params;
}

bool
nsMultiplexInputStream::Deserialize(const InputStreamParams& aParams,
                                    const FileDescriptorArray& aFileDescriptors)
{
  if (aParams.type() !=
      InputStreamParams::TMultiplexInputStreamParams) {
    NS_ERROR("Received unknown parameters from the other process!");
    return false;
  }

  const MultiplexInputStreamParams& params =
    aParams.get_MultiplexInputStreamParams();

  const InfallibleTArray<InputStreamParams>& streams = params.streams();

  uint32_t streamCount = streams.Length();
  for (uint32_t index = 0; index < streamCount; index++) {
    nsCOMPtr<nsIInputStream> stream =
      DeserializeInputStream(streams[index], aFileDescriptors);
    if (!stream) {
      NS_WARNING("Deserialize failed!");
      return false;
    }

    if (NS_FAILED(AppendStream(stream))) {
      NS_WARNING("AppendStream failed!");
      return false;
    }
  }

  mCurrentStream = params.currentStream();
  mStatus = params.status();
  mStartedReadingCurrent = params.startedReadingCurrent();

  return true;
}
