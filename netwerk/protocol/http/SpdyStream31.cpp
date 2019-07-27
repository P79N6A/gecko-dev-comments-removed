






#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include "mozilla/Telemetry.h"
#include "nsAlgorithm.h"
#include "nsHttp.h"
#include "nsHttpHandler.h"
#include "nsHttpRequestHead.h"
#include "nsISocketTransport.h"
#include "nsISupportsPriority.h"
#include "prnetdb.h"
#include "SpdyPush31.h"
#include "SpdySession31.h"
#include "SpdyStream31.h"
#include "TunnelUtils.h"

#include <algorithm>

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {

SpdyStream31::SpdyStream31(nsAHttpTransaction *httpTransaction,
                           SpdySession31 *spdySession,
                           int32_t priority)
  : mStreamID(0)
  , mSession(spdySession)
  , mUpstreamState(GENERATING_SYN_STREAM)
  , mRequestHeadersDone(0)
  , mSynFrameGenerated(0)
  , mSentFinOnData(0)
  , mQueued(0)
  , mTransaction(httpTransaction)
  , mSocketTransport(spdySession->SocketTransport())
  , mSegmentReader(nullptr)
  , mSegmentWriter(nullptr)
  , mChunkSize(spdySession->SendingChunkSize())
  , mRequestBlockedOnRead(0)
  , mRecvdFin(0)
  , mFullyOpen(0)
  , mSentWaitingFor(0)
  , mReceivedData(0)
  , mSetTCPSocketBuffer(0)
  , mCountAsActive(0)
  , mTxInlineFrameSize(SpdySession31::kDefaultBufferSize)
  , mTxInlineFrameUsed(0)
  , mTxStreamFrameSize(0)
  , mZlib(spdySession->UpstreamZlib())
  , mDecompressBufferSize(SpdySession31::kDefaultBufferSize)
  , mDecompressBufferUsed(0)
  , mDecompressedBytes(0)
  , mRequestBodyLenRemaining(0)
  , mPriority(priority)
  , mLocalUnacked(0)
  , mBlockedOnRwin(false)
  , mTotalSent(0)
  , mTotalRead(0)
  , mPushSource(nullptr)
  , mIsTunnel(false)
  , mPlainTextTunnel(false)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  LOG3(("SpdyStream31::SpdyStream31 %p", this));

  mRemoteWindow = spdySession->GetServerInitialStreamWindow();
  mLocalWindow = spdySession->PushAllowance();

  mTxInlineFrame = new uint8_t[mTxInlineFrameSize];
  mDecompressBuffer = new char[mDecompressBufferSize];
}

SpdyStream31::~SpdyStream31()
{
  ClearTransactionsBlockedOnTunnel();
  mStreamID = SpdySession31::kDeadStreamID;
}






nsresult
SpdyStream31::ReadSegments(nsAHttpSegmentReader *reader,
                           uint32_t count,
                           uint32_t *countRead)
{
  LOG3(("SpdyStream31 %p ReadSegments reader=%p count=%d state=%x",
        this, reader, count, mUpstreamState));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  nsresult rv = NS_ERROR_UNEXPECTED;
  mRequestBlockedOnRead = 0;

  
  
  if (count > (mChunkSize + 8)) {
    uint32_t numchunks = count / (mChunkSize + 8);
    count = numchunks * (mChunkSize + 8);
  }

  switch (mUpstreamState) {
  case GENERATING_SYN_STREAM:
  case GENERATING_REQUEST_BODY:
  case SENDING_REQUEST_BODY:
    
    
    mSegmentReader = reader;
    rv = mTransaction->ReadSegments(this, count, countRead);
    mSegmentReader = nullptr;
    LOG3(("SpdyStream31::ReadSegments %p trans readsegments rv %x read=%d\n",
          this, rv, *countRead));
    
    
    if (NS_SUCCEEDED(rv) &&
        mUpstreamState == GENERATING_SYN_STREAM &&
        !mRequestHeadersDone)
      mSession->TransactionHasDataToWrite(this);

    
    
    
    
    

    
    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK && !mTxInlineFrameUsed)
      mRequestBlockedOnRead = 1;

    
    
    
    if (mUpstreamState == GENERATING_SYN_STREAM && NS_SUCCEEDED(rv)) {
      LOG3(("SpdyStream31 %p ReadSegments forcing OnReadSegment call\n", this));
      uint32_t wasted = 0;
      mSegmentReader = reader;
      OnReadSegment("", 0, &wasted);
      mSegmentReader = nullptr;
    }

    
    
    if (!mBlockedOnRwin && mSynFrameGenerated &&
        !mTxInlineFrameUsed && NS_SUCCEEDED(rv) && (!*countRead)) {
      LOG3(("SpdyStream31::ReadSegments %p 0x%X: Sending request data complete, "
            "mUpstreamState=%x finondata=%d",this, mStreamID,
            mUpstreamState, mSentFinOnData));
      if (mSentFinOnData) {
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
    
    
    if (!mSentFinOnData) {
      mSegmentReader = reader;
      rv = TransmitFrame(nullptr, nullptr, false);
      mSegmentReader = nullptr;
      MOZ_ASSERT(NS_FAILED(rv) || !mTxInlineFrameUsed,
                 "Transmit Frame should be all or nothing");
      if (NS_SUCCEEDED(rv))
        ChangeState(UPSTREAM_COMPLETE);
    }
    else {
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
    MOZ_ASSERT(false, "SpdyStream31::ReadSegments unknown state");
    break;
  }

  return rv;
}





nsresult
SpdyStream31::WriteSegments(nsAHttpSegmentWriter *writer,
                            uint32_t count,
                            uint32_t *countWritten)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(!mSegmentWriter, "segment writer in progress");

  LOG3(("SpdyStream31::WriteSegments %p count=%d state=%x",
        this, count, mUpstreamState));

  mSegmentWriter = writer;
  nsresult rv = mTransaction->WriteSegments(this, count, countWritten);
  mSegmentWriter = nullptr;

  return rv;
}

PLDHashOperator
SpdyStream31::hdrHashEnumerate(const nsACString &key,
                               nsAutoPtr<nsCString> &value,
                               void *closure)
{
  SpdyStream31 *self = static_cast<SpdyStream31 *>(closure);

  self->CompressToFrame(key);
  self->CompressToFrame(value.get());
  return PL_DHASH_NEXT;
}

void
SpdyStream31::CreatePushHashKey(const nsCString &scheme,
                                const nsCString &hostHeader,
                                uint64_t serial,
                                const nsCSubstring &pathInfo,
                                nsCString &outOrigin,
                                nsCString &outKey)
{
  outOrigin = scheme;
  outOrigin.AppendLiteral("://");
  outOrigin.Append(hostHeader);

  outKey = outOrigin;
  outKey.AppendLiteral("/[spdy3_1.");
  outKey.AppendInt(serial);
  outKey.Append(']');
  outKey.Append(pathInfo);
}


nsresult
SpdyStream31::ParseHttpRequestHeaders(const char *buf,
                                      uint32_t avail,
                                      uint32_t *countUsed)
{
  
  

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(mUpstreamState == GENERATING_SYN_STREAM);
  MOZ_ASSERT(!mRequestHeadersDone);

  LOG3(("SpdyStream31::ParseHttpRequestHeaders %p avail=%d state=%x",
        this, avail, mUpstreamState));

  mFlatHttpRequestHeaders.Append(buf, avail);

  
  
  int32_t endHeader = mFlatHttpRequestHeaders.Find("\r\n\r\n");

  if (endHeader == kNotFound) {
    
    LOG3(("SpdyStream31::ParseHttpRequestHeaders %p "
          "Need more header bytes. Len = %d",
          this, mFlatHttpRequestHeaders.Length()));
    *countUsed = avail;
    return NS_OK;
  }

  
  
  
  uint32_t oldLen = mFlatHttpRequestHeaders.Length();
  mFlatHttpRequestHeaders.SetLength(endHeader + 2);
  *countUsed = avail - (oldLen - endHeader) + 4;
  mRequestHeadersDone = 1;

  nsAutoCString hostHeader;
  nsAutoCString hashkey;
  mTransaction->RequestHead()->GetHeader(nsHttp::Host, hostHeader);

  CreatePushHashKey(nsDependentCString(mTransaction->RequestHead()->IsHTTPS() ? "https" : "http"),
                    hostHeader, mSession->Serial(),
                    mTransaction->RequestHead()->RequestURI(),
                    mOrigin, hashkey);

  
  if (mTransaction->RequestHead()->IsGet()) {
    
    nsILoadGroupConnectionInfo *loadGroupCI = mTransaction->LoadGroupConnectionInfo();
    SpdyPushCache *cache = nullptr;
    if (loadGroupCI)
      loadGroupCI->GetSpdyPushCache(&cache);

    SpdyPushedStream31 *pushedStream = nullptr;
    
    
    
    if (cache)
      pushedStream = cache->RemovePushedStreamSpdy31(hashkey);

    if (pushedStream) {
      LOG3(("Pushed Stream Match located id=0x%X key=%s\n",
            pushedStream->StreamID(), hashkey.get()));
      pushedStream->SetConsumerStream(this);
      mPushSource = pushedStream;
      mSentFinOnData = 1;

      
      
      
      
      mSession->DecrementConcurrent(this);

      
      
      mSession->ConnectPushedStream(this);
      mSynFrameGenerated = 1;
      return NS_OK;
    }
  }
  return NS_OK;
}

nsresult
SpdyStream31::GenerateSynFrame()
{
  
  
  
  mStreamID = mSession->RegisterStreamID(this);
  MOZ_ASSERT(mStreamID & 1, "Spdy Stream Channel ID must be odd");
  MOZ_ASSERT(!mSynFrameGenerated);

  mSynFrameGenerated = 1;

  if (mStreamID >= 0x80000000) {
    
    
    
    
    
    
    
    
    LOG3(("Stream assigned out of range ID: 0x%X", mStreamID));
    return NS_ERROR_UNEXPECTED;
  }

  
  

  mTxInlineFrame[0] = SpdySession31::kFlag_Control;
  mTxInlineFrame[1] = SpdySession31::kVersion;
  mTxInlineFrame[2] = 0;
  mTxInlineFrame[3] = SpdySession31::CONTROL_TYPE_SYN_STREAM;
  

  uint32_t networkOrderID = PR_htonl(mStreamID);
  memcpy(mTxInlineFrame + 8, &networkOrderID, 4);

  
  
  memset (mTxInlineFrame + 12, 0, 4);

  
  
  

  if (mPriority >= nsISupportsPriority::PRIORITY_LOWEST)
    mTxInlineFrame[16] = 7 << 5;
  else if (mPriority <= nsISupportsPriority::PRIORITY_HIGHEST)
    mTxInlineFrame[16] = 0 << 5;
  else {
    
    
    PR_STATIC_ASSERT(nsISupportsPriority::PRIORITY_LOWEST == 20);
    PR_STATIC_ASSERT(nsISupportsPriority::PRIORITY_HIGHEST == -20);

    
    
    
    
    uint8_t calculatedPriority = 3 + ((mPriority + 1) / 5);
    MOZ_ASSERT (!(calculatedPriority & 0xf8),
                "Calculated Priority Out Of Range");
    mTxInlineFrame[16] = calculatedPriority << 5;
  }

  
  mTxInlineFrame[17] = 0;

  nsCString versionHeader;
  if (mTransaction->RequestHead()->Version() == NS_HTTP_VERSION_1_1)
    versionHeader = NS_LITERAL_CSTRING("HTTP/1.1");
  else
    versionHeader = NS_LITERAL_CSTRING("HTTP/1.0");

  
  
  
  nsClassHashtable<nsCStringHashKey, nsCString>
    hdrHash(mTransaction->RequestHead()->Headers().Count());

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

    
    if (name.EqualsLiteral("connection") ||
        name.EqualsLiteral("keep-alive") ||
        name.EqualsLiteral("host") ||
        name.EqualsLiteral("accept-encoding") ||
        name.EqualsLiteral("te") ||
        name.EqualsLiteral("transfer-encoding"))
      continue;

    nsCString *val = hdrHash.Get(name);
    if (!val) {
      val = new nsCString();
      hdrHash.Put(name, val);
    }

    int32_t valueIndex = colonIndex + 1;
    while (valueIndex < crlfIndex && beginBuffer[valueIndex] == ' ')
      ++valueIndex;

    nsDependentCSubstring v = Substring(beginBuffer + valueIndex,
                                        beginBuffer + crlfIndex);
    if (!val->IsEmpty())
      val->Append(static_cast<char>(0));
    val->Append(v);

    if (name.EqualsLiteral("content-length")) {
      int64_t len;
      if (nsHttp::ParseInt64(val->get(), nullptr, &len))
        mRequestBodyLenRemaining = len;
    }
  }

  mTxInlineFrameUsed = 18;

  
  
  

  const char *methodHeader = mTransaction->RequestHead()->Method().get();
  LOG3(("Stream method %p 0x%X %s\n", this, mStreamID, methodHeader));

  
  uint16_t count = hdrHash.Count() + 4; 
  if (mTransaction->RequestHead()->IsConnect()) {
    mRequestBodyLenRemaining = 0x0fffffffffffffffULL;
  } else {
    ++count; 
  }
  CompressToFrame(count);

  
  
  CompressToFrame(NS_LITERAL_CSTRING(":method"));
  CompressToFrame(methodHeader, strlen(methodHeader));

  CompressToFrame(NS_LITERAL_CSTRING(":path"));
  if (!mTransaction->RequestHead()->IsConnect()) {
    CompressToFrame(mTransaction->RequestHead()->Path());
  } else {
    MOZ_ASSERT(mTransaction->QuerySpdyConnectTransaction());
    mIsTunnel = true;
    
    nsHttpConnectionInfo *ci = mTransaction->ConnectionInfo();
    if (!ci) {
      return NS_ERROR_UNEXPECTED;
    }
    nsAutoCString route;
    route = ci->GetHost();
    route.Append(':');
    route.AppendInt(ci->Port());
    CompressToFrame(route);
  }

  CompressToFrame(NS_LITERAL_CSTRING(":version"));
  CompressToFrame(versionHeader);

  nsAutoCString hostHeader;
  mTransaction->RequestHead()->GetHeader(nsHttp::Host, hostHeader);
  CompressToFrame(NS_LITERAL_CSTRING(":host"));
  CompressToFrame(hostHeader);

  if (!mTransaction->RequestHead()->IsConnect()) {
    
    CompressToFrame(NS_LITERAL_CSTRING(":scheme"));
    CompressToFrame(nsDependentCString(mTransaction->RequestHead()->IsHTTPS() ? "https" : "http"));
  }

  hdrHash.Enumerate(hdrHashEnumerate, this);
  CompressFlushFrame();

  
  (reinterpret_cast<uint32_t *>(mTxInlineFrame.get()))[1] =
    PR_htonl(mTxInlineFrameUsed - 8);

  MOZ_ASSERT(!mTxInlineFrame[4], "Size greater than 24 bits");

  
  

  if (mTransaction->RequestHead()->IsGet() ||
      mTransaction->RequestHead()->IsHead()) {
    
    

    mSentFinOnData = 1;
    mTxInlineFrame[4] = SpdySession31::kFlag_Data_FIN;
  }
  else if (mTransaction->RequestHead()->IsPost() ||
           mTransaction->RequestHead()->IsPut() ||
           mTransaction->RequestHead()->IsConnect() ||
           mTransaction->RequestHead()->IsOptions()) {
    
    
  }
  else if (!mRequestBodyLenRemaining) {
    
    
    mSentFinOnData = 1;
    mTxInlineFrame[4] = SpdySession31::kFlag_Data_FIN;
  }

  Telemetry::Accumulate(Telemetry::SPDY_SYN_SIZE, mTxInlineFrameUsed - 18);

  
  uint32_t ratio =
    (mTxInlineFrameUsed - 18) * 100 /
    (11 + mTransaction->RequestHead()->RequestURI().Length() +
     mFlatHttpRequestHeaders.Length());

  Telemetry::Accumulate(Telemetry::SPDY_SYN_RATIO, ratio);
  return NS_OK;
}

void
SpdyStream31::AdjustInitialWindow()
{
  MOZ_ASSERT(mSession->PushAllowance() <= ASpdySession::kInitialRwin);

  
  
  

  
  
  
  SpdyStream31 *stream = this;
  if (!mStreamID) {
    MOZ_ASSERT(mPushSource);
    if (!mPushSource)
      return;
    stream = mPushSource;
    MOZ_ASSERT(stream->mStreamID);
    MOZ_ASSERT(!(stream->mStreamID & 1)); 

    
    
    if (stream->RecvdFin())
      return;
  }

  
  

  
  
  
  
  uint64_t toack64 = (ASpdySession::kInitialRwin - mSession->PushAllowance()) +
    stream->mLocalUnacked;
  stream->mLocalUnacked = 0;
  if (toack64 > 0x7fffffff) {
    stream->mLocalUnacked = toack64 - 0x7fffffff;
    toack64 = 0x7fffffff;
  }
  uint32_t toack = static_cast<uint32_t>(toack64);
  if (!toack)
    return;
  toack = PR_htonl(toack);

  EnsureBuffer(mTxInlineFrame, mTxInlineFrameUsed + 16,
               mTxInlineFrameUsed, mTxInlineFrameSize);

  unsigned char *packet = mTxInlineFrame.get() + mTxInlineFrameUsed;
  mTxInlineFrameUsed += 16;

  memset(packet, 0, 8);
  packet[0] = SpdySession31::kFlag_Control;
  packet[1] = SpdySession31::kVersion;
  packet[3] = SpdySession31::CONTROL_TYPE_WINDOW_UPDATE;
  packet[7] = 8; 

  uint32_t id = PR_htonl(stream->mStreamID);
  memcpy(packet + 8, &id, 4);
  memcpy(packet + 12, &toack, 4);

  stream->mLocalWindow += PR_ntohl(toack);
  LOG3(("AdjustInitialwindow %p 0x%X %u\n",
        this, stream->mStreamID, PR_ntohl(toack)));
}

void
SpdyStream31::UpdateTransportReadEvents(uint32_t count)
{
  mTotalRead += count;
  if (!mSocketTransport) {
    return;
  }

  mTransaction->OnTransportStatus(mSocketTransport,
                                  NS_NET_STATUS_RECEIVING_FROM,
                                  mTotalRead);
}

void
SpdyStream31::UpdateTransportSendEvents(uint32_t count)
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
SpdyStream31::TransmitFrame(const char *buf,
                            uint32_t *countUsed,
                            bool forceCommitment)
{
  
  
  

  
  
  

  LOG3(("SpdyStream31::TransmitFrame %p inline=%d stream=%d",
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
      mTxStreamFrameSize < SpdySession31::kDefaultBufferSize &&
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
  LOG3(("SpdyStream31::TransmitFrame for inline BufferOutput session=%p "
        "stream=%p result %x len=%d",
        mSession, this, rv, transmittedCount));

  MOZ_ASSERT(rv != NS_BASE_STREAM_WOULD_BLOCK,
             "inconsistent inline commitment result");

  if (NS_FAILED(rv))
    return rv;

  MOZ_ASSERT(transmittedCount == mTxInlineFrameUsed,
             "inconsistent inline commitment count");

  SpdySession31::LogIO(mSession, this, "Writing from Inline Buffer",
                       reinterpret_cast<char*>(mTxInlineFrame.get()),
                       transmittedCount);

  if (mTxStreamFrameSize) {
    if (!buf) {
      
      MOZ_ASSERT(false, "Stream transmit with null buf argument to "
                 "TransmitFrame()");
      LOG(("Stream transmit with null buf argument to TransmitFrame()\n"));
      return NS_ERROR_UNEXPECTED;
    }

    
    
    if (mSession->AmountOfOutputBuffered()) {
      rv = mSession->BufferOutput(buf, mTxStreamFrameSize,
                                  &transmittedCount);
    } else {
      rv = mSession->OnReadSegment(buf, mTxStreamFrameSize,
                                   &transmittedCount);
    }

    LOG3(("SpdyStream31::TransmitFrame for regular session=%p "
          "stream=%p result %x len=%d",
          mSession, this, rv, transmittedCount));

    MOZ_ASSERT(rv != NS_BASE_STREAM_WOULD_BLOCK,
               "inconsistent stream commitment result");

    if (NS_FAILED(rv))
      return rv;

    MOZ_ASSERT(transmittedCount == mTxStreamFrameSize,
               "inconsistent stream commitment count");

    SpdySession31::LogIO(mSession, this, "Writing from Transaction Buffer",
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
SpdyStream31::ChangeState(enum stateType newState)
{
  LOG3(("SpdyStream31::ChangeState() %p from %X to %X",
        this, mUpstreamState, newState));
  mUpstreamState = newState;
  return;
}

void
SpdyStream31::GenerateDataFrameHeader(uint32_t dataLength, bool lastFrame)
{
  LOG3(("SpdyStream31::GenerateDataFrameHeader %p len=%d last=%d id=0x%X\n",
        this, dataLength, lastFrame, mStreamID));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(!mTxInlineFrameUsed, "inline frame not empty");
  MOZ_ASSERT(!mTxStreamFrameSize, "stream frame not empty");
  MOZ_ASSERT(!(dataLength & 0xff000000), "datalength > 24 bits");
  MOZ_ASSERT(mStreamID != 0);
  MOZ_ASSERT(mStreamID != SpdySession31::kDeadStreamID);

  (reinterpret_cast<uint32_t *>(mTxInlineFrame.get()))[0] = PR_htonl(mStreamID);
  (reinterpret_cast<uint32_t *>(mTxInlineFrame.get()))[1] =
    PR_htonl(dataLength);

  MOZ_ASSERT(!(mTxInlineFrame[0] & 0x80), "control bit set unexpectedly");
  MOZ_ASSERT(!mTxInlineFrame[4], "flag bits set unexpectedly");

  mTxInlineFrameUsed = 8;
  mTxStreamFrameSize = dataLength;

  if (lastFrame) {
    mTxInlineFrame[4] |= SpdySession31::kFlag_Data_FIN;
    if (dataLength)
      mSentFinOnData = 1;
  }
}

void
SpdyStream31::CompressToFrame(const nsACString &str)
{
  CompressToFrame(str.BeginReading(), str.Length());
}

void
SpdyStream31::CompressToFrame(const nsACString *str)
{
  CompressToFrame(str->BeginReading(), str->Length());
}




const unsigned char SpdyStream31::kDictionary[] = {
  0x00, 0x00, 0x00, 0x07, 0x6f, 0x70, 0x74, 0x69,   
  0x6f, 0x6e, 0x73, 0x00, 0x00, 0x00, 0x04, 0x68,   
  0x65, 0x61, 0x64, 0x00, 0x00, 0x00, 0x04, 0x70,   
  0x6f, 0x73, 0x74, 0x00, 0x00, 0x00, 0x03, 0x70,   
  0x75, 0x74, 0x00, 0x00, 0x00, 0x06, 0x64, 0x65,   
  0x6c, 0x65, 0x74, 0x65, 0x00, 0x00, 0x00, 0x05,   
  0x74, 0x72, 0x61, 0x63, 0x65, 0x00, 0x00, 0x00,   
  0x06, 0x61, 0x63, 0x63, 0x65, 0x70, 0x74, 0x00,   
  0x00, 0x00, 0x0e, 0x61, 0x63, 0x63, 0x65, 0x70,   
  0x74, 0x2d, 0x63, 0x68, 0x61, 0x72, 0x73, 0x65,   
  0x74, 0x00, 0x00, 0x00, 0x0f, 0x61, 0x63, 0x63,   
  0x65, 0x70, 0x74, 0x2d, 0x65, 0x6e, 0x63, 0x6f,   
  0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00, 0x0f,   
  0x61, 0x63, 0x63, 0x65, 0x70, 0x74, 0x2d, 0x6c,   
  0x61, 0x6e, 0x67, 0x75, 0x61, 0x67, 0x65, 0x00,   
  0x00, 0x00, 0x0d, 0x61, 0x63, 0x63, 0x65, 0x70,   
  0x74, 0x2d, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x73,   
  0x00, 0x00, 0x00, 0x03, 0x61, 0x67, 0x65, 0x00,   
  0x00, 0x00, 0x05, 0x61, 0x6c, 0x6c, 0x6f, 0x77,   
  0x00, 0x00, 0x00, 0x0d, 0x61, 0x75, 0x74, 0x68,   
  0x6f, 0x72, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f,   
  0x6e, 0x00, 0x00, 0x00, 0x0d, 0x63, 0x61, 0x63,   
  0x68, 0x65, 0x2d, 0x63, 0x6f, 0x6e, 0x74, 0x72,   
  0x6f, 0x6c, 0x00, 0x00, 0x00, 0x0a, 0x63, 0x6f,   
  0x6e, 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e,   
  0x00, 0x00, 0x00, 0x0c, 0x63, 0x6f, 0x6e, 0x74,   
  0x65, 0x6e, 0x74, 0x2d, 0x62, 0x61, 0x73, 0x65,   
  0x00, 0x00, 0x00, 0x10, 0x63, 0x6f, 0x6e, 0x74,   
  0x65, 0x6e, 0x74, 0x2d, 0x65, 0x6e, 0x63, 0x6f,   
  0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00, 0x10,   
  0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d,   
  0x6c, 0x61, 0x6e, 0x67, 0x75, 0x61, 0x67, 0x65,   
  0x00, 0x00, 0x00, 0x0e, 0x63, 0x6f, 0x6e, 0x74,   
  0x65, 0x6e, 0x74, 0x2d, 0x6c, 0x65, 0x6e, 0x67,   
  0x74, 0x68, 0x00, 0x00, 0x00, 0x10, 0x63, 0x6f,   
  0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x6c, 0x6f,   
  0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00,   
  0x00, 0x0b, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e,   
  0x74, 0x2d, 0x6d, 0x64, 0x35, 0x00, 0x00, 0x00,   
  0x0d, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74,   
  0x2d, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x00, 0x00,   
  0x00, 0x0c, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e,   
  0x74, 0x2d, 0x74, 0x79, 0x70, 0x65, 0x00, 0x00,   
  0x00, 0x04, 0x64, 0x61, 0x74, 0x65, 0x00, 0x00,   
  0x00, 0x04, 0x65, 0x74, 0x61, 0x67, 0x00, 0x00,   
  0x00, 0x06, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74,   
  0x00, 0x00, 0x00, 0x07, 0x65, 0x78, 0x70, 0x69,   
  0x72, 0x65, 0x73, 0x00, 0x00, 0x00, 0x04, 0x66,   
  0x72, 0x6f, 0x6d, 0x00, 0x00, 0x00, 0x04, 0x68,   
  0x6f, 0x73, 0x74, 0x00, 0x00, 0x00, 0x08, 0x69,   
  0x66, 0x2d, 0x6d, 0x61, 0x74, 0x63, 0x68, 0x00,   
  0x00, 0x00, 0x11, 0x69, 0x66, 0x2d, 0x6d, 0x6f,   
  0x64, 0x69, 0x66, 0x69, 0x65, 0x64, 0x2d, 0x73,   
  0x69, 0x6e, 0x63, 0x65, 0x00, 0x00, 0x00, 0x0d,   
  0x69, 0x66, 0x2d, 0x6e, 0x6f, 0x6e, 0x65, 0x2d,   
  0x6d, 0x61, 0x74, 0x63, 0x68, 0x00, 0x00, 0x00,   
  0x08, 0x69, 0x66, 0x2d, 0x72, 0x61, 0x6e, 0x67,   
  0x65, 0x00, 0x00, 0x00, 0x13, 0x69, 0x66, 0x2d,   
  0x75, 0x6e, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x69,   
  0x65, 0x64, 0x2d, 0x73, 0x69, 0x6e, 0x63, 0x65,   
  0x00, 0x00, 0x00, 0x0d, 0x6c, 0x61, 0x73, 0x74,   
  0x2d, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x69, 0x65,   
  0x64, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63,   
  0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00,   
  0x0c, 0x6d, 0x61, 0x78, 0x2d, 0x66, 0x6f, 0x72,   
  0x77, 0x61, 0x72, 0x64, 0x73, 0x00, 0x00, 0x00,   
  0x06, 0x70, 0x72, 0x61, 0x67, 0x6d, 0x61, 0x00,   
  0x00, 0x00, 0x12, 0x70, 0x72, 0x6f, 0x78, 0x79,   
  0x2d, 0x61, 0x75, 0x74, 0x68, 0x65, 0x6e, 0x74,   
  0x69, 0x63, 0x61, 0x74, 0x65, 0x00, 0x00, 0x00,   
  0x13, 0x70, 0x72, 0x6f, 0x78, 0x79, 0x2d, 0x61,   
  0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x7a, 0x61,   
  0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x05,   
  0x72, 0x61, 0x6e, 0x67, 0x65, 0x00, 0x00, 0x00,   
  0x07, 0x72, 0x65, 0x66, 0x65, 0x72, 0x65, 0x72,   
  0x00, 0x00, 0x00, 0x0b, 0x72, 0x65, 0x74, 0x72,   
  0x79, 0x2d, 0x61, 0x66, 0x74, 0x65, 0x72, 0x00,   
  0x00, 0x00, 0x06, 0x73, 0x65, 0x72, 0x76, 0x65,   
  0x72, 0x00, 0x00, 0x00, 0x02, 0x74, 0x65, 0x00,   
  0x00, 0x00, 0x07, 0x74, 0x72, 0x61, 0x69, 0x6c,   
  0x65, 0x72, 0x00, 0x00, 0x00, 0x11, 0x74, 0x72,   
  0x61, 0x6e, 0x73, 0x66, 0x65, 0x72, 0x2d, 0x65,   
  0x6e, 0x63, 0x6f, 0x64, 0x69, 0x6e, 0x67, 0x00,   
  0x00, 0x00, 0x07, 0x75, 0x70, 0x67, 0x72, 0x61,   
  0x64, 0x65, 0x00, 0x00, 0x00, 0x0a, 0x75, 0x73,   
  0x65, 0x72, 0x2d, 0x61, 0x67, 0x65, 0x6e, 0x74,   
  0x00, 0x00, 0x00, 0x04, 0x76, 0x61, 0x72, 0x79,   
  0x00, 0x00, 0x00, 0x03, 0x76, 0x69, 0x61, 0x00,   
  0x00, 0x00, 0x07, 0x77, 0x61, 0x72, 0x6e, 0x69,   
  0x6e, 0x67, 0x00, 0x00, 0x00, 0x10, 0x77, 0x77,   
  0x77, 0x2d, 0x61, 0x75, 0x74, 0x68, 0x65, 0x6e,   
  0x74, 0x69, 0x63, 0x61, 0x74, 0x65, 0x00, 0x00,   
  0x00, 0x06, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64,   
  0x00, 0x00, 0x00, 0x03, 0x67, 0x65, 0x74, 0x00,   
  0x00, 0x00, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75,   
  0x73, 0x00, 0x00, 0x00, 0x06, 0x32, 0x30, 0x30,   
  0x20, 0x4f, 0x4b, 0x00, 0x00, 0x00, 0x07, 0x76,   
  0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x00, 0x00,   
  0x00, 0x08, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x31,   
  0x2e, 0x31, 0x00, 0x00, 0x00, 0x03, 0x75, 0x72,   
  0x6c, 0x00, 0x00, 0x00, 0x06, 0x70, 0x75, 0x62,   
  0x6c, 0x69, 0x63, 0x00, 0x00, 0x00, 0x0a, 0x73,   
  0x65, 0x74, 0x2d, 0x63, 0x6f, 0x6f, 0x6b, 0x69,   
  0x65, 0x00, 0x00, 0x00, 0x0a, 0x6b, 0x65, 0x65,   
  0x70, 0x2d, 0x61, 0x6c, 0x69, 0x76, 0x65, 0x00,   
  0x00, 0x00, 0x06, 0x6f, 0x72, 0x69, 0x67, 0x69,   
  0x6e, 0x31, 0x30, 0x30, 0x31, 0x30, 0x31, 0x32,   
  0x30, 0x31, 0x32, 0x30, 0x32, 0x32, 0x30, 0x35,   
  0x32, 0x30, 0x36, 0x33, 0x30, 0x30, 0x33, 0x30,   
  0x32, 0x33, 0x30, 0x33, 0x33, 0x30, 0x34, 0x33,   
  0x30, 0x35, 0x33, 0x30, 0x36, 0x33, 0x30, 0x37,   
  0x34, 0x30, 0x32, 0x34, 0x30, 0x35, 0x34, 0x30,   
  0x36, 0x34, 0x30, 0x37, 0x34, 0x30, 0x38, 0x34,   
  0x30, 0x39, 0x34, 0x31, 0x30, 0x34, 0x31, 0x31,   
  0x34, 0x31, 0x32, 0x34, 0x31, 0x33, 0x34, 0x31,   
  0x34, 0x34, 0x31, 0x35, 0x34, 0x31, 0x36, 0x34,   
  0x31, 0x37, 0x35, 0x30, 0x32, 0x35, 0x30, 0x34,   
  0x35, 0x30, 0x35, 0x32, 0x30, 0x33, 0x20, 0x4e,   
  0x6f, 0x6e, 0x2d, 0x41, 0x75, 0x74, 0x68, 0x6f,   
  0x72, 0x69, 0x74, 0x61, 0x74, 0x69, 0x76, 0x65,   
  0x20, 0x49, 0x6e, 0x66, 0x6f, 0x72, 0x6d, 0x61,   
  0x74, 0x69, 0x6f, 0x6e, 0x32, 0x30, 0x34, 0x20,   
  0x4e, 0x6f, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x65,   
  0x6e, 0x74, 0x33, 0x30, 0x31, 0x20, 0x4d, 0x6f,   
  0x76, 0x65, 0x64, 0x20, 0x50, 0x65, 0x72, 0x6d,   
  0x61, 0x6e, 0x65, 0x6e, 0x74, 0x6c, 0x79, 0x34,   
  0x30, 0x30, 0x20, 0x42, 0x61, 0x64, 0x20, 0x52,   
  0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x34, 0x30,   
  0x31, 0x20, 0x55, 0x6e, 0x61, 0x75, 0x74, 0x68,   
  0x6f, 0x72, 0x69, 0x7a, 0x65, 0x64, 0x34, 0x30,   
  0x33, 0x20, 0x46, 0x6f, 0x72, 0x62, 0x69, 0x64,   
  0x64, 0x65, 0x6e, 0x34, 0x30, 0x34, 0x20, 0x4e,   
  0x6f, 0x74, 0x20, 0x46, 0x6f, 0x75, 0x6e, 0x64,   
  0x35, 0x30, 0x30, 0x20, 0x49, 0x6e, 0x74, 0x65,   
  0x72, 0x6e, 0x61, 0x6c, 0x20, 0x53, 0x65, 0x72,   
  0x76, 0x65, 0x72, 0x20, 0x45, 0x72, 0x72, 0x6f,   
  0x72, 0x35, 0x30, 0x31, 0x20, 0x4e, 0x6f, 0x74,   
  0x20, 0x49, 0x6d, 0x70, 0x6c, 0x65, 0x6d, 0x65,   
  0x6e, 0x74, 0x65, 0x64, 0x35, 0x30, 0x33, 0x20,   
  0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x20,   
  0x55, 0x6e, 0x61, 0x76, 0x61, 0x69, 0x6c, 0x61,   
  0x62, 0x6c, 0x65, 0x4a, 0x61, 0x6e, 0x20, 0x46,   
  0x65, 0x62, 0x20, 0x4d, 0x61, 0x72, 0x20, 0x41,   
  0x70, 0x72, 0x20, 0x4d, 0x61, 0x79, 0x20, 0x4a,   
  0x75, 0x6e, 0x20, 0x4a, 0x75, 0x6c, 0x20, 0x41,   
  0x75, 0x67, 0x20, 0x53, 0x65, 0x70, 0x74, 0x20,   
  0x4f, 0x63, 0x74, 0x20, 0x4e, 0x6f, 0x76, 0x20,   
  0x44, 0x65, 0x63, 0x20, 0x30, 0x30, 0x3a, 0x30,   
  0x30, 0x3a, 0x30, 0x30, 0x20, 0x4d, 0x6f, 0x6e,   
  0x2c, 0x20, 0x54, 0x75, 0x65, 0x2c, 0x20, 0x57,   
  0x65, 0x64, 0x2c, 0x20, 0x54, 0x68, 0x75, 0x2c,   
  0x20, 0x46, 0x72, 0x69, 0x2c, 0x20, 0x53, 0x61,   
  0x74, 0x2c, 0x20, 0x53, 0x75, 0x6e, 0x2c, 0x20,   
  0x47, 0x4d, 0x54, 0x63, 0x68, 0x75, 0x6e, 0x6b,   
  0x65, 0x64, 0x2c, 0x74, 0x65, 0x78, 0x74, 0x2f,   
  0x68, 0x74, 0x6d, 0x6c, 0x2c, 0x69, 0x6d, 0x61,   
  0x67, 0x65, 0x2f, 0x70, 0x6e, 0x67, 0x2c, 0x69,   
  0x6d, 0x61, 0x67, 0x65, 0x2f, 0x6a, 0x70, 0x67,   
  0x2c, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x2f, 0x67,   
  0x69, 0x66, 0x2c, 0x61, 0x70, 0x70, 0x6c, 0x69,   
  0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x78,   
  0x6d, 0x6c, 0x2c, 0x61, 0x70, 0x70, 0x6c, 0x69,   
  0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x78,   
  0x68, 0x74, 0x6d, 0x6c, 0x2b, 0x78, 0x6d, 0x6c,   
  0x2c, 0x74, 0x65, 0x78, 0x74, 0x2f, 0x70, 0x6c,   
  0x61, 0x69, 0x6e, 0x2c, 0x74, 0x65, 0x78, 0x74,   
  0x2f, 0x6a, 0x61, 0x76, 0x61, 0x73, 0x63, 0x72,   
  0x69, 0x70, 0x74, 0x2c, 0x70, 0x75, 0x62, 0x6c,   
  0x69, 0x63, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74,   
  0x65, 0x6d, 0x61, 0x78, 0x2d, 0x61, 0x67, 0x65,   
  0x3d, 0x67, 0x7a, 0x69, 0x70, 0x2c, 0x64, 0x65,   
  0x66, 0x6c, 0x61, 0x74, 0x65, 0x2c, 0x73, 0x64,   
  0x63, 0x68, 0x63, 0x68, 0x61, 0x72, 0x73, 0x65,   
  0x74, 0x3d, 0x75, 0x74, 0x66, 0x2d, 0x38, 0x63,   
  0x68, 0x61, 0x72, 0x73, 0x65, 0x74, 0x3d, 0x69,   
  0x73, 0x6f, 0x2d, 0x38, 0x38, 0x35, 0x39, 0x2d,   
  0x31, 0x2c, 0x75, 0x74, 0x66, 0x2d, 0x2c, 0x2a,   
  0x2c, 0x65, 0x6e, 0x71, 0x3d, 0x30, 0x2e          
};


nsresult
SpdyStream31::Uncompress(z_stream *context,
                         char *blockStart,
                         uint32_t blockLen)
{
  
  EnsureBuffer(mDecompressBuffer, SpdySession31::kDefaultBufferSize,
               mDecompressBufferUsed, mDecompressBufferSize);

  mDecompressedBytes += blockLen;

  context->avail_in = blockLen;
  context->next_in = reinterpret_cast<unsigned char *>(blockStart);
  bool triedDictionary = false;

  do {
    context->next_out =
      reinterpret_cast<unsigned char *>(mDecompressBuffer.get()) +
      mDecompressBufferUsed;
    context->avail_out = mDecompressBufferSize - mDecompressBufferUsed;
    int zlib_rv = inflate(context, Z_NO_FLUSH);
    LOG3(("SpdyStream31::Uncompress %p zlib_rv %d\n", this, zlib_rv));

    if (zlib_rv == Z_NEED_DICT) {
      if (triedDictionary) {
        LOG3(("SpdyStream31::Uncompress %p Dictionary Error\n", this));
        return NS_ERROR_ILLEGAL_VALUE;
      }

      triedDictionary = true;
      inflateSetDictionary(context, kDictionary, sizeof(kDictionary));
    } else if (zlib_rv == Z_DATA_ERROR) {
      LOG3(("SpdyStream31::Uncompress %p inflate returned data err\n", this));
      return NS_ERROR_ILLEGAL_VALUE;
    } else  if (zlib_rv < Z_OK) { 
      LOG3(("SpdyStream31::Uncompress %p inflate returned %d\n", this, zlib_rv));
      return NS_ERROR_FAILURE;
    }

    
    

    mDecompressBufferUsed += mDecompressBufferSize - mDecompressBufferUsed -
      context->avail_out;

    
    
    if (zlib_rv == Z_OK &&
        !context->avail_out && context->avail_in) {
      LOG3(("SpdyStream31::Uncompress %p Large Headers - so far %d",
            this, mDecompressBufferSize));
      EnsureBuffer(mDecompressBuffer, mDecompressBufferSize + 4096,
                   mDecompressBufferUsed, mDecompressBufferSize);
    }
  }
  while (context->avail_in);
  return NS_OK;
}


nsresult
SpdyStream31::FindHeader(nsCString name,
                         nsDependentCSubstring &value)
{
  const unsigned char *nvpair = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + 4;
  const unsigned char *lastHeaderByte = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + mDecompressBufferUsed;
  if (lastHeaderByte < nvpair)
    return NS_ERROR_ILLEGAL_VALUE;

  do {
    uint32_t numPairs = PR_ntohl(reinterpret_cast<const uint32_t *>(nvpair)[-1]);

    for (uint32_t index = 0; index < numPairs; ++index) {
      if (lastHeaderByte < nvpair + 4)
        return NS_ERROR_ILLEGAL_VALUE;
      uint32_t nameLen = (nvpair[0] << 24) + (nvpair[1] << 16) +
        (nvpair[2] << 8) + nvpair[3];
      if (lastHeaderByte < nvpair + 4 + nameLen)
        return NS_ERROR_ILLEGAL_VALUE;
      nsDependentCSubstring nameString =
        Substring(reinterpret_cast<const char *>(nvpair) + 4,
                  reinterpret_cast<const char *>(nvpair) + 4 + nameLen);
      if (lastHeaderByte < nvpair + 8 + nameLen)
        return NS_ERROR_ILLEGAL_VALUE;
      uint32_t valueLen = (nvpair[4 + nameLen] << 24) + (nvpair[5 + nameLen] << 16) +
        (nvpair[6 + nameLen] << 8) + nvpair[7 + nameLen];
      if (lastHeaderByte < nvpair + 8 + nameLen + valueLen)
        return NS_ERROR_ILLEGAL_VALUE;
      if (nameString.Equals(name)) {
        value.Assign(((char *)nvpair) + 8 + nameLen, valueLen);
        return NS_OK;
      }

      
      nvpair += 8 + nameLen + valueLen;
    }

    
    
    nvpair += 4;
  } while (lastHeaderByte >= nvpair);

  return NS_ERROR_NOT_AVAILABLE;
}




nsresult
SpdyStream31::ConvertHeaders(nsACString &aHeadersOut)
{
  
  nsDependentCSubstring status, version;
  nsresult rv = FindHeader(NS_LITERAL_CSTRING(":status"),
                           status);
  if (NS_FAILED(rv))
    return (rv == NS_ERROR_NOT_AVAILABLE) ? NS_ERROR_ILLEGAL_VALUE : rv;

  rv = FindHeader(NS_LITERAL_CSTRING(":version"),
                  version);
  if (NS_FAILED(rv))
    return (rv == NS_ERROR_NOT_AVAILABLE) ? NS_ERROR_ILLEGAL_VALUE : rv;

  if (mDecompressedBytes && mDecompressBufferUsed) {
    Telemetry::Accumulate(Telemetry::SPDY_SYN_REPLY_SIZE, mDecompressedBytes);
    uint32_t ratio =
      mDecompressedBytes * 100 / mDecompressBufferUsed;
    Telemetry::Accumulate(Telemetry::SPDY_SYN_REPLY_RATIO, ratio);
  }

  aHeadersOut.Truncate();
  aHeadersOut.SetCapacity(mDecompressBufferUsed + 64);

  
  

  
  

  aHeadersOut.Append(version);
  aHeadersOut.Append(' ');
  aHeadersOut.Append(status);
  aHeadersOut.AppendLiteral("\r\n");

  const unsigned char *nvpair = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + 4;
  const unsigned char *lastHeaderByte = reinterpret_cast<unsigned char *>
    (mDecompressBuffer.get()) + mDecompressBufferUsed;
  if (lastHeaderByte < nvpair)
    return NS_ERROR_ILLEGAL_VALUE;

  do {
    uint32_t numPairs = PR_ntohl(reinterpret_cast<const uint32_t *>(nvpair)[-1]);

    for (uint32_t index = 0; index < numPairs; ++index) {
      if (lastHeaderByte < nvpair + 4)
        return NS_ERROR_ILLEGAL_VALUE;

      uint32_t nameLen = (nvpair[0] << 24) + (nvpair[1] << 16) +
        (nvpair[2] << 8) + nvpair[3];
      if (lastHeaderByte < nvpair + 4 + nameLen)
        return NS_ERROR_ILLEGAL_VALUE;

      nsDependentCSubstring nameString =
        Substring(reinterpret_cast<const char *>(nvpair) + 4,
                  reinterpret_cast<const char *>(nvpair) + 4 + nameLen);

      if (lastHeaderByte < nvpair + 8 + nameLen)
        return NS_ERROR_ILLEGAL_VALUE;

      
      
      
      
      for (char *cPtr = nameString.BeginWriting();
           cPtr && cPtr < nameString.EndWriting();
           ++cPtr) {
        if (*cPtr <= 'Z' && *cPtr >= 'A') {
          nsCString toLog(nameString);

          LOG3(("SpdyStream31::ConvertHeaders session=%p stream=%p "
                "upper case response header found. [%s]\n",
                mSession, this, toLog.get()));

          return NS_ERROR_ILLEGAL_VALUE;
        }

        
        if (*cPtr == '\0')
          return NS_ERROR_ILLEGAL_VALUE;
      }

      
      
      
      
      if (nameString.EqualsLiteral("transfer-encoding")) {
        LOG3(("SpdyStream31::ConvertHeaders session=%p stream=%p "
              "transfer-encoding found. Chunked is invalid and no TE sent.",
              mSession, this));

        return NS_ERROR_ILLEGAL_VALUE;
      }

      uint32_t valueLen =
        (nvpair[4 + nameLen] << 24) + (nvpair[5 + nameLen] << 16) +
        (nvpair[6 + nameLen] << 8)  +   nvpair[7 + nameLen];

      if (lastHeaderByte < nvpair + 8 + nameLen + valueLen)
        return NS_ERROR_ILLEGAL_VALUE;

      
      if (!nameString.IsEmpty() && nameString[0] != ':' &&
          !nameString.EqualsLiteral("connection") &&
          !nameString.EqualsLiteral("keep-alive")) {
        nsDependentCSubstring valueString =
          Substring(reinterpret_cast<const char *>(nvpair) + 8 + nameLen,
                    reinterpret_cast<const char *>(nvpair) + 8 + nameLen +
                    valueLen);

        aHeadersOut.Append(nameString);
        aHeadersOut.AppendLiteral(": ");

        
        for (char *cPtr = valueString.BeginWriting();
             cPtr && cPtr < valueString.EndWriting();
             ++cPtr) {
          if (*cPtr != 0) {
            aHeadersOut.Append(*cPtr);
            continue;
          }

          
          aHeadersOut.AppendLiteral("\r\n");
          aHeadersOut.Append(nameString);
          aHeadersOut.AppendLiteral(": ");
        }

        aHeadersOut.AppendLiteral("\r\n");
      }
      
      nvpair += 8 + nameLen + valueLen;
    }

    
    
    nvpair += 4;
  } while (lastHeaderByte >= nvpair);

  

  aHeadersOut.AppendLiteral("X-Firefox-Spdy: 3.1\r\n\r\n");
  LOG (("decoded response headers are:\n%s",
        aHeadersOut.BeginReading()));

  
  mDecompressBuffer = nullptr;
  mDecompressBufferSize = 0;
  mDecompressBufferUsed = 0;

  if (mIsTunnel && !mPlainTextTunnel) {
    aHeadersOut.Truncate();
    LOG(("SpdyStream31::ConvertHeaders %p 0x%X headers removed for tunnel\n",
         this, mStreamID));
  }

  return NS_OK;
}

void
SpdyStream31::ExecuteCompress(uint32_t flushMode)
{
  
  

  do
  {
    uint32_t avail = mTxInlineFrameSize - mTxInlineFrameUsed;
    if (avail < 1) {
      EnsureBuffer(mTxInlineFrame, mTxInlineFrameSize + 2000,
                   mTxInlineFrameUsed, mTxInlineFrameSize);
      avail = mTxInlineFrameSize - mTxInlineFrameUsed;
    }

    mZlib->next_out = mTxInlineFrame + mTxInlineFrameUsed;
    mZlib->avail_out = avail;
    deflate(mZlib, flushMode);
    mTxInlineFrameUsed += avail - mZlib->avail_out;
  } while (mZlib->avail_in > 0 || !mZlib->avail_out);
}

void
SpdyStream31::CompressToFrame(uint32_t data)
{
  
  
  data = PR_htonl(data);

  mZlib->next_in = reinterpret_cast<unsigned char *> (&data);
  mZlib->avail_in = 4;
  ExecuteCompress(Z_NO_FLUSH);
}


void
SpdyStream31::CompressToFrame(const char *data, uint32_t len)
{
  
  

  uint32_t networkLen = PR_htonl(len);

  
  mZlib->next_in = reinterpret_cast<unsigned char *> (&networkLen);
  mZlib->avail_in = 4;
  ExecuteCompress(Z_NO_FLUSH);

  
  mZlib->next_in = (unsigned char *)data;
  mZlib->avail_in = len;
  ExecuteCompress(Z_NO_FLUSH);
}

void
SpdyStream31::CompressFlushFrame()
{
  mZlib->next_in = (unsigned char *) "";
  mZlib->avail_in = 0;
  ExecuteCompress(Z_SYNC_FLUSH);
}

bool
SpdyStream31::GetFullyOpen()
{
  return mFullyOpen;
}

nsresult
SpdyStream31::SetFullyOpen()
{
  MOZ_ASSERT(!mFullyOpen);
  mFullyOpen = 1;
  if (mIsTunnel) {
    int32_t code = 0;
    nsDependentCSubstring statusSubstring;
    nsresult rv = FindHeader(NS_LITERAL_CSTRING(":status"), statusSubstring);
    if (NS_SUCCEEDED(rv)) {
      nsCString status(statusSubstring);
      nsresult errcode;
      code = status.ToInteger(&errcode);
    }

    LOG3(("SpdyStream31::SetFullyOpen %p Tunnel Response code %d", this, code));
    if ((code / 100) != 2) {
      MapStreamToPlainText();
    }

    MapStreamToHttpConnection();
    ClearTransactionsBlockedOnTunnel();
  }
  return NS_OK;
}

void
SpdyStream31::Close(nsresult reason)
{
  mTransaction->Close(reason);
}

void
SpdyStream31::UpdateRemoteWindow(int32_t delta)
{
  mRemoteWindow += delta;

  
  
  if (mBlockedOnRwin && mSession->RemoteSessionWindow() > 0 &&
      mRemoteWindow > 0) {
    
    mSession->TransactionHasDataToWrite(this);
  }
}





nsresult
SpdyStream31::OnReadSegment(const char *buf,
                            uint32_t count,
                            uint32_t *countRead)
{
  LOG3(("SpdyStream31::OnReadSegment %p count=%d state=%x",
        this, count, mUpstreamState));

  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(mSegmentReader, "OnReadSegment with null mSegmentReader");

  nsresult rv = NS_ERROR_UNEXPECTED;
  uint32_t dataLength;

  switch (mUpstreamState) {
  case GENERATING_SYN_STREAM:
    
    
    
    
    
    

    if (!mRequestHeadersDone) {
      if (NS_FAILED(rv = ParseHttpRequestHeaders(buf, count, countRead))) {
        return rv;
      }
    }

    if (mRequestHeadersDone && !mSynFrameGenerated) {
      if (!mSession->TryToActivate(this)) {
        LOG3(("SpdyStream31::OnReadSegment %p cannot activate now. queued.\n", this));
        return *countRead ? NS_OK : NS_BASE_STREAM_WOULD_BLOCK;
      }
      if (NS_FAILED(rv = GenerateSynFrame())) {
        return rv;
      }
    }

    LOG3(("ParseHttpRequestHeaders %p used %d of %d. "
          "requestheadersdone = %d mSynFrameGenerated = %d\n",
          this, *countRead, count, mRequestHeadersDone, mSynFrameGenerated));
    if (mSynFrameGenerated) {
      AdjustInitialWindow();
      rv = TransmitFrame(nullptr, nullptr, true);
      if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        
        MOZ_ASSERT(false, "Transmit Frame SYN_FRAME must at least buffer data");
        rv = NS_ERROR_UNEXPECTED;
      }

      ChangeState(GENERATING_REQUEST_BODY);
      break;
    }
    MOZ_ASSERT(*countRead == count, "Header parsing not complete but unused data");
    break;

  case GENERATING_REQUEST_BODY:
    if ((mRemoteWindow <= 0) || (mSession->RemoteSessionWindow() <= 0)) {
      *countRead = 0;
      LOG3(("SpdyStream31 this=%p, id 0x%X request body suspended because "
            "remote window is stream=%ld session=%ld.\n", this, mStreamID,
            mRemoteWindow, mSession->RemoteSessionWindow()));
      mBlockedOnRwin = true;
      return NS_BASE_STREAM_WOULD_BLOCK;
    }
    mBlockedOnRwin = false;

    dataLength = std::min(count, mChunkSize);

    if (dataLength > mRemoteWindow)
      dataLength = static_cast<uint32_t>(mRemoteWindow);

    if (dataLength > mSession->RemoteSessionWindow())
      dataLength = static_cast<uint32_t>(mSession->RemoteSessionWindow());

    LOG3(("SpdyStream31 this=%p id 0x%X remote window is stream %ld and "
          "session %ld. Chunk is %d\n",
          this, mStreamID, mRemoteWindow, mSession->RemoteSessionWindow(),
          dataLength));
    mRemoteWindow -= dataLength;
    mSession->DecrementRemoteSessionWindow(dataLength);

    LOG3(("SpdyStream31 %p id %x request len remaining %u, "
          "count avail %u, chunk used %u",
          this, mStreamID, mRequestBodyLenRemaining, count, dataLength));
    if (!dataLength && mRequestBodyLenRemaining) {
      return NS_BASE_STREAM_WOULD_BLOCK;
    }
    if (dataLength > mRequestBodyLenRemaining) {
      return NS_ERROR_UNEXPECTED;
    }
    mRequestBodyLenRemaining -= dataLength;
    GenerateDataFrameHeader(dataLength, !mRequestBodyLenRemaining);
    ChangeState(SENDING_REQUEST_BODY);
    

  case SENDING_REQUEST_BODY:
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
      ChangeState(GENERATING_REQUEST_BODY);
    break;

  case SENDING_FIN_STREAM:
    MOZ_ASSERT(false, "resuming partial fin stream out of OnReadSegment");
    break;

  default:
    MOZ_ASSERT(false, "SpdyStream31::OnReadSegment non-write state");
    break;
  }

  return rv;
}





nsresult
SpdyStream31::OnWriteSegment(char *buf,
                             uint32_t count,
                             uint32_t *countWritten)
{
  LOG3(("SpdyStream31::OnWriteSegment %p count=%d state=%x 0x%X\n",
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



void
SpdyStream31::ClearTransactionsBlockedOnTunnel()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  if (!mIsTunnel) {
    return;
  }
  gHttpHandler->ConnMgr()->ProcessPendingQ(mTransaction->ConnectionInfo());
}

void
SpdyStream31::MapStreamToPlainText()
{
  nsRefPtr<SpdyConnectTransaction> qiTrans(mTransaction->QuerySpdyConnectTransaction());
  MOZ_ASSERT(qiTrans);
  mPlainTextTunnel = true;
  qiTrans->ForcePlainText();
}

void
SpdyStream31::MapStreamToHttpConnection()
{
  nsRefPtr<SpdyConnectTransaction> qiTrans(mTransaction->QuerySpdyConnectTransaction());
  MOZ_ASSERT(qiTrans);
  qiTrans->MapStreamToHttpConnection(mSocketTransport,
                                     mTransaction->ConnectionInfo());
}

} 
} 
