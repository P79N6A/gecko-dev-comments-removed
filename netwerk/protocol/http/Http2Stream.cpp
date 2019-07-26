






#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include <algorithm>

#include "Http2Compression.h"
#include "Http2Session.h"
#include "Http2Stream.h"
#include "Http2Push.h"

#include "mozilla/Telemetry.h"
#include "nsAlgorithm.h"
#include "nsHttp.h"
#include "nsHttpHandler.h"
#include "nsHttpRequestHead.h"
#include "nsISocketTransport.h"
#include "prnetdb.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {

Http2Stream::Http2Stream(nsAHttpTransaction *httpTransaction,
                         Http2Session *session,
                         int32_t priority)
  : mStreamID(0),
    mSession(session),
    mUpstreamState(GENERATING_HEADERS),
    mState(IDLE),
    mAllHeadersSent(0),
    mAllHeadersReceived(0),
    mTransaction(httpTransaction),
    mSocketTransport(session->SocketTransport()),
    mSegmentReader(nullptr),
    mSegmentWriter(nullptr),
    mChunkSize(session->SendingChunkSize()),
    mRequestBlockedOnRead(0),
    mRecvdFin(0),
    mRecvdReset(0),
    mSentReset(0),
    mCountAsActive(0),
    mSentFin(0),
    mSentWaitingFor(0),
    mSetTCPSocketBuffer(0),
    mTxInlineFrameSize(Http2Session::kDefaultBufferSize),
    mTxInlineFrameUsed(0),
    mTxStreamFrameSize(0),
    mRequestBodyLenRemaining(0),
    mLocalUnacked(0),
    mBlockedOnRwin(false),
    mTotalSent(0),
    mTotalRead(0),
    mPushSource(nullptr)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  LOG3(("Http2Stream::Http2Stream %p", this));

  mServerReceiveWindow = session->GetServerInitialStreamWindow();
  mClientReceiveWindow = session->PushAllowance();

  mTxInlineFrame = new uint8_t[mTxInlineFrameSize];

  PR_STATIC_ASSERT(nsISupportsPriority::PRIORITY_LOWEST <= kNormalPriority);

  
  
  
  int32_t httpPriority;
  if (priority >= nsISupportsPriority::PRIORITY_LOWEST) {
    httpPriority = kWorstPriority;
  } else if (priority <= nsISupportsPriority::PRIORITY_HIGHEST) {
    httpPriority = kBestPriority;
  } else {
    httpPriority = kNormalPriority + priority;
  }
  MOZ_ASSERT(httpPriority >= 0);
  mPriority = static_cast<uint32_t>(httpPriority);
}

Http2Stream::~Http2Stream()
{
  mStreamID = Http2Session::kDeadStreamID;
}






nsresult
Http2Stream::ReadSegments(nsAHttpSegmentReader *reader,
                          uint32_t count,
                          uint32_t *countRead)
{
  LOG3(("Http2Stream %p ReadSegments reader=%p count=%d state=%x",
        this, reader, count, mUpstreamState));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  nsresult rv = NS_ERROR_UNEXPECTED;
  mRequestBlockedOnRead = 0;

  if (mRecvdFin || mRecvdReset) {
    
    LOG3(("Http2Stream %p ReadSegments request stream aborted due to"
          " response side closure\n", this));
    return NS_ERROR_ABORT;
  }

  
  
  if (count > (mChunkSize + 8)) {
    uint32_t numchunks = count / (mChunkSize + 8);
    count = numchunks * (mChunkSize + 8);
  }

  switch (mUpstreamState) {
  case GENERATING_HEADERS:
  case GENERATING_BODY:
  case SENDING_BODY:
    
    
    mSegmentReader = reader;
    rv = mTransaction->ReadSegments(this, count, countRead);
    mSegmentReader = nullptr;

    
    
    if (NS_SUCCEEDED(rv) &&
        mUpstreamState == GENERATING_HEADERS &&
        !mAllHeadersSent)
      mSession->TransactionHasDataToWrite(this);

    
    
    
    
    

    
    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK && !mTxInlineFrameUsed)
      mRequestBlockedOnRead = 1;

    
    
    if (!mBlockedOnRwin &&
        !mTxInlineFrameUsed && NS_SUCCEEDED(rv) && (!*countRead)) {
      LOG3(("Http2Stream::ReadSegments %p 0x%X: Sending request data complete, "
            "mUpstreamState=%x",this, mStreamID, mUpstreamState));
      if (mSentFin) {
        ChangeState(UPSTREAM_COMPLETE);
      } else {
        GenerateDataFrameHeader(0, true);
        ChangeState(SENDING_FIN_STREAM);
        mSession->TransactionHasDataToWrite(this);
        rv = NS_BASE_STREAM_WOULD_BLOCK;
      }
    }
    break;

  case SENDING_FIN_STREAM:
    
    
    if (!mSentFin) {
      mSegmentReader = reader;
      rv = TransmitFrame(nullptr, nullptr, false);
      mSegmentReader = nullptr;
      MOZ_ASSERT(NS_FAILED(rv) || !mTxInlineFrameUsed,
                 "Transmit Frame should be all or nothing");
      if (NS_SUCCEEDED(rv))
        ChangeState(UPSTREAM_COMPLETE);
    } else {
      rv = NS_OK;
      mTxInlineFrameUsed = 0;         
      ChangeState(UPSTREAM_COMPLETE);
    }

    *countRead = 0;

    
    break;

  case UPSTREAM_COMPLETE:
    *countRead = 0;
    rv = NS_OK;
    break;

  default:
    MOZ_ASSERT(false, "Http2Stream::ReadSegments unknown state");
    break;
  }

  return rv;
}





nsresult
Http2Stream::WriteSegments(nsAHttpSegmentWriter *writer,
                           uint32_t count,
                           uint32_t *countWritten)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(!mSegmentWriter, "segment writer in progress");

  LOG3(("Http2Stream::WriteSegments %p count=%d state=%x",
        this, count, mUpstreamState));

  mSegmentWriter = writer;
  nsresult rv = mTransaction->WriteSegments(this, count, countWritten);
  mSegmentWriter = nullptr;

  return rv;
}

void
Http2Stream::CreatePushHashKey(const nsCString &scheme,
                               const nsCString &hostHeader,
                               uint64_t serial,
                               const nsCSubstring &pathInfo,
                               nsCString &outOrigin,
                               nsCString &outKey)
{
  outOrigin = scheme;
  outOrigin.Append(NS_LITERAL_CSTRING("://"));
  outOrigin.Append(hostHeader);

  outKey = outOrigin;
  outKey.Append(NS_LITERAL_CSTRING("/[http2."));
  outKey.AppendInt(serial);
  outKey.Append(NS_LITERAL_CSTRING("]"));
  outKey.Append(pathInfo);
}

nsresult
Http2Stream::ParseHttpRequestHeaders(const char *buf,
                                     uint32_t avail,
                                     uint32_t *countUsed)
{
  
  

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(mUpstreamState == GENERATING_HEADERS);

  LOG3(("Http2Stream::ParseHttpRequestHeaders %p avail=%d state=%x",
        this, avail, mUpstreamState));

  mFlatHttpRequestHeaders.Append(buf, avail);

  
  
  int32_t endHeader = mFlatHttpRequestHeaders.Find("\r\n\r\n");

  if (endHeader == kNotFound) {
    
    LOG3(("Http2Stream::ParseHttpRequestHeaders %p "
          "Need more header bytes. Len = %d",
          this, mFlatHttpRequestHeaders.Length()));
    *countUsed = avail;
    return NS_OK;
  }

  
  
  
  uint32_t oldLen = mFlatHttpRequestHeaders.Length();
  mFlatHttpRequestHeaders.SetLength(endHeader + 2);
  *countUsed = avail - (oldLen - endHeader) + 4;
  mAllHeadersSent = 1;

  nsCString hostHeader;
  nsCString hashkey;
  mTransaction->RequestHead()->GetHeader(nsHttp::Host, hostHeader);

  CreatePushHashKey(NS_LITERAL_CSTRING("https"),
                    hostHeader, mSession->Serial(),
                    mTransaction->RequestHead()->RequestURI(),
                    mOrigin, hashkey);

  
  if (mTransaction->RequestHead()->Method() == nsHttp::Get) {
    
    nsILoadGroupConnectionInfo *loadGroupCI = mTransaction->LoadGroupConnectionInfo();
    SpdyPushCache *cache = nullptr;
    if (loadGroupCI)
      loadGroupCI->GetSpdyPushCache(&cache);

    Http2PushedStream *pushedStream = nullptr;
    
    
    
    if (cache)
      pushedStream = cache->RemovePushedStreamHttp2(hashkey);

    if (pushedStream) {
      LOG3(("Pushed Stream Match located id=0x%X key=%s\n",
            pushedStream->StreamID(), hashkey.get()));
      pushedStream->SetConsumerStream(this);
      mPushSource = pushedStream;
      SetSentFin(true);
      AdjustPushedPriority();

      
      
      mSession->ConnectPushedStream(this);
      return NS_OK;
    }
  }

  
  
  
  mStreamID = mSession->RegisterStreamID(this);
  MOZ_ASSERT(mStreamID & 1, "Http2 Stream Channel ID must be odd");
  LOG3(("Stream ID 0x%X [session=%p] for URI %s\n",
        mStreamID, mSession,
        nsCString(mTransaction->RequestHead()->RequestURI()).get()));

  if (mStreamID >= 0x80000000) {
    
    
    
    
    
    
    
    
    LOG3(("Stream assigned out of range ID: 0x%X", mStreamID));
    return NS_ERROR_UNEXPECTED;
  }

  
  

  nsCString compressedData;
  mSession->Compressor()->EncodeHeaderBlock(mFlatHttpRequestHeaders,
                                            nsCString(mTransaction->RequestHead()->Method().get()),
                                            mTransaction->RequestHead()->RequestURI(),
                                            hostHeader,
                                            NS_LITERAL_CSTRING("https"),
                                            compressedData);

  
  
  uint8_t firstFrameFlags =  Http2Session::kFlag_PRIORITY;

  if (mTransaction->RequestHead()->Method() == nsHttp::Get ||
      mTransaction->RequestHead()->Method() == nsHttp::Connect ||
      mTransaction->RequestHead()->Method() == nsHttp::Head) {
    
    

    SetSentFin(true);
    firstFrameFlags |= Http2Session::kFlag_END_STREAM;
  } else if (mTransaction->RequestHead()->Method() == nsHttp::Post ||
             mTransaction->RequestHead()->Method() == nsHttp::Put ||
             mTransaction->RequestHead()->Method() == nsHttp::Options) {
    
  } else if (!mRequestBodyLenRemaining) {
    
    
    SetSentFin(true);
    firstFrameFlags |= Http2Session::kFlag_END_STREAM;
  }

  
  
  
  
  
  

  MOZ_ASSERT(!mTxInlineFrameUsed);

  uint32_t dataLength = compressedData.Length();
  uint32_t maxFrameData = Http2Session::kMaxFrameData - 4; 
  uint32_t numFrames = 1;

  if (dataLength > maxFrameData) {
    numFrames += ((dataLength - maxFrameData) + Http2Session::kMaxFrameData - 1) /
      Http2Session::kMaxFrameData;
    MOZ_ASSERT (numFrames > 1);
  }

  

  uint32_t messageSize = dataLength;
  messageSize += 12; 
  messageSize += (numFrames - 1) * 8; 

  Http2Session::EnsureBuffer(mTxInlineFrame,
                             dataLength + messageSize,
                             mTxInlineFrameUsed,
                             mTxInlineFrameSize);

  mTxInlineFrameUsed += messageSize;
  LOG3(("%p Generating %d bytes of HEADERS for stream 0x%X at priority %u frames %u\n",
        this, mTxInlineFrameUsed, mStreamID, mPriority, numFrames));

  uint32_t outputOffset = 0;
  uint32_t compressedDataOffset = 0;
  for (uint32_t idx = 0; idx < numFrames; ++idx) {
    uint32_t flags, frameLen;
    bool lastFrame = (idx == numFrames - 1);

    flags = 0;
    frameLen = maxFrameData;
    if (!idx) {
      flags |= firstFrameFlags;
      
      maxFrameData = Http2Session::kMaxFrameData;
    }
    if (lastFrame) {
      frameLen = dataLength;
      flags |= Http2Session::kFlag_END_HEADERS;
    }
    dataLength -= frameLen;

    mSession->CreateFrameHeader(
      mTxInlineFrame.get() + outputOffset,
      frameLen + (idx ? 0 : 4),
      (idx) ? Http2Session::FRAME_TYPE_CONTINUATION : Http2Session::FRAME_TYPE_HEADERS,
      flags, mStreamID);
    outputOffset += 8;

    if (!idx) {
      uint32_t priority = PR_htonl(mPriority);
      memcpy (mTxInlineFrame.get() + outputOffset, &priority, 4);
      outputOffset += 4;
    }

    memcpy(mTxInlineFrame.get() + outputOffset,
           compressedData.BeginReading() + compressedDataOffset, frameLen);
    compressedDataOffset += frameLen;
    outputOffset += frameLen;
  }

  Telemetry::Accumulate(Telemetry::SPDY_SYN_SIZE, compressedData.Length());

  
  uint32_t ratio =
    compressedData.Length() * 100 /
    (11 + mTransaction->RequestHead()->RequestURI().Length() +
     mFlatHttpRequestHeaders.Length());

  const char *beginBuffer = mFlatHttpRequestHeaders.BeginReading();
  int32_t crlfIndex = mFlatHttpRequestHeaders.Find("\r\n");
  while (true) {
    int32_t startIndex = crlfIndex + 2;

    crlfIndex = mFlatHttpRequestHeaders.Find("\r\n", false, startIndex);
    if (crlfIndex == -1)
      break;

    int32_t colonIndex = mFlatHttpRequestHeaders.Find(":", false, startIndex,
                                                      crlfIndex - startIndex);
    if (colonIndex == -1)
      break;

    nsDependentCSubstring name = Substring(beginBuffer + startIndex,
                                           beginBuffer + colonIndex);
    
    ToLowerCase(name);

    if (name.Equals("content-length")) {
      nsCString *val = new nsCString();
      int32_t valueIndex = colonIndex + 1;
      while (valueIndex < crlfIndex && beginBuffer[valueIndex] == ' ')
        ++valueIndex;

      nsDependentCSubstring v = Substring(beginBuffer + valueIndex,
                                          beginBuffer + crlfIndex);
      val->Append(v);

      int64_t len;
      if (nsHttp::ParseInt64(val->get(), nullptr, &len))
        mRequestBodyLenRemaining = len;
      break;
    }
  }

  mFlatHttpRequestHeaders.Truncate();
  Telemetry::Accumulate(Telemetry::SPDY_SYN_RATIO, ratio);
  return NS_OK;
}

void
Http2Stream::AdjustInitialWindow()
{
  
  
  
  

  
  
  
  Http2Stream *stream = this;
  if (!mStreamID) {
    MOZ_ASSERT(mPushSource);
    if (!mPushSource)
      return;
    stream = mPushSource;
    MOZ_ASSERT(stream->mStreamID);
    MOZ_ASSERT(!(stream->mStreamID & 1)); 

    
    
    if (stream->RecvdFin() || stream->RecvdReset())
      return;
  }

  uint8_t *packet = mTxInlineFrame.get() + mTxInlineFrameUsed;
  Http2Session::EnsureBuffer(mTxInlineFrame,
                             mTxInlineFrameUsed + 12,
                             mTxInlineFrameUsed,
                             mTxInlineFrameSize);
  mTxInlineFrameUsed += 12;

  mSession->CreateFrameHeader(packet, 4,
                              Http2Session::FRAME_TYPE_WINDOW_UPDATE,
                              0, stream->mStreamID);

  MOZ_ASSERT(mClientReceiveWindow <= ASpdySession::kInitialRwin);
  uint32_t bump = ASpdySession::kInitialRwin - mClientReceiveWindow;
  mClientReceiveWindow += bump;
  bump = PR_htonl(bump);
  memcpy(packet + 8, &bump, 4);
  LOG3(("AdjustInitialwindow increased flow control window %p 0x%X\n",
        this, stream->mStreamID));
}

void
Http2Stream::AdjustPushedPriority()
{
  
  

  if (mStreamID || !mPushSource)
    return;

  MOZ_ASSERT(mPushSource->mStreamID && !(mPushSource->mStreamID & 1));

  
  
  if (mPushSource->RecvdFin() || mPushSource->RecvdReset())
    return;

  uint8_t *packet = mTxInlineFrame.get() + mTxInlineFrameUsed;
  Http2Session::EnsureBuffer(mTxInlineFrame,
                             mTxInlineFrameUsed + 12,
                             mTxInlineFrameUsed,
                             mTxInlineFrameSize);
  mTxInlineFrameUsed += 12;

  mSession->CreateFrameHeader(packet, 4,
                              Http2Session::FRAME_TYPE_PRIORITY, 0,
                              mPushSource->mStreamID);

  uint32_t newPriority = PR_htonl(mPriority);
  mPushSource->SetPriority(newPriority);
  memcpy(packet + 8, &newPriority, 4);

  LOG3(("AdjustPushedPriority %p id 0x%X to %X\n", this, mPushSource->mStreamID,
        newPriority));
}

void
Http2Stream::UpdateTransportReadEvents(uint32_t count)
{
  mTotalRead += count;
  mTransaction->OnTransportStatus(mSocketTransport,
                                  NS_NET_STATUS_RECEIVING_FROM,
                                  mTotalRead);
}

void
Http2Stream::UpdateTransportSendEvents(uint32_t count)
{
  mTotalSent += count;

  
  
  
  
  
  
  
  
  
  
  
  uint32_t bufferSize = gHttpHandler->SpdySendBufferSize();
  if ((mTotalSent > bufferSize) && !mSetTCPSocketBuffer) {
    mSetTCPSocketBuffer = 1;
    mSocketTransport->SetSendBufferSize(bufferSize);
  }

  if (mUpstreamState != SENDING_FIN_STREAM)
    mTransaction->OnTransportStatus(mSocketTransport,
                                    NS_NET_STATUS_SENDING_TO,
                                    mTotalSent);

  if (!mSentWaitingFor && !mRequestBodyLenRemaining) {
    mSentWaitingFor = 1;
    mTransaction->OnTransportStatus(mSocketTransport,
                                    NS_NET_STATUS_WAITING_FOR,
                                    0);
  }
}

nsresult
Http2Stream::TransmitFrame(const char *buf,
                           uint32_t *countUsed,
                           bool forceCommitment)
{
  
  
  

  
  
  

  LOG3(("Http2Stream::TransmitFrame %p inline=%d stream=%d",
        this, mTxInlineFrameUsed, mTxStreamFrameSize));
  if (countUsed)
    *countUsed = 0;

  if (!mTxInlineFrameUsed) {
    MOZ_ASSERT(!buf);
    return NS_OK;
  }

  MOZ_ASSERT(mTxInlineFrameUsed, "empty stream frame in transmit");
  MOZ_ASSERT(mSegmentReader, "TransmitFrame with null mSegmentReader");
  MOZ_ASSERT((buf && countUsed) || (!buf && !countUsed),
             "TransmitFrame arguments inconsistent");

  uint32_t transmittedCount;
  nsresult rv;

  
  
  
  
  if (mTxStreamFrameSize && mTxInlineFrameUsed &&
      mTxStreamFrameSize < Http2Session::kDefaultBufferSize &&
      mTxInlineFrameUsed + mTxStreamFrameSize < mTxInlineFrameSize) {
    LOG3(("Coalesce Transmit"));
    memcpy (mTxInlineFrame + mTxInlineFrameUsed,
            buf, mTxStreamFrameSize);
    if (countUsed)
      *countUsed += mTxStreamFrameSize;
    mTxInlineFrameUsed += mTxStreamFrameSize;
    mTxStreamFrameSize = 0;
  }

  rv =
    mSegmentReader->CommitToSegmentSize(mTxStreamFrameSize + mTxInlineFrameUsed,
                                        forceCommitment);

  if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
    MOZ_ASSERT(!forceCommitment, "forceCommitment with WOULD_BLOCK");
    mSession->TransactionHasDataToWrite(this);
  }
  if (NS_FAILED(rv))     
    return rv;

  
  
  
  

  rv = mSession->BufferOutput(reinterpret_cast<char*>(mTxInlineFrame.get()),
                              mTxInlineFrameUsed,
                              &transmittedCount);
  LOG3(("Http2Stream::TransmitFrame for inline BufferOutput session=%p "
        "stream=%p result %x len=%d",
        mSession, this, rv, transmittedCount));

  MOZ_ASSERT(rv != NS_BASE_STREAM_WOULD_BLOCK,
             "inconsistent inline commitment result");

  if (NS_FAILED(rv))
    return rv;

  MOZ_ASSERT(transmittedCount == mTxInlineFrameUsed,
             "inconsistent inline commitment count");

  Http2Session::LogIO(mSession, this, "Writing from Inline Buffer",
                       reinterpret_cast<char*>(mTxInlineFrame.get()),
                       transmittedCount);

  if (mTxStreamFrameSize) {
    if (!buf) {
      
      MOZ_ASSERT(false, "Stream transmit with null buf argument to "
                 "TransmitFrame()");
      LOG3(("Stream transmit with null buf argument to TransmitFrame()\n"));
      return NS_ERROR_UNEXPECTED;
    }

    
    
    if (mSession->AmountOfOutputBuffered()) {
      rv = mSession->BufferOutput(buf, mTxStreamFrameSize,
                                  &transmittedCount);
    } else {
      rv = mSession->OnReadSegment(buf, mTxStreamFrameSize,
                                   &transmittedCount);
    }

    LOG3(("Http2Stream::TransmitFrame for regular session=%p "
          "stream=%p result %x len=%d",
          mSession, this, rv, transmittedCount));

    MOZ_ASSERT(rv != NS_BASE_STREAM_WOULD_BLOCK,
               "inconsistent stream commitment result");

    if (NS_FAILED(rv))
      return rv;

    MOZ_ASSERT(transmittedCount == mTxStreamFrameSize,
               "inconsistent stream commitment count");

    Http2Session::LogIO(mSession, this, "Writing from Transaction Buffer",
                         buf, transmittedCount);

    *countUsed += mTxStreamFrameSize;
  }

  mSession->FlushOutputQueue();

  
  UpdateTransportSendEvents(mTxInlineFrameUsed + mTxStreamFrameSize);

  mTxInlineFrameUsed = 0;
  mTxStreamFrameSize = 0;

  return NS_OK;
}

void
Http2Stream::ChangeState(enum upstreamStateType newState)
{
  LOG3(("Http2Stream::ChangeState() %p from %X to %X",
        this, mUpstreamState, newState));
  mUpstreamState = newState;
}

void
Http2Stream::GenerateDataFrameHeader(uint32_t dataLength, bool lastFrame)
{
  LOG3(("Http2Stream::GenerateDataFrameHeader %p len=%d last=%d",
        this, dataLength, lastFrame));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(!mTxInlineFrameUsed, "inline frame not empty");
  MOZ_ASSERT(!mTxStreamFrameSize, "stream frame not empty");

  uint8_t frameFlags = 0;
  if (lastFrame) {
    frameFlags |= Http2Session::kFlag_END_STREAM;
    if (dataLength)
      SetSentFin(true);
  }

  mSession->CreateFrameHeader(mTxInlineFrame.get(),
                              dataLength,
                              Http2Session::FRAME_TYPE_DATA,
                              frameFlags, mStreamID);

  mTxInlineFrameUsed = 8;
  mTxStreamFrameSize = dataLength;
}



nsresult
Http2Stream::ConvertResponseHeaders(Http2Decompressor *decompressor,
                                    nsACString &aHeadersIn,
                                    nsACString &aHeadersOut)
{
  aHeadersOut.Truncate();
  aHeadersOut.SetCapacity(aHeadersIn.Length() + 512);

  nsresult rv =
    decompressor->DecodeHeaderBlock(reinterpret_cast<const uint8_t *>(aHeadersIn.BeginReading()),
                                    aHeadersIn.Length(),
                                    aHeadersOut);
  if (NS_FAILED(rv)) {
    LOG3(("Http2Stream::ConvertHeaders %p decode Error\n", this));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsAutoCString status;
  decompressor->GetStatus(status);
  if (status.IsEmpty()) {
    LOG3(("Http2Stream::ConvertHeaders %p Error - no status\n", this));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (aHeadersIn.Length() && aHeadersOut.Length()) {
    Telemetry::Accumulate(Telemetry::SPDY_SYN_REPLY_SIZE, aHeadersIn.Length());
    uint32_t ratio =
      aHeadersIn.Length() * 100 / aHeadersOut.Length();
    Telemetry::Accumulate(Telemetry::SPDY_SYN_REPLY_RATIO, ratio);
  }

  aHeadersIn.Truncate();
  aHeadersOut.Append(NS_LITERAL_CSTRING("X-Firefox-Spdy: " NS_HTTP2_DRAFT_TOKEN "\r\n\r\n"));
  LOG (("decoded response headers are:\n%s", aHeadersOut.BeginReading()));
  return NS_OK;
}



nsresult
Http2Stream::ConvertPushHeaders(Http2Decompressor *decompressor,
                                nsACString &aHeadersIn,
                                nsACString &aHeadersOut)
{
  aHeadersOut.Truncate();
  nsresult rv =
    decompressor->DecodeHeaderBlock(reinterpret_cast<const uint8_t *>(aHeadersIn.BeginReading()),
                                    aHeadersIn.Length(),
                                    aHeadersOut);
  if (NS_FAILED(rv)) {
    LOG3(("Http2Stream::ConvertPushHeaders %p Error\n", this));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsCString method;
  decompressor->GetHost(mHeaderHost);
  decompressor->GetScheme(mHeaderScheme);
  decompressor->GetPath(mHeaderPath);

  if (mHeaderHost.IsEmpty() || mHeaderScheme.IsEmpty() || mHeaderPath.IsEmpty()) {
    LOG3(("Http2Stream::ConvertPushHeaders %p Error - missing required "
          "host=%s scheme=%s path=%s\n", this, mHeaderHost.get(), mHeaderScheme.get(),
          mHeaderPath.get()));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  decompressor->GetMethod(method);
  if (!method.Equals(NS_LITERAL_CSTRING("GET"))) {
    LOG3(("Http2Stream::ConvertPushHeaders %p Error - method not supported: %s\n",
          this, method.get()));
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  aHeadersIn.Truncate();
  LOG (("decoded push headers are:\n%s", aHeadersOut.BeginReading()));
  return NS_OK;
}

void
Http2Stream::Close(nsresult reason)
{
  mTransaction->Close(reason);
}

bool
Http2Stream::AllowFlowControlledWrite()
{
  if (!mSession->ServerUsesFlowControl())
    return true;

  return (mSession->ServerSessionWindow() > 0) && (mServerReceiveWindow > 0);
}

void
Http2Stream::UpdateServerReceiveWindow(int32_t delta)
{
  mServerReceiveWindow += delta;

  if (mBlockedOnRwin && AllowFlowControlledWrite()) {
    LOG3(("Http2Stream::UpdateServerReceived UnPause %p 0x%X "
          "Open stream window\n", this, mStreamID));
    mSession->TransactionHasDataToWrite(this);  }
}

void
Http2Stream::SetPriority(uint32_t newVal)
{
  mPriority = std::min(newVal, 0x7fffffffU);
}

void
Http2Stream::SetRecvdFin(bool aStatus)
{
  mRecvdFin = aStatus ? 1 : 0;
  if (!aStatus)
    return;

  if (mState == OPEN || mState == RESERVED_BY_REMOTE) {
    mState = CLOSED_BY_REMOTE;
  } else if (mState == CLOSED_BY_LOCAL) {
    mState = CLOSED;
  }
}

void
Http2Stream::SetSentFin(bool aStatus)
{
  mSentFin = aStatus ? 1 : 0;
  if (!aStatus)
    return;

  if (mState == OPEN || mState == RESERVED_BY_REMOTE) {
    mState = CLOSED_BY_LOCAL;
  } else if (mState == CLOSED_BY_REMOTE) {
    mState = CLOSED;
  }
}

void
Http2Stream::SetRecvdReset(bool aStatus)
{
  mRecvdReset = aStatus ? 1 : 0;
  if (!aStatus)
    return;
  mState = CLOSED;
}

void
Http2Stream::SetSentReset(bool aStatus)
{
  mSentReset = aStatus ? 1 : 0;
  if (!aStatus)
    return;
  mState = CLOSED;
}





nsresult
Http2Stream::OnReadSegment(const char *buf,
                           uint32_t count,
                           uint32_t *countRead)
{
  LOG3(("Http2Stream::OnReadSegment %p count=%d state=%x",
        this, count, mUpstreamState));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(mSegmentReader, "OnReadSegment with null mSegmentReader");

  nsresult rv = NS_ERROR_UNEXPECTED;
  uint32_t dataLength;

  switch (mUpstreamState) {
  case GENERATING_HEADERS:
    
    
    
    
    
    

    rv = ParseHttpRequestHeaders(buf, count, countRead);
    if (NS_FAILED(rv))
      return rv;
    LOG3(("ParseHttpRequestHeaders %p used %d of %d. complete = %d",
          this, *countRead, count, mAllHeadersSent));
    if (mAllHeadersSent) {
      SetHTTPState(OPEN);
      AdjustInitialWindow();
      
      rv = TransmitFrame(nullptr, nullptr, true);
      ChangeState(GENERATING_BODY);
      break;
    }
    MOZ_ASSERT(*countRead == count, "Header parsing not complete but unused data");
    break;

  case GENERATING_BODY:
    
    
    if (!AllowFlowControlledWrite()) {
      *countRead = 0;
      LOG3(("Http2Stream this=%p, id 0x%X request body suspended because "
            "remote window is stream=%ld session=%ld.\n", this, mStreamID,
            mServerReceiveWindow, mSession->ServerSessionWindow()));
      mBlockedOnRwin = true;
      return NS_BASE_STREAM_WOULD_BLOCK;
    }
    mBlockedOnRwin = false;

    
    
    
    dataLength = std::min(count, mChunkSize);

    if (dataLength > Http2Session::kMaxFrameData)
      dataLength = Http2Session::kMaxFrameData;

    if (mSession->ServerUsesFlowControl()) {
      if (dataLength > mSession->ServerSessionWindow())
        dataLength = static_cast<uint32_t>(mSession->ServerSessionWindow());

      if (dataLength > mServerReceiveWindow)
        dataLength = static_cast<uint32_t>(mServerReceiveWindow);
    }

    LOG3(("Http2Stream this=%p id 0x%X send calculation "
          "avail=%d chunksize=%d stream window=%d session window=%d "
          "max frame=%d USING=%d\n", this, mStreamID,
          count, mChunkSize, mServerReceiveWindow, mSession->ServerSessionWindow(),
          Http2Session::kMaxFrameData, dataLength));

    if (mSession->ServerUsesFlowControl()) {
      mSession->DecrementServerSessionWindow(dataLength);
      mServerReceiveWindow -= dataLength;
    }

    LOG3(("Http2Stream %p id %x request len remaining %d, "
          "count avail %d, chunk used %d",
          this, mStreamID, mRequestBodyLenRemaining, count, dataLength));
    if (dataLength > mRequestBodyLenRemaining)
      return NS_ERROR_UNEXPECTED;
    mRequestBodyLenRemaining -= dataLength;
    GenerateDataFrameHeader(dataLength, !mRequestBodyLenRemaining);
    ChangeState(SENDING_BODY);
    

  case SENDING_BODY:
    MOZ_ASSERT(mTxInlineFrameUsed, "OnReadSegment Send Data Header 0b");
    rv = TransmitFrame(buf, countRead, false);
    MOZ_ASSERT(NS_FAILED(rv) || !mTxInlineFrameUsed,
               "Transmit Frame should be all or nothing");

    LOG3(("TransmitFrame() rv=%x returning %d data bytes. "
          "Header is %d Body is %d.",
          rv, *countRead, mTxInlineFrameUsed, mTxStreamFrameSize));

    
    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK && *countRead)
      rv = NS_OK;

    
    if (!mTxInlineFrameUsed)
      ChangeState(GENERATING_BODY);
    break;

  case SENDING_FIN_STREAM:
    MOZ_ASSERT(false, "resuming partial fin stream out of OnReadSegment");
    break;

  default:
    MOZ_ASSERT(false, "Http2Stream::OnReadSegment non-write state");
    break;
  }

  return rv;
}





nsresult
Http2Stream::OnWriteSegment(char *buf,
                            uint32_t count,
                            uint32_t *countWritten)
{
  LOG3(("Http2Stream::OnWriteSegment %p count=%d state=%x 0x%X\n",
        this, count, mUpstreamState, mStreamID));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(mSegmentWriter);

  if (!mPushSource)
    return mSegmentWriter->OnWriteSegment(buf, count, countWritten);

  nsresult rv;
  rv = mPushSource->GetBufferedData(buf, count, countWritten);
  if (NS_FAILED(rv))
    return rv;

  mSession->ConnectPushedStream(this);
  return NS_OK;
}

} 
} 

