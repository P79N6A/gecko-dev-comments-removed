





#include "nsHttp.h"
#include "SpdySession2.h"
#include "SpdyStream2.h"
#include "nsAlgorithm.h"
#include "prnetdb.h"
#include "nsHttpRequestHead.h"
#include "mozilla/Telemetry.h"
#include "nsISocketTransport.h"
#include "nsISupportsPriority.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {

SpdyStream2::SpdyStream2(nsAHttpTransaction *httpTransaction,
                       SpdySession2 *spdySession,
                       nsISocketTransport *socketTransport,
                       uint32_t chunkSize,
                       z_stream *compressionContext,
                       int32_t priority)
  : mUpstreamState(GENERATING_SYN_STREAM),
    mTransaction(httpTransaction),
    mSession(spdySession),
    mSocketTransport(socketTransport),
    mSegmentReader(nullptr),
    mSegmentWriter(nullptr),
    mStreamID(0),
    mChunkSize(chunkSize),
    mSynFrameComplete(0),
    mRequestBlockedOnRead(0),
    mSentFinOnData(0),
    mRecvdFin(0),
    mFullyOpen(0),
    mSentWaitingFor(0),
    mTxInlineFrameSize(SpdySession2::kDefaultBufferSize),
    mTxInlineFrameUsed(0),
    mTxStreamFrameSize(0),
    mZlib(compressionContext),
    mRequestBodyLenRemaining(0),
    mPriority(priority),
    mTotalSent(0),
    mTotalRead(0)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  LOG3(("SpdyStream2::SpdyStream2 %p", this));

  mTxInlineFrame = new char[mTxInlineFrameSize];
}

SpdyStream2::~SpdyStream2()
{
  mStreamID = SpdySession2::kDeadStreamID;
}






nsresult
SpdyStream2::ReadSegments(nsAHttpSegmentReader *reader,
                         uint32_t count,
                         uint32_t *countRead)
{
  LOG3(("SpdyStream2 %p ReadSegments reader=%p count=%d state=%x",
        this, reader, count, mUpstreamState));

  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  nsresult rv = NS_ERROR_UNEXPECTED;
  mRequestBlockedOnRead = 0;

  switch (mUpstreamState) {
  case GENERATING_SYN_STREAM:
  case GENERATING_REQUEST_BODY:
  case SENDING_REQUEST_BODY:
    
    
    mSegmentReader = reader;
    rv = mTransaction->ReadSegments(this, count, countRead);
    mSegmentReader = nullptr;

    
    
    if (NS_SUCCEEDED(rv) &&
        mUpstreamState == GENERATING_SYN_STREAM &&
        !mSynFrameComplete)
      mSession->TransactionHasDataToWrite(this);
    
    
    
    
    
    

    
    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK && !mTxInlineFrameUsed)
      mRequestBlockedOnRead = 1;

    if (!mTxInlineFrameUsed && NS_SUCCEEDED(rv) && (!*countRead)) {
      LOG3(("ReadSegments %p: Sending request data complete, mUpstreamState=%x",
            this, mUpstreamState));
      if (mSentFinOnData) {
        ChangeState(UPSTREAM_COMPLETE);
      }
      else {
        GenerateDataFrameHeader(0, true);
        ChangeState(SENDING_FIN_STREAM);
        mSession->TransactionHasDataToWrite(this);
        rv = NS_BASE_STREAM_WOULD_BLOCK;
      }
    }

    break;

  case SENDING_SYN_STREAM:
    
    
    mSegmentReader = reader;
    rv = TransmitFrame(nullptr, nullptr);
    mSegmentReader = nullptr;
    *countRead = 0;
    if (NS_SUCCEEDED(rv)) {
      NS_ABORT_IF_FALSE(!mTxInlineFrameUsed,
                        "Transmit Frame should be all or nothing");
    
      if (mSentFinOnData) {
        ChangeState(UPSTREAM_COMPLETE);
        rv = NS_OK;
      }
      else {
        rv = NS_BASE_STREAM_WOULD_BLOCK;
        ChangeState(GENERATING_REQUEST_BODY);
        mSession->TransactionHasDataToWrite(this);
      }
    }
    break;

  case SENDING_FIN_STREAM:
    
    
    if (!mSentFinOnData) {
      mSegmentReader = reader;
      rv = TransmitFrame(nullptr, nullptr);
      mSegmentReader = nullptr;
      NS_ABORT_IF_FALSE(NS_FAILED(rv) || !mTxInlineFrameUsed,
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
    NS_ABORT_IF_FALSE(false, "SpdyStream2::ReadSegments unknown state");
    break;
  }

  return rv;
}






nsresult
SpdyStream2::WriteSegments(nsAHttpSegmentWriter *writer,
                          uint32_t count,
                          uint32_t *countWritten)
{
  LOG3(("SpdyStream2::WriteSegments %p count=%d state=%x",
        this, count, mUpstreamState));
  
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(!mSegmentWriter, "segment writer in progress");

  mSegmentWriter = writer;
  nsresult rv = mTransaction->WriteSegments(writer, count, countWritten);
  mSegmentWriter = nullptr;
  return rv;
}

PLDHashOperator
SpdyStream2::hdrHashEnumerate(const nsACString &key,
                             nsAutoPtr<nsCString> &value,
                             void *closure)
{
  SpdyStream2 *self = static_cast<SpdyStream2 *>(closure);

  self->CompressToFrame(key);
  self->CompressToFrame(value.get());
  return PL_DHASH_NEXT;
}

nsresult
SpdyStream2::ParseHttpRequestHeaders(const char *buf,
                                    uint32_t avail,
                                    uint32_t *countUsed)
{
  
  

  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(mUpstreamState == GENERATING_SYN_STREAM, "wrong state");

  LOG3(("SpdyStream2::ParseHttpRequestHeaders %p avail=%d state=%x",
        this, avail, mUpstreamState));

  mFlatHttpRequestHeaders.Append(buf, avail);

  
  
  int32_t endHeader = mFlatHttpRequestHeaders.Find("\r\n\r\n");
  
  if (endHeader == kNotFound) {
    
    LOG3(("SpdyStream2::ParseHttpRequestHeaders %p "
          "Need more header bytes. Len = %d",
          this, mFlatHttpRequestHeaders.Length()));
    *countUsed = avail;
    return NS_OK;
  }
           
  
  
  
  uint32_t oldLen = mFlatHttpRequestHeaders.Length();
  mFlatHttpRequestHeaders.SetLength(endHeader + 2);
  *countUsed = avail - (oldLen - endHeader) + 4;
  mSynFrameComplete = 1;

  
  
  
  mStreamID = mSession->RegisterStreamID(this);
  NS_ABORT_IF_FALSE(mStreamID & 1,
                    "Spdy Stream Channel ID must be odd");

  if (mStreamID >= 0x80000000) {
    
    
    
    
    
    
    
    
    LOG3(("Stream assigned out of range ID: 0x%X", mStreamID));
    return NS_ERROR_UNEXPECTED;
  }

  
  

  mTxInlineFrame[0] = SpdySession2::kFlag_Control;
  mTxInlineFrame[1] = 2;                          
  mTxInlineFrame[2] = 0;
  mTxInlineFrame[3] = SpdySession2::CONTROL_TYPE_SYN_STREAM;
  
  
  uint32_t networkOrderID = PR_htonl(mStreamID);
  memcpy(mTxInlineFrame + 8, &networkOrderID, 4);
  
  
  
  memset (mTxInlineFrame + 12, 0, 4);

  
  
  
  
  
  
  
  
  
  if (mPriority >= nsISupportsPriority::PRIORITY_LOW)
    mTxInlineFrame[16] = SpdySession2::kPri03;
  else if (mPriority >= nsISupportsPriority::PRIORITY_NORMAL)
    mTxInlineFrame[16] = SpdySession2::kPri02;
  else if (mPriority >= nsISupportsPriority::PRIORITY_HIGH)
    mTxInlineFrame[16] = SpdySession2::kPri01;
  else
    mTxInlineFrame[16] = SpdySession2::kPri00;

  mTxInlineFrame[17] = 0;                         
  
  const char *methodHeader = mTransaction->RequestHead()->Method().get();

  nsCString hostHeader;
  mTransaction->RequestHead()->GetHeader(nsHttp::Host, hostHeader);

  nsCString versionHeader;
  if (mTransaction->RequestHead()->Version() == NS_HTTP_VERSION_1_1)
    versionHeader = NS_LITERAL_CSTRING("HTTP/1.1");
  else
    versionHeader = NS_LITERAL_CSTRING("HTTP/1.0");

  nsClassHashtable<nsCStringHashKey, nsCString> hdrHash;
  
  
  
  
  hdrHash.Init(1 + (mTransaction->RequestHead()->Headers().Count() * 2));
  
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

    if (name.Equals("method") ||
        name.Equals("version") ||
        name.Equals("scheme") ||
        name.Equals("keep-alive") ||
        name.Equals("accept-encoding") ||
        name.Equals("te") ||
        name.Equals("connection") ||
        name.Equals("url"))
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

    if (name.Equals("content-length")) {
      int64_t len;
      if (nsHttp::ParseInt64(val->get(), nullptr, &len))
        mRequestBodyLenRemaining = len;
    }
  }
  
  mTxInlineFrameUsed = 18;

  
  
  

  
  uint16_t count = hdrHash.Count() + 4; 
  CompressToFrame(count);

  

  CompressToFrame(NS_LITERAL_CSTRING("method"));
  CompressToFrame(methodHeader, strlen(methodHeader));
  CompressToFrame(NS_LITERAL_CSTRING("scheme"));
  CompressToFrame(NS_LITERAL_CSTRING("https"));
  CompressToFrame(NS_LITERAL_CSTRING("url"));
  CompressToFrame(mTransaction->RequestHead()->RequestURI());
  CompressToFrame(NS_LITERAL_CSTRING("version"));
  CompressToFrame(versionHeader);
  
  hdrHash.Enumerate(hdrHashEnumerate, this);
  CompressFlushFrame();
  
  
  (reinterpret_cast<uint32_t *>(mTxInlineFrame.get()))[1] =
    PR_htonl(mTxInlineFrameUsed - 8);

  NS_ABORT_IF_FALSE(!mTxInlineFrame[4],
                    "Size greater than 24 bits");
  
  
  

  if (mTransaction->RequestHead()->Method() == nsHttp::Get ||
      mTransaction->RequestHead()->Method() == nsHttp::Connect ||
      mTransaction->RequestHead()->Method() == nsHttp::Head) {
    
    

    mSentFinOnData = 1;
    mTxInlineFrame[4] = SpdySession2::kFlag_Data_FIN;
  }
  else if (mTransaction->RequestHead()->Method() == nsHttp::Post ||
           mTransaction->RequestHead()->Method() == nsHttp::Put ||
           mTransaction->RequestHead()->Method() == nsHttp::Options) {
    
    
  }
  else if (!mRequestBodyLenRemaining) {
    
    
    mSentFinOnData = 1;
    mTxInlineFrame[4] = SpdySession2::kFlag_Data_FIN;
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
SpdyStream2::UpdateTransportReadEvents(uint32_t count)
{
  mTotalRead += count;

  mTransaction->OnTransportStatus(mSocketTransport,
                                  NS_NET_STATUS_RECEIVING_FROM,
                                  mTotalRead);
}

void
SpdyStream2::UpdateTransportSendEvents(uint32_t count)
{
  mTotalSent += count;

  if (mUpstreamState != SENDING_FIN_STREAM)
    mTransaction->OnTransportStatus(mSocketTransport,
                                    NS_NET_STATUS_SENDING_TO,
                                    mTotalSent);

  if (!mSentWaitingFor && !mRequestBodyLenRemaining) {
    mSentWaitingFor = 1;
    mTransaction->OnTransportStatus(mSocketTransport,
                                    NS_NET_STATUS_WAITING_FOR,
                                    LL_ZERO);
  }
}

nsresult
SpdyStream2::TransmitFrame(const char *buf,
                          uint32_t *countUsed)
{
  
  
  

  
  
  

  NS_ABORT_IF_FALSE(mTxInlineFrameUsed, "empty stream frame in transmit");
  NS_ABORT_IF_FALSE(mSegmentReader, "TransmitFrame with null mSegmentReader");
  NS_ABORT_IF_FALSE((buf && countUsed) || (!buf && !countUsed),
                    "TransmitFrame arguments inconsistent");

  uint32_t transmittedCount;
  nsresult rv;
  
  LOG3(("SpdyStream2::TransmitFrame %p inline=%d stream=%d",
        this, mTxInlineFrameUsed, mTxStreamFrameSize));
  if (countUsed)
    *countUsed = 0;

  
  
  
  
  if (mTxStreamFrameSize && mTxInlineFrameUsed &&
      mTxStreamFrameSize < SpdySession2::kDefaultBufferSize &&
      mTxInlineFrameUsed + mTxStreamFrameSize < mTxInlineFrameSize) {
    LOG3(("Coalesce Transmit"));
    memcpy (mTxInlineFrame + mTxInlineFrameUsed,
            buf, mTxStreamFrameSize);
    if (countUsed)
      *countUsed += mTxStreamFrameSize;
    mTxInlineFrameUsed += mTxStreamFrameSize;
    mTxStreamFrameSize = 0;
  }

  rv = mSegmentReader->CommitToSegmentSize(mTxStreamFrameSize +
                                           mTxInlineFrameUsed);
  if (rv == NS_BASE_STREAM_WOULD_BLOCK)
    mSession->TransactionHasDataToWrite(this);
  if (NS_FAILED(rv))     
    return rv;

  
  
  
  
  
  rv = mSegmentReader->OnReadSegment(mTxInlineFrame, mTxInlineFrameUsed,
                                     &transmittedCount);
  LOG3(("SpdyStream2::TransmitFrame for inline session=%p "
        "stream=%p result %x len=%d",
        mSession, this, rv, transmittedCount));

  NS_ABORT_IF_FALSE(rv != NS_BASE_STREAM_WOULD_BLOCK,
                    "inconsistent inline commitment result");

  if (NS_FAILED(rv))
    return rv;

  NS_ABORT_IF_FALSE(transmittedCount == mTxInlineFrameUsed,
                    "inconsistent inline commitment count");
    
  SpdySession2::LogIO(mSession, this, "Writing from Inline Buffer",
                     mTxInlineFrame, transmittedCount);

  if (mTxStreamFrameSize) {
    if (!buf) {
      
      NS_ABORT_IF_FALSE(false, "Stream transmit with null buf argument to "
                        "TransmitFrame()");
      LOG(("Stream transmit with null buf argument to TransmitFrame()\n"));
      return NS_ERROR_UNEXPECTED;
    }

    rv = mSegmentReader->OnReadSegment(buf, mTxStreamFrameSize,
                                       &transmittedCount);

    LOG3(("SpdyStream2::TransmitFrame for regular session=%p "
          "stream=%p result %x len=%d",
          mSession, this, rv, transmittedCount));
  
    NS_ABORT_IF_FALSE(rv != NS_BASE_STREAM_WOULD_BLOCK,
                      "inconsistent stream commitment result");

    if (NS_FAILED(rv))
      return rv;

    NS_ABORT_IF_FALSE(transmittedCount == mTxStreamFrameSize,
                      "inconsistent stream commitment count");
    
    SpdySession2::LogIO(mSession, this, "Writing from Transaction Buffer",
                       buf, transmittedCount);

    *countUsed += mTxStreamFrameSize;
  }
  
  
  UpdateTransportSendEvents(mTxInlineFrameUsed + mTxStreamFrameSize);

  mTxInlineFrameUsed = 0;
  mTxStreamFrameSize = 0;

  return NS_OK;
}

void
SpdyStream2::ChangeState(enum stateType newState)
{
  LOG3(("SpdyStream2::ChangeState() %p from %X to %X",
        this, mUpstreamState, newState));
  mUpstreamState = newState;
  return;
}

void
SpdyStream2::GenerateDataFrameHeader(uint32_t dataLength, bool lastFrame)
{
  LOG3(("SpdyStream2::GenerateDataFrameHeader %p len=%d last=%d",
        this, dataLength, lastFrame));

  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(!mTxInlineFrameUsed, "inline frame not empty");
  NS_ABORT_IF_FALSE(!mTxStreamFrameSize, "stream frame not empty");
  NS_ABORT_IF_FALSE(!(dataLength & 0xff000000), "datalength > 24 bits");
  
  (reinterpret_cast<uint32_t *>(mTxInlineFrame.get()))[0] = PR_htonl(mStreamID);
  (reinterpret_cast<uint32_t *>(mTxInlineFrame.get()))[1] =
    PR_htonl(dataLength);
  
  NS_ABORT_IF_FALSE(!(mTxInlineFrame[0] & 0x80),
                    "control bit set unexpectedly");
  NS_ABORT_IF_FALSE(!mTxInlineFrame[4], "flag bits set unexpectedly");
  
  mTxInlineFrameUsed = 8;
  mTxStreamFrameSize = dataLength;

  if (lastFrame) {
    mTxInlineFrame[4] |= SpdySession2::kFlag_Data_FIN;
    if (dataLength)
      mSentFinOnData = 1;
  }
}

void
SpdyStream2::CompressToFrame(const nsACString &str)
{
  CompressToFrame(str.BeginReading(), str.Length());
}

void
SpdyStream2::CompressToFrame(const nsACString *str)
{
  CompressToFrame(str->BeginReading(), str->Length());
}








const char *SpdyStream2::kDictionary =
  "optionsgetheadpostputdeletetraceacceptaccept-charsetaccept-encodingaccept-"
  "languageauthorizationexpectfromhostif-modified-sinceif-matchif-none-matchi"
  "f-rangeif-unmodifiedsincemax-forwardsproxy-authorizationrangerefererteuser"
  "-agent10010120020120220320420520630030130230330430530630740040140240340440"
  "5406407408409410411412413414415416417500501502503504505accept-rangesageeta"
  "glocationproxy-authenticatepublicretry-afterservervarywarningwww-authentic"
  "ateallowcontent-basecontent-encodingcache-controlconnectiondatetrailertran"
  "sfer-encodingupgradeviawarningcontent-languagecontent-lengthcontent-locati"
  "oncontent-md5content-rangecontent-typeetagexpireslast-modifiedset-cookieMo"
  "ndayTuesdayWednesdayThursdayFridaySaturdaySundayJanFebMarAprMayJunJulAugSe"
  "pOctNovDecchunkedtext/htmlimage/pngimage/jpgimage/gifapplication/xmlapplic"
  "ation/xhtmltext/plainpublicmax-agecharset=iso-8859-1utf-8gzipdeflateHTTP/1"
  ".1statusversionurl";


void *
SpdyStream2::zlib_allocator(void *opaque, uInt items, uInt size)
{
  return moz_xmalloc(items * size);
}


void
SpdyStream2::zlib_destructor(void *opaque, void *addr)
{
  moz_free(addr);
}

void
SpdyStream2::ExecuteCompress(uint32_t flushMode)
{
  
  

  do
  {
    uint32_t avail = mTxInlineFrameSize - mTxInlineFrameUsed;
    if (avail < 1) {
      SpdySession2::EnsureBuffer(mTxInlineFrame,
                                mTxInlineFrameSize + 2000,
                                mTxInlineFrameUsed,
                                mTxInlineFrameSize);
      avail = mTxInlineFrameSize - mTxInlineFrameUsed;
    }

    mZlib->next_out = reinterpret_cast<unsigned char *> (mTxInlineFrame.get()) +
      mTxInlineFrameUsed;
    mZlib->avail_out = avail;
    deflate(mZlib, flushMode);
    mTxInlineFrameUsed += avail - mZlib->avail_out;
  } while (mZlib->avail_in > 0 || !mZlib->avail_out);
}

void
SpdyStream2::CompressToFrame(uint16_t data)
{
  
  
  
  data = PR_htons(data);

  mZlib->next_in = reinterpret_cast<unsigned char *> (&data);
  mZlib->avail_in = 2;
  ExecuteCompress(Z_NO_FLUSH);
}


void
SpdyStream2::CompressToFrame(const char *data, uint32_t len)
{
  
  

  
  
  if (len > 0xffff)
    len = 0xffff;

  uint16_t networkLen = PR_htons(len);
  
  
  mZlib->next_in = reinterpret_cast<unsigned char *> (&networkLen);
  mZlib->avail_in = 2;
  ExecuteCompress(Z_NO_FLUSH);
  
  
  mZlib->next_in = (unsigned char *)data;
  mZlib->avail_in = len;
  ExecuteCompress(Z_NO_FLUSH);
}

void
SpdyStream2::CompressFlushFrame()
{
  mZlib->next_in = (unsigned char *) "";
  mZlib->avail_in = 0;
  ExecuteCompress(Z_SYNC_FLUSH);
}

void
SpdyStream2::Close(nsresult reason)
{
  mTransaction->Close(reason);
}





nsresult
SpdyStream2::OnReadSegment(const char *buf,
                          uint32_t count,
                          uint32_t *countRead)
{
  LOG3(("SpdyStream2::OnReadSegment %p count=%d state=%x",
        this, count, mUpstreamState));

  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(mSegmentReader, "OnReadSegment with null mSegmentReader");
  
  nsresult rv = NS_ERROR_UNEXPECTED;
  uint32_t dataLength;

  switch (mUpstreamState) {
  case GENERATING_SYN_STREAM:
    
    
    
    
    
    

    rv = ParseHttpRequestHeaders(buf, count, countRead);
    if (NS_FAILED(rv))
      return rv;
    LOG3(("ParseHttpRequestHeaders %p used %d of %d. complete = %d",
          this, *countRead, count, mSynFrameComplete));
    if (mSynFrameComplete) {
      NS_ABORT_IF_FALSE(mTxInlineFrameUsed,
                        "OnReadSegment SynFrameComplete 0b");
      rv = TransmitFrame(nullptr, nullptr);
      NS_ABORT_IF_FALSE(NS_FAILED(rv) || !mTxInlineFrameUsed,
                        "Transmit Frame should be all or nothing");

      
      
      
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK && *countRead)
        rv = NS_OK;

      
      
      
      
      
      

      if (mTxInlineFrameUsed)
        ChangeState(SENDING_SYN_STREAM);
      else
        ChangeState(GENERATING_REQUEST_BODY);
      break;
    }
    NS_ABORT_IF_FALSE(*countRead == count,
                      "Header parsing not complete but unused data");
    break;

  case GENERATING_REQUEST_BODY:
    dataLength = NS_MIN(count, mChunkSize);
    LOG3(("SpdyStream2 %p id %x request len remaining %d, "
          "count avail %d, chunk used %d",
          this, mStreamID, mRequestBodyLenRemaining, count, dataLength));
    if (dataLength > mRequestBodyLenRemaining)
      return NS_ERROR_UNEXPECTED;
    mRequestBodyLenRemaining -= dataLength;
    GenerateDataFrameHeader(dataLength, !mRequestBodyLenRemaining);
    ChangeState(SENDING_REQUEST_BODY);
    

  case SENDING_REQUEST_BODY:
    NS_ABORT_IF_FALSE(mTxInlineFrameUsed, "OnReadSegment Send Data Header 0b");
    rv = TransmitFrame(buf, countRead);
    NS_ABORT_IF_FALSE(NS_FAILED(rv) || !mTxInlineFrameUsed,
                      "Transmit Frame should be all or nothing");

    LOG3(("TransmitFrame() rv=%x returning %d data bytes. "
          "Header is %d Body is %d.",
          rv, *countRead, mTxInlineFrameUsed, mTxStreamFrameSize));

    
    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK && *countRead)
      rv = NS_OK;

    
    if (!mTxInlineFrameUsed)
        ChangeState(GENERATING_REQUEST_BODY);
    break;

  case SENDING_SYN_STREAM:
    rv = NS_BASE_STREAM_WOULD_BLOCK;
    break;

  case SENDING_FIN_STREAM:
    NS_ABORT_IF_FALSE(false,
                      "resuming partial fin stream out of OnReadSegment");
    break;
    
  default:
    NS_ABORT_IF_FALSE(false, "SpdyStream2::OnReadSegment non-write state");
    break;
  }
  
  return rv;
}





nsresult
SpdyStream2::OnWriteSegment(char *buf,
                           uint32_t count,
                           uint32_t *countWritten)
{
  LOG3(("SpdyStream2::OnWriteSegment %p count=%d state=%x",
        this, count, mUpstreamState));

  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(mSegmentWriter, "OnWriteSegment with null mSegmentWriter");

  return mSegmentWriter->OnWriteSegment(buf, count, countWritten);
}

} 
} 

