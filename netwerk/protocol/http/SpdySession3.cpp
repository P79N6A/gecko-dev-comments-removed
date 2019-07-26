





#include "nsHttp.h"
#include "SpdySession3.h"
#include "SpdyStream3.h"
#include "nsHttpConnection.h"
#include "nsHttpHandler.h"
#include "prnetdb.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Preferences.h"
#include "prprf.h"
#include <algorithm>

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {




NS_IMPL_THREADSAFE_ADDREF(SpdySession3)
NS_IMPL_THREADSAFE_RELEASE(SpdySession3)
NS_INTERFACE_MAP_BEGIN(SpdySession3)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsAHttpConnection)
NS_INTERFACE_MAP_END

SpdySession3::SpdySession3(nsAHttpTransaction *aHttpTransaction,
                         nsISocketTransport *aSocketTransport,
                         int32_t firstPriority)
  : mSocketTransport(aSocketTransport),
    mSegmentReader(nullptr),
    mSegmentWriter(nullptr),
    mSendingChunkSize(ASpdySession::kSendingChunkSize),
    mNextStreamID(1),
    mConcurrentHighWater(0),
    mDownstreamState(BUFFERING_FRAME_HEADER),
    mInputFrameBufferSize(kDefaultBufferSize),
    mInputFrameBufferUsed(0),
    mInputFrameDataLast(false),
    mInputFrameDataStream(nullptr),
    mNeedsCleanup(nullptr),
    mShouldGoAway(false),
    mClosed(false),
    mCleanShutdown(false),
    mDataPending(false),
    mGoAwayID(0),
    mMaxConcurrent(kDefaultMaxConcurrent),
    mConcurrent(0),
    mServerPushedResources(0),
    mServerInitialWindow(kDefaultServerRwin),
    mOutputQueueSize(kDefaultQueueSize),
    mOutputQueueUsed(0),
    mOutputQueueSent(0),
    mLastReadEpoch(PR_IntervalNow()),
    mPingSentEpoch(0),
    mNextPingID(1)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  LOG3(("SpdySession3::SpdySession3 %p transaction 1 = %p",
        this, aHttpTransaction));
  
  mStreamIDHash.Init();
  mStreamTransactionHash.Init();
  mConnection = aHttpTransaction->Connection();
  mInputFrameBuffer = new char[mInputFrameBufferSize];
  mOutputQueueBuffer = new char[mOutputQueueSize];
  zlibInit();
  
  mSendingChunkSize = gHttpHandler->SpdySendingChunkSize();
  GenerateSettings();

  if (!aHttpTransaction->IsNullTransaction())
    AddStream(aHttpTransaction, firstPriority);
  mLastDataReadEpoch = mLastReadEpoch;
  
  mPingThreshold = gHttpHandler->SpdyPingThreshold();
}

PLDHashOperator
SpdySession3::ShutdownEnumerator(nsAHttpTransaction *key,
                                nsAutoPtr<SpdyStream3> &stream,
                                void *closure)
{
  SpdySession3 *self = static_cast<SpdySession3 *>(closure);
 
  
  
  
  
  if (self->mCleanShutdown && (stream->StreamID() > self->mGoAwayID))
    self->CloseStream(stream, NS_ERROR_NET_RESET); 
  else
    self->CloseStream(stream, NS_ERROR_ABORT);

  return PL_DHASH_NEXT;
}

SpdySession3::~SpdySession3()
{
  LOG3(("SpdySession3::~SpdySession3 %p mDownstreamState=%X",
        this, mDownstreamState));

  inflateEnd(&mDownstreamZlib);
  deflateEnd(&mUpstreamZlib);
  
  mStreamTransactionHash.Enumerate(ShutdownEnumerator, this);
  Telemetry::Accumulate(Telemetry::SPDY_PARALLEL_STREAMS, mConcurrentHighWater);
  Telemetry::Accumulate(Telemetry::SPDY_REQUEST_PER_CONN, (mNextStreamID - 1) / 2);
  Telemetry::Accumulate(Telemetry::SPDY_SERVER_INITIATED_STREAMS,
                        mServerPushedResources);
}

void
SpdySession3::LogIO(SpdySession3 *self, SpdyStream3 *stream, const char *label,
                   const char *data, uint32_t datalen)
{
  if (!LOG4_ENABLED())
    return;
  
  LOG4(("SpdySession3::LogIO %p stream=%p id=0x%X [%s]",
        self, stream, stream ? stream->StreamID() : 0, label));

  
  char linebuf[128];
  uint32_t index;
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

typedef nsresult  (*Control_FX) (SpdySession3 *self);
static Control_FX sControlFunctions[] = 
{
  nullptr,
  SpdySession3::HandleSynStream,
  SpdySession3::HandleSynReply,
  SpdySession3::HandleRstStream,
  SpdySession3::HandleSettings,
  SpdySession3::HandleNoop,
  SpdySession3::HandlePing,
  SpdySession3::HandleGoAway,
  SpdySession3::HandleHeaders,
  SpdySession3::HandleWindowUpdate
};

bool
SpdySession3::RoomForMoreConcurrent()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  return (mConcurrent < mMaxConcurrent);
}

bool
SpdySession3::RoomForMoreStreams()
{
  if (mNextStreamID + mStreamTransactionHash.Count() * 2 > kMaxStreamID)
    return false;

  return !mShouldGoAway;
}

PRIntervalTime
SpdySession3::IdleTime()
{
  return PR_IntervalNow() - mLastDataReadEpoch;
}

void
SpdySession3::ReadTimeoutTick(PRIntervalTime now)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(mNextPingID & 1, "Ping Counter Not Odd");

    if (!mPingThreshold)
      return;

    LOG(("SpdySession3::ReadTimeoutTick %p delta since last read %ds\n",
         this, PR_IntervalToSeconds(now - mLastReadEpoch)));

    if ((now - mLastReadEpoch) < mPingThreshold) {
      
      if (mPingSentEpoch)
        mPingSentEpoch = 0;
      return;
    }

    if (mPingSentEpoch) {
      LOG(("SpdySession3::ReadTimeoutTick %p handle outstanding ping\n"));
      if ((now - mPingSentEpoch) >= gHttpHandler->SpdyPingTimeout()) {
        LOG(("SpdySession3::ReadTimeoutTick %p Ping Timer Exhaustion\n",
             this));
        mPingSentEpoch = 0;
        Close(NS_ERROR_NET_TIMEOUT);
      }
      return;
    }
    
    LOG(("SpdySession3::ReadTimeoutTick %p generating ping 0x%X\n",
         this, mNextPingID));

    if (mNextPingID == 0xffffffff) {
      LOG(("SpdySession3::ReadTimeoutTick %p cannot form ping - ids exhausted\n",
           this));
      return;
    }

    mPingSentEpoch = PR_IntervalNow();
    if (!mPingSentEpoch)
      mPingSentEpoch = 1; 
    GeneratePing(mNextPingID);
    mNextPingID += 2;
    ResumeRecv(); 

    if (mNextPingID == 0xffffffff) {
      LOG(("SpdySession3::ReadTimeoutTick %p "
           "ping ids exhausted marking goaway\n", this));
      mShouldGoAway = true;
    }
}

uint32_t
SpdySession3::RegisterStreamID(SpdyStream3 *stream)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  LOG3(("SpdySession3::RegisterStreamID session=%p stream=%p id=0x%X "
        "concurrent=%d",this, stream, mNextStreamID, mConcurrent));

  NS_ABORT_IF_FALSE(mNextStreamID < 0xfffffff0,
                    "should have stopped admitting streams");
  
  uint32_t result = mNextStreamID;
  mNextStreamID += 2;

  
  
  
  if (mNextStreamID >= kMaxStreamID)
    mShouldGoAway = true;

  
  if (mStreamIDHash.Get(result)) {
    LOG3(("   New ID already present\n"));
    NS_ABORT_IF_FALSE(false, "New ID already present in mStreamIDHash");
    mShouldGoAway = true;
    return kDeadStreamID;
  }

  mStreamIDHash.Put(result, stream);
  return result;
}

bool
SpdySession3::AddStream(nsAHttpTransaction *aHttpTransaction,
                       int32_t aPriority)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(!mStreamTransactionHash.Get(aHttpTransaction),
                    "AddStream duplicate transaction pointer");

  
  if (mStreamTransactionHash.Get(aHttpTransaction)) {
    LOG3(("   New transaction already present\n"));
    NS_ABORT_IF_FALSE(false, "New transaction already present in hash");
    return false;
  }

  aHttpTransaction->SetConnection(this);
  SpdyStream3 *stream = new SpdyStream3(aHttpTransaction,
                                      this,
                                      mSocketTransport,
                                      mSendingChunkSize,
                                      &mUpstreamZlib,
                                      aPriority);

  
  LOG3(("SpdySession3::AddStream session=%p stream=%p NextID=0x%X (tentative)",
        this, stream, mNextStreamID));

  mStreamTransactionHash.Put(aHttpTransaction, stream);

  if (RoomForMoreConcurrent()) {
    LOG3(("SpdySession3::AddStream %p stream %p activated immediately.",
          this, stream));
    ActivateStream(stream);
  }
  else {
    LOG3(("SpdySession3::AddStream %p stream %p queued.",
          this, stream));
    mQueuedStreams.Push(stream);
  }
  
  return true;
}

void
SpdySession3::ActivateStream(SpdyStream3 *stream)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  mConcurrent++;
  if (mConcurrent > mConcurrentHighWater)
    mConcurrentHighWater = mConcurrent;
  LOG3(("SpdySession3::AddStream %p activating stream %p Currently %d "
        "streams in session, high water mark is %d",
        this, stream, mConcurrent, mConcurrentHighWater));

  mReadyForWrite.Push(stream);
  SetWriteCallbacks();

  
  
  
  if (mSegmentReader) {
    uint32_t countRead;
    ReadSegments(nullptr, kDefaultBufferSize, &countRead);
  }
}

void
SpdySession3::ProcessPending()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  while (RoomForMoreConcurrent()) {
    SpdyStream3 *stream = static_cast<SpdyStream3 *>(mQueuedStreams.PopFront());
    if (!stream)
      return;
    LOG3(("SpdySession3::ProcessPending %p stream %p activated from queue.",
          this, stream));
    ActivateStream(stream);
  }
}

nsresult
SpdySession3::NetworkRead(nsAHttpSegmentWriter *writer, char *buf,
                         uint32_t count, uint32_t *countWritten)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  if (!count) {
    *countWritten = 0;
    return NS_OK;
  }

  nsresult rv = writer->OnWriteSegment(buf, count, countWritten);
  if (NS_SUCCEEDED(rv) && *countWritten > 0)
    mLastReadEpoch = PR_IntervalNow();
  return rv;
}

void
SpdySession3::SetWriteCallbacks()
{
  if (mConnection && (GetWriteQueueSize() || mOutputQueueUsed))
      mConnection->ResumeSend();
}

void
SpdySession3::RealignOutputQueue()
{
  mOutputQueueUsed -= mOutputQueueSent;
  memmove(mOutputQueueBuffer.get(),
          mOutputQueueBuffer.get() + mOutputQueueSent,
          mOutputQueueUsed);
  mOutputQueueSent = 0;
}

void
SpdySession3::FlushOutputQueue()
{
  if (!mSegmentReader || !mOutputQueueUsed)
    return;
  
  nsresult rv;
  uint32_t countRead;
  uint32_t avail = mOutputQueueUsed - mOutputQueueSent;

  rv = mSegmentReader->
    OnReadSegment(mOutputQueueBuffer.get() + mOutputQueueSent, avail,
                                     &countRead);
  LOG3(("SpdySession3::FlushOutputQueue %p sz=%d rv=%x actual=%d",
        this, avail, rv, countRead));
  
  
  if (NS_FAILED(rv))
    return;
  
  if (countRead == avail) {
    mOutputQueueUsed = 0;
    mOutputQueueSent = 0;
    return;
  }

  mOutputQueueSent += countRead;

  
  
  
  if ((mOutputQueueSent >= kQueueMinimumCleanup) &&
      ((mOutputQueueSize - mOutputQueueUsed) < kQueueTailRoom)) {
    RealignOutputQueue();
  }
}

void
SpdySession3::DontReuse()
{
  mShouldGoAway = true;
  if (!mStreamTransactionHash.Count())
    Close(NS_OK);
}

uint32_t
SpdySession3::GetWriteQueueSize()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  return mReadyForWrite.GetSize();
}

void
SpdySession3::ChangeDownstreamState(enum stateType newState)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  LOG3(("SpdyStream3::ChangeDownstreamState() %p from %X to %X",
        this, mDownstreamState, newState));
  mDownstreamState = newState;
}

void
SpdySession3::ResetDownstreamState()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  LOG3(("SpdyStream3::ResetDownstreamState() %p", this));
  ChangeDownstreamState(BUFFERING_FRAME_HEADER);

  if (mInputFrameDataLast && mInputFrameDataStream) {
    mInputFrameDataLast = false;
    if (!mInputFrameDataStream->RecvdFin()) {
      mInputFrameDataStream->SetRecvdFin(true);
      --mConcurrent;
      ProcessPending();
    }
  }
  mInputFrameBufferUsed = 0;
  mInputFrameDataStream = nullptr;
}

template<typename T> void
SpdySession3::EnsureBuffer(nsAutoArrayPtr<T> &buf,
                          uint32_t newSize,
                          uint32_t preserve,
                          uint32_t &objSize)
{
  if (objSize >= newSize)
      return;
  
  
  
  

  objSize = (newSize + 2048 + 4095) & ~4095;

  MOZ_STATIC_ASSERT(sizeof(T) == 1, "sizeof(T) must be 1");
  nsAutoArrayPtr<T> tmp(new T[objSize]);
  memcpy(tmp, buf, preserve);
  buf = tmp;
}


template void
SpdySession3::EnsureBuffer(nsAutoArrayPtr<char> &buf,
                           uint32_t newSize,
                           uint32_t preserve,
                           uint32_t &objSize);

template void
SpdySession3::EnsureBuffer(nsAutoArrayPtr<uint8_t> &buf,
                           uint32_t newSize,
                           uint32_t preserve,
                           uint32_t &objSize);

void
SpdySession3::zlibInit()
{
  mDownstreamZlib.zalloc = SpdyStream3::zlib_allocator;
  mDownstreamZlib.zfree = SpdyStream3::zlib_destructor;
  mDownstreamZlib.opaque = Z_NULL;

  inflateInit(&mDownstreamZlib);

  mUpstreamZlib.zalloc = SpdyStream3::zlib_allocator;
  mUpstreamZlib.zfree = SpdyStream3::zlib_destructor;
  mUpstreamZlib.opaque = Z_NULL;

  
  
  deflateInit(&mUpstreamZlib, Z_NO_COMPRESSION);
  deflateSetDictionary(&mUpstreamZlib,
                       SpdyStream3::kDictionary,
                       sizeof(SpdyStream3::kDictionary));
}



nsresult
SpdySession3::UncompressAndDiscard(uint32_t offset,
                                   uint32_t blockLen)
{
  char *blockStart = mInputFrameBuffer + offset;
  unsigned char trash[2048];
  mDownstreamZlib.avail_in = blockLen;
  mDownstreamZlib.next_in = reinterpret_cast<unsigned char *>(blockStart);
  bool triedDictionary = false;

  do {
    mDownstreamZlib.next_out = trash;
    mDownstreamZlib.avail_out = sizeof(trash);
    int zlib_rv = inflate(&mDownstreamZlib, Z_NO_FLUSH);

    if (zlib_rv == Z_NEED_DICT) {
      if (triedDictionary) {
        LOG3(("SpdySession3::UncompressAndDiscard %p Dictionary Error\n", this));
        return NS_ERROR_FAILURE;
      }
      
      triedDictionary = true;
      inflateSetDictionary(&mDownstreamZlib, SpdyStream3::kDictionary,
                           sizeof(SpdyStream3::kDictionary));
    }

    if (zlib_rv == Z_DATA_ERROR || zlib_rv == Z_MEM_ERROR)
      return NS_ERROR_FAILURE;
  }
  while (mDownstreamZlib.avail_in);
  return NS_OK;
}

void
SpdySession3::GeneratePing(uint32_t aID)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::GeneratePing %p 0x%X\n", this, aID));

  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 12,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 12;

  packet[0] = kFlag_Control;
  packet[1] = kVersion;
  packet[2] = 0;
  packet[3] = CONTROL_TYPE_PING;
  packet[4] = 0;                                  
  packet[5] = 0;
  packet[6] = 0;
  packet[7] = 4;                                  
  
  aID = PR_htonl(aID);
  memcpy(packet + 8, &aID, 4);

  LogIO(this, nullptr, "Generate Ping", packet, 12);
  FlushOutputQueue();
}

void
SpdySession3::GenerateRstStream(uint32_t aStatusCode, uint32_t aID)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::GenerateRst %p 0x%X %d\n", this, aID, aStatusCode));

  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 16,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 16;

  packet[0] = kFlag_Control;
  packet[1] = kVersion;
  packet[2] = 0;
  packet[3] = CONTROL_TYPE_RST_STREAM;
  packet[4] = 0;                                  
  packet[5] = 0;
  packet[6] = 0;
  packet[7] = 8;                                  
  
  aID = PR_htonl(aID);
  memcpy(packet + 8, &aID, 4);
  aStatusCode = PR_htonl(aStatusCode);
  memcpy(packet + 12, &aStatusCode, 4);

  LogIO(this, nullptr, "Generate Reset", packet, 16);
  FlushOutputQueue();
}

void
SpdySession3::GenerateGoAway(uint32_t aStatusCode)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::GenerateGoAway %p code=%X\n", this, aStatusCode));

  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 16,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 16;

  memset(packet, 0, 16);
  packet[0] = kFlag_Control;
  packet[1] = kVersion;
  packet[3] = CONTROL_TYPE_GOAWAY;
  packet[7] = 8;                                  
  
  
  

  
  aStatusCode = PR_htonl(aStatusCode);
  memcpy(packet + 12, &aStatusCode, 4);

  LogIO(this, nullptr, "Generate GoAway", packet, 16);
  FlushOutputQueue();
}

void
SpdySession3::GenerateSettings()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::GenerateSettings %p\n", this));

  static const uint32_t maxDataLen = 4 + 3 * 8; 
  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 8 + maxDataLen,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;

  memset(packet, 0, 8 + maxDataLen);
  packet[0] = kFlag_Control;
  packet[1] = kVersion;
  packet[3] = CONTROL_TYPE_SETTINGS;

  uint8_t numberOfEntries = 0;

  
  
  
  

  
  
  packet[15 + 8 * numberOfEntries] = SETTINGS_TYPE_MAX_CONCURRENT;
  
  numberOfEntries++;

  nsRefPtr<nsHttpConnectionInfo> ci;
  uint32_t cwnd = 0;
  GetConnectionInfo(getter_AddRefs(ci));
  if (ci)
    cwnd = gHttpHandler->ConnMgr()->GetSpdyCWNDSetting(ci);
  if (cwnd) {
    packet[12 + 8 * numberOfEntries] = PERSISTED_VALUE;
    packet[15 + 8 * numberOfEntries] = SETTINGS_TYPE_CWND;
    LOG(("SpdySession3::GenerateSettings %p sending CWND %u\n", this, cwnd));
    cwnd = PR_htonl(cwnd);
    memcpy(packet + 16 + 8 * numberOfEntries, &cwnd, 4);
    numberOfEntries++;
  }
  
  packet[15 + 8 * numberOfEntries] = SETTINGS_TYPE_INITIAL_WINDOW;
  uint32_t rwin = PR_htonl(kInitialRwin);
  memcpy(packet + 16 + 8 * numberOfEntries, &rwin, 4);
  numberOfEntries++;

  uint32_t dataLen = 4 + 8 * numberOfEntries;
  mOutputQueueUsed += 8 + dataLen;
  packet[7] = dataLen;
  packet[11] = numberOfEntries;

  LogIO(this, nullptr, "Generate Settings", packet, 8 + dataLen);
  FlushOutputQueue();
}



bool
SpdySession3::VerifyStream(SpdyStream3 *aStream, uint32_t aOptionalID = 0)
{
  
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

#ifndef DEBUG
  
  return true;
#endif

  if (!aStream)
    return true;

  uint32_t test = 0;
  
  do {
    if (aStream->StreamID() == kDeadStreamID)
      break;

    nsAHttpTransaction *trans = aStream->Transaction();

    test++;  
    if (!trans)
      break;

    test++;
    if (mStreamTransactionHash.Get(trans) != aStream)
      break;
    
    if (aStream->StreamID()) {
      SpdyStream3 *idStream = mStreamIDHash.Get(aStream->StreamID());

      test++;
      if (idStream != aStream)
        break;

      if (aOptionalID) {
        test++;
        if (idStream->StreamID() != aOptionalID)
          break;
      }
    }

    
    return true;
  } while (0);

  LOG(("SpdySession3 %p VerifyStream Failure %p stream->id=0x%X "
       "optionalID=0x%X trans=%p test=%d\n",
       this, aStream, aStream->StreamID(),
       aOptionalID, aStream->Transaction(), test));
  NS_ABORT_IF_FALSE(false, "VerifyStream");
  return false;
}

void
SpdySession3::CleanupStream(SpdyStream3 *aStream, nsresult aResult,
                           rstReason aResetCode)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::CleanupStream %p %p 0x%X %X\n",
        this, aStream, aStream->StreamID(), aResult));

  if (!VerifyStream(aStream)) {
    LOG(("SpdySession3::CleanupStream failed to verify stream\n"));
    return;
  }

  if (!aStream->RecvdFin() && aStream->StreamID()) {
    LOG3(("Stream had not processed recv FIN, sending RST code %X\n",
          aResetCode));
    GenerateRstStream(aResetCode, aStream->StreamID());
    --mConcurrent;
    ProcessPending();
  }
  
  CloseStream(aStream, aResult);

  
  
  mStreamIDHash.Remove(aStream->StreamID());

  
  
  
  mStreamTransactionHash.Remove(aStream->Transaction());

  if (mShouldGoAway && !mStreamTransactionHash.Count())
    Close(NS_OK);
}

void
SpdySession3::CloseStream(SpdyStream3 *aStream, nsresult aResult)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::CloseStream %p %p 0x%x %X\n",
        this, aStream, aStream->StreamID(), aResult));

  
  if (aStream == mInputFrameDataStream) {
    LOG3(("Stream had active partial read frame on close"));
    ChangeDownstreamState(DISCARDING_DATA_FRAME);
    mInputFrameDataStream = nullptr;
  }

  
  
  uint32_t size = mReadyForWrite.GetSize();
  for (uint32_t count = 0; count < size; ++count) {
    SpdyStream3 *stream = static_cast<SpdyStream3 *>(mReadyForWrite.PopFront());
    if (stream != aStream)
      mReadyForWrite.Push(stream);
  }

  
  
  size = mQueuedStreams.GetSize();
  for (uint32_t count = 0; count < size; ++count) {
    SpdyStream3 *stream = static_cast<SpdyStream3 *>(mQueuedStreams.PopFront());
    if (stream != aStream)
      mQueuedStreams.Push(stream);
  }

  
  aStream->Close(aResult);
}

nsresult
SpdySession3::HandleSynStream(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_SYN_STREAM,
                    "wrong control type");
  
  if (self->mInputFrameDataSize < 18) {
    LOG3(("SpdySession3::HandleSynStream %p SYN_STREAM too short data=%d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t streamID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  uint32_t associatedID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[3]);

  LOG3(("SpdySession3::HandleSynStream %p recv SYN_STREAM (push) "
        "for ID 0x%X associated with 0x%X.",
        self, streamID, associatedID));
    
  if (streamID & 0x01) {                   
    LOG3(("SpdySession3::HandleSynStream %p recvd SYN_STREAM id must be even.",
          self));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  ++(self->mServerPushedResources);

  
  
  if (streamID >= kMaxStreamID)
    self->mShouldGoAway = true;

  
  
  nsresult rv =
    self->UncompressAndDiscard(18, self->mInputFrameDataSize - 10);
  if (NS_FAILED(rv)) {
    LOG(("SpdySession3::HandleSynStream uncompress failed\n"));
    return rv;
  }

  
  self->GenerateRstStream(RST_REFUSED_STREAM, streamID);
  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
SpdySession3::SetInputFrameDataStream(uint32_t streamID)
{
  mInputFrameDataStream = mStreamIDHash.Get(streamID);
  if (VerifyStream(mInputFrameDataStream, streamID))
    return NS_OK;

  LOG(("SpdySession3::SetInputFrameDataStream failed to verify 0x%X\n",
       streamID));
  mInputFrameDataStream = nullptr;
  return NS_ERROR_UNEXPECTED;
}

nsresult
SpdySession3::HandleSynReply(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_SYN_REPLY,
                    "wrong control type");

  if (self->mInputFrameDataSize < 4) {
    LOG3(("SpdySession3::HandleSynReply %p SYN REPLY too short data=%d",
          self, self->mInputFrameDataSize));
    
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  LOG3(("SpdySession3::HandleSynReply %p lookup via streamID in syn_reply.\n",
        self));
  uint32_t streamID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  nsresult rv = self->SetInputFrameDataStream(streamID);
  if (NS_FAILED(rv))
    return rv;

  if (!self->mInputFrameDataStream) {
    
    

    LOG3(("SpdySession3::HandleSynReply %p lookup streamID in syn_reply "
          "0x%X failed. NextStreamID = 0x%X\n",
          self, streamID, self->mNextStreamID));

    if (streamID >= self->mNextStreamID)
      self->GenerateRstStream(RST_INVALID_STREAM, streamID);
    
    if (NS_FAILED(self->UncompressAndDiscard(12,
                                             self->mInputFrameDataSize - 4))) {
      LOG(("SpdySession3::HandleSynReply uncompress failed\n"));
      
      return NS_ERROR_FAILURE;
    }

    self->ResetDownstreamState();
    return NS_OK;
  }

  
  
  
  
  
  rv = self->mInputFrameDataStream->Uncompress(&self->mDownstreamZlib,
                                               self->mInputFrameBuffer + 12,
                                               self->mInputFrameDataSize - 4);

  if (NS_FAILED(rv)) {
    LOG(("SpdySession3::HandleSynReply uncompress failed\n"));
    return NS_ERROR_FAILURE;
  }

  if (self->mInputFrameDataStream->GetFullyOpen()) {
    
    
    
    
    
    
    
    
    
    
    
    LOG3(("SpdySession3::HandleSynReply %p dup SYN_REPLY for 0x%X"
          " recvdfin=%d", self, self->mInputFrameDataStream->StreamID(),
          self->mInputFrameDataStream->RecvdFin()));

    self->CleanupStream(self->mInputFrameDataStream, NS_ERROR_ALREADY_OPENED,
                        self->mInputFrameDataStream->RecvdFin() ? 
                        RST_STREAM_ALREADY_CLOSED : RST_STREAM_IN_USE);
    self->ResetDownstreamState();
    return NS_OK;
  }
  self->mInputFrameDataStream->SetFullyOpen();

  self->mInputFrameDataLast = self->mInputFrameBuffer[4] & kFlag_Data_FIN;
  self->mInputFrameDataStream->UpdateTransportReadEvents(self->mInputFrameDataSize);
  self->mLastDataReadEpoch = self->mLastReadEpoch;

  if (self->mInputFrameBuffer[4] & ~kFlag_Data_FIN) {
    LOG3(("SynReply %p had undefined flag set 0x%X\n", self, streamID));
    self->CleanupStream(self->mInputFrameDataStream, NS_ERROR_ILLEGAL_VALUE,
                        RST_PROTOCOL_ERROR);
    self->ResetDownstreamState();
    return NS_OK;
  }

  if (!self->mInputFrameDataLast) {
    
    
    self->ResetDownstreamState();
    return NS_OK;
  }

  rv = self->ResponseHeadersComplete();
  if (rv == NS_ERROR_ILLEGAL_VALUE) {
    LOG3(("SpdySession3::HandleSynReply %p PROTOCOL_ERROR detected 0x%X\n",
          self, streamID));
    self->CleanupStream(self->mInputFrameDataStream, rv, RST_PROTOCOL_ERROR);
    self->ResetDownstreamState();
    rv = NS_OK;
  }
  return rv;
}




nsresult
SpdySession3::ResponseHeadersComplete()
{
  LOG3(("SpdySession3::ResponseHeadersComplete %p for 0x%X fin=%d",
        this, mInputFrameDataStream->StreamID(), mInputFrameDataLast));

  
  
  
  

  mFlatHTTPResponseHeadersOut = 0;
  nsresult rv = mInputFrameDataStream->ConvertHeaders(mFlatHTTPResponseHeaders);
  if (NS_FAILED(rv))
    return rv;

  ChangeDownstreamState(PROCESSING_COMPLETE_HEADERS);
  return NS_OK;
}

nsresult
SpdySession3::HandleRstStream(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_RST_STREAM,
                    "wrong control type");

  if (self->mInputFrameDataSize != 8) {
    LOG3(("SpdySession3::HandleRstStream %p RST_STREAM wrong length data=%d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint8_t flags = reinterpret_cast<uint8_t *>(self->mInputFrameBuffer.get())[4];

  uint32_t streamID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);

  self->mDownstreamRstReason =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[3]);

  LOG3(("SpdySession3::HandleRstStream %p RST_STREAM Reason Code %u ID %x "
        "flags %x", self, self->mDownstreamRstReason, streamID, flags));

  if (flags != 0) {
    LOG3(("SpdySession3::HandleRstStream %p RST_STREAM with flags is illegal",
          self));
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  if (self->mDownstreamRstReason == RST_INVALID_STREAM ||
      self->mDownstreamRstReason == RST_STREAM_IN_USE ||
      self->mDownstreamRstReason == RST_FLOW_CONTROL_ERROR) {
    
    LOG3(("SpdySession3::HandleRstStream %p No Reset Processing Needed.\n"));
    self->ResetDownstreamState();
    return NS_OK;
  }

  nsresult rv = self->SetInputFrameDataStream(streamID);

  if (!self->mInputFrameDataStream) {
    if (NS_FAILED(rv))
      LOG(("SpdySession3::HandleRstStream %p lookup streamID for RST Frame "
           "0x%X failed reason = %d :: VerifyStream Failed\n", self, streamID,
           self->mDownstreamRstReason));

    LOG3(("SpdySession3::HandleRstStream %p lookup streamID for RST Frame "
          "0x%X failed reason = %d", self, streamID,
          self->mDownstreamRstReason));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  self->ChangeDownstreamState(PROCESSING_CONTROL_RST_STREAM);
  return NS_OK;
}

PLDHashOperator
SpdySession3::UpdateServerRwinEnumerator(nsAHttpTransaction *key,
                                         nsAutoPtr<SpdyStream3> &stream,
                                         void *closure)
{
  int32_t delta = *(static_cast<int32_t *>(closure));
  stream->UpdateRemoteWindow(delta);
  return PL_DHASH_NEXT;
}

nsresult
SpdySession3::HandleSettings(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_SETTINGS,
                    "wrong control type");

  if (self->mInputFrameDataSize < 4) {
    LOG3(("SpdySession3::HandleSettings %p SETTINGS wrong length data=%d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t numEntries =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);

  
  
  
  if ((self->mInputFrameDataSize - 4) < (numEntries * 8)) {
    LOG3(("SpdySession3::HandleSettings %p SETTINGS wrong length data=%d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  LOG3(("SpdySession3::HandleSettings %p SETTINGS Control Frame with %d entries",
        self, numEntries));

  for (uint32_t index = 0; index < numEntries; ++index) {
    unsigned char *setting = reinterpret_cast<unsigned char *>
      (self->mInputFrameBuffer.get()) + 12 + index * 8;

    uint32_t flags = setting[0];
    uint32_t id = PR_ntohl(reinterpret_cast<uint32_t *>(setting)[0]) & 0xffffff;
    uint32_t value =  PR_ntohl(reinterpret_cast<uint32_t *>(setting)[1]);

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
      if (flags & PERSIST_VALUE)
      {
        nsRefPtr<nsHttpConnectionInfo> ci;
        self->GetConnectionInfo(getter_AddRefs(ci));
        if (ci)
          gHttpHandler->ConnMgr()->ReportSpdyCWNDSetting(ci, value);
      }
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_CWND, value);
      break;

    case SETTINGS_TYPE_DOWNLOAD_RETRANS_RATE:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_RETRANS, value);
      break;
      
    case SETTINGS_TYPE_INITIAL_WINDOW:
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_IW, value >> 10);
      {
        int32_t delta = value - self->mServerInitialWindow;
        self->mServerInitialWindow = value;

        
        self->mStreamTransactionHash.Enumerate(UpdateServerRwinEnumerator,
                                               &delta);
      }
      break;
      
    default:
      break;
    }
    
  }
  
  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
SpdySession3::HandleNoop(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_NOOP,
                    "wrong control type");

  
  

  LOG3(("SpdySession3::HandleNoop %p NOP.", self));

  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
SpdySession3::HandlePing(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_PING,
                    "wrong control type");

  if (self->mInputFrameDataSize != 4) {
    LOG3(("SpdySession3::HandlePing %p PING had wrong amount of data %d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t pingID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);

  LOG3(("SpdySession3::HandlePing %p PING ID 0x%X.", self, pingID));

  if (pingID & 0x01) {
    
    self->mPingSentEpoch = 0;
  }
  else {
    
    self->GeneratePing(pingID);
  }
    
  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
SpdySession3::HandleGoAway(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_GOAWAY,
                    "wrong control type");

  if (self->mInputFrameDataSize != 8) {
    LOG3(("SpdySession3::HandleGoAway %p GOAWAY had wrong amount of data %d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  self->mShouldGoAway = true;
  self->mGoAwayID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  self->mCleanShutdown = true;
  
  LOG3(("SpdySession3::HandleGoAway %p GOAWAY Last-Good-ID 0x%X status 0x%X\n",
        self, self->mGoAwayID, 
        PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[3])));

  self->ResumeRecv();
  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
SpdySession3::HandleHeaders(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_HEADERS,
                    "wrong control type");

  if (self->mInputFrameDataSize < 4) {
    LOG3(("SpdySession3::HandleHeaders %p HEADERS had wrong amount of data %d",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t streamID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  LOG3(("SpdySession3::HandleHeaders %p HEADERS for Stream 0x%X.\n",
        self, streamID));
  nsresult rv = self->SetInputFrameDataStream(streamID);
  if (NS_FAILED(rv))
    return rv;

  if (!self->mInputFrameDataStream) {
    LOG3(("SpdySession3::HandleHeaders %p lookup streamID 0x%X failed.\n",
          self, streamID));
    if (streamID >= self->mNextStreamID)
      self->GenerateRstStream(RST_INVALID_STREAM, streamID);

    if (NS_FAILED(self->UncompressAndDiscard(12,
                                             self->mInputFrameDataSize - 4))) {
      LOG(("SpdySession3::HandleSynReply uncompress failed\n"));
      
      return NS_ERROR_FAILURE;
    }
    self->ResetDownstreamState();
    return NS_OK;
  }

  
  
  
  
  
  rv = self->mInputFrameDataStream->Uncompress(&self->mDownstreamZlib,
                                               self->mInputFrameBuffer + 12,
                                               self->mInputFrameDataSize - 4);
  if (NS_FAILED(rv)) {
    LOG(("SpdySession3::HandleHeaders uncompress failed\n"));
    return NS_ERROR_FAILURE;
  }

  self->mInputFrameDataLast = self->mInputFrameBuffer[4] & kFlag_Data_FIN;
  self->mInputFrameDataStream->
    UpdateTransportReadEvents(self->mInputFrameDataSize);
  self->mLastDataReadEpoch = self->mLastReadEpoch;

  if (self->mInputFrameBuffer[4] & ~kFlag_Data_FIN) {
    LOG3(("Headers %p had undefined flag set 0x%X\n", self, streamID));
    self->CleanupStream(self->mInputFrameDataStream, NS_ERROR_ILLEGAL_VALUE,
                        RST_PROTOCOL_ERROR);
    self->ResetDownstreamState();
    return NS_OK;
  }

  if (!self->mInputFrameDataLast) {
    
    self->ResetDownstreamState();
    return NS_OK;
  }

  rv = self->ResponseHeadersComplete();
  if (rv == NS_ERROR_ILLEGAL_VALUE) {
    LOG3(("SpdySession3::HanndleHeaders %p PROTOCOL_ERROR detected 0x%X\n",
          self, streamID));
    self->CleanupStream(self->mInputFrameDataStream, rv, RST_PROTOCOL_ERROR);
    self->ResetDownstreamState();
    rv = NS_OK;
  }
  return rv;
}

nsresult
SpdySession3::HandleWindowUpdate(SpdySession3 *self)
{
  NS_ABORT_IF_FALSE(self->mFrameControlType == CONTROL_TYPE_WINDOW_UPDATE,
                    "wrong control type");

  if (self->mInputFrameDataSize < 8) {
    LOG3(("SpdySession3::HandleWindowUpdate %p Window Update wrong length %d\n",
          self, self->mInputFrameDataSize));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t delta =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[3]);
  delta &= 0x7fffffff;
  uint32_t streamID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  streamID &= 0x7fffffff;

  LOG3(("SpdySession3::HandleWindowUpdate %p len=%d for Stream 0x%X.\n",
        self, delta, streamID));
  nsresult rv = self->SetInputFrameDataStream(streamID);
  if (NS_FAILED(rv))
    return rv;

  if (!self->mInputFrameDataStream) {
    LOG3(("SpdySession3::HandleWindowUpdate %p lookup streamID 0x%X failed.\n",
          self, streamID));
    if (streamID >= self->mNextStreamID)
      self->GenerateRstStream(RST_INVALID_STREAM, streamID);
    self->ResetDownstreamState();
    return NS_OK;
  }

  int64_t oldRemoteWindow = self->mInputFrameDataStream->RemoteWindow();
  self->mInputFrameDataStream->UpdateRemoteWindow(delta);
  
  LOG3(("SpdySession3::HandleWindowUpdate %p stream 0x%X window "
        "%d increased by %d.\n", self, streamID, oldRemoteWindow, delta));

  
  
  if (oldRemoteWindow <= 0 &&
      self->mInputFrameDataStream->RemoteWindow() > 0) {
    self->mReadyForWrite.Push(self->mInputFrameDataStream);
    self->SetWriteCallbacks();
  }

  self->ResetDownstreamState();
  self->ResumeRecv();
  return NS_OK;
}






void
SpdySession3::OnTransportStatus(nsITransport* aTransport,
                               nsresult aStatus,
                               uint64_t aProgress)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  switch (aStatus) {
    
    
  case NS_NET_STATUS_RESOLVING_HOST:
  case NS_NET_STATUS_RESOLVED_HOST:
  case NS_NET_STATUS_CONNECTING_TO:
  case NS_NET_STATUS_CONNECTED_TO:
  {
    SpdyStream3 *target = mStreamIDHash.Get(1);
    if (target)
      target->Transaction()->OnTransportStatus(aTransport, aStatus, aProgress);
    break;
  }

  default:
    
    
    
    

    
    
    
    
    
    
    

    
    
    
    
    
    

    
    
    

    break;
  }
}






nsresult
SpdySession3::ReadSegments(nsAHttpSegmentReader *reader,
                          uint32_t count,
                          uint32_t *countRead)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  NS_ABORT_IF_FALSE(!mSegmentReader || !reader || (mSegmentReader == reader),
                    "Inconsistent Write Function Callback");

  if (reader)
    mSegmentReader = reader;

  nsresult rv;
  *countRead = 0;

  LOG3(("SpdySession3::ReadSegments %p", this));

  SpdyStream3 *stream = static_cast<SpdyStream3 *>(mReadyForWrite.PopFront());
  if (!stream) {
    LOG3(("SpdySession3 %p could not identify a stream to write; suspending.",
          this));
    FlushOutputQueue();
    SetWriteCallbacks();
    return NS_BASE_STREAM_WOULD_BLOCK;
  }
  
  LOG3(("SpdySession3 %p will write from SpdyStream3 %p 0x%X "
        "block-input=%d block-output=%d\n", this, stream, stream->StreamID(),
        stream->RequestBlockedOnRead(), stream->BlockedOnRwin()));

  rv = stream->ReadSegments(this, count, countRead);

  
  
  
  
  FlushOutputQueue();

  
  
  ResumeRecv();

  if (stream->RequestBlockedOnRead()) {
    
    
    
    
    
    LOG3(("SpdySession3::ReadSegments %p dealing with block on read", this));

    
    
    if (GetWriteQueueSize())
      rv = NS_OK;
    else
      rv = NS_BASE_STREAM_WOULD_BLOCK;
    SetWriteCallbacks();
    return rv;
  }
  
  if (NS_FAILED(rv)) {
    LOG3(("SpdySession3::ReadSegments %p returning FAIL code %X",
          this, rv));
    if (rv != NS_BASE_STREAM_WOULD_BLOCK)
      CleanupStream(stream, rv, RST_CANCEL);
    return rv;
  }
  
  if (*countRead > 0) {
    LOG3(("SpdySession3::ReadSegments %p stream=%p countread=%d",
          this, stream, *countRead));
    mReadyForWrite.Push(stream);
    SetWriteCallbacks();
    return rv;
  }

  if (stream->BlockedOnRwin()) {
    LOG3(("SpdySession3 %p will stream %p 0x%X suspended for flow control\n",
          this, stream, stream->StreamID()));
    return NS_BASE_STREAM_WOULD_BLOCK;
  }
  
  LOG3(("SpdySession3::ReadSegments %p stream=%p stream send complete",
        this, stream));
  
  
  
  SetWriteCallbacks();

  return rv;
}














nsresult
SpdySession3::WriteSegments(nsAHttpSegmentWriter *writer,
                           uint32_t count,
                           uint32_t *countWritten)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  nsresult rv;
  *countWritten = 0;

  if (mClosed)
    return NS_ERROR_FAILURE;

  SetWriteCallbacks();
  
  
  
  
  
  if (mDownstreamState == BUFFERING_FRAME_HEADER) {
    
    
    
    
    NS_ABORT_IF_FALSE(mInputFrameBufferUsed < 8,
                      "Frame Buffer Used Too Large for State");

    rv = NetworkRead(writer, mInputFrameBuffer + mInputFrameBufferUsed,
                     8 - mInputFrameBufferUsed, countWritten);

    if (NS_FAILED(rv)) {
      LOG3(("SpdySession3 %p buffering frame header read failure %x\n",
            this, rv));
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        ResumeRecv();
      return rv;
    }

    LogIO(this, nullptr, "Reading Frame Header",
          mInputFrameBuffer + mInputFrameBufferUsed, *countWritten);

    mInputFrameBufferUsed += *countWritten;

    if (mInputFrameBufferUsed < 8)
    {
      LOG3(("SpdySession3::WriteSegments %p "
            "BUFFERING FRAME HEADER incomplete size=%d",
            this, mInputFrameBufferUsed));
      return rv;
    }

    
    
    mInputFrameDataSize =
      PR_ntohl(reinterpret_cast<uint32_t *>(mInputFrameBuffer.get())[1]);
    mInputFrameDataSize &= 0x00ffffff;
    mInputFrameDataRead = 0;
    
    if (mInputFrameBuffer[0] & kFlag_Control) {
      EnsureBuffer(mInputFrameBuffer, mInputFrameDataSize + 8, 8,
                   mInputFrameBufferSize);
      ChangeDownstreamState(BUFFERING_CONTROL_FRAME);
      
      
      
      uint16_t version =
        PR_ntohs(reinterpret_cast<uint16_t *>(mInputFrameBuffer.get())[0]);
      version &= 0x7fff;
      
      mFrameControlType =
        PR_ntohs(reinterpret_cast<uint16_t *>(mInputFrameBuffer.get())[1]);
      
      LOG3(("SpdySession3::WriteSegments %p - Control Frame Identified "
            "type %d version %d data len %d",
            this, mFrameControlType, version, mInputFrameDataSize));

      if (mFrameControlType >= CONTROL_TYPE_LAST ||
          mFrameControlType <= CONTROL_TYPE_FIRST)
        return NS_ERROR_ILLEGAL_VALUE;

      if (version != kVersion)
        return NS_ERROR_ILLEGAL_VALUE;
    }
    else {
      ChangeDownstreamState(PROCESSING_DATA_FRAME);

      Telemetry::Accumulate(Telemetry::SPDY_CHUNK_RECVD,
                            mInputFrameDataSize >> 10);
      mLastDataReadEpoch = mLastReadEpoch;

      uint32_t streamID =
        PR_ntohl(reinterpret_cast<uint32_t *>(mInputFrameBuffer.get())[0]);
      rv = SetInputFrameDataStream(streamID);
      if (NS_FAILED(rv)) {
        LOG(("SpdySession3::WriteSegments %p lookup streamID 0x%X failed. "
              "probably due to verification.\n", this, streamID));
        return rv;
      }
      if (!mInputFrameDataStream) {
        LOG3(("SpdySession3::WriteSegments %p lookup streamID 0x%X failed. "
              "Next = 0x%X", this, streamID, mNextStreamID));
        if (streamID >= mNextStreamID)
          GenerateRstStream(RST_INVALID_STREAM, streamID);
        ChangeDownstreamState(DISCARDING_DATA_FRAME);
      }
      else if (mInputFrameDataStream->RecvdFin()) {
        LOG3(("SpdySession3::WriteSegments %p streamID 0x%X "
              "Data arrived for already server closed stream.\n",
              this, streamID));
        GenerateRstStream(RST_STREAM_ALREADY_CLOSED, streamID);
        ChangeDownstreamState(DISCARDING_DATA_FRAME);
      }
      else if (!mInputFrameDataStream->RecvdData()) {
        LOG3(("SpdySession3 %p First Data Frame Flushes Headers stream 0x%X\n",
              this, streamID));

        mInputFrameDataStream->SetRecvdData(true);
        rv = ResponseHeadersComplete();
        if (rv == NS_ERROR_ILLEGAL_VALUE) {
          LOG3(("SpdySession3 %p PROTOCOL_ERROR detected 0x%X\n",
                this, streamID));
          CleanupStream(mInputFrameDataStream, rv, RST_PROTOCOL_ERROR);
          ChangeDownstreamState(DISCARDING_DATA_FRAME);
        }
        else {
          mDataPending = true;
        }
      }

      mInputFrameDataLast = (mInputFrameBuffer[4] & kFlag_Data_FIN);
      LOG3(("Start Processing Data Frame. "
            "Session=%p Stream ID 0x%X Stream Ptr %p Fin=%d Len=%d",
            this, streamID, mInputFrameDataStream, mInputFrameDataLast,
            mInputFrameDataSize));
      UpdateLocalRwin(mInputFrameDataStream, mInputFrameDataSize);
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
    else if (mDownstreamRstReason == RST_FRAME_TOO_LARGE)
      rv = NS_ERROR_FILE_TOO_BIG;
    else
      rv = NS_ERROR_ILLEGAL_VALUE;

    if (mDownstreamRstReason != RST_REFUSED_STREAM &&
        mDownstreamRstReason != RST_CANCEL)
      mShouldGoAway = true;

    
    SpdyStream3 *stream = mInputFrameDataStream;
    ResetDownstreamState();
    LOG3(("SpdySession3::WriteSegments cleanup stream on recv of rst "
          "session=%p stream=%p 0x%X\n", this, stream,
          stream ? stream->StreamID() : 0));
    CleanupStream(stream, rv, RST_CANCEL);
    return NS_OK;
  }

  if (mDownstreamState == PROCESSING_DATA_FRAME ||
      mDownstreamState == PROCESSING_COMPLETE_HEADERS) {

    
    
    NS_ABORT_IF_FALSE(!mNeedsCleanup, "cleanup stream set unexpectedly");
    mNeedsCleanup = nullptr;                     

    mSegmentWriter = writer;
    rv = mInputFrameDataStream->WriteSegments(this, count, countWritten);
    mSegmentWriter = nullptr;

    mLastDataReadEpoch = mLastReadEpoch;

    if (rv == NS_BASE_STREAM_CLOSED) {
      
      
      SpdyStream3 *stream = mInputFrameDataStream;

      
      
      mDownstreamState = PROCESSING_DATA_FRAME;

      if (mInputFrameDataRead == mInputFrameDataSize)
        ResetDownstreamState();
      LOG3(("SpdySession3::WriteSegments session=%p stream=%p 0x%X "
            "needscleanup=%p. cleanup stream based on "
            "stream->writeSegments returning BASE_STREAM_CLOSED\n",
            this, stream, stream ? stream->StreamID() : 0,
            mNeedsCleanup));
      CleanupStream(stream, NS_OK, RST_CANCEL);
      NS_ABORT_IF_FALSE(!mNeedsCleanup, "double cleanup out of data frame");
      mNeedsCleanup = nullptr;                     
      return NS_OK;
    }
    
    if (mNeedsCleanup) {
      LOG3(("SpdySession3::WriteSegments session=%p stream=%p 0x%X "
            "cleanup stream based on mNeedsCleanup.\n",
            this, mNeedsCleanup, mNeedsCleanup ? mNeedsCleanup->StreamID() : 0));
      CleanupStream(mNeedsCleanup, NS_OK, RST_CANCEL);
      mNeedsCleanup = nullptr;
    }

    return rv;
  }

  if (mDownstreamState == DISCARDING_DATA_FRAME) {
    char trash[4096];
    uint32_t count = std::min(4096U, mInputFrameDataSize - mInputFrameDataRead);

    if (!count) {
      ResetDownstreamState();
      ResumeRecv();
      return NS_BASE_STREAM_WOULD_BLOCK;
    }

    rv = NetworkRead(writer, trash, count, countWritten);

    if (NS_FAILED(rv)) {
      LOG3(("SpdySession3 %p discard frame read failure %x\n", this, rv));
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        ResumeRecv();
      return rv;
    }

    LogIO(this, nullptr, "Discarding Frame", trash, *countWritten);

    mInputFrameDataRead += *countWritten;

    if (mInputFrameDataRead == mInputFrameDataSize)
      ResetDownstreamState();
    return rv;
  }
  
  if (mDownstreamState != BUFFERING_CONTROL_FRAME) {
    
    NS_ABORT_IF_FALSE(false, "Not in Bufering Control Frame State");
    return NS_ERROR_UNEXPECTED;
  }

  NS_ABORT_IF_FALSE(mInputFrameBufferUsed == 8,
                    "Frame Buffer Header Not Present");

  rv = NetworkRead(writer, mInputFrameBuffer + 8 + mInputFrameDataRead,
                   mInputFrameDataSize - mInputFrameDataRead, countWritten);

  if (NS_FAILED(rv)) {
    LOG3(("SpdySession3 %p buffering control frame read failure %x\n",
          this, rv));
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK)
      ResumeRecv();
    return rv;
  }

  LogIO(this, nullptr, "Reading Control Frame",
        mInputFrameBuffer + 8 + mInputFrameDataRead, *countWritten);

  mInputFrameDataRead += *countWritten;

  if (mInputFrameDataRead != mInputFrameDataSize)
    return NS_OK;

  
  
  
  if (mFrameControlType >= CONTROL_TYPE_LAST ||
      mFrameControlType <= CONTROL_TYPE_FIRST) 
  {
    NS_ABORT_IF_FALSE(false, "control type out of range");
    return NS_ERROR_ILLEGAL_VALUE;
  }
  rv = sControlFunctions[mFrameControlType](this);

  NS_ABORT_IF_FALSE(NS_FAILED(rv) ||
                    mDownstreamState != BUFFERING_CONTROL_FRAME,
                    "Control Handler returned OK but did not change state");

  if (mShouldGoAway && !mStreamTransactionHash.Count())
    Close(NS_OK);
  return rv;
}

void
SpdySession3::UpdateLocalRwin(SpdyStream3 *stream,
                              uint32_t bytes)
{
  
  
  if (!stream || stream->RecvdFin())
    return;

  LOG3(("SpdySession3::UpdateLocalRwin %p 0x%X %d\n",
        this, stream->StreamID(), bytes));
  stream->DecrementLocalWindow(bytes);

  
  
  uint64_t unacked = stream->LocalUnAcked();

  if (unacked < kMinimumToAck) {
    
    PR_STATIC_ASSERT(kMinimumToAck < kInitialRwin);
    PR_STATIC_ASSERT((kInitialRwin - kMinimumToAck) > 1024 * 1024);

    return;
  }

  
  
  uint32_t toack = unacked & 0x7fffffff;
  
  LOG3(("SpdySession3::UpdateLocalRwin Ack %p 0x%X %d\n",
        this, stream->StreamID(), toack));
  stream->IncrementLocalWindow(toack);
    
  static const uint32_t dataLen = 8;
  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + 8 + dataLen,
               mOutputQueueUsed, mOutputQueueSize);
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 8 + dataLen;

  memset(packet, 0, 8 + dataLen);
  packet[0] = kFlag_Control;
  packet[1] = kVersion;
  packet[3] = CONTROL_TYPE_WINDOW_UPDATE;
  packet[7] = dataLen;
  
  uint32_t id = PR_htonl(stream->StreamID());
  memcpy(packet + 8, &id, 4);
  toack = PR_htonl(toack);
  memcpy(packet + 12, &toack, 4);

  LogIO(this, stream, "Window Update", packet, 8 + dataLen);
  FlushOutputQueue();
}

void
SpdySession3::Close(nsresult aReason)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

  if (mClosed)
    return;

  LOG3(("SpdySession3::Close %p %X", this, aReason));

  mClosed = true;

  NS_ABORT_IF_FALSE(mStreamTransactionHash.Count() ==
                    mStreamIDHash.Count(),
                    "index corruption");
  mStreamTransactionHash.Enumerate(ShutdownEnumerator, this);
  mStreamIDHash.Clear();
  mStreamTransactionHash.Clear();

  if (NS_SUCCEEDED(aReason))
    GenerateGoAway(OK);
  mConnection = nullptr;
  mSegmentReader = nullptr;
  mSegmentWriter = nullptr;
}

void
SpdySession3::CloseTransaction(nsAHttpTransaction *aTransaction,
                              nsresult aResult)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::CloseTransaction %p %p %x", this, aTransaction, aResult));

  

  
  SpdyStream3 *stream = mStreamTransactionHash.Get(aTransaction);
  if (!stream) {
    LOG3(("SpdySession3::CloseTransaction %p %p %x - not found.",
          this, aTransaction, aResult));
    return;
  }
  LOG3(("SpdySession3::CloseTranscation probably a cancel. "
        "this=%p, trans=%p, result=%x, streamID=0x%X stream=%p",
        this, aTransaction, aResult, stream->StreamID(), stream));
  CleanupStream(stream, aResult, RST_CANCEL);
  ResumeRecv();
}






nsresult
SpdySession3::OnReadSegment(const char *buf,
                           uint32_t count,
                           uint32_t *countRead)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  
  nsresult rv;
  
  
  
  if (mOutputQueueUsed)
    FlushOutputQueue();

  if (!mOutputQueueUsed && mSegmentReader) {
    
    rv = mSegmentReader->OnReadSegment(buf, count, countRead);

    if (rv == NS_BASE_STREAM_WOULD_BLOCK)
      *countRead = 0;
    else if (NS_FAILED(rv))
      return rv;
    
    if (*countRead < count) {
      uint32_t required = count - *countRead;
      
      
      
      EnsureBuffer(mOutputQueueBuffer, required, 0, mOutputQueueSize);
      memcpy(mOutputQueueBuffer.get(), buf + *countRead, required);
      mOutputQueueUsed = required;
    }
    
    *countRead = count;
    return NS_OK;
  }

  
  
  

  
  
  
  

  if ((mOutputQueueUsed + count) > (mOutputQueueSize - kQueueReserved))
    return NS_BASE_STREAM_WOULD_BLOCK;
  
  memcpy(mOutputQueueBuffer.get() + mOutputQueueUsed, buf, count);
  mOutputQueueUsed += count;
  *countRead = count;

  FlushOutputQueue();

  return NS_OK;
}

nsresult
SpdySession3::CommitToSegmentSize(uint32_t count, bool forceCommitment)
{
  if (mOutputQueueUsed)
    FlushOutputQueue();

  
  if ((mOutputQueueUsed + count) <= (mOutputQueueSize - kQueueReserved))
    return NS_OK;

  
  
  if (mOutputQueueUsed && !forceCommitment)
    return NS_BASE_STREAM_WOULD_BLOCK;

  if (mOutputQueueUsed) {
    
    
    RealignOutputQueue();

    
    if ((mOutputQueueUsed + count) <= (mOutputQueueSize - kQueueReserved))
      return NS_OK;
  }

  
  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + count + kQueueReserved,
               mOutputQueueUsed, mOutputQueueSize);

  NS_ABORT_IF_FALSE((mOutputQueueUsed + count) <=
                    (mOutputQueueSize - kQueueReserved),
                    "buffer not as large as expected");

  return NS_OK;
}





nsresult
SpdySession3::OnWriteSegment(char *buf,
                            uint32_t count,
                            uint32_t *countWritten)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  nsresult rv;

  if (!mSegmentWriter) {
    
    
    return NS_ERROR_FAILURE;
  }
  
  if (mDownstreamState == PROCESSING_DATA_FRAME) {

    if (mInputFrameDataLast &&
        mInputFrameDataRead == mInputFrameDataSize) {
      *countWritten = 0;
      SetNeedsCleanup();
      return NS_BASE_STREAM_CLOSED;
    }
    
    count = std::min(count, mInputFrameDataSize - mInputFrameDataRead);
    rv = NetworkRead(mSegmentWriter, buf, count, countWritten);
    if (NS_FAILED(rv))
      return rv;

    LogIO(this, mInputFrameDataStream, "Reading Data Frame",
          buf, *countWritten);

    mInputFrameDataRead += *countWritten;
    
    mInputFrameDataStream->UpdateTransportReadEvents(*countWritten);
    if ((mInputFrameDataRead == mInputFrameDataSize) && !mInputFrameDataLast)
      ResetDownstreamState();

    return rv;
  }
  
  if (mDownstreamState == PROCESSING_COMPLETE_HEADERS) {
    
    if (mFlatHTTPResponseHeaders.Length() == mFlatHTTPResponseHeadersOut &&
        mInputFrameDataLast) {
      *countWritten = 0;
      SetNeedsCleanup();
      return NS_BASE_STREAM_CLOSED;
    }
      
    count = std::min(count,
                   mFlatHTTPResponseHeaders.Length() -
                   mFlatHTTPResponseHeadersOut);
    memcpy(buf,
           mFlatHTTPResponseHeaders.get() + mFlatHTTPResponseHeadersOut,
           count);
    mFlatHTTPResponseHeadersOut += count;
    *countWritten = count;

    if (mFlatHTTPResponseHeaders.Length() == mFlatHTTPResponseHeadersOut) {
      if (mDataPending) {
        
        
        
        mDataPending = false;
        ChangeDownstreamState(PROCESSING_DATA_FRAME);
      }
      else if (!mInputFrameDataLast) {
        
        
        
        
        ResetDownstreamState();
      }
    }
    
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}

void
SpdySession3::SetNeedsCleanup()
{
  LOG3(("SpdySession3::SetNeedsCleanup %p - recorded downstream fin of "
        "stream %p 0x%X", this, mInputFrameDataStream,
        mInputFrameDataStream->StreamID()));

  
  NS_ABORT_IF_FALSE(!mNeedsCleanup, "mNeedsCleanup unexpectedly set");
  mNeedsCleanup = mInputFrameDataStream;
  ResetDownstreamState();
}





void
SpdySession3::TransactionHasDataToWrite(nsAHttpTransaction *caller)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::TransactionHasDataToWrite %p trans=%p", this, caller));

  
  

  SpdyStream3 *stream = mStreamTransactionHash.Get(caller);
  if (!stream || !VerifyStream(stream)) {
    LOG3(("SpdySession3::TransactionHasDataToWrite %p caller %p not found",
          this, caller));
    return;
  }
  
  LOG3(("SpdySession3::TransactionHasDataToWrite %p ID is 0x%X\n",
        this, stream->StreamID()));

  mReadyForWrite.Push(stream);
}

void
SpdySession3::TransactionHasDataToWrite(SpdyStream3 *stream)
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  LOG3(("SpdySession3::TransactionHasDataToWrite %p stream=%p ID=%x",
        this, stream, stream->StreamID()));

  mReadyForWrite.Push(stream);
  SetWriteCallbacks();
}

bool
SpdySession3::IsPersistent()
{
  return true;
}

nsresult
SpdySession3::TakeTransport(nsISocketTransport **,
                           nsIAsyncInputStream **,
                           nsIAsyncOutputStream **)
{
  NS_ABORT_IF_FALSE(false, "TakeTransport of SpdySession3");
  return NS_ERROR_UNEXPECTED;
}

nsHttpConnection *
SpdySession3::TakeHttpConnection()
{
  NS_ABORT_IF_FALSE(false, "TakeHttpConnection of SpdySession3");
  return nullptr;
}

uint32_t
SpdySession3::CancelPipeline(nsresult reason)
{
  
  return 0;
}

nsAHttpTransaction::Classifier
SpdySession3::Classification()
{
  if (!mConnection)
    return nsAHttpTransaction::CLASS_GENERAL;
  return mConnection->Classification();
}







void
SpdySession3::SetConnection(nsAHttpConnection *)
{
  
  NS_ABORT_IF_FALSE(false, "SpdySession3::SetConnection()");
}

void
SpdySession3::GetSecurityCallbacks(nsIInterfaceRequestor **)
{
  
  NS_ABORT_IF_FALSE(false, "SpdySession3::GetSecurityCallbacks()");
}

void
SpdySession3::SetProxyConnectFailed()
{
  NS_ABORT_IF_FALSE(false, "SpdySession3::SetProxyConnectFailed()");
}

bool
SpdySession3::IsDone()
{
  return !mStreamTransactionHash.Count();
}

nsresult
SpdySession3::Status()
{
  NS_ABORT_IF_FALSE(false, "SpdySession3::Status()");
  return NS_ERROR_UNEXPECTED;
}

uint32_t
SpdySession3::Caps()
{
  NS_ABORT_IF_FALSE(false, "SpdySession3::Caps()");
  return 0;
}

uint64_t
SpdySession3::Available()
{
  NS_ABORT_IF_FALSE(false, "SpdySession3::Available()");
  return 0;
}

nsHttpRequestHead *
SpdySession3::RequestHead()
{
  NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  NS_ABORT_IF_FALSE(false,
                    "SpdySession3::RequestHead() "
                    "should not be called after SPDY is setup");
  return NULL;
}

uint32_t
SpdySession3::Http1xTransactionCount()
{
  return 0;
}


static PLDHashOperator
TakeStream(nsAHttpTransaction *key,
           nsAutoPtr<SpdyStream3> &stream,
           void *closure)
{
  nsTArray<nsRefPtr<nsAHttpTransaction> > *list =
    static_cast<nsTArray<nsRefPtr<nsAHttpTransaction> > *>(closure);

  list->AppendElement(key);

  
  
  return PL_DHASH_REMOVE;
}

nsresult
SpdySession3::TakeSubTransactions(
    nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions)
{
  
  

  LOG3(("SpdySession3::TakeSubTransactions %p\n", this));

  if (mConcurrentHighWater > 0)
    return NS_ERROR_ALREADY_OPENED;

  LOG3(("   taking %d\n", mStreamTransactionHash.Count()));

  mStreamTransactionHash.Enumerate(TakeStream, &outTransactions);
  return NS_OK;
}

nsresult
SpdySession3::AddTransaction(nsAHttpTransaction *)
{
  
  

  NS_ABORT_IF_FALSE(false,
                    "SpdySession3::AddTransaction() should not be called");

  return NS_ERROR_NOT_IMPLEMENTED;
}

uint32_t
SpdySession3::PipelineDepth()
{
  return IsDone() ? 0 : 1;
}

nsresult
SpdySession3::SetPipelinePosition(int32_t position)
{
  
  

  NS_ABORT_IF_FALSE(false,
                    "SpdySession3::SetPipelinePosition() should not be called");

  return NS_ERROR_NOT_IMPLEMENTED;
}

int32_t
SpdySession3::PipelinePosition()
{
    return 0;
}





nsAHttpConnection *
SpdySession3::Connection()
{
  NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
  return mConnection;
}

nsresult
SpdySession3::OnHeadersAvailable(nsAHttpTransaction *transaction,
                                nsHttpRequestHead *requestHead,
                                nsHttpResponseHead *responseHead,
                                bool *reset)
{
  return mConnection->OnHeadersAvailable(transaction,
                                         requestHead,
                                         responseHead,
                                         reset);
}

bool
SpdySession3::IsReused()
{
  return mConnection->IsReused();
}

nsresult
SpdySession3::PushBack(const char *buf, uint32_t len)
{
  return mConnection->PushBack(buf, len);
}

} 
} 
