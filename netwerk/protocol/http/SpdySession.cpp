






































#include "nsHttp.h"
#include "SpdySession.h"
#include "SpdyStream.h"
#include "nsHttpConnection.h"
#include "prnetdb.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Preferences.h"
#include "prprf.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {




NS_IMPL_THREADSAFE_ADDREF(SpdySession)
NS_IMPL_THREADSAFE_RELEASE(SpdySession)
NS_INTERFACE_MAP_BEGIN(SpdySession)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsAHttpConnection)
NS_INTERFACE_MAP_END

SpdySession::SpdySession(nsAHttpTransaction *aHttpTransaction,
                         nsISocketTransport *aSocketTransport,
                         PRInt32 firstPriority)
  : mSocketTransport(aSocketTransport),
    mSegmentReader(nsnull),
    mSegmentWriter(nsnull),
    mSendingChunkSize(kSendingChunkSize),
    mNextStreamID(1),
    mConcurrentHighWater(0),
    mDownstreamState(BUFFERING_FRAME_HEADER),
    mPartialFrame(nsnull),
    mFrameBufferSize(kDefaultBufferSize),
    mFrameBufferUsed(0),
    mFrameDataLast(false),
    mFrameDataStream(nsnull),
    mNeedsCleanup(nsnull),
    mDecompressBufferSize(kDefaultBufferSize),
    mDecompressBufferUsed(0),
    mShouldGoAway(false),
    mClosed(false),
    mCleanShutdown(false),
    mGoAwayID(0),
    mMaxConcurrent(kDefaultMaxConcurrent),
    mConcurrent(0),
    mServerPushedResources(0),
    mOutputQueueSize(kDefaultQueueSize),
    mOutputQueueUsed(0),
    mOutputQueueSent(0)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  LOG3(("SpdySession::SpdySession %p transaction 1 = %p",
        this, aHttpTransaction));
  
  mStreamIDHash.Init();
  mStreamTransactionHash.Init();
  mConnection = aHttpTransaction->Connection();
  mFrameBuffer = new char[mFrameBufferSize];
  mDecompressBuffer = new char[mDecompressBufferSize];
  mOutputQueueBuffer = new char[mOutputQueueSize];
  zlibInit();
  
  mSendingChunkSize =
    Preferences::GetInt("network.http.spdy.chunk-size", kSendingChunkSize);
  AddStream(aHttpTransaction, firstPriority);
}

PLDHashOperator
SpdySession::Shutdown(nsAHttpTransaction *key,
                      nsAutoPtr<SpdyStream> &stream,
                      void *closure)
{
  SpdySession *self = static_cast<SpdySession *>(closure);
  
  if (self->mCleanShutdown &&
      self->mGoAwayID < stream->StreamID())
    stream->Close(NS_ERROR_NET_RESET); 
  else
    stream->Close(NS_ERROR_ABORT);

  return PL_DHASH_NEXT;
}

SpdySession::~SpdySession()
{
  LOG3(("SpdySession::~SpdySession %p", this));

  inflateEnd(&mDownstreamZlib);
  deflateEnd(&mUpstreamZlib);
  
  mStreamTransactionHash.Enumerate(Shutdown, this);
  Telemetry::Accumulate(Telemetry::SPDY_PARALLEL_STREAMS, mConcurrentHighWater);
  Telemetry::Accumulate(Telemetry::SPDY_TOTAL_STREAMS, (mNextStreamID - 1) / 2);
  Telemetry::Accumulate(Telemetry::SPDY_SERVER_INITIATED_STREAMS,
                        mServerPushedResources);
}

void
SpdySession::LogIO(SpdySession *self, SpdyStream *stream, const char *label,
                   const char *data, PRUint32 datalen)
{
  if (!LOG4_ENABLED())
    return;
  
  LOG4(("SpdySession::LogIO %p stream=%p id=0x%X [%s]",
        self, stream, stream ? stream->StreamID() : 0, label));

  
  char linebuf[128];
  PRUint32 index;
  char *line = linebuf;

  linebuf[127] = 0;

  for (index = 0; index < datalen; ++index) {
    if (!(index % 16)) {
      if (index) {
        *line = 0;
        LOG4(("%s", linebuf));
      }
      line = linebuf;
      PR_snprintf(line, 128, "%08X: ", index);
      line += 10;
    }
    PR_snprintf(line, 128 - (line - linebuf), "%02X ",
                ((unsigned char *)data)[index]);
    line += 3;
  }
  if (index) {
    *line = 0;
    LOG4(("%s", linebuf));
  }
}

typedef nsresult  (*Control_FX) (SpdySession *self);
static Control_FX sControlFunctions[] = 
{
  nsnull,
  SpdySession::HandleSynStream,
  SpdySession::HandleSynReply,
  SpdySession::HandleRstStream,
  SpdySession::HandleSettings,
  SpdySession::HandleNoop,
  SpdySession::HandlePing,
  SpdySession::HandleGoAway,
  SpdySession::HandleHeaders,
  SpdySession::HandleWindowUpdate
};

bool
SpdySession::RoomForMoreConcurrent()
{
  return (mConcurrent < mMaxConcurrent);
}

bool
SpdySession::RoomForMoreStreams()
{
  if (mNextStreamID + mStreamTransactionHash.Count() * 2 > kMaxStreamID)
    return false;

  return !mShouldGoAway;
}

PRUint32
SpdySession::RegisterStreamID(SpdyStream *stream)
{
  LOG3(("SpdySession::RegisterStreamID session=%p stream=%p id=0x%X "
        "concurrent=%d",this, stream, mNextStreamID, mConcurrent));

  NS_ABORT_IF_FALSE(mNextStreamID < 0xfffffff0,
                    "should have stopped admitting streams");
  
  PRUint32 result = mNextStreamID;
  mNextStreamID += 2;

  
  
  
  if (mNextStreamID >= kMaxStreamID)
    mShouldGoAway = true;

  mStreamIDHash.Put(result, stream);
  return result;
}

bool
SpdySession::AddStream(nsAHttpTransaction *aHttpTransaction,
                       PRInt32 aPriority)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(!mStreamTransactionHash.Get(aHttpTransaction),
                    "AddStream duplicate transaction pointer");

  aHttpTransaction->SetConnection(this);
  SpdyStream *stream = new SpdyStream(aHttpTransaction,
                                      this,
                                      mSocketTransport,
                                      mSendingChunkSize,
                                      &mUpstreamZlib,
                                      aPriority);

  
  LOG3(("SpdySession::AddStream session=%p stream=%p NextID=0x%X (tentative)",
        this, stream, mNextStreamID));

  mStreamTransactionHash.Put(aHttpTransaction, stream);

  if (RoomForMoreConcurrent()) {
    LOG3(("SpdySession::AddStream %p stream %p activated immediately.",
          this, stream));
    ActivateStream(stream);
  }
  else {
    LOG3(("SpdySession::AddStream %p stream %p queued.",
          this, stream));
    mQueuedStreams.Push(stream);
  }
  
  return true;
}

void
SpdySession::ActivateStream(SpdyStream *stream)
{
  mConcurrent++;
  if (mConcurrent > mConcurrentHighWater)
    mConcurrentHighWater = mConcurrent;
  LOG3(("SpdySession::AddStream %p activating stream %p Currently %d"
        "streams in session, high water mark is %d",
        this, stream, mConcurrent, mConcurrentHighWater));

  mReadyForWrite.Push(stream);
  SetWriteCallbacks(stream->Transaction());

  
  PRUint32 countRead;
  ReadSegments(nsnull, kDefaultBufferSize, &countRead);
}

void
SpdySession::ProcessPending()
{
  while (RoomForMoreConcurrent()) {
    SpdyStream *stream = static_cast<SpdyStream *>(mQueuedStreams.PopFront());
    if (!stream)
      return;
    LOG3(("SpdySession::ProcessPending %p stream %p activated from queue.",
          this, stream));
    ActivateStream(stream);
  }
}

void
SpdySession::SetWriteCallbacks(nsAHttpTransaction *aTrans)
{
  if (mConnection && (WriteQueueSize() || mOutputQueueUsed))
      mConnection->ResumeSend(aTrans);
}

void
SpdySession::FlushOutputQueue()
{
  if (!mSegmentReader || !mOutputQueueUsed)
    return;
  
  nsresult rv;
  PRUint32 countRead;
  PRUint32 avail = mOutputQueueUsed - mOutputQueueSent;

  rv = mSegmentReader->
    OnReadSegment(mOutputQueueBuffer.get() + mOutputQueueSent, avail,
                                     &countRead);
  LOG3(("SpdySession::FlushOutputQueue %p sz=%d rv=%x actual=%d",
        this, avail, rv, countRead));
  
  
  if (NS_FAILED(rv))
    return;
  
  if (countRead == avail) {
    mOutputQueueUsed = 0;
    mOutputQueueSent = 0;
    return;
  }

  mOutputQueueSent += countRead;
  if (mOutputQueueSize - mOutputQueueUsed < kQueueTailRoom) {
    
    

    mOutputQueueUsed -= mOutputQueueSent;
    memmove(mOutputQueueBuffer.get(),
            mOutputQueueBuffer.get() + mOutputQueueSent,
            mOutputQueueUsed);
    mOutputQueueSent = 0;
  }
}

void
SpdySession::DontReuse()
{
  mShouldGoAway = true;
  if(!mStreamTransactionHash.Count())
    Close(NS_OK);
}

PRUint32
SpdySession::WriteQueueSize()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  PRUint32 count = mUrgentForWrite.GetSize() + mReadyForWrite.GetSize();

  if (mPartialFrame)
    ++count;
  return count;
}

void
SpdySession::ChangeDownstreamState(enum stateType newState)
{
  LOG3(("SpdyStream::ChangeDownstreamState() %p from %X to %X",
        this, mDownstreamState, newState));
  mDownstreamState = newState;

  if (mDownstreamState == BUFFERING_FRAME_HEADER) {
    if (mFrameDataLast && mFrameDataStream) {
      mFrameDataLast = 0;
      if (!mFrameDataStream->RecvdFin()) {
        mFrameDataStream->SetRecvdFin(true);
        --mConcurrent;
        ProcessPending();
      }
    }
    mFrameBufferUsed = 0;
    mFrameDataStream = nsnull;
  }
  
  return;
}

void
SpdySession::EnsureBuffer(nsAutoArrayPtr<char> &buf,
                          PRUint32 newSize,
                          PRUint32 preserve,
                          PRUint32 &objSize)
{
  if (objSize >= newSize)
    return;
  
  objSize = newSize;
  nsAutoArrayPtr<char> tmp(new char[objSize]);
  memcpy (tmp, buf, preserve);
  buf = tmp;
}

void
SpdySession::zlibInit()
{
  mDownstreamZlib.zalloc = SpdyStream::zlib_allocator;
  mDownstreamZlib.zfree = SpdyStream::zlib_destructor;
  mDownstreamZlib.opaque = Z_NULL;

  inflateInit(&mDownstreamZlib);

  mUpstreamZlib.zalloc = SpdyStream::zlib_allocator;
  mUpstreamZlib.zfree = SpdyStream::zlib_destructor;
  mUpstreamZlib.opaque = Z_NULL;

  deflateInit(&mUpstreamZlib, Z_DEFAULT_COMPRESSION);
  deflateSetDictionary(&mUpstreamZlib,
                       reinterpret_cast<const unsigned char *>
                       (SpdyStream::kDictionary),
                       strlen(SpdyStream::kDictionary) + 1);

}

nsresult
SpdySession::DownstreamUncompress(char *blockStart, PRUint32 blockLen)
{
  mDecompressBufferUsed = 0;

  mDownstreamZlib.avail_in = blockLen;
  mDownstreamZlib.next_in = reinterpret_cast<unsigned char *>(blockStart);

  do {
    mDownstreamZlib.next_out =
      reinterpret_cast<unsigned char *>(mDecompressBuffer.get()) +
      mDecompressBufferUsed;
    mDownstreamZlib.avail_out = mDecompressBufferSize - mDecompressBufferUsed;
    int zlib_rv = inflate(&mDownstreamZlib, Z_NO_FLUSH);

    if (zlib_rv == Z_NEED_DICT)
      inflateSetDictionary(&mDownstreamZlib,
                           reinterpret_cast<const unsigned char *>
                           (SpdyStream::kDictionary),
                           strlen(SpdyStream::kDictionary) + 1);
    
    if (zlib_rv == Z_DATA_ERROR || zlib_rv == Z_MEM_ERROR)
      return NS_ERROR_FAILURE;

    mDecompressBufferUsed += mDecompressBufferSize -mDecompressBufferUsed -
      mDownstreamZlib.avail_out;

    if (mDownstreamZlib.avail_out) {
      LOG3(("SpdySession::DownstreamUncompress %p Large Headers - so far %d",
            this, mDecompressBufferSize));
      EnsureBuffer(mDecompressBuffer,
                   mDecompressBufferSize + 4096,
                   mDecompressBufferUsed,
                   mDecompressBufferSize);
    }
  }
  while (mDownstreamZlib.avail_in);
  return NS_OK;
}

nsresult
SpdySession::FindHeader(nsCString name,
                        nsDependentCSubstring &value)
{
  const unsigned char *nvpair = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + 2;
  const unsigned char *lastHeaderByte = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + mDecompressBufferUsed;
  if (lastHeaderByte < nvpair)
    return NS_ERROR_ILLEGAL_VALUE;
  PRUint16 numPairs =
    PR_ntohs(reinterpret_cast<PRUint16 *>(mDecompressBuffer.get())[0]);
  for (PRUint16 index = 0; index < numPairs; ++index) {
    if (lastHeaderByte < nvpair + 2)
      return NS_ERROR_ILLEGAL_VALUE;
    PRUint32 nameLen = (nvpair[0] << 8) + nvpair[1];
    if (lastHeaderByte < nvpair + 2 + nameLen)
      return NS_ERROR_ILLEGAL_VALUE;
    nsDependentCSubstring nameString =
      Substring (reinterpret_cast<const char *>(nvpair) + 2,
                 reinterpret_cast<const char *>(nvpair) + 2 + nameLen);
    if (lastHeaderByte < nvpair + 4 + nameLen)
      return NS_ERROR_ILLEGAL_VALUE;
    PRUint16 valueLen = (nvpair[2 + nameLen] << 8) + nvpair[3 + nameLen];
    if (lastHeaderByte < nvpair + 4 + nameLen + valueLen)
      return NS_ERROR_ILLEGAL_VALUE;
    if (nameString.Equals(name)) {
      value.Assign(((char *)nvpair) + 4 + nameLen, valueLen);
      return NS_OK;
    }
    nvpair += 4 + nameLen + valueLen;
  }
  return NS_ERROR_NOT_AVAILABLE;
}

nsresult
SpdySession::ConvertHeaders(nsDependentCSubstring &status,
                            nsDependentCSubstring &version)
{

  mFlatHTTPResponseHeaders.Truncate();
  mFlatHTTPResponseHeadersOut = 0;
  mFlatHTTPResponseHeaders.SetCapacity(mDecompressBufferUsed + 64);

  
  

  
  
  
  mFlatHTTPResponseHeaders.Append(version);
  mFlatHTTPResponseHeaders.Append(NS_LITERAL_CSTRING(" "));
  mFlatHTTPResponseHeaders.Append(status);
  mFlatHTTPResponseHeaders.Append(NS_LITERAL_CSTRING("\r\n"));

  const unsigned char *nvpair = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + 2;
  const unsigned char *lastHeaderByte = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + mDecompressBufferUsed;

  if (lastHeaderByte < nvpair)
    return NS_ERROR_ILLEGAL_VALUE;

  PRUint16 numPairs =
    PR_ntohs(reinterpret_cast<PRUint16 *>(mDecompressBuffer.get())[0]);

  for (PRUint16 index = 0; index < numPairs; ++index) {
    if (lastHeaderByte < nvpair + 2)
      return NS_ERROR_ILLEGAL_VALUE;

    PRUint32 nameLen = (nvpair[0] << 8) + nvpair[1];
    if (lastHeaderByte < nvpair + 2 + nameLen)
      return NS_ERROR_ILLEGAL_VALUE;

    nsDependentCSubstring nameString =
      Substring (reinterpret_cast<const char *>(nvpair) + 2,
                 reinterpret_cast<const char *>(nvpair) + 2 + nameLen);

    
    
    if (nameString.FindChar(0) != -1)
      return NS_ERROR_ILLEGAL_VALUE;

    if (lastHeaderByte < nvpair + 4 + nameLen)
      return NS_ERROR_ILLEGAL_VALUE;
    PRUint16 valueLen = (nvpair[2 + nameLen] << 8) + nvpair[3 + nameLen];
    if (lastHeaderByte < nvpair + 4 + nameLen + valueLen)
      return NS_ERROR_ILLEGAL_VALUE;
    
    
    for (char *cPtr = nameString.BeginWriting();
         cPtr && cPtr < nameString.EndWriting();
         ++cPtr) {
      if (*cPtr <= 'Z' && *cPtr >= 'A') {
        nsCString toLog(nameString);

        LOG3(("SpdySession::ConvertHeaders session=%p stream=%p "
              "upper case response header found. [%s]\n",
              this, mFrameDataStream, toLog.get()));

        return NS_ERROR_ILLEGAL_VALUE;
      }
    }

    
    
    
    
    if (nameString.Equals(NS_LITERAL_CSTRING("transfer-encoding"))) {
      LOG3(("SpdySession::ConvertHeaders session=%p stream=%p "
            "transfer-encoding found. Chunked is invalid and no TE sent.",
            this, mFrameDataStream));

      return NS_ERROR_ILLEGAL_VALUE;
    }

    if (!nameString.Equals(NS_LITERAL_CSTRING("version")) &&
        !nameString.Equals(NS_LITERAL_CSTRING("status")) &&
        !nameString.Equals(NS_LITERAL_CSTRING("connection")) &&
        !nameString.Equals(NS_LITERAL_CSTRING("keep-alive"))) {
      nsDependentCSubstring valueString =
        Substring (reinterpret_cast<const char *>(nvpair) + 4 + nameLen,
                   reinterpret_cast<const char *>(nvpair) + 4 + nameLen +
                   valueLen);
      
      mFlatHTTPResponseHeaders.Append(nameString);
      mFlatHTTPResponseHeaders.Append(NS_LITERAL_CSTRING(": "));

      PRInt32 valueIndex;
      
      while ((valueIndex = valueString.FindChar(0)) != -1) {
        nsCString replacement = NS_LITERAL_CSTRING("\r\n");
        replacement.Append(nameString);
        replacement.Append(NS_LITERAL_CSTRING(": "));
        valueString.Replace(valueIndex, 1, replacement);
      }

      mFlatHTTPResponseHeaders.Append(valueString);
      mFlatHTTPResponseHeaders.Append(NS_LITERAL_CSTRING("\r\n"));
    }
    nvpair += 4 + nameLen + valueLen;
  }

  mFlatHTTPResponseHeaders.Append(
    NS_LITERAL_CSTRING("X-Firefox-Spdy: 1\r\n\r\n"));
  LOG (("decoded response headers are:\n%s",
        mFlatHTTPResponseHeaders.get()));
  
  return NS_OK;
}

void
SpdySession::GeneratePing(PRUint32 aID)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession::GeneratePing %p 0x%X\n", this, aID));

  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 12,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 12;

  packet[0] = kFlag_Control;
  packet[1] = 2;                                  
  packet[2] = 0;
  packet[3] = CONTROL_TYPE_PING;
  packet[4] = 0;                                  
  packet[5] = 0;
  packet[6] = 0;
  packet[7] = 4;                                  
  
  aID = PR_htonl(aID);
  memcpy (packet + 8, &aID, 4);

  FlushOutputQueue();
}

void
SpdySession::GenerateRstStream(PRUint32 aStatusCode, PRUint32 aID)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession::GenerateRst %p 0x%X %d\n", this, aID, aStatusCode));

  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 16,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 16;

  packet[0] = kFlag_Control;
  packet[1] = 2;                                  
  packet[2] = 0;
  packet[3] = CONTROL_TYPE_RST_STREAM;
  packet[4] = 0;                                  
  packet[5] = 0;
  packet[6] = 0;
  packet[7] = 8;                                  
  
  aID = PR_htonl(aID);
  memcpy (packet + 8, &aID, 4);
  aStatusCode = PR_htonl(aStatusCode);
  memcpy (packet + 12, &aStatusCode, 4);

  FlushOutputQueue();
}

void
SpdySession::GenerateGoAway()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession::GenerateGoAway %p\n", this));

  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 12,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 12;

  memset (packet, 0, 12);
  packet[0] = kFlag_Control;
  packet[1] = 2;                                  
  packet[3] = CONTROL_TYPE_GOAWAY;
  packet[7] = 4;                                  
  
  
  

  FlushOutputQueue();
}

void
SpdySession::CleanupStream(SpdyStream *aStream, nsresult aResult)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession::CleanupStream %p %p 0x%x %X\n",
        this, aStream, aStream->StreamID(), aResult));

  nsresult abortCode = NS_OK;

  if (!aStream->RecvdFin() && aStream->StreamID()) {
    LOG3(("Stream had not processed recv FIN, sending RST"));
    GenerateRstStream(RST_CANCEL, aStream->StreamID());
    --mConcurrent;
    ProcessPending();
  }
  
  
  if (mPartialFrame == aStream) {
    LOG3(("Stream had active partial write frame - need to abort session"));
    abortCode = aResult;
    if (NS_SUCCEEDED(abortCode))
      abortCode = NS_ERROR_ABORT;
    
    mPartialFrame = nsnull;
  }
  
  
  if (aStream == mFrameDataStream) {
    LOG3(("Stream had active partial read frame on close"));
    ChangeDownstreamState(DISCARD_DATA_FRAME);
    mFrameDataStream = nsnull;
  }

  
  
  PRUint32 size = mReadyForWrite.GetSize();
  for (PRUint32 count = 0; count < size; ++count) {
    SpdyStream *stream = static_cast<SpdyStream *>(mReadyForWrite.PopFront());
    if (stream != aStream)
      mReadyForWrite.Push(stream);
  }

  
  
  size = mUrgentForWrite.GetSize();
  for (PRUint32 count = 0; count < size; ++count) {
    SpdyStream *stream = static_cast<SpdyStream *>(mUrgentForWrite.PopFront());
    if (stream != aStream)
      mUrgentForWrite.Push(stream);
  }

  
  
  mStreamIDHash.Remove(aStream->StreamID());

  
  aStream->Close(aResult);

  
  
  
  mStreamTransactionHash.Remove(aStream->Transaction());

  if (NS_FAILED(abortCode))
    Close(abortCode);
  else if (mShouldGoAway && !mStreamTransactionHash.Count())
    Close(NS_OK);
}

nsresult
SpdySession::HandleSynStream(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_SYN_STREAM,
                    "wrong control type");
  
  if (self->mFrameDataSize < 12) {
    LOG3(("SpdySession::HandleSynStream %p SYN_STREAM too short data=%d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUint32 streamID =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);

  LOG3(("SpdySession::HandleSynStream %p recv SYN_STREAM (push) for ID 0x%X.",
        self, streamID));
    
  if (streamID & 0x01) {                   
    LOG3(("SpdySession::HandleSynStream %p recvd SYN_STREAM id must be even.",
          self));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  ++(self->mServerPushedResources);

  
  
  if (streamID >= kMaxStreamID)
    self->mShouldGoAway = true;

  
  self->GenerateRstStream(RST_REFUSED_STREAM, streamID);
  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}

nsresult
SpdySession::HandleSynReply(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_SYN_REPLY,
                    "wrong control type");

  if (self->mFrameDataSize < 8) {
    LOG3(("SpdySession::HandleSynReply %p SYN REPLY too short data=%d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  PRUint32 streamID =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);
  self->mFrameDataStream = self->mStreamIDHash.Get(streamID);
  if (!self->mFrameDataStream) {
    LOG3(("SpdySession::HandleSynReply %p lookup streamID in syn_reply "
          "0x%X failed. NextStreamID = 0x%x", self, streamID,
          self->mNextStreamID));
    if (streamID >= self->mNextStreamID)
      self->GenerateRstStream(RST_INVALID_STREAM, streamID);
    
    
    
    
    self->DownstreamUncompress(self->mFrameBuffer + 14,
                               self->mFrameDataSize - 6);
    self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
    return NS_OK;
  }
  
  if (!self->mFrameDataStream->SetFullyOpen()) {
    
    
    
    
    
    

    self->GenerateRstStream(RST_PROTOCOL_ERROR, streamID);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  self->mFrameDataLast = self->mFrameBuffer[4] & kFlag_Data_FIN;

  if (self->mFrameBuffer[4] & kFlag_Data_UNI) {
    LOG3(("SynReply had unidirectional flag set on it - nonsensical"));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  LOG3(("SpdySession::HandleSynReply %p SYN_REPLY for 0x%X fin=%d",
        self, streamID, self->mFrameDataLast));
  
  
  
  
  
  
  
  
  

  nsresult rv = self->DownstreamUncompress(self->mFrameBuffer + 14,
                                           self->mFrameDataSize - 6);
  if (NS_FAILED(rv))
    return rv;
  
  Telemetry::Accumulate(Telemetry::SPDY_SYN_REPLY_SIZE,
                        self->mFrameDataSize - 6);
  PRUint32 ratio =
    (self->mFrameDataSize - 6) * 100 / self->mDecompressBufferUsed;
  Telemetry::Accumulate(Telemetry::SPDY_SYN_REPLY_RATIO, ratio);

  
  nsDependentCSubstring status, version;
  rv = self->FindHeader(NS_LITERAL_CSTRING("status"), status);
  if (NS_FAILED(rv))
    return rv;

  rv = self->FindHeader(NS_LITERAL_CSTRING("version"), version);
  if (NS_FAILED(rv))
    return rv;

  rv = self->ConvertHeaders(status, version);
  if (NS_FAILED(rv))
    return rv;

  self->ChangeDownstreamState(PROCESSING_CONTROL_SYN_REPLY);
  return NS_OK;
}

nsresult
SpdySession::HandleRstStream(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_RST_STREAM,
                    "wrong control type");

  if (self->mFrameDataSize != 8) {
    LOG3(("SpdySession::HandleRstStream %p RST_STREAM wrong length data=%d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUint32 streamID =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);

  self->mDownstreamRstReason =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[3]);

  LOG3(("SpdySession::HandleRstStream %p RST_STREAM Reason Code %u ID %x",
        self, self->mDownstreamRstReason, streamID));

  if (self->mDownstreamRstReason == RST_INVALID_STREAM ||
      self->mDownstreamRstReason == RST_FLOW_CONTROL_ERROR) {
    
    self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
    return NS_OK;
  }

  self->mFrameDataStream = self->mStreamIDHash.Get(streamID);
  if (!self->mFrameDataStream) {
    LOG3(("SpdySession::HandleRstStream %p lookup streamID for RST Frame "
          "0x%X failed", self, streamID));
    return NS_ERROR_ILLEGAL_VALUE;
  }
    
  self->ChangeDownstreamState(PROCESSING_CONTROL_RST_STREAM);
  return NS_OK;
}

nsresult
SpdySession::HandleSettings(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_SETTINGS,
                    "wrong control type");

  if (self->mFrameDataSize < 4) {
    LOG3(("SpdySession::HandleSettings %p SETTINGS wrong length data=%d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUint32 numEntries =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);

  
  
  
  if ((self->mFrameDataSize - 4) < (numEntries * 8)) {
    LOG3(("SpdySession::HandleSettings %p SETTINGS wrong length data=%d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  LOG3(("SpdySession::HandleSettings %p SETTINGS Control Frame with %d entries",
        self, numEntries));

  for (PRUint32 index = 0; index < numEntries; ++index) {
    
    
    
    
    
    unsigned char *setting = reinterpret_cast<unsigned char *>
      (self->mFrameBuffer.get()) + 12 + index * 8;

    PRUint32 id = (setting[2] << 16) + (setting[1] << 8) + setting[0];
    PRUint32 flags = setting[3];
    PRUint32 value =  PR_ntohl(reinterpret_cast<PRUint32 *>(setting)[1]);

    LOG3(("Settings ID %d, Flags %X, Value %d", id, flags, value));

    switch (id)
    {
    case SETTINGS_TYPE_UPLOAD_BW:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_UL_BW, value);
      break;
      
    case SETTINGS_TYPE_DOWNLOAD_BW:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_DL_BW, value);
      break;
      
    case SETTINGS_TYPE_RTT:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_RTT, value);
      break;
      
    case SETTINGS_TYPE_MAX_CONCURRENT:
      self->mMaxConcurrent = value;
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_MAX_STREAMS, value);
      break;
      
    case SETTINGS_TYPE_CWND:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_CWND, value);
      break;
      
    case SETTINGS_TYPE_DOWNLOAD_RETRANS_RATE:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_RETRANS, value);
      break;
      
    case SETTINGS_TYPE_INITIAL_WINDOW:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_IW, value >> 10);
      break;
      
    default:
      break;
    }
    
  }
  
  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}

nsresult
SpdySession::HandleNoop(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_NOOP,
                    "wrong control type");

  if (self->mFrameDataSize != 0) {
    LOG3(("SpdySession::HandleNoop %p NOP had data %d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  LOG3(("SpdySession::HandleNoop %p NOP.", self));

  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}

nsresult
SpdySession::HandlePing(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_PING,
                    "wrong control type");

  if (self->mFrameDataSize != 4) {
    LOG3(("SpdySession::HandlePing %p PING had wrong amount of data %d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUint32 pingID =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);

  LOG3(("SpdySession::HandlePing %p PING ID 0x%X.", self, pingID));

  if (pingID & 0x01) {
    
      
    LOG3(("SpdySession::HandlePing %p PING ID from server was odd.",
          self));
  }
  else {
    self->GeneratePing(pingID);
  }
    
  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}

nsresult
SpdySession::HandleGoAway(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_GOAWAY,
                    "wrong control type");

  if (self->mFrameDataSize != 4) {
    LOG3(("SpdySession::HandleGoAway %p GOAWAY had wrong amount of data %d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  self->mShouldGoAway = true;
  self->mGoAwayID =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);
  self->mCleanShutdown = true;
  
  LOG3(("SpdySession::HandleGoAway %p GOAWAY Last-Good-ID 0x%X.",
        self, self->mGoAwayID));
  self->ResumeRecv(self);
  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}

nsresult
SpdySession::HandleHeaders(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_HEADERS,
                    "wrong control type");

  if (self->mFrameDataSize < 10) {
    LOG3(("SpdySession::HandleHeaders %p HEADERS had wrong amount of data %d",
          self, self->mFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUint32 streamID =
    PR_ntohl(reinterpret_cast<PRUint32 *>(self->mFrameBuffer.get())[2]);

  
  

  LOG3(("SpdySession::HandleHeaders %p HEADERS for Stream 0x%X. "
        "They are ignored in the HTTP/SPDY mapping.",
        self, streamID));
  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}

nsresult
SpdySession::HandleWindowUpdate(SpdySession *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_WINDOW_UPDATE,
                    "wrong control type");
  LOG3(("SpdySession::HandleWindowUpdate %p WINDOW UPDATE was "
        "received. WINDOW UPDATE is no longer defined in v2. Ignoring.",
        self));

  self->ChangeDownstreamState(BUFFERING_FRAME_HEADER);
  return NS_OK;
}


struct transportStatus
{
  nsITransport *transport;
  nsresult status;
  PRUint64 progress;
};

static PLDHashOperator
StreamTransportStatus(nsAHttpTransaction *key,
                      nsAutoPtr<SpdyStream> &stream,
                      void *closure)
{
  struct transportStatus *status =
    static_cast<struct transportStatus *>(closure);

  stream->Transaction()->OnTransportStatus(status->transport,
                                           status->status,
                                           status->progress);
  return PL_DHASH_NEXT;
}







void
SpdySession::OnTransportStatus(nsITransport* aTransport,
                               nsresult aStatus,
                               PRUint64 aProgress)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  
  if (aStatus == nsISocketTransport::STATUS_RECEIVING_FROM)
    return;

  
  if (aStatus == nsISocketTransport::STATUS_SENDING_TO)
    return;

  struct transportStatus status;
  
  status.transport = aTransport;
  status.status = aStatus;
  status.progress = aProgress;

  mStreamTransactionHash.Enumerate(StreamTransportStatus, &status);
}






nsresult
SpdySession::ReadSegments(nsAHttpSegmentReader *reader,
                          PRUint32 count,
                          PRUint32 *countRead)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  nsresult rv;
  *countRead = 0;

  
  
  
  
  

  LOG3(("SpdySession::ReadSegments %p partial frame stream=%p",
        this, mPartialFrame));

  SpdyStream *stream = mPartialFrame;
  mPartialFrame = nsnull;

  if (!stream)
    stream = static_cast<SpdyStream *>(mUrgentForWrite.PopFront());
  if (!stream)
    stream = static_cast<SpdyStream *>(mReadyForWrite.PopFront());
  if (!stream) {
    LOG3(("SpdySession %p could not identify a stream to write; suspending.",
          this));
    FlushOutputQueue();
    SetWriteCallbacks(nsnull);
    return NS_BASE_STREAM_WOULD_BLOCK;
  }
  
  LOG3(("SpdySession %p will write from SpdyStream %p", this, stream));

  NS_ABORT_IF_FALSE(!mSegmentReader || !reader || (mSegmentReader == reader),
                    "Inconsistent Write Function Callback");

  if (reader)
    mSegmentReader = reader;
  rv = stream->ReadSegments(this, count, countRead);

  FlushOutputQueue();

  if (stream->BlockedOnWrite()) {

    
    
    

    LOG3(("SpdySession::ReadSegments %p dealing with block on write", this));

    NS_ABORT_IF_FALSE(!mPartialFrame, "partial frame should be empty");

    mPartialFrame = stream;
    SetWriteCallbacks(stream->Transaction());
    return rv;
  }

  if (stream->RequestBlockedOnRead()) {
    
    
    
    
    
    LOG3(("SpdySession::ReadSegments %p dealing with block on read", this));

    
    
    if (WriteQueueSize())
      rv = NS_OK;
    else
      rv = NS_BASE_STREAM_WOULD_BLOCK;
    SetWriteCallbacks(stream->Transaction());
    return rv;
  }
  
  NS_ABORT_IF_FALSE(rv != NS_BASE_STREAM_WOULD_BLOCK,
                    "Stream Would Block inconsistency");
  
  if (NS_FAILED(rv)) {
    LOG3(("SpdySession::ReadSegments %p returning FAIL code %X",
          this, rv));
    return rv;
  }
  
  if (*countRead > 0) {
    LOG3(("SpdySession::ReadSegments %p stream=%p generated end of frame %d",
          this, stream, *countRead));
    mReadyForWrite.Push(stream);
    SetWriteCallbacks(stream->Transaction());
    return rv;
  }
  
  LOG3(("SpdySession::ReadSegments %p stream=%p stream send complete",
        this, stream));
  
  
  
  stream->Transaction()->
    OnTransportStatus(mSocketTransport, nsISocketTransport::STATUS_WAITING_FOR,
                      LL_ZERO);
  
  mConnection->ResumeRecv(stream->Transaction());

  
  
  SetWriteCallbacks(stream->Transaction());

  return rv;
}














nsresult
SpdySession::WriteSegments(nsAHttpSegmentWriter *writer,
                           PRUint32 count,
                           PRUint32 *countWritten)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  nsresult rv;
  *countWritten = 0;

  if (mClosed)
    return NS_ERROR_FAILURE;

  SetWriteCallbacks(nsnull);
  
  
  
  
  
  if (mDownstreamState == BUFFERING_FRAME_HEADER) {
    
    
    
    
    NS_ABORT_IF_FALSE(mFrameBufferUsed < 8,
                      "Frame Buffer Used Too Large for State");

    rv = writer->OnWriteSegment(mFrameBuffer + mFrameBufferUsed,
                                8 - mFrameBufferUsed,
                                countWritten);
    if (NS_FAILED(rv)) {
      if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        ResumeRecv(nsnull);
      }
      return rv;
    }

    LogIO(this, nsnull, "Reading Frame Header",
          mFrameBuffer + mFrameBufferUsed, *countWritten);

    mFrameBufferUsed += *countWritten;

    if (mFrameBufferUsed < 8)
    {
      LOG3(("SpdySession::WriteSegments %p "
            "BUFFERING FRAME HEADER incomplete size=%d",
            this, mFrameBufferUsed));
      return rv;
    }

    
    
    mFrameDataSize =
      PR_ntohl(reinterpret_cast<PRUint32 *>(mFrameBuffer.get())[1]);
    mFrameDataSize &= 0x00ffffff;
    mFrameDataRead = 0;
    
    if (mFrameBuffer[0] & kFlag_Control) {
      EnsureBuffer(mFrameBuffer, mFrameDataSize + 8, 8, mFrameBufferSize);
      ChangeDownstreamState(BUFFERING_CONTROL_FRAME);
      
      
      
      PRUint16 version =
        PR_ntohs(reinterpret_cast<PRUint16 *>(mFrameBuffer.get())[0]);
      version &= 0x7fff;
      
      mFrameControlType =
        PR_ntohs(reinterpret_cast<PRUint16 *>(mFrameBuffer.get())[1]);
      
      LOG3(("SpdySession::WriteSegments %p - Control Frame Identified "
            "type %d version %d data len %d",
            this, mFrameControlType, version, mFrameDataSize));

      if (mFrameControlType >= CONTROL_TYPE_LAST ||
          mFrameControlType <= CONTROL_TYPE_FIRST)
        return NS_ERROR_ILLEGAL_VALUE;

      
      
      
      if (version != 2) {
        return NS_ERROR_ILLEGAL_VALUE;
      }
    }
    else {
      ChangeDownstreamState(PROCESSING_DATA_FRAME);

      PRUint32 streamID =
        PR_ntohl(reinterpret_cast<PRUint32 *>(mFrameBuffer.get())[0]);
      mFrameDataStream = mStreamIDHash.Get(streamID);
      if (!mFrameDataStream) {
        LOG3(("SpdySession::WriteSegments %p lookup streamID 0x%X failed. "
              "Next = 0x%x", this, streamID, mNextStreamID));
        if (streamID >= mNextStreamID)
          GenerateRstStream(RST_INVALID_STREAM, streamID);
          ChangeDownstreamState(DISCARD_DATA_FRAME);
      }
      mFrameDataLast = (mFrameBuffer[4] & kFlag_Data_FIN);
      Telemetry::Accumulate(Telemetry::SPDY_CHUNK_RECVD, mFrameDataSize >> 10);
      LOG3(("Start Processing Data Frame. "
            "Session=%p Stream ID 0x%x Stream Ptr %p Fin=%d Len=%d",
            this, streamID, mFrameDataStream, mFrameDataLast, mFrameDataSize));

      if (mFrameBuffer[4] & kFlag_Data_ZLIB) {
        LOG3(("Data flag has ZLIB flag set which is not valid >=2 spdy"));
        return NS_ERROR_ILLEGAL_VALUE;
      }
    }
  }

  if (mDownstreamState == PROCESSING_CONTROL_RST_STREAM) {
    if (mDownstreamRstReason == RST_REFUSED_STREAM)
      rv = NS_ERROR_NET_RESET;            
    else if (mDownstreamRstReason == RST_CANCEL ||
             mDownstreamRstReason == RST_PROTOCOL_ERROR ||
             mDownstreamRstReason == RST_INTERNAL_ERROR ||
             mDownstreamRstReason == RST_UNSUPPORTED_VERSION)
      rv = NS_ERROR_NET_INTERRUPT;
    else
      rv = NS_ERROR_ILLEGAL_VALUE;

    if (mDownstreamRstReason != RST_REFUSED_STREAM &&
        mDownstreamRstReason != RST_CANCEL)
      mShouldGoAway = true;

    
    SpdyStream *stream = mFrameDataStream;
    ChangeDownstreamState(BUFFERING_FRAME_HEADER);
    CleanupStream(stream, rv);
    return NS_OK;
  }

  if (mDownstreamState == PROCESSING_DATA_FRAME ||
      mDownstreamState == PROCESSING_CONTROL_SYN_REPLY) {

    mSegmentWriter = writer;
    rv = mFrameDataStream->WriteSegments(this, count, countWritten);
    mSegmentWriter = nsnull;

    if (rv == NS_BASE_STREAM_CLOSED) {
      
      
      SpdyStream *stream = mFrameDataStream;
      if (mFrameDataRead == mFrameDataSize)
        ChangeDownstreamState(BUFFERING_FRAME_HEADER);
      CleanupStream(stream, NS_OK);
      NS_ABORT_IF_FALSE(!mNeedsCleanup, "double cleanup out of data frame");
      return NS_OK;
    }
    
    if (mNeedsCleanup) {
      CleanupStream(mNeedsCleanup, NS_OK);
      mNeedsCleanup = nsnull;
    }

    

    return rv;
  }

  if (mDownstreamState == DISCARD_DATA_FRAME) {
    char trash[4096];
    PRUint32 count = NS_MIN(4096U, mFrameDataSize - mFrameDataRead);

    if (!count) {
      ChangeDownstreamState(BUFFERING_FRAME_HEADER);
      *countWritten = 1;
      return NS_OK;
    }

    rv = writer->OnWriteSegment(trash, count, countWritten);

    if (NS_FAILED(rv)) {
      
      ResumeRecv(nsnull);
      return rv;
    }

    LogIO(this, nsnull, "Discarding Frame", trash, *countWritten);

    mFrameDataRead += *countWritten;

    if (mFrameDataRead == mFrameDataSize)
      ChangeDownstreamState(BUFFERING_FRAME_HEADER);
    return rv;
  }
  
  NS_ABORT_IF_FALSE(mDownstreamState == BUFFERING_CONTROL_FRAME,
                    "Not in Bufering Control Frame State");
  NS_ABORT_IF_FALSE(mFrameBufferUsed == 8,
                    "Frame Buffer Header Not Present");

  rv = writer->OnWriteSegment(mFrameBuffer + 8 + mFrameDataRead,
                              mFrameDataSize - mFrameDataRead,
                              countWritten);
  if (NS_FAILED(rv)) {
    
    ResumeRecv(nsnull);
    return rv;
  }

  LogIO(this, nsnull, "Reading Control Frame",
        mFrameBuffer + 8 + mFrameDataRead, *countWritten);

  mFrameDataRead += *countWritten;

  if (mFrameDataRead != mFrameDataSize)
    return NS_OK;

  rv = sControlFunctions[mFrameControlType](this);

  NS_ABORT_IF_FALSE(NS_FAILED(rv) ||
                    mDownstreamState != BUFFERING_CONTROL_FRAME,
                    "Control Handler returned OK but did not change state");

  if (mShouldGoAway && !mStreamTransactionHash.Count())
    Close(NS_OK);
  return rv;
}

void
SpdySession::Close(nsresult aReason)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  if (mClosed)
    return;

  LOG3(("SpdySession::Close %p %X", this, aReason));

  mClosed = true;
  mStreamTransactionHash.Enumerate(Shutdown, this);
  GenerateGoAway();
  mConnection = nsnull;
}

void
SpdySession::CloseTransaction(nsAHttpTransaction *aTransaction,
                              nsresult aResult)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession::CloseTransaction %p %p %x", this, aTransaction, aResult));

  

  
  SpdyStream *stream = mStreamTransactionHash.Get(aTransaction);
  if (!stream) {
    LOG3(("SpdySession::CloseTransaction %p %p %x - not found.",
          this, aTransaction, aResult));
    return;
  }
  LOG3(("SpdySession::CloseTranscation probably a cancel. "
        "this=%p, trans=%p, result=%x, streamID=0x%X stream=%p",
        this, aTransaction, aResult, stream->StreamID(), stream));
  CleanupStream(stream, aResult);
  ResumeRecv(this);
}






nsresult
SpdySession::OnReadSegment(const char *buf,
                           PRUint32 count,
                           PRUint32 *countRead)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  nsresult rv;
  
  if (!mOutputQueueUsed && mSegmentReader) {

    
    rv = mSegmentReader->OnReadSegment(buf, count, countRead);
    if (NS_SUCCEEDED(rv) || (rv != NS_BASE_STREAM_WOULD_BLOCK))
      return rv;
  }
  
  if (mOutputQueueUsed + count > mOutputQueueSize)
    FlushOutputQueue();

  if (mOutputQueueUsed + count > mOutputQueueSize)
    count = mOutputQueueSize - mOutputQueueUsed;

  if (!count)
    return NS_BASE_STREAM_WOULD_BLOCK;
  
  memcpy(mOutputQueueBuffer.get() + mOutputQueueUsed, buf, count);
  mOutputQueueUsed += count;
  *countRead = count;

  FlushOutputQueue();
    
  return NS_OK;
}





nsresult
SpdySession::OnWriteSegment(char *buf,
                            PRUint32 count,
                            PRUint32 *countWritten)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(mSegmentWriter, "OnWriteSegment with null mSegmentWriter");
  nsresult rv;

  if (mDownstreamState == PROCESSING_DATA_FRAME) {

    if (mFrameDataLast &&
        mFrameDataRead == mFrameDataSize) {
      
      mNeedsCleanup = mFrameDataStream;

      LOG3(("SpdySession::OnWriteSegment %p - recorded downstream fin of "
            "stream %p 0x%X", this, mFrameDataStream,
            mFrameDataStream->StreamID()));
      *countWritten = 0;
      ChangeDownstreamState(BUFFERING_FRAME_HEADER);
      return NS_BASE_STREAM_CLOSED;
    }
    
    count = NS_MIN(count, mFrameDataSize - mFrameDataRead);
    rv = mSegmentWriter->OnWriteSegment(buf, count, countWritten);
    if (NS_FAILED(rv))
      return rv;

    LogIO(this, mFrameDataStream, "Reading Data Frame", buf, *countWritten);

    mFrameDataRead += *countWritten;
    
    if ((mFrameDataRead == mFrameDataSize) && !mFrameDataLast)
      ChangeDownstreamState(BUFFERING_FRAME_HEADER);

    return rv;
  }
  
  if (mDownstreamState == PROCESSING_CONTROL_SYN_REPLY) {
    
    if (mFlatHTTPResponseHeaders.Length() == mFlatHTTPResponseHeadersOut &&
        mFrameDataLast) {
      *countWritten = 0;
      ChangeDownstreamState(BUFFERING_FRAME_HEADER);
      return NS_BASE_STREAM_CLOSED;
    }
      
    count = NS_MIN(count,
                   mFlatHTTPResponseHeaders.Length() -
                   mFlatHTTPResponseHeadersOut);
    memcpy(buf,
           mFlatHTTPResponseHeaders.get() + mFlatHTTPResponseHeadersOut,
           count);
    mFlatHTTPResponseHeadersOut += count;
    *countWritten = count;

    if (mFlatHTTPResponseHeaders.Length() == mFlatHTTPResponseHeadersOut &&
        !mFrameDataLast)
      ChangeDownstreamState(BUFFERING_FRAME_HEADER);
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}





nsresult
SpdySession::ResumeSend(nsAHttpTransaction *caller)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession::ResumeSend %p caller=%p", this, caller));

  
  

  if (!mConnection)
    return NS_ERROR_FAILURE;

  SpdyStream *stream = mStreamTransactionHash.Get(caller);
  if (stream)
    mReadyForWrite.Push(stream);
  else
    LOG3(("SpdySession::ResumeSend %p caller %p not found", this, caller));
  
  return mConnection->ResumeSend(caller);
}

nsresult
SpdySession::ResumeRecv(nsAHttpTransaction *caller)
{
  if (!mConnection)
    return NS_ERROR_FAILURE;

  return mConnection->ResumeRecv(caller);
}

bool
SpdySession::IsPersistent()
{
  return PR_TRUE;
}

nsresult
SpdySession::TakeTransport(nsISocketTransport **,
                           nsIAsyncInputStream **,
                           nsIAsyncOutputStream **)
{
  NS_ABORT_IF_FALSE(false, "TakeTransport of SpdySession");
  return NS_ERROR_UNEXPECTED;
}

nsHttpConnection *
SpdySession::TakeHttpConnection()
{
  NS_ABORT_IF_FALSE(false, "TakeHttpConnection of SpdySession");
  return nsnull;
}

nsISocketTransport *
SpdySession::Transport()
{
    if (!mConnection)
        return nsnull;
    return mConnection->Transport();
}







void
SpdySession::SetConnection(nsAHttpConnection *)
{
  
  NS_ABORT_IF_FALSE(false, "SpdySession::SetConnection()");
}

void
SpdySession::GetSecurityCallbacks(nsIInterfaceRequestor **,
                                  nsIEventTarget **)
{
  
  NS_ABORT_IF_FALSE(false, "SpdySession::GetSecurityCallbacks()");
}

void
SpdySession::SetSSLConnectFailed()
{
  NS_ABORT_IF_FALSE(false, "SpdySession::SetSSLConnectFailed()");
}

bool
SpdySession::IsDone()
{
  NS_ABORT_IF_FALSE(false, "SpdySession::IsDone()");
  return PR_FALSE;
}

nsresult
SpdySession::Status()
{
  NS_ABORT_IF_FALSE(false, "SpdySession::Status()");
  return NS_ERROR_UNEXPECTED;
}

PRUint32
SpdySession::Available()
{
  NS_ABORT_IF_FALSE(false, "SpdySession::Available()");
  return 0;
}

nsHttpRequestHead *
SpdySession::RequestHead()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(false,
                    "SpdySession::RequestHead() "
                    "should not be called after SPDY is setup");
  return NULL;
}





nsAHttpConnection *
SpdySession::Connection()
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    return mConnection;
}

nsresult
SpdySession::OnHeadersAvailable(nsAHttpTransaction *transaction,
                                nsHttpRequestHead *requestHead,
                                nsHttpResponseHead *responseHead,
                                bool *reset)
{
  return mConnection->OnHeadersAvailable(transaction,
                                         requestHead,
                                         responseHead,
                                         reset);
}

void
SpdySession::GetConnectionInfo(nsHttpConnectionInfo **connInfo)
{
  mConnection->GetConnectionInfo(connInfo);
}

void
SpdySession::GetSecurityInfo(nsISupports **supports)
{
  mConnection->GetSecurityInfo(supports);
}

bool
SpdySession::IsReused()
{
  return mConnection->IsReused();
}

nsresult
SpdySession::PushBack(const char *buf, PRUint32 len)
{
  return mConnection->PushBack(buf, len);
}

bool
SpdySession::LastTransactionExpectedNoContent()
{
  return mConnection->LastTransactionExpectedNoContent();
}

void
SpdySession::SetLastTransactionExpectedNoContent(bool val)
{
  mConnection->SetLastTransactionExpectedNoContent(val);
}

} 
} 

