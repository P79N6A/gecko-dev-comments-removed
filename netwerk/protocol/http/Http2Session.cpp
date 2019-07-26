






#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include <algorithm>

#include "Http2Session.h"
#include "Http2Stream.h"
#include "Http2Push.h"

#include "mozilla/Telemetry.h"
#include "mozilla/Preferences.h"
#include "nsHttp.h"
#include "nsHttpHandler.h"
#include "nsHttpConnection.h"
#include "nsILoadGroup.h"
#include "nsISSLSocketControl.h"
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "prprf.h"
#include "prnetdb.h"
#include "sslt.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {




NS_IMPL_ADDREF(Http2Session)
NS_IMPL_RELEASE(Http2Session)
NS_INTERFACE_MAP_BEGIN(Http2Session)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsAHttpConnection)
NS_INTERFACE_MAP_END



const uint8_t Http2Session::kMagicHello[] = {
  0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54,
  0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30, 0x0d, 0x0a,
  0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a
};

#define RETURN_SESSION_ERROR(o,x)  \
do {                             \
  (o)->mGoAwayReason = (x);      \
  return NS_ERROR_ILLEGAL_VALUE; \
  } while (0)

Http2Session::Http2Session(nsISocketTransport *aSocketTransport)
  : mSocketTransport(aSocketTransport)
  , mSegmentReader(nullptr)
  , mSegmentWriter(nullptr)
  , mNextStreamID(3) 
  , mConcurrentHighWater(0)
  , mDownstreamState(BUFFERING_OPENING_SETTINGS)
  , mInputFrameBufferSize(kDefaultBufferSize)
  , mInputFrameBufferUsed(0)
  , mInputFrameFinal(false)
  , mInputFrameDataStream(nullptr)
  , mNeedsCleanup(nullptr)
  , mDownstreamRstReason(NO_HTTP_ERROR)
  , mExpectedHeaderID(0)
  , mExpectedPushPromiseID(0)
  , mContinuedPromiseStream(0)
  , mShouldGoAway(false)
  , mClosed(false)
  , mCleanShutdown(false)
  , mTLSProfileConfirmed(false)
  , mGoAwayReason(NO_HTTP_ERROR)
  , mGoAwayID(0)
  , mOutgoingGoAwayID(0)
  , mMaxConcurrent(kDefaultMaxConcurrent)
  , mConcurrent(0)
  , mServerPushedResources(0)
  , mServerInitialStreamWindow(kDefaultRwin)
  , mLocalSessionWindow(kDefaultRwin)
  , mServerSessionWindow(kDefaultRwin)
  , mOutputQueueSize(kDefaultQueueSize)
  , mOutputQueueUsed(0)
  , mOutputQueueSent(0)
  , mLastReadEpoch(PR_IntervalNow())
  , mPingSentEpoch(0)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  static uint64_t sSerial;
  mSerial = ++sSerial;

  LOG3(("Http2Session::Http2Session %p serial=0x%X\n", this, mSerial));

  mInputFrameBuffer = new char[mInputFrameBufferSize];
  mOutputQueueBuffer = new char[mOutputQueueSize];
  mDecompressBuffer.SetCapacity(kDefaultBufferSize);
  mDecompressor.SetCompressor(&mCompressor);

  mPushAllowance = gHttpHandler->SpdyPushAllowance();

  mSendingChunkSize = gHttpHandler->SpdySendingChunkSize();
  SendHello();

  mLastDataReadEpoch = mLastReadEpoch;

  mPingThreshold = gHttpHandler->SpdyPingThreshold();
}



template<typename charType> static void
CopyAsNetwork32(charType dest,   
                uint32_t number) 
{
  number = PR_htonl(number);
  memcpy(dest, &number, sizeof(number));
}

template void CopyAsNetwork32(char *dest, uint32_t number);
template void CopyAsNetwork32(uint8_t *dest, uint32_t number);

PLDHashOperator
Http2Session::ShutdownEnumerator(nsAHttpTransaction *key,
                                 nsAutoPtr<Http2Stream> &stream,
                                 void *closure)
{
  Http2Session *self = static_cast<Http2Session *>(closure);

  
  
  
  
  
  
  if (self->mCleanShutdown &&
      (stream->StreamID() > self->mGoAwayID || !stream->HasRegisteredID())) {
    self->CloseStream(stream, NS_ERROR_NET_RESET); 
  } else {
    self->CloseStream(stream, NS_ERROR_ABORT);
  }

  return PL_DHASH_NEXT;
}

PLDHashOperator
Http2Session::GoAwayEnumerator(nsAHttpTransaction *key,
                               nsAutoPtr<Http2Stream> &stream,
                               void *closure)
{
  Http2Session *self = static_cast<Http2Session *>(closure);

  
  
  
  
  if ((stream->StreamID() > self->mGoAwayID && (stream->StreamID() & 1)) ||
      !stream->HasRegisteredID()) {
    self->mGoAwayStreamsToRestart.Push(stream);
  }

  return PL_DHASH_NEXT;
}

Http2Session::~Http2Session()
{
  LOG3(("Http2Session::~Http2Session %p mDownstreamState=%X",
        this, mDownstreamState));

  mStreamTransactionHash.Enumerate(ShutdownEnumerator, this);
  Telemetry::Accumulate(Telemetry::SPDY_PARALLEL_STREAMS, mConcurrentHighWater);
  Telemetry::Accumulate(Telemetry::SPDY_REQUEST_PER_CONN, (mNextStreamID - 1) / 2);
  Telemetry::Accumulate(Telemetry::SPDY_SERVER_INITIATED_STREAMS,
                        mServerPushedResources);
}

void
Http2Session::LogIO(Http2Session *self, Http2Stream *stream,
                    const char *label,
                    const char *data, uint32_t datalen)
{
  if (!LOG4_ENABLED())
    return;

  LOG4(("Http2Session::LogIO %p stream=%p id=0x%X [%s]",
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
                (reinterpret_cast<const uint8_t *>(data))[index]);
    line += 3;
  }
  if (index) {
    *line = 0;
    LOG4(("%s", linebuf));
  }
}

typedef nsresult (*Http2ControlFx) (Http2Session *self);
static Http2ControlFx sControlFunctions[] = {
  nullptr, 
  Http2Session::RecvHeaders,
  Http2Session::RecvPriority,
  Http2Session::RecvRstStream,
  Http2Session::RecvSettings,
  Http2Session::RecvPushPromise,
  Http2Session::RecvPing,
  Http2Session::RecvGoAway,
  Http2Session::RecvWindowUpdate,
  Http2Session::RecvContinuation,
  Http2Session::RecvAltSvc,
  Http2Session::RecvBlocked
};

bool
Http2Session::RoomForMoreConcurrent()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  return (mConcurrent < mMaxConcurrent);
}

bool
Http2Session::RoomForMoreStreams()
{
  if (mNextStreamID + mStreamTransactionHash.Count() * 2 > kMaxStreamID)
    return false;

  return !mShouldGoAway;
}

PRIntervalTime
Http2Session::IdleTime()
{
  return PR_IntervalNow() - mLastDataReadEpoch;
}

uint32_t
Http2Session::ReadTimeoutTick(PRIntervalTime now)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  LOG3(("Http2Session::ReadTimeoutTick %p delta since last read %ds\n",
       this, PR_IntervalToSeconds(now - mLastReadEpoch)));

  if (!mPingThreshold)
    return UINT32_MAX;

  if ((now - mLastReadEpoch) < mPingThreshold) {
    
    if (mPingSentEpoch)
      mPingSentEpoch = 0;

    return PR_IntervalToSeconds(mPingThreshold) -
      PR_IntervalToSeconds(now - mLastReadEpoch);
  }

  if (mPingSentEpoch) {
    LOG3(("Http2Session::ReadTimeoutTick %p handle outstanding ping\n"));
    if ((now - mPingSentEpoch) >= gHttpHandler->SpdyPingTimeout()) {
      LOG3(("Http2Session::ReadTimeoutTick %p Ping Timer Exhaustion\n", this));
      mPingSentEpoch = 0;
      Close(NS_ERROR_NET_TIMEOUT);
      return UINT32_MAX;
    }
    return 1; 
  }

  LOG3(("Http2Session::ReadTimeoutTick %p generating ping\n", this));

  mPingSentEpoch = PR_IntervalNow();
  if (!mPingSentEpoch)
    mPingSentEpoch = 1; 
  GeneratePing(false);
  ResumeRecv(); 

  
  
  Http2PushedStream *deleteMe;
  TimeStamp timestampNow;
  do {
    deleteMe = nullptr;

    for (uint32_t index = mPushedStreams.Length();
         index > 0 ; --index) {
      Http2PushedStream *pushedStream = mPushedStreams[index - 1];

      if (timestampNow.IsNull())
        timestampNow = TimeStamp::Now(); 

      
      
      if (pushedStream->IsOrphaned(timestampNow))
      {
        LOG3(("Http2Session Timeout Pushed Stream %p 0x%X\n",
              this, pushedStream->StreamID()));
        deleteMe = pushedStream;
        break; 
      }
    }
    if (deleteMe)
      CleanupStream(deleteMe, NS_ERROR_ABORT, CANCEL_ERROR);

  } while (deleteMe);

  return 1; 
}

uint32_t
Http2Session::RegisterStreamID(Http2Stream *stream, uint32_t aNewID)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(mNextStreamID < 0xfffffff0,
             "should have stopped admitting streams");
  MOZ_ASSERT(!(aNewID & 1),
             "0 for autoassign pull, otherwise explicit even push assignment");

  if (!aNewID) {
    
    aNewID = mNextStreamID;
    MOZ_ASSERT(aNewID & 1, "pull ID must be odd.");
    mNextStreamID += 2;
  }

  LOG3(("Http2Session::RegisterStreamID session=%p stream=%p id=0x%X "
        "concurrent=%d",this, stream, aNewID, mConcurrent));

  
  
  
  if (aNewID >= kMaxStreamID)
    mShouldGoAway = true;

  
  if (mStreamIDHash.Get(aNewID)) {
    LOG3(("   New ID already present\n"));
    MOZ_ASSERT(false, "New ID already present in mStreamIDHash");
    mShouldGoAway = true;
    return kDeadStreamID;
  }

  mStreamIDHash.Put(aNewID, stream);
  return aNewID;
}

bool
Http2Session::AddStream(nsAHttpTransaction *aHttpTransaction,
                        int32_t aPriority,
                        bool aUseTunnel,
                        nsIInterfaceRequestor *aCallbacks)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  
  if (mStreamTransactionHash.Get(aHttpTransaction)) {
    LOG3(("   New transaction already present\n"));
    MOZ_ASSERT(false, "AddStream duplicate transaction pointer");
    return false;
  }

  if (!mConnection) {
    mConnection = aHttpTransaction->Connection();
  }

  aHttpTransaction->SetConnection(this);

  if (aUseTunnel) {
    LOG3(("Http2Session::AddStream session=%p trans=%p OnTunnel",
          this, aHttpTransaction));
    DispatchOnTunnel(aHttpTransaction, aCallbacks);
    return true;
  }

  Http2Stream *stream = new Http2Stream(aHttpTransaction, this, aPriority);

  LOG3(("Http2Session::AddStream session=%p stream=%p serial=%u "
        "NextID=0x%X (tentative)", this, stream, mSerial, mNextStreamID));

  mStreamTransactionHash.Put(aHttpTransaction, stream);

  if (RoomForMoreConcurrent()) {
    LOG3(("Http2Session::AddStream %p stream %p activated immediately.",
          this, stream));
    ActivateStream(stream);
  } else {
    LOG3(("Http2Session::AddStream %p stream %p queued.", this, stream));
    mQueuedStreams.Push(stream);
  }

  if (!(aHttpTransaction->Caps() & NS_HTTP_ALLOW_KEEPALIVE)) {
    LOG3(("Http2Session::AddStream %p transaction %p forces keep-alive off.\n",
          this, aHttpTransaction));
    DontReuse();
  }

  return true;
}

void
Http2Session::ActivateStream(Http2Stream *stream)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(!stream->StreamID() || (stream->StreamID() & 1),
             "Do not activate pushed streams");

  MOZ_ASSERT(!stream->CountAsActive());
  stream->SetCountAsActive(true);
  ++mConcurrent;

  if (mConcurrent > mConcurrentHighWater)
    mConcurrentHighWater = mConcurrent;
  LOG3(("Http2Session::AddStream %p activating stream %p Currently %d "
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
Http2Session::ProcessPending()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  while (RoomForMoreConcurrent()) {
    Http2Stream *stream = static_cast<Http2Stream *>(mQueuedStreams.PopFront());
    if (!stream)
      return;
    LOG3(("Http2Session::ProcessPending %p stream %p activated from queue.",
          this, stream));
    ActivateStream(stream);
  }
}

nsresult
Http2Session::NetworkRead(nsAHttpSegmentWriter *writer, char *buf,
                          uint32_t count, uint32_t *countWritten)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

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
Http2Session::SetWriteCallbacks()
{
  if (mConnection && (GetWriteQueueSize() || mOutputQueueUsed))
    mConnection->ResumeSend();
}

void
Http2Session::RealignOutputQueue()
{
  mOutputQueueUsed -= mOutputQueueSent;
  memmove(mOutputQueueBuffer.get(),
          mOutputQueueBuffer.get() + mOutputQueueSent,
          mOutputQueueUsed);
  mOutputQueueSent = 0;
}

void
Http2Session::FlushOutputQueue()
{
  if (!mSegmentReader || !mOutputQueueUsed)
    return;

  nsresult rv;
  uint32_t countRead;
  uint32_t avail = mOutputQueueUsed - mOutputQueueSent;

  rv = mSegmentReader->
    OnReadSegment(mOutputQueueBuffer.get() + mOutputQueueSent, avail,
                  &countRead);
  LOG3(("Http2Session::FlushOutputQueue %p sz=%d rv=%x actual=%d",
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
Http2Session::DontReuse()
{
  mShouldGoAway = true;
  if (!mStreamTransactionHash.Count())
    Close(NS_OK);
}

uint32_t
Http2Session::GetWriteQueueSize()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  return mReadyForWrite.GetSize();
}

void
Http2Session::ChangeDownstreamState(enum internalStateType newState)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  LOG3(("Http2Session::ChangeDownstreamState() %p from %X to %X",
        this, mDownstreamState, newState));
  mDownstreamState = newState;
}

void
Http2Session::ResetDownstreamState()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  LOG3(("Http2Session::ResetDownstreamState() %p", this));
  ChangeDownstreamState(BUFFERING_FRAME_HEADER);

  if (mInputFrameFinal && mInputFrameDataStream) {
    mInputFrameFinal = false;
    LOG3(("  SetRecvdFin id=0x%x\n", mInputFrameDataStream->StreamID()));
    mInputFrameDataStream->SetRecvdFin(true);
    MaybeDecrementConcurrent(mInputFrameDataStream);
  }
  mInputFrameBufferUsed = 0;
  mInputFrameDataStream = nullptr;
}



template<typename charType> void
Http2Session::CreateFrameHeader(charType dest, uint16_t frameLength,
                                uint8_t frameType, uint8_t frameFlags,
                                uint32_t streamID)
{
  MOZ_ASSERT(frameLength <= kMaxFrameData, "framelength too large");
  MOZ_ASSERT(!(streamID & 0x80000000));

  frameLength = PR_htons(frameLength);

  memcpy(dest, &frameLength, 2);
  dest[2] = frameType;
  dest[3] = frameFlags;
  CopyAsNetwork32(dest + 4, streamID);
}

char *
Http2Session::EnsureOutputBuffer(uint32_t spaceNeeded)
{
  
  
  EnsureBuffer(mOutputQueueBuffer, mOutputQueueUsed + spaceNeeded,
               mOutputQueueUsed, mOutputQueueSize);
  return mOutputQueueBuffer.get() + mOutputQueueUsed;
}

template void
Http2Session::CreateFrameHeader(char *dest, uint16_t frameLength,
                                uint8_t frameType, uint8_t frameFlags,
                                uint32_t streamID);

template void
Http2Session::CreateFrameHeader(uint8_t *dest, uint16_t frameLength,
                                uint8_t frameType, uint8_t frameFlags,
                                uint32_t streamID);

void
Http2Session::MaybeDecrementConcurrent(Http2Stream *aStream)
{
  LOG3(("MaybeDecrementConcurrent %p id=0x%X concurrent=%d active=%d\n",
        this, aStream->StreamID(), mConcurrent, aStream->CountAsActive()));

  if (!aStream->CountAsActive())
    return;

  MOZ_ASSERT(mConcurrent);
  aStream->SetCountAsActive(false);
  --mConcurrent;
  ProcessPending();
}



nsresult
Http2Session::UncompressAndDiscard()
{
  nsresult rv;
  nsAutoCString trash;

  rv = mDecompressor.DecodeHeaderBlock(reinterpret_cast<const uint8_t *>(mDecompressBuffer.BeginReading()),
                                       mDecompressBuffer.Length(), trash);
  mDecompressBuffer.Truncate();
  if (NS_FAILED(rv)) {
    LOG3(("Http2Session::UncompressAndDiscard %p Compression Error\n",
          this));
    mGoAwayReason = COMPRESSION_ERROR;
    return rv;
  }
  return NS_OK;
}

void
Http2Session::GeneratePing(bool isAck)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::GeneratePing %p isAck=%d\n", this, isAck));

  char *packet = EnsureOutputBuffer(16);
  mOutputQueueUsed += 16;

  if (isAck) {
    CreateFrameHeader(packet, 8, FRAME_TYPE_PING, kFlag_ACK, 0);
    memcpy(packet + 8, mInputFrameBuffer.get() + 8, 8);
  } else {
    CreateFrameHeader(packet, 8, FRAME_TYPE_PING, 0, 0);
    memset(packet + 8, 0, 8);
  }

  LogIO(this, nullptr, "Generate Ping", packet, 16);
  FlushOutputQueue();
}

void
Http2Session::GenerateSettingsAck()
{
  
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::GenerateSettingsAck %p\n", this));

  char *packet = EnsureOutputBuffer(8);
  mOutputQueueUsed += 8;
  CreateFrameHeader(packet, 0, FRAME_TYPE_SETTINGS, kFlag_ACK, 0);
  LogIO(this, nullptr, "Generate Settings ACK", packet, 8);
  FlushOutputQueue();
}

void
Http2Session::GeneratePriority(uint32_t aID, uint8_t aPriorityWeight)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::GeneratePriority %p %X %X\n",
        this, aID, aPriorityWeight));

  char *packet = EnsureOutputBuffer(13);
  mOutputQueueUsed += 13;

  CreateFrameHeader(packet, 5, FRAME_TYPE_PRIORITY, 0, aID);
  CopyAsNetwork32(packet + 8, 0);
  memcpy(packet + 12, &aPriorityWeight, 1);
  LogIO(this, nullptr, "Generate Priority", packet, 13);
  FlushOutputQueue();
}

void
Http2Session::GenerateRstStream(uint32_t aStatusCode, uint32_t aID)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  
  
  Http2Stream *stream = mStreamIDHash.Get(aID);
  if (stream) {
    if (stream->SentReset())
      return;
    stream->SetSentReset(true);
  }

  LOG3(("Http2Session::GenerateRst %p 0x%X %d\n", this, aID, aStatusCode));

  char *packet = EnsureOutputBuffer(12);
  mOutputQueueUsed += 12;
  CreateFrameHeader(packet, 4, FRAME_TYPE_RST_STREAM, 0, aID);

  CopyAsNetwork32(packet + 8, aStatusCode);

  LogIO(this, nullptr, "Generate Reset", packet, 12);
  FlushOutputQueue();
}

void
Http2Session::GenerateGoAway(uint32_t aStatusCode)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::GenerateGoAway %p code=%X\n", this, aStatusCode));

  char *packet = EnsureOutputBuffer(16);
  mOutputQueueUsed += 16;

  CreateFrameHeader(packet, 8, FRAME_TYPE_GOAWAY, 0, 0);

  
  CopyAsNetwork32(packet + 8, mOutgoingGoAwayID);

  
  CopyAsNetwork32(packet + 12, aStatusCode);

  LogIO(this, nullptr, "Generate GoAway", packet, 16);
  FlushOutputQueue();
}





void
Http2Session::SendHello()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::SendHello %p\n", this));

  
  
  static const uint32_t maxSettings = 4;
  static const uint32_t maxDataLen = 24 + 8 + maxSettings * 5 + 12;
  char *packet = EnsureOutputBuffer(maxDataLen);
  memcpy(packet, kMagicHello, 24);
  mOutputQueueUsed += 24;
  LogIO(this, nullptr, "Magic Connection Header", packet, 24);

  packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  memset(packet, 0, maxDataLen - 24);

  
  uint8_t numberOfEntries = 0;

  
  
  
  
  

  if (!gHttpHandler->AllowPush()) {
    
    
    packet[8 + 5 * numberOfEntries] = SETTINGS_TYPE_ENABLE_PUSH;
    
    numberOfEntries++;

    packet[8 + 5 * numberOfEntries] = SETTINGS_TYPE_MAX_CONCURRENT;
    
    numberOfEntries++;
  }

  
  
  packet[8 + 5 * numberOfEntries] = SETTINGS_TYPE_INITIAL_WINDOW;
  CopyAsNetwork32(packet + 9 + 5 * numberOfEntries, mPushAllowance);
  numberOfEntries++;

  
  
  packet[8 + 5 * numberOfEntries] = SETTINGS_TYPE_COMPRESS_DATA;
  
  numberOfEntries++;

  MOZ_ASSERT(numberOfEntries <= maxSettings);
  uint32_t dataLen = 5 * numberOfEntries;
  CreateFrameHeader(packet, dataLen, FRAME_TYPE_SETTINGS, 0, 0);
  mOutputQueueUsed += 8 + dataLen;

  LogIO(this, nullptr, "Generate Settings", packet, 8 + dataLen);

  
  uint32_t sessionWindowBump = ASpdySession::kInitialRwin - kDefaultRwin;
  if (kDefaultRwin >= ASpdySession::kInitialRwin)
    goto sendHello_complete;

  
  mLocalSessionWindow = ASpdySession::kInitialRwin;

  packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  CreateFrameHeader(packet, 4, FRAME_TYPE_WINDOW_UPDATE, 0, 0);
  mOutputQueueUsed += 12;
  CopyAsNetwork32(packet + 8, sessionWindowBump);

  LOG3(("Session Window increase at start of session %p %u\n",
        this, sessionWindowBump));
  LogIO(this, nullptr, "Session Window Bump ", packet, 12);

sendHello_complete:
  FlushOutputQueue();
}



bool
Http2Session::VerifyStream(Http2Stream *aStream, uint32_t aOptionalID = 0)
{
  
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

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
      Http2Stream *idStream = mStreamIDHash.Get(aStream->StreamID());

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

  LOG3(("Http2Session %p VerifyStream Failure %p stream->id=0x%X "
       "optionalID=0x%X trans=%p test=%d\n",
       this, aStream, aStream->StreamID(),
       aOptionalID, aStream->Transaction(), test));

  MOZ_ASSERT(false, "VerifyStream");
  return false;
}

void
Http2Session::CleanupStream(Http2Stream *aStream, nsresult aResult,
                            errorType aResetCode)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::CleanupStream %p %p 0x%X %X\n",
        this, aStream, aStream ? aStream->StreamID() : 0, aResult));
  if (!aStream) {
    return;
  }

  Http2PushedStream *pushSource = nullptr;

  if (NS_SUCCEEDED(aResult) && aStream->DeferCleanupOnSuccess()) {
    LOG3(("Http2Session::CleanupStream 0x%X deferred\n", aStream->StreamID()));
    return;
  }

  if (!VerifyStream(aStream)) {
    LOG3(("Http2Session::CleanupStream failed to verify stream\n"));
    return;
  }

  pushSource = aStream->PushSource();

  if (!aStream->RecvdFin() && !aStream->RecvdReset() && aStream->StreamID()) {
    LOG3(("Stream had not processed recv FIN, sending RST code %X\n",
          aResetCode));
    GenerateRstStream(aResetCode, aStream->StreamID());
  }

  CloseStream(aStream, aResult);

  
  
  uint32_t id = aStream->StreamID();
  if (id > 0) {
    mStreamIDHash.Remove(id);
    if (!(id & 1))
      mPushedStreams.RemoveElement(aStream);
  }

  RemoveStreamFromQueues(aStream);

  
  
  
  mStreamTransactionHash.Remove(aStream->Transaction());

  if (mShouldGoAway && !mStreamTransactionHash.Count())
    Close(NS_OK);

  if (pushSource) {
    pushSource->SetDeferCleanupOnSuccess(false);
    CleanupStream(pushSource, aResult, aResetCode);
  }
}

static void RemoveStreamFromQueue(Http2Stream *aStream, nsDeque &queue)
{
  uint32_t size = queue.GetSize();
  for (uint32_t count = 0; count < size; ++count) {
    Http2Stream *stream = static_cast<Http2Stream *>(queue.PopFront());
    if (stream != aStream)
      queue.Push(stream);
  }
}

void
Http2Session::RemoveStreamFromQueues(Http2Stream *aStream)
{
  RemoveStreamFromQueue(aStream, mReadyForWrite);
  RemoveStreamFromQueue(aStream, mQueuedStreams);
  RemoveStreamFromQueue(aStream, mReadyForRead);
}

void
Http2Session::CloseStream(Http2Stream *aStream, nsresult aResult)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::CloseStream %p %p 0x%x %X\n",
        this, aStream, aStream->StreamID(), aResult));

  MaybeDecrementConcurrent(aStream);

  
  if (aStream == mInputFrameDataStream) {
    LOG3(("Stream had active partial read frame on close"));
    ChangeDownstreamState(DISCARDING_DATA_FRAME);
    mInputFrameDataStream = nullptr;
  }

  RemoveStreamFromQueues(aStream);

  if (aStream->IsTunnel()) {
    UnRegisterTunnel(aStream);
  }

  
  aStream->Close(aResult);
}

nsresult
Http2Session::SetInputFrameDataStream(uint32_t streamID)
{
  mInputFrameDataStream = mStreamIDHash.Get(streamID);
  if (VerifyStream(mInputFrameDataStream, streamID))
    return NS_OK;

  LOG3(("Http2Session::SetInputFrameDataStream failed to verify 0x%X\n",
       streamID));
  mInputFrameDataStream = nullptr;
  return NS_ERROR_UNEXPECTED;
}

nsresult
Http2Session::ParsePadding(uint8_t &paddingControlBytes, uint16_t &paddingLength)
{
  if (mInputFrameFlags & kFlag_PAD_HIGH) {
    uint8_t paddingHighValue = *reinterpret_cast<uint8_t *>(mInputFrameBuffer + 8);
    paddingLength = static_cast<uint16_t>(paddingHighValue) * 256;
    ++paddingControlBytes;
  }

  if (mInputFrameFlags & kFlag_PAD_LOW) {
    uint8_t paddingLowValue = *reinterpret_cast<uint8_t *>(mInputFrameBuffer + 8 + paddingControlBytes);
    paddingLength += paddingLowValue;
    ++paddingControlBytes;
  }

  if (paddingLength > mInputFrameDataSize) {
    
    LOG3(("Http2Session::ParsePadding %p stream 0x%x PROTOCOL_ERROR "
          "paddingLength %d > frame size %d\n",
          this, mInputFrameID, paddingLength, mInputFrameDataSize));
    RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
  }

  return NS_OK;
}

nsresult
Http2Session::RecvHeaders(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_HEADERS);

  
  
  bool endHeadersFlag = self->mInputFrameFlags & kFlag_END_HEADERS;

  if (endHeadersFlag)
    self->mExpectedHeaderID = 0;
  else
    self->mExpectedHeaderID = self->mInputFrameID;

  uint32_t priorityLen = 0;
  if (self->mInputFrameFlags & kFlag_PRIORITY) {
    priorityLen = 5;
  }
  self->SetInputFrameDataStream(self->mInputFrameID);

  
  
  uint16_t paddingLength = 0;
  uint8_t paddingControlBytes = 0;

  nsresult rv = self->ParsePadding(paddingControlBytes, paddingLength);
  if (NS_FAILED(rv)) {
    return rv;
  }

  LOG3(("Http2Session::RecvHeaders %p stream 0x%X priorityLen=%d stream=%p "
        "end_stream=%d end_headers=%d priority_group=%d "
        "paddingLength=%d pad_high_flag=%d pad_low_flag=%d\n",
        self, self->mInputFrameID, priorityLen, self->mInputFrameDataStream,
        self->mInputFrameFlags & kFlag_END_STREAM,
        self->mInputFrameFlags & kFlag_END_HEADERS,
        self->mInputFrameFlags & kFlag_PRIORITY,
        paddingLength,
        self->mInputFrameFlags & kFlag_PAD_HIGH,
        self->mInputFrameFlags & kFlag_PAD_LOW));

  if (!self->mInputFrameDataStream) {
    
    

    LOG3(("Http2Session::RecvHeaders %p lookup mInputFrameID stream "
          "0x%X failed. NextStreamID = 0x%X\n",
          self, self->mInputFrameID, self->mNextStreamID));

    if (self->mInputFrameID >= self->mNextStreamID)
      self->GenerateRstStream(PROTOCOL_ERROR, self->mInputFrameID);

    self->mDecompressBuffer.Append(self->mInputFrameBuffer + 8 + paddingControlBytes + priorityLen,
                                   self->mInputFrameDataSize - paddingControlBytes - priorityLen - paddingLength);

    if (self->mInputFrameFlags & kFlag_END_HEADERS) {
      rv = self->UncompressAndDiscard();
      if (NS_FAILED(rv)) {
        LOG3(("Http2Session::RecvHeaders uncompress failed\n"));
        
        self->mGoAwayReason = COMPRESSION_ERROR;
        return rv;
      }
    }

    self->ResetDownstreamState();
    return NS_OK;
  }

  
  self->mDecompressBuffer.Append(self->mInputFrameBuffer + 8 + paddingControlBytes + priorityLen,
                                 self->mInputFrameDataSize - paddingControlBytes - priorityLen - paddingLength);

  self->mInputFrameDataStream->UpdateTransportReadEvents(self->mInputFrameDataSize);
  self->mLastDataReadEpoch = self->mLastReadEpoch;

  if (!endHeadersFlag) { 
    self->ResetDownstreamState();
    return NS_OK;
  }

  rv = self->ResponseHeadersComplete();
  if (rv == NS_ERROR_ILLEGAL_VALUE) {
    LOG3(("Http2Session::RecvHeaders %p PROTOCOL_ERROR detected stream 0x%X\n",
          self, self->mInputFrameID));
    self->CleanupStream(self->mInputFrameDataStream, rv, PROTOCOL_ERROR);
    self->ResetDownstreamState();
    rv = NS_OK;
  }
  return rv;
}




nsresult
Http2Session::ResponseHeadersComplete()
{
  LOG3(("Http2Session::ResponseHeadersComplete %p for 0x%X fin=%d",
        this, mInputFrameDataStream->StreamID(), mInputFrameFinal));

  
  if (mInputFrameDataStream->AllHeadersReceived())
    return NS_OK;
  mInputFrameDataStream->SetAllHeadersReceived();

  
  
  
  

  nsresult rv;
  mFlatHTTPResponseHeadersOut = 0;
  rv = mInputFrameDataStream->ConvertResponseHeaders(&mDecompressor,
                                                     mDecompressBuffer,
                                                     mFlatHTTPResponseHeaders);
  if (rv == NS_ERROR_ABORT) {
    LOG(("Http2Session::ResponseHeadersComplete ConvertResponseHeaders aborted\n"));
    if (mInputFrameDataStream->IsTunnel()) {
      gHttpHandler->ConnMgr()->CancelTransactions(
        mInputFrameDataStream->Transaction()->ConnectionInfo(),
        NS_ERROR_CONNECTION_REFUSED);
    }
    CleanupStream(mInputFrameDataStream, rv, CANCEL_ERROR);
    ResetDownstreamState();
    return NS_OK;
  } else if (NS_FAILED(rv)) {
    return rv;
  }

  ChangeDownstreamState(PROCESSING_COMPLETE_HEADERS);
  return NS_OK;
}

nsresult
Http2Session::RecvPriority(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_PRIORITY);

  if (self->mInputFrameDataSize != 5) {
    LOG3(("Http2Session::RecvPriority %p wrong length data=%d\n",
          self, self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  if (!self->mInputFrameID) {
    LOG3(("Http2Session::RecvPriority %p stream ID of 0.\n", self));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  nsresult rv = self->SetInputFrameDataStream(self->mInputFrameID);
  if (NS_FAILED(rv))
    return rv;

  uint32_t newPriorityDependency =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  bool exclusive = !!(newPriorityDependency & 0x80000000);
  newPriorityDependency &= 0x7fffffff;
  uint8_t newPriorityWeight = *(self->mInputFrameBuffer.get() + 12);
  if (self->mInputFrameDataStream) {
    self->mInputFrameDataStream->SetPriorityDependency(newPriorityDependency,
                                                       newPriorityWeight,
                                                       exclusive);
  }

  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
Http2Session::RecvRstStream(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_RST_STREAM);

  if (self->mInputFrameDataSize != 4) {
    LOG3(("Http2Session::RecvRstStream %p RST_STREAM wrong length data=%d",
          self, self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  if (!self->mInputFrameID) {
    LOG3(("Http2Session::RecvRstStream %p stream ID of 0.\n", self));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  self->mDownstreamRstReason =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);

  LOG3(("Http2Session::RecvRstStream %p RST_STREAM Reason Code %u ID %x\n",
        self, self->mDownstreamRstReason, self->mInputFrameID));

  self->SetInputFrameDataStream(self->mInputFrameID);
  if (!self->mInputFrameDataStream) {
    
    self->ResetDownstreamState();
    return NS_OK;
  }

  self->mInputFrameDataStream->SetRecvdReset(true);
  self->MaybeDecrementConcurrent(self->mInputFrameDataStream);
  self->ChangeDownstreamState(PROCESSING_CONTROL_RST_STREAM);
  return NS_OK;
}

PLDHashOperator
Http2Session::UpdateServerRwinEnumerator(nsAHttpTransaction *key,
                                         nsAutoPtr<Http2Stream> &stream,
                                         void *closure)
{
  int32_t delta = *(static_cast<int32_t *>(closure));
  stream->UpdateServerReceiveWindow(delta);
  return PL_DHASH_NEXT;
}

nsresult
Http2Session::RecvSettings(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_SETTINGS);

  if (self->mInputFrameID) {
    LOG3(("Http2Session::RecvSettings %p needs stream ID of 0. 0x%X\n",
          self, self->mInputFrameID));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  if (self->mInputFrameDataSize % 5) {
    
    
    LOG3(("Http2Session::RecvSettings %p SETTINGS wrong length data=%d",
          self, self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  uint32_t numEntries = self->mInputFrameDataSize / 5;
  LOG3(("Http2Session::RecvSettings %p SETTINGS Control Frame "
        "with %d entries ack=%X", self, numEntries,
        self->mInputFrameFlags & kFlag_ACK));

  if ((self->mInputFrameFlags & kFlag_ACK) && self->mInputFrameDataSize) {
    LOG3(("Http2Session::RecvSettings %p ACK with non zero payload is err\n"));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  for (uint32_t index = 0; index < numEntries; ++index) {
    uint8_t *setting = reinterpret_cast<uint8_t *>
      (self->mInputFrameBuffer.get()) + 8 + index * 5;

    uint8_t id = setting[0];
    uint32_t value = PR_ntohl(*reinterpret_cast<uint32_t *>(setting + 1));
    LOG3(("Settings ID %d, Value %d", id, value));

    switch (id)
    {
    case SETTINGS_TYPE_HEADER_TABLE_SIZE:
      LOG3(("Compression header table setting received: %d\n", value));
      self->mCompressor.SetMaxBufferSize(value);
      break;

    case SETTINGS_TYPE_ENABLE_PUSH:
      LOG3(("Client received an ENABLE Push SETTING. Odd.\n"));
      
      break;

    case SETTINGS_TYPE_MAX_CONCURRENT:
      self->mMaxConcurrent = value;
      Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_MAX_STREAMS, value);
      break;

    case SETTINGS_TYPE_INITIAL_WINDOW:
      {
        Telemetry::Accumulate(Telemetry::SPDY_SETTINGS_IW, value >> 10);
        int32_t delta = value - self->mServerInitialStreamWindow;
        self->mServerInitialStreamWindow = value;

        
        
        self->mStreamTransactionHash.Enumerate(UpdateServerRwinEnumerator,
                                               &delta);
      }
      break;

    case SETTINGS_TYPE_COMPRESS_DATA:
      LOG3(("Received DATA compression setting: %d\n", value));
      
      break;

    default:
      break;
    }
  }

  self->ResetDownstreamState();

  if (!(self->mInputFrameFlags & kFlag_ACK))
    self->GenerateSettingsAck();

  return NS_OK;
}

nsresult
Http2Session::RecvPushPromise(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_PUSH_PROMISE);

  
  
  uint16_t paddingLength = 0;
  uint8_t paddingControlBytes = 0;
  nsresult rv = self->ParsePadding(paddingControlBytes, paddingLength);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  uint32_t promiseLen;
  uint32_t promisedID;

  if (self->mExpectedPushPromiseID) {
    promiseLen = 0; 
    promisedID = self->mContinuedPromiseStream;
  } else {
    promiseLen = 4;
    promisedID =
      PR_ntohl(*reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get() + 8 + paddingControlBytes));
    promisedID &= 0x7fffffff;
  }

  uint32_t associatedID = self->mInputFrameID;

  if (self->mInputFrameFlags & kFlag_END_PUSH_PROMISE) {
    self->mExpectedPushPromiseID = 0;
    self->mContinuedPromiseStream = 0;
  } else {
    self->mExpectedPushPromiseID = self->mInputFrameID;
    self->mContinuedPromiseStream = promisedID;
  }

  if (paddingLength > self->mInputFrameDataSize) {
    
    LOG3(("Http2Session::RecvPushPromise %p ID 0x%X assoc ID 0x%X "
          "PROTOCOL_ERROR paddingLength %d > frame size %d\n",
          self, promisedID, associatedID, paddingLength,
          self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  LOG3(("Http2Session::RecvPushPromise %p ID 0x%X assoc ID 0x%X "
        "paddingLength %d pad_high_flag %d pad_low_flag %d.\n",
        self, promisedID, associatedID, paddingLength,
        self->mInputFrameFlags & kFlag_PAD_HIGH,
        self->mInputFrameFlags & kFlag_PAD_LOW));

  if (!associatedID || !promisedID || (promisedID & 1)) {
    LOG3(("Http2Session::RecvPushPromise %p ID invalid.\n", self));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  
  rv = self->SetInputFrameDataStream(associatedID);
  if (NS_FAILED(rv))
    return rv;

  Http2Stream *associatedStream = self->mInputFrameDataStream;
  ++(self->mServerPushedResources);

  
  
  if (promisedID >= kMaxStreamID)
    self->mShouldGoAway = true;

  bool resetStream = true;
  SpdyPushCache *cache = nullptr;

  if (self->mShouldGoAway) {
    LOG3(("Http2Session::RecvPushPromise %p push while in GoAway "
          "mode refused.\n", self));
    self->GenerateRstStream(REFUSED_STREAM_ERROR, promisedID);
  } else if (!gHttpHandler->AllowPush()) {
    
    LOG3(("Http2Session::RecvPushPromise Push Recevied when Disabled\n"));
    self->GenerateRstStream(REFUSED_STREAM_ERROR, promisedID);
  } else if (!(self->mInputFrameFlags & kFlag_END_PUSH_PROMISE)) {
    LOG3(("Http2Session::RecvPushPromise no support for multi frame push\n"));
    self->GenerateRstStream(REFUSED_STREAM_ERROR, promisedID);
  } else if (!associatedStream) {
    LOG3(("Http2Session::RecvPushPromise %p lookup associated ID failed.\n", self));
    self->GenerateRstStream(PROTOCOL_ERROR, promisedID);
  } else {
    nsILoadGroupConnectionInfo *loadGroupCI = associatedStream->LoadGroupConnectionInfo();
    if (loadGroupCI) {
      loadGroupCI->GetSpdyPushCache(&cache);
      if (!cache) {
        cache = new SpdyPushCache();
        if (!cache || NS_FAILED(loadGroupCI->SetSpdyPushCache(cache))) {
          delete cache;
          cache = nullptr;
        }
      }
    }
    if (!cache) {
      
      LOG3(("Http2Session::RecvPushPromise Push Recevied without loadgroup cache\n"));
      self->GenerateRstStream(REFUSED_STREAM_ERROR, promisedID);
    } else {
      resetStream = false;
    }
  }

  if (resetStream) {
    
    
    self->mDecompressBuffer.Append(self->mInputFrameBuffer + 8 + paddingControlBytes + promiseLen,
                                   self->mInputFrameDataSize - paddingControlBytes - promiseLen - paddingLength);
    if (self->mInputFrameFlags & kFlag_END_PUSH_PROMISE) {
      rv = self->UncompressAndDiscard();
      if (NS_FAILED(rv)) {
        LOG3(("Http2Session::RecvPushPromise uncompress failed\n"));
        self->mGoAwayReason = COMPRESSION_ERROR;
        return rv;
      }
    }
    self->ResetDownstreamState();
    return NS_OK;
  }

  
  nsRefPtr<Http2PushTransactionBuffer> transactionBuffer =
    new Http2PushTransactionBuffer();
  transactionBuffer->SetConnection(self);
  Http2PushedStream *pushedStream =
    new Http2PushedStream(transactionBuffer, self,
                                 associatedStream, promisedID);

  
  
  
  
  self->mStreamTransactionHash.Put(transactionBuffer, pushedStream);
  self->mPushedStreams.AppendElement(pushedStream);

  self->mDecompressBuffer.Append(self->mInputFrameBuffer + 8 + paddingControlBytes + promiseLen,
                                 self->mInputFrameDataSize - paddingControlBytes - promiseLen - paddingLength);

  nsAutoCString requestHeaders;
  rv = pushedStream->ConvertPushHeaders(&self->mDecompressor,
                                        self->mDecompressBuffer, requestHeaders);

  if (rv == NS_ERROR_NOT_IMPLEMENTED) {
    LOG3(("Http2Session::PushPromise Semantics not Implemented\n"));
    self->GenerateRstStream(REFUSED_STREAM_ERROR, promisedID);
    return NS_OK;
  }

  if (NS_FAILED(rv))
    return rv;

  if (self->RegisterStreamID(pushedStream, promisedID) == kDeadStreamID) {
    LOG3(("Http2Session::RecvPushPromise registerstreamid failed\n"));
    self->mGoAwayReason = INTERNAL_ERROR;
    return NS_ERROR_FAILURE;
  }

  if (promisedID > self->mOutgoingGoAwayID)
    self->mOutgoingGoAwayID = promisedID;

  
  
  uint32_t notUsed;
  pushedStream->ReadSegments(nullptr, 1, &notUsed);

  nsAutoCString key;
  if (!pushedStream->GetHashKey(key)) {
    LOG3(("Http2Session::RecvPushPromise one of :authority :scheme :path missing from push\n"));
    self->CleanupStream(pushedStream, NS_ERROR_FAILURE, PROTOCOL_ERROR);
    self->ResetDownstreamState();
    return NS_OK;
  }

  if (!associatedStream->Origin().Equals(pushedStream->Origin())) {
    LOG3(("Http2Session::RecvPushPromise pushed stream mismatched origin\n"));
    self->CleanupStream(pushedStream, NS_ERROR_FAILURE, REFUSED_STREAM_ERROR);
    self->ResetDownstreamState();
    return NS_OK;
  }

  if (!cache->RegisterPushedStreamHttp2(key, pushedStream)) {
    LOG3(("Http2Session::RecvPushPromise registerPushedStream Failed\n"));
    self->CleanupStream(pushedStream, NS_ERROR_FAILURE, INTERNAL_ERROR);
    self->ResetDownstreamState();
    return NS_OK;
  }

  static_assert(Http2Stream::kWorstPriority >= 0,
                "kWorstPriority out of range");
  uint8_t priorityWeight = (nsISupportsPriority::PRIORITY_LOWEST + 1) -
    (Http2Stream::kWorstPriority - Http2Stream::kNormalPriority);
  pushedStream->SetPriority(Http2Stream::kWorstPriority);
  self->GeneratePriority(promisedID, priorityWeight);
  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
Http2Session::RecvPing(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_PING);

  LOG3(("Http2Session::RecvPing %p PING Flags 0x%X.", self,
        self->mInputFrameFlags));

  if (self->mInputFrameDataSize != 8) {
    LOG3(("Http2Session::RecvPing %p PING had wrong amount of data %d",
          self, self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, FRAME_SIZE_ERROR);
  }

  if (self->mInputFrameID) {
    LOG3(("Http2Session::RecvPing %p PING needs stream ID of 0. 0x%X\n",
          self, self->mInputFrameID));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  if (self->mInputFrameFlags & kFlag_ACK) {
    
    self->mPingSentEpoch = 0;
  } else {
    
    self->GeneratePing(true);
  }

  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
Http2Session::RecvGoAway(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_GOAWAY);

  if (self->mInputFrameDataSize < 8) {
    
    
    LOG3(("Http2Session::RecvGoAway %p GOAWAY had wrong amount of data %d",
          self, self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  if (self->mInputFrameID) {
    LOG3(("Http2Session::RecvGoAway %p GOAWAY had non zero stream ID 0x%X\n",
          self, self->mInputFrameID));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  self->mShouldGoAway = true;
  self->mGoAwayID =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  self->mGoAwayID &= 0x7fffffff;
  self->mCleanShutdown = true;
  uint32_t statusCode =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[3]);

  
  
  
  self->mStreamTransactionHash.Enumerate(GoAwayEnumerator, self);

  
  uint32_t size = self->mGoAwayStreamsToRestart.GetSize();
  for (uint32_t count = 0; count < size; ++count) {
    Http2Stream *stream =
      static_cast<Http2Stream *>(self->mGoAwayStreamsToRestart.PopFront());

    self->CloseStream(stream, NS_ERROR_NET_RESET);
    if (stream->HasRegisteredID())
      self->mStreamIDHash.Remove(stream->StreamID());
    self->mStreamTransactionHash.Remove(stream->Transaction());
  }

  
  
  
  size = self->mQueuedStreams.GetSize();
  for (uint32_t count = 0; count < size; ++count) {
    Http2Stream *stream =
      static_cast<Http2Stream *>(self->mQueuedStreams.PopFront());
    self->CloseStream(stream, NS_ERROR_NET_RESET);
    self->mStreamTransactionHash.Remove(stream->Transaction());
  }

  LOG3(("Http2Session::RecvGoAway %p GOAWAY Last-Good-ID 0x%X status 0x%X "
        "live streams=%d\n", self, self->mGoAwayID, statusCode,
        self->mStreamTransactionHash.Count()));

  self->ResetDownstreamState();
  return NS_OK;
}

PLDHashOperator
Http2Session::RestartBlockedOnRwinEnumerator(nsAHttpTransaction *key,
                                             nsAutoPtr<Http2Stream> &stream,
                                             void *closure)
{
  Http2Session *self = static_cast<Http2Session *>(closure);
  MOZ_ASSERT(self->mServerSessionWindow > 0);

  if (!stream->BlockedOnRwin() || stream->ServerReceiveWindow() <= 0)
    return PL_DHASH_NEXT;

  self->mReadyForWrite.Push(stream);
  self->SetWriteCallbacks();
  return PL_DHASH_NEXT;
}

nsresult
Http2Session::RecvWindowUpdate(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_WINDOW_UPDATE);

  if (self->mInputFrameDataSize != 4) {
    LOG3(("Http2Session::RecvWindowUpdate %p Window Update wrong length %d\n",
          self, self->mInputFrameDataSize));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  uint32_t delta =
    PR_ntohl(reinterpret_cast<uint32_t *>(self->mInputFrameBuffer.get())[2]);
  delta &= 0x7fffffff;

  LOG3(("Http2Session::RecvWindowUpdate %p len=%d Stream 0x%X.\n",
        self, delta, self->mInputFrameID));

  if (self->mInputFrameID) { 
    nsresult rv = self->SetInputFrameDataStream(self->mInputFrameID);
    if (NS_FAILED(rv))
      return rv;

    if (!self->mInputFrameDataStream) {
      LOG3(("Http2Session::RecvWindowUpdate %p lookup streamID 0x%X failed.\n",
            self, self->mInputFrameID));
      
      if (self->mInputFrameID >= self->mNextStreamID)
        self->GenerateRstStream(PROTOCOL_ERROR, self->mInputFrameID);
      self->ResetDownstreamState();
      return NS_OK;
    }

    int64_t oldRemoteWindow = self->mInputFrameDataStream->ServerReceiveWindow();
    self->mInputFrameDataStream->UpdateServerReceiveWindow(delta);
    if (self->mInputFrameDataStream->ServerReceiveWindow() >= 0x80000000) {
      
      
      LOG3(("Http2Session::RecvWindowUpdate %p stream window "
            "exceeds 2^31 - 1\n", self));
      self->CleanupStream(self->mInputFrameDataStream, NS_ERROR_ILLEGAL_VALUE,
                          FLOW_CONTROL_ERROR);
      self->ResetDownstreamState();
      return NS_OK;
    }

    LOG3(("Http2Session::RecvWindowUpdate %p stream 0x%X window "
          "%d increased by %d now %d.\n", self, self->mInputFrameID,
          oldRemoteWindow, delta, oldRemoteWindow + delta));

  } else { 
    int64_t oldRemoteWindow = self->mServerSessionWindow;
    self->mServerSessionWindow += delta;

    if (self->mServerSessionWindow >= 0x80000000) {
      
      
      LOG3(("Http2Session::RecvWindowUpdate %p session window "
            "exceeds 2^31 - 1\n", self));
      RETURN_SESSION_ERROR(self, FLOW_CONTROL_ERROR);
    }

    if ((oldRemoteWindow <= 0) && (self->mServerSessionWindow > 0)) {
      LOG3(("Http2Session::RecvWindowUpdate %p restart session window\n",
            self));
      self->mStreamTransactionHash.Enumerate(RestartBlockedOnRwinEnumerator, self);
    }
    LOG3(("Http2Session::RecvWindowUpdate %p session window "
          "%d increased by %d now %d.\n", self,
          oldRemoteWindow, delta, oldRemoteWindow + delta));
  }

  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
Http2Session::RecvContinuation(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_CONTINUATION);
  MOZ_ASSERT(self->mInputFrameID);
  MOZ_ASSERT(self->mExpectedPushPromiseID || self->mExpectedHeaderID);
  MOZ_ASSERT(!(self->mExpectedPushPromiseID && self->mExpectedHeaderID));

  LOG3(("Http2Session::RecvContinuation %p Flags 0x%X id 0x%X "
        "promise id 0x%X header id 0x%X\n",
        self, self->mInputFrameFlags, self->mInputFrameID,
        self->mExpectedPushPromiseID, self->mExpectedHeaderID));

  self->SetInputFrameDataStream(self->mInputFrameID);

  if (!self->mInputFrameDataStream) {
    LOG3(("Http2Session::RecvContination stream ID 0x%X not found.",
          self->mInputFrameID));
    RETURN_SESSION_ERROR(self, PROTOCOL_ERROR);
  }

  
  if (self->mExpectedHeaderID) {
    self->mInputFrameFlags &= ~kFlag_PRIORITY;
    return RecvHeaders(self);
  }

  
  if (self->mInputFrameFlags & kFlag_END_HEADERS) {
    self->mInputFrameFlags &= ~kFlag_END_HEADERS;
    self->mInputFrameFlags |= kFlag_END_PUSH_PROMISE;
  }
  return RecvPushPromise(self);
}

nsresult
Http2Session::RecvAltSvc(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_ALTSVC);
  LOG3(("Http2Session::RecvAltSvc %p Flags 0x%X id 0x%X\n", self,
        self->mInputFrameFlags, self->mInputFrameID));

  
  self->ResetDownstreamState();
  return NS_OK;
}

nsresult
Http2Session::RecvBlocked(Http2Session *self)
{
  MOZ_ASSERT(self->mInputFrameType == FRAME_TYPE_BLOCKED);
  LOG3(("Http2Session::RecvBlocked %p id 0x%X\n", self, self->mInputFrameID));

  if (self->mInputFrameDataSize) {
    RETURN_SESSION_ERROR(self, FRAME_SIZE_ERROR);
  }

  
  self->ResetDownstreamState();
  return NS_OK;
}






void
Http2Session::OnTransportStatus(nsITransport* aTransport,
                                nsresult aStatus, uint64_t aProgress)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  switch (aStatus) {
    
    
  case NS_NET_STATUS_RESOLVING_HOST:
  case NS_NET_STATUS_RESOLVED_HOST:
  case NS_NET_STATUS_CONNECTING_TO:
  case NS_NET_STATUS_CONNECTED_TO:
  {
    Http2Stream *target = mStreamIDHash.Get(1);
    nsAHttpTransaction *transaction = target ? target->Transaction() : nullptr;
    if (transaction)
      transaction->OnTransportStatus(aTransport, aStatus, aProgress);
    break;
  }

  default:
    
    
    
    

    
    
    
    
    
    
    

    
    
    
    
    
    

    
    
    

    break;
  }
}






nsresult
Http2Session::ReadSegments(nsAHttpSegmentReader *reader,
                           uint32_t count, uint32_t *countRead)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  MOZ_ASSERT(!mSegmentReader || !reader || (mSegmentReader == reader),
             "Inconsistent Write Function Callback");

  nsresult rv = ConfirmTLSProfile();
  if (NS_FAILED(rv))
    return rv;

  if (reader)
    mSegmentReader = reader;

  *countRead = 0;

  LOG3(("Http2Session::ReadSegments %p", this));

  Http2Stream *stream = static_cast<Http2Stream *>(mReadyForWrite.PopFront());
  if (!stream) {
    LOG3(("Http2Session %p could not identify a stream to write; suspending.",
          this));
    FlushOutputQueue();
    SetWriteCallbacks();
    return NS_BASE_STREAM_WOULD_BLOCK;
  }

  LOG3(("Http2Session %p will write from Http2Stream %p 0x%X "
        "block-input=%d block-output=%d\n", this, stream, stream->StreamID(),
        stream->RequestBlockedOnRead(), stream->BlockedOnRwin()));

  rv = stream->ReadSegments(this, count, countRead);

  
  
  
  
  FlushOutputQueue();

  
  
  ResumeRecv();

  if (stream->RequestBlockedOnRead()) {

    
    
    

    LOG3(("Http2Session::ReadSegments %p dealing with block on read", this));

    
    
    if (GetWriteQueueSize()) {
      rv = NS_OK;
    } else {
      rv = NS_BASE_STREAM_WOULD_BLOCK;
    }
    SetWriteCallbacks();
    return rv;
  }

  if (NS_FAILED(rv)) {
    LOG3(("Http2Session::ReadSegments %p returning FAIL code %X",
          this, rv));
    if (rv != NS_BASE_STREAM_WOULD_BLOCK)
      CleanupStream(stream, rv, CANCEL_ERROR);
    return rv;
  }

  if (*countRead > 0) {
    LOG3(("Http2Session::ReadSegments %p stream=%p countread=%d",
          this, stream, *countRead));
    mReadyForWrite.Push(stream);
    SetWriteCallbacks();
    return rv;
  }

  if (stream->BlockedOnRwin()) {
    LOG3(("Http2Session %p will stream %p 0x%X suspended for flow control\n",
          this, stream, stream->StreamID()));
    return NS_BASE_STREAM_WOULD_BLOCK;
  }

  LOG3(("Http2Session::ReadSegments %p stream=%p stream send complete",
        this, stream));

  
  
  SetWriteCallbacks();

  return rv;
}

nsresult
Http2Session::ReadyToProcessDataFrame(enum internalStateType newState)
{
  MOZ_ASSERT(newState == PROCESSING_DATA_FRAME ||
             newState == DISCARDING_DATA_FRAME_PADDING);
  ChangeDownstreamState(newState);

  Telemetry::Accumulate(Telemetry::SPDY_CHUNK_RECVD,
                        mInputFrameDataSize >> 10);
  mLastDataReadEpoch = mLastReadEpoch;

  if (!mInputFrameID) {
    LOG3(("Http2Session::ReadyToProcessDataFrame %p data frame stream 0\n",
          this));
    RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
  }

  if (mInputFrameFlags & kFlag_COMPRESSED) {
    LOG3(("Http2Session::ReadyToProcessDataFrame %p streamID 0x%X compressed\n",
          this, mInputFrameID));
    RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
  }

  nsresult rv = SetInputFrameDataStream(mInputFrameID);
  if (NS_FAILED(rv)) {
    LOG3(("Http2Session::ReadyToProcessDataFrame %p lookup streamID 0x%X "
          "failed. probably due to verification.\n", this, mInputFrameID));
    return rv;
  }
  if (!mInputFrameDataStream) {
    LOG3(("Http2Session::ReadyToProcessDataFrame %p lookup streamID 0x%X "
          "failed. Next = 0x%X", this, mInputFrameID, mNextStreamID));
    if (mInputFrameID >= mNextStreamID)
      GenerateRstStream(PROTOCOL_ERROR, mInputFrameID);
    ChangeDownstreamState(DISCARDING_DATA_FRAME);
  } else if (mInputFrameDataStream->RecvdFin() ||
            mInputFrameDataStream->RecvdReset() ||
            mInputFrameDataStream->SentReset()) {
    LOG3(("Http2Session::ReadyToProcessDataFrame %p streamID 0x%X "
          "Data arrived for already server closed stream.\n",
          this, mInputFrameID));
    if (mInputFrameDataStream->RecvdFin() || mInputFrameDataStream->RecvdReset())
      GenerateRstStream(STREAM_CLOSED_ERROR, mInputFrameID);
    ChangeDownstreamState(DISCARDING_DATA_FRAME);
  }

  LOG3(("Start Processing Data Frame. "
        "Session=%p Stream ID 0x%X Stream Ptr %p Fin=%d Len=%d",
        this, mInputFrameID, mInputFrameDataStream, mInputFrameFinal,
        mInputFrameDataSize));
  UpdateLocalRwin(mInputFrameDataStream, mInputFrameDataSize);

  return NS_OK;
}














nsresult
Http2Session::WriteSegments(nsAHttpSegmentWriter *writer,
                            uint32_t count, uint32_t *countWritten)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  LOG3(("Http2Session::WriteSegments %p InternalState %X\n",
        this, mDownstreamState));

  *countWritten = 0;

  if (mClosed)
    return NS_ERROR_FAILURE;

  nsresult rv = ConfirmTLSProfile();
  if (NS_FAILED(rv))
    return rv;

  SetWriteCallbacks();

  
  
  
  Http2Stream *pushConnectedStream =
    static_cast<Http2Stream *>(mReadyForRead.PopFront());
  if (pushConnectedStream) {
    LOG3(("Http2Session::WriteSegments %p processing pushed stream 0x%X\n",
          this, pushConnectedStream->StreamID()));
    mSegmentWriter = writer;
    rv = pushConnectedStream->WriteSegments(this, count, countWritten);
    mSegmentWriter = nullptr;

    
    
    if (NS_SUCCEEDED(rv) && !*countWritten &&
        pushConnectedStream->PushSource() &&
        pushConnectedStream->PushSource()->GetPushComplete()) {
      rv = NS_BASE_STREAM_CLOSED;
    }

    if (rv == NS_BASE_STREAM_CLOSED) {
      CleanupStream(pushConnectedStream, NS_OK, CANCEL_ERROR);
      rv = NS_OK;
    }

    
    
    
    if (NS_SUCCEEDED(rv) || rv == NS_BASE_STREAM_WOULD_BLOCK) {
      rv = NS_BASE_STREAM_WOULD_BLOCK;
      ResumeRecv();
    }

    return rv;
  }

  
  

  
  
  

  if (mDownstreamState == BUFFERING_OPENING_SETTINGS ||
      mDownstreamState == BUFFERING_FRAME_HEADER) {
    
    
    

    MOZ_ASSERT(mInputFrameBufferUsed < 8,
               "Frame Buffer Used Too Large for State");

    rv = NetworkRead(writer, mInputFrameBuffer + mInputFrameBufferUsed,
                     8 - mInputFrameBufferUsed, countWritten);

    if (NS_FAILED(rv)) {
      LOG3(("Http2Session %p buffering frame header read failure %x\n",
            this, rv));
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        rv = NS_OK;
      return rv;
    }

    LogIO(this, nullptr, "Reading Frame Header",
          mInputFrameBuffer + mInputFrameBufferUsed, *countWritten);

    mInputFrameBufferUsed += *countWritten;

    if (mInputFrameBufferUsed < 8)
    {
      LOG3(("Http2Session::WriteSegments %p "
            "BUFFERING FRAME HEADER incomplete size=%d",
            this, mInputFrameBufferUsed));
      return rv;
    }

    
    mInputFrameDataSize =
      PR_ntohs(reinterpret_cast<uint16_t *>(mInputFrameBuffer.get())[0]);
    mInputFrameType = reinterpret_cast<uint8_t *>(mInputFrameBuffer.get())[2];
    mInputFrameFlags = reinterpret_cast<uint8_t *>(mInputFrameBuffer.get())[3];
    mInputFrameID =
      PR_ntohl(reinterpret_cast<uint32_t *>(mInputFrameBuffer.get())[1]);
    mInputFrameID &= 0x7fffffff;
    mInputFrameDataRead = 0;

    if (mInputFrameType == FRAME_TYPE_DATA || mInputFrameType == FRAME_TYPE_HEADERS)  {
      mInputFrameFinal = mInputFrameFlags & kFlag_END_STREAM;
    } else {
      mInputFrameFinal = 0;
    }

    mPaddingLength = 0;
    if (mInputFrameType == FRAME_TYPE_DATA ||
        mInputFrameType == FRAME_TYPE_HEADERS ||
        mInputFrameType == FRAME_TYPE_PUSH_PROMISE ||
        mInputFrameType == FRAME_TYPE_CONTINUATION) {
      if ((mInputFrameFlags & kFlag_PAD_HIGH) &&
          !(mInputFrameFlags & kFlag_PAD_LOW)) {
        LOG3(("Http2Session::WriteSegments %p PROTOCOL_ERROR pad_high present "
              "without pad_low\n", this));
        RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
      }
    }

    if (mInputFrameDataSize >= 0x4000) {
      
      
      LOG3(("Http2Session::WriteSegments %p WARNING Frame Length bits past 14 are not 0 %08X\n",
            this, mInputFrameDataSize));
      mInputFrameDataSize &= 0x3fff;
    }

    LOG3(("Http2Session::WriteSegments[%p::%x] Frame Header Read "
          "type %X data len %u flags %x id 0x%X",
          this, mSerial, mInputFrameType, mInputFrameDataSize, mInputFrameFlags,
          mInputFrameID));

    
    
    if (mExpectedHeaderID &&
        ((mInputFrameType != FRAME_TYPE_CONTINUATION) ||
         (mExpectedHeaderID != mInputFrameID))) {
      LOG3(("Expected CONINUATION OF HEADERS for ID 0x%X\n", mExpectedHeaderID));
      RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
    }

    
    
    if (mExpectedPushPromiseID &&
        ((mInputFrameType != FRAME_TYPE_CONTINUATION) ||
         (mExpectedPushPromiseID != mInputFrameID))) {
      LOG3(("Expected CONTINUATION of PUSH PROMISE for ID 0x%X\n",
            mExpectedPushPromiseID));
      RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
    }

    if (mDownstreamState == BUFFERING_OPENING_SETTINGS &&
        mInputFrameType != FRAME_TYPE_SETTINGS) {
      LOG3(("First Frame Type Must Be Settings\n"));
      RETURN_SESSION_ERROR(this, PROTOCOL_ERROR);
    }

    if (mInputFrameType != FRAME_TYPE_DATA) { 
      EnsureBuffer(mInputFrameBuffer, mInputFrameDataSize + 8, 8,
                   mInputFrameBufferSize);
      ChangeDownstreamState(BUFFERING_CONTROL_FRAME);
    } else if (mInputFrameFlags & (kFlag_PAD_LOW | kFlag_PAD_HIGH)) {
      ChangeDownstreamState(PROCESSING_DATA_FRAME_PADDING_CONTROL);
    } else {
      rv = ReadyToProcessDataFrame(PROCESSING_DATA_FRAME);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  if (mDownstreamState == PROCESSING_DATA_FRAME_PADDING_CONTROL) {
    uint32_t numControlBytes = 0;
    if (mInputFrameFlags & kFlag_PAD_LOW) {
      ++numControlBytes;
    }
    if (mInputFrameFlags & kFlag_PAD_HIGH) {
      ++numControlBytes;
    }

    MOZ_ASSERT(numControlBytes,
               "Processing padding control with no control bytes!");
    MOZ_ASSERT(mInputFrameBufferUsed < (8 + numControlBytes),
               "Frame buffer used too large for state");

    rv = NetworkRead(writer, mInputFrameBuffer + mInputFrameBufferUsed,
                     (8 + numControlBytes) - mInputFrameBufferUsed,
                     countWritten);

    if (NS_FAILED(rv)) {
      LOG3(("Http2Session %p buffering data frame padding control read failure %x\n",
            this, rv));
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        rv = NS_OK;
      return rv;
    }

    LogIO(this, nullptr, "Reading Data Frame Padding Control",
          mInputFrameBuffer + mInputFrameBufferUsed, *countWritten);

    mInputFrameBufferUsed += *countWritten;

    if (mInputFrameBufferUsed - 8 < numControlBytes) {
      LOG3(("Http2Session::WriteSegments %p "
            "BUFFERING DATA FRAME CONTROL PADDING incomplete size=%d",
            this, mInputFrameBufferUsed - 8));
      return rv;
    }

    mInputFrameDataRead += numControlBytes;

    char *control = mInputFrameBuffer + 8;
    if (mInputFrameFlags & kFlag_PAD_HIGH) {
      mPaddingLength = static_cast<uint16_t>(*control) * 256;
      ++control;
    }
    mPaddingLength += static_cast<uint8_t>(*control);

    LOG3(("Http2Session::WriteSegments %p stream 0x%X mPaddingLength=%d", this,
          mInputFrameID, mPaddingLength));

    if (numControlBytes + mPaddingLength == mInputFrameDataSize) {
      
      LOG3(("Http2Session::WriteSegments %p stream 0x%X frame with only padding",
            this, mInputFrameID));
      rv = ReadyToProcessDataFrame(DISCARDING_DATA_FRAME_PADDING);
      if (NS_FAILED(rv)) {
        return rv;
      }
    } else {
      LOG3(("Http2Session::WriteSegments %p stream 0x%X ready to read HTTP data",
            this, mInputFrameID));
      rv = ReadyToProcessDataFrame(PROCESSING_DATA_FRAME);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  if (mDownstreamState == PROCESSING_CONTROL_RST_STREAM) {
    nsresult streamCleanupCode;

    
    
    
    if (mDownstreamRstReason == REFUSED_STREAM_ERROR) {
      streamCleanupCode = NS_ERROR_NET_RESET;      
    } else {
      streamCleanupCode = NS_ERROR_NET_INTERRUPT;
    }

    if (mDownstreamRstReason == COMPRESSION_ERROR)
      mShouldGoAway = true;

    
    Http2Stream *stream = mInputFrameDataStream;
    ResetDownstreamState();
    LOG3(("Http2Session::WriteSegments cleanup stream on recv of rst "
          "session=%p stream=%p 0x%X\n", this, stream,
          stream ? stream->StreamID() : 0));
    CleanupStream(stream, streamCleanupCode, CANCEL_ERROR);
    return NS_OK;
  }

  if (mDownstreamState == PROCESSING_DATA_FRAME ||
      mDownstreamState == PROCESSING_COMPLETE_HEADERS) {

    
    
    MOZ_ASSERT(!mNeedsCleanup, "cleanup stream set unexpectedly");
    mNeedsCleanup = nullptr;                     

    Http2Stream *stream = mInputFrameDataStream;
    mSegmentWriter = writer;
    rv = mInputFrameDataStream->WriteSegments(this, count, countWritten);
    mSegmentWriter = nullptr;

    mLastDataReadEpoch = mLastReadEpoch;

    if (SoftStreamError(rv)) {
      
      
      

      
      
      mDownstreamState = PROCESSING_DATA_FRAME;

      if (mInputFrameDataRead == mInputFrameDataSize)
        ResetDownstreamState();
      LOG3(("Http2Session::WriteSegments session=%p stream=%p 0x%X "
            "needscleanup=%p. cleanup stream based on "
            "stream->writeSegments returning code %x\n",
            this, stream, stream ? stream->StreamID() : 0,
            mNeedsCleanup, rv));
      CleanupStream(stream, NS_OK, CANCEL_ERROR);
      MOZ_ASSERT(!mNeedsCleanup || mNeedsCleanup == stream);
      mNeedsCleanup = nullptr;
      return NS_OK;
    }

    if (mNeedsCleanup) {
      LOG3(("Http2Session::WriteSegments session=%p stream=%p 0x%X "
            "cleanup stream based on mNeedsCleanup.\n",
            this, mNeedsCleanup, mNeedsCleanup ? mNeedsCleanup->StreamID() : 0));
      CleanupStream(mNeedsCleanup, NS_OK, CANCEL_ERROR);
      mNeedsCleanup = nullptr;
    }

    if (NS_FAILED(rv)) {
      LOG3(("Http2Session %p data frame read failure %x\n", this, rv));
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        rv = NS_OK;
    }

    return rv;
  }

  if (mDownstreamState == DISCARDING_DATA_FRAME ||
      mDownstreamState == DISCARDING_DATA_FRAME_PADDING) {
    char trash[4096];
    uint32_t count = std::min(4096U, mInputFrameDataSize - mInputFrameDataRead);
    LOG3(("Http2Session::WriteSegments %p trying to discard %d bytes of data",
          this, count));

    if (!count) {
      ResetDownstreamState();
      ResumeRecv();
      return NS_BASE_STREAM_WOULD_BLOCK;
    }

    rv = NetworkRead(writer, trash, count, countWritten);

    if (NS_FAILED(rv)) {
      LOG3(("Http2Session %p discard frame read failure %x\n", this, rv));
      
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        rv = NS_OK;
      return rv;
    }

    LogIO(this, nullptr, "Discarding Frame", trash, *countWritten);

    mInputFrameDataRead += *countWritten;

    if (mInputFrameDataRead == mInputFrameDataSize) {
      Http2Stream *streamToCleanup = nullptr;
      if (mInputFrameFinal) {
        streamToCleanup = mInputFrameDataStream;
      }

      ResetDownstreamState();

      if (streamToCleanup) {
        CleanupStream(streamToCleanup, NS_OK, CANCEL_ERROR);
      }
    }
    return rv;
  }

  if (mDownstreamState != BUFFERING_CONTROL_FRAME) {
    MOZ_ASSERT(false); 
    return NS_ERROR_UNEXPECTED;
  }

  MOZ_ASSERT(mInputFrameBufferUsed == 8, "Frame Buffer Header Not Present");
  MOZ_ASSERT(mInputFrameDataSize + 8 <= mInputFrameBufferSize,
             "allocation for control frame insufficient");

  rv = NetworkRead(writer, mInputFrameBuffer + 8 + mInputFrameDataRead,
                   mInputFrameDataSize - mInputFrameDataRead, countWritten);

  if (NS_FAILED(rv)) {
    LOG3(("Http2Session %p buffering control frame read failure %x\n",
          this, rv));
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK)
      rv = NS_OK;
    return rv;
  }

  LogIO(this, nullptr, "Reading Control Frame",
        mInputFrameBuffer + 8 + mInputFrameDataRead, *countWritten);

  mInputFrameDataRead += *countWritten;

  if (mInputFrameDataRead != mInputFrameDataSize)
    return NS_OK;

  MOZ_ASSERT(mInputFrameType != FRAME_TYPE_DATA);
  if (mInputFrameType < FRAME_TYPE_LAST) {
    rv = sControlFunctions[mInputFrameType](this);
  } else {
    
    
    LOG3(("Http2Session %p unknow frame type %x ignored\n",
          this, mInputFrameType));
    ResetDownstreamState();
    rv = NS_OK;
  }

  MOZ_ASSERT(NS_FAILED(rv) ||
             mDownstreamState != BUFFERING_CONTROL_FRAME,
             "Control Handler returned OK but did not change state");

  if (mShouldGoAway && !mStreamTransactionHash.Count())
    Close(NS_OK);
  return rv;
}

void
Http2Session::UpdateLocalStreamWindow(Http2Stream *stream, uint32_t bytes)
{
  if (!stream) 
    return;

  
  
  if (!stream || stream->RecvdFin() || stream->RecvdReset() ||
      mInputFrameFinal) {
    return;
  }

  stream->DecrementClientReceiveWindow(bytes);

  
  
  uint64_t unacked = stream->LocalUnAcked();
  int64_t  localWindow = stream->ClientReceiveWindow();

  LOG3(("Http2Session::UpdateLocalStreamWindow this=%p id=0x%X newbytes=%u "
        "unacked=%llu localWindow=%lld\n",
        this, stream->StreamID(), bytes, unacked, localWindow));

  if (!unacked)
    return;

  if ((unacked < kMinimumToAck) && (localWindow > kEmergencyWindowThreshold))
    return;

  if (!stream->HasSink()) {
    LOG3(("Http2Session::UpdateLocalStreamWindow %p 0x%X Pushed Stream Has No Sink\n",
          this, stream->StreamID()));
    return;
  }

  
  
  uint32_t toack = (unacked <= 0x7fffffffU) ? unacked : 0x7fffffffU;

  LOG3(("Http2Session::UpdateLocalStreamWindow Ack this=%p id=0x%X acksize=%d\n",
        this, stream->StreamID(), toack));
  stream->IncrementClientReceiveWindow(toack);

  
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 12;
  MOZ_ASSERT(mOutputQueueUsed <= mOutputQueueSize);

  CreateFrameHeader(packet, 4, FRAME_TYPE_WINDOW_UPDATE, 0, stream->StreamID());
  CopyAsNetwork32(packet + 8, toack);

  LogIO(this, stream, "Stream Window Update", packet, 12);
  
  
}

void
Http2Session::UpdateLocalSessionWindow(uint32_t bytes)
{
  if (!bytes)
    return;

  mLocalSessionWindow -= bytes;

  LOG3(("Http2Session::UpdateLocalSessionWindow this=%p newbytes=%u "
        "localWindow=%lld\n", this, bytes, mLocalSessionWindow));

  
  
  if ((mLocalSessionWindow > (ASpdySession::kInitialRwin - kMinimumToAck)) &&
      (mLocalSessionWindow > kEmergencyWindowThreshold))
    return;

  
  uint64_t toack64 = ASpdySession::kInitialRwin - mLocalSessionWindow;
  uint32_t toack = (toack64 <= 0x7fffffffU) ? toack64 : 0x7fffffffU;

  LOG3(("Http2Session::UpdateLocalSessionWindow Ack this=%p acksize=%u\n",
        this, toack));
  mLocalSessionWindow += toack;

  
  char *packet = mOutputQueueBuffer.get() + mOutputQueueUsed;
  mOutputQueueUsed += 12;
  MOZ_ASSERT(mOutputQueueUsed <= mOutputQueueSize);

  CreateFrameHeader(packet, 4, FRAME_TYPE_WINDOW_UPDATE, 0, 0);
  CopyAsNetwork32(packet + 8, toack);

  LogIO(this, nullptr, "Session Window Update", packet, 12);
  
}

void
Http2Session::UpdateLocalRwin(Http2Stream *stream, uint32_t bytes)
{
  
  
  EnsureOutputBuffer(16 * 2);

  UpdateLocalStreamWindow(stream, bytes);
  UpdateLocalSessionWindow(bytes);
  FlushOutputQueue();
}

void
Http2Session::Close(nsresult aReason)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

  if (mClosed)
    return;

  LOG3(("Http2Session::Close %p %X", this, aReason));

  mClosed = true;

  mStreamTransactionHash.Enumerate(ShutdownEnumerator, this);
  mStreamIDHash.Clear();
  mStreamTransactionHash.Clear();

  uint32_t goAwayReason;
  if (mGoAwayReason != NO_HTTP_ERROR) {
    goAwayReason = mGoAwayReason;
  } else if (NS_SUCCEEDED(aReason)) {
    goAwayReason = NO_HTTP_ERROR;
  } else if (aReason == NS_ERROR_ILLEGAL_VALUE) {
    goAwayReason = PROTOCOL_ERROR;
  } else {
    goAwayReason = INTERNAL_ERROR;
  }
  GenerateGoAway(goAwayReason);
  mConnection = nullptr;
  mSegmentReader = nullptr;
  mSegmentWriter = nullptr;
}

nsHttpConnectionInfo *
Http2Session::ConnectionInfo()
{
  nsRefPtr<nsHttpConnectionInfo> ci;
  GetConnectionInfo(getter_AddRefs(ci));
  return ci.get();
}

void
Http2Session::CloseTransaction(nsAHttpTransaction *aTransaction,
                               nsresult aResult)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::CloseTransaction %p %p %x", this, aTransaction, aResult));

  

  
  Http2Stream *stream = mStreamTransactionHash.Get(aTransaction);
  if (!stream) {
    LOG3(("Http2Session::CloseTransaction %p %p %x - not found.",
          this, aTransaction, aResult));
    return;
  }
  LOG3(("Http2Session::CloseTranscation probably a cancel. "
        "this=%p, trans=%p, result=%x, streamID=0x%X stream=%p",
        this, aTransaction, aResult, stream->StreamID(), stream));
  CleanupStream(stream, aResult, CANCEL_ERROR);
  ResumeRecv();
}





nsresult
Http2Session::OnReadSegment(const char *buf,
                            uint32_t count, uint32_t *countRead)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  nsresult rv;

  
  
  if (mOutputQueueUsed)
    FlushOutputQueue();

  if (!mOutputQueueUsed && mSegmentReader) {
    
    rv = mSegmentReader->OnReadSegment(buf, count, countRead);

    if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
      *countRead = 0;
    } else if (NS_FAILED(rv)) {
      return rv;
    }

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
Http2Session::CommitToSegmentSize(uint32_t count, bool forceCommitment)
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

  
  EnsureOutputBuffer(count + kQueueReserved);

  MOZ_ASSERT((mOutputQueueUsed + count) <= (mOutputQueueSize - kQueueReserved),
             "buffer not as large as expected");

  return NS_OK;
}





nsresult
Http2Session::OnWriteSegment(char *buf,
                             uint32_t count, uint32_t *countWritten)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  nsresult rv;

  if (!mSegmentWriter) {
    
    
    return NS_ERROR_FAILURE;
  }

  if (mDownstreamState == PROCESSING_DATA_FRAME) {

    if (mInputFrameFinal &&
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
    if (mPaddingLength && (mInputFrameDataSize - mInputFrameDataRead <= mPaddingLength)) {
      
      
      
      
      
      ChangeDownstreamState(DISCARDING_DATA_FRAME_PADDING);
      uint32_t paddingRead = mPaddingLength - (mInputFrameDataSize - mInputFrameDataRead);
      LOG3(("Http2Session::OnWriteSegment %p stream 0x%X len=%d read=%d "
            "crossed from HTTP data into padding (%d of %d) countWritten=%d",
            this, mInputFrameID, mInputFrameDataSize, mInputFrameDataRead,
            paddingRead, mPaddingLength, *countWritten));
      *countWritten -= paddingRead;
      LOG3(("Http2Session::OnWriteSegment %p stream 0x%X new countWritten=%d",
            this, mInputFrameID, *countWritten));
    }

    mInputFrameDataStream->UpdateTransportReadEvents(*countWritten);
    if ((mInputFrameDataRead == mInputFrameDataSize) && !mInputFrameFinal)
      ResetDownstreamState();

    return rv;
  }

  if (mDownstreamState == PROCESSING_COMPLETE_HEADERS) {

    if (mFlatHTTPResponseHeaders.Length() == mFlatHTTPResponseHeadersOut &&
        mInputFrameFinal) {
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
      if (!mInputFrameFinal) {
        
        
        
        
        ResetDownstreamState();
      }
    }

    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}

void
Http2Session::SetNeedsCleanup()
{
  LOG3(("Http2Session::SetNeedsCleanup %p - recorded downstream fin of "
        "stream %p 0x%X", this, mInputFrameDataStream,
        mInputFrameDataStream->StreamID()));

  
  MOZ_ASSERT(!mNeedsCleanup, "mNeedsCleanup unexpectedly set");
  mNeedsCleanup = mInputFrameDataStream;
  ResetDownstreamState();
}

void
Http2Session::ConnectPushedStream(Http2Stream *stream)
{
  mReadyForRead.Push(stream);
  ForceRecv();
}

uint32_t
Http2Session::FindTunnelCount(nsHttpConnectionInfo *aConnInfo)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  uint32_t rv = 0;
  mTunnelHash.Get(aConnInfo->HashKey(), &rv);
  return rv;
}

void
Http2Session::RegisterTunnel(Http2Stream *aTunnel)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  nsHttpConnectionInfo *ci = aTunnel->Transaction()->ConnectionInfo();
  uint32_t newcount = FindTunnelCount(ci) + 1;
  mTunnelHash.Remove(ci->HashKey());
  mTunnelHash.Put(ci->HashKey(), newcount);
  LOG3(("Http2Stream::RegisterTunnel %p stream=%p tunnels=%d [%s]",
        this, aTunnel, newcount, ci->HashKey().get()));
}

void
Http2Session::UnRegisterTunnel(Http2Stream *aTunnel)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  nsHttpConnectionInfo *ci = aTunnel->Transaction()->ConnectionInfo();
  MOZ_ASSERT(FindTunnelCount(ci));
  uint32_t newcount = FindTunnelCount(ci) - 1;
  mTunnelHash.Remove(ci->HashKey());
  if (newcount) {
    mTunnelHash.Put(ci->HashKey(), newcount);
  }
  LOG3(("Http2Session::UnRegisterTunnel %p stream=%p tunnels=%d [%s]",
        this, aTunnel, newcount, ci->HashKey().get()));
}

void
Http2Session::DispatchOnTunnel(nsAHttpTransaction *aHttpTransaction,
                               nsIInterfaceRequestor *aCallbacks)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  nsHttpTransaction *trans = aHttpTransaction->QueryHttpTransaction();
  nsHttpConnectionInfo *ci = aHttpTransaction->ConnectionInfo();
  MOZ_ASSERT(trans);

  LOG3(("Http2Session::DispatchOnTunnel %p trans=%p", this, trans));

  aHttpTransaction->SetConnection(nullptr);

  
  
  trans->SetDontRouteViaWildCard(true);

  if (FindTunnelCount(ci) < gHttpHandler->MaxConnectionsPerOrigin()) {
    LOG3(("Http2Session::DispatchOnTunnel %p create on new tunnel %s",
          this, ci->HashKey().get()));
    nsRefPtr<SpdyConnectTransaction> connectTrans =
      new SpdyConnectTransaction(ci, aCallbacks,
                                 trans->Caps(), trans, this);
    AddStream(connectTrans, trans->Priority(),
              false, nullptr);
    Http2Stream *tunnel = mStreamTransactionHash.Get(connectTrans);
    MOZ_ASSERT(tunnel);
    RegisterTunnel(tunnel);
  }

  
  
  
  trans->EnableKeepAlive();
  gHttpHandler->InitiateTransaction(trans, trans->Priority());
}


nsresult
Http2Session::BufferOutput(const char *buf,
                           uint32_t count,
                           uint32_t *countRead)
{
  nsAHttpSegmentReader *old = mSegmentReader;
  mSegmentReader = nullptr;
  nsresult rv = OnReadSegment(buf, count, countRead);
  mSegmentReader = old;
  return rv;
}

nsresult
Http2Session::ConfirmTLSProfile()
{
  if (mTLSProfileConfirmed)
    return NS_OK;

  LOG3(("Http2Session::ConfirmTLSProfile %p mConnection=%p\n",
        this, mConnection.get()));

  if (!gHttpHandler->EnforceHttp2TlsProfile()) {
    LOG3(("Http2Session::ConfirmTLSProfile %p passed due to configuration bypass\n", this));
    mTLSProfileConfirmed = true;
    return NS_OK;
  }

  if (!mConnection)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> securityInfo;
  mConnection->GetSecurityInfo(getter_AddRefs(securityInfo));
  nsCOMPtr<nsISSLSocketControl> ssl = do_QueryInterface(securityInfo);
  LOG3(("Http2Session::ConfirmTLSProfile %p sslsocketcontrol=%p\n", this, ssl.get()));
  if (!ssl)
    return NS_ERROR_FAILURE;

  int16_t version = ssl->GetSSLVersionUsed();
  LOG3(("Http2Session::ConfirmTLSProfile %p version=%x\n", this, version));
  if (version < nsISSLSocketControl::TLS_VERSION_1_2) {
    LOG3(("Http2Session::ConfirmTLSProfile %p FAILED due to lack of TLS1.2\n", this));
    RETURN_SESSION_ERROR(this, INADEQUATE_SECURITY);
  }

  uint16_t kea = ssl->GetKEAUsed();
  if (kea != ssl_kea_dh && kea != ssl_kea_ecdh) {
    LOG3(("Http2Session::ConfirmTLSProfile %p FAILED due to invalid KEA %d\n",
          this, kea));
    RETURN_SESSION_ERROR(this, INADEQUATE_SECURITY);
  }

  uint32_t keybits = ssl->GetKEAKeyBits();
  if (kea == ssl_kea_dh && keybits < 2048) {
    LOG3(("Http2Session::ConfirmTLSProfile %p FAILED due to DH %d < 2048\n",
          this, keybits));
    RETURN_SESSION_ERROR(this, INADEQUATE_SECURITY);
  } else if (kea == ssl_kea_ecdh && keybits < 256) { 
    LOG3(("Http2Session::ConfirmTLSProfile %p FAILED due to ECDH %d < 256\n",
          this, keybits));
    RETURN_SESSION_ERROR(this, INADEQUATE_SECURITY);
  }

  


  




  mTLSProfileConfirmed = true;
  return NS_OK;
}






void
Http2Session::TransactionHasDataToWrite(nsAHttpTransaction *caller)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::TransactionHasDataToWrite %p trans=%p", this, caller));

  
  

  Http2Stream *stream = mStreamTransactionHash.Get(caller);
  if (!stream || !VerifyStream(stream)) {
    LOG3(("Http2Session::TransactionHasDataToWrite %p caller %p not found",
          this, caller));
    return;
  }

  LOG3(("Http2Session::TransactionHasDataToWrite %p ID is 0x%X\n",
        this, stream->StreamID()));

  mReadyForWrite.Push(stream);
  SetWriteCallbacks();

  
  
  
  ForceSend();

}

void
Http2Session::TransactionHasDataToWrite(Http2Stream *stream)
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  LOG3(("Http2Session::TransactionHasDataToWrite %p stream=%p ID=0x%x",
        this, stream, stream->StreamID()));

  mReadyForWrite.Push(stream);
  SetWriteCallbacks();
  ForceSend();
}

bool
Http2Session::IsPersistent()
{
  return true;
}

nsresult
Http2Session::TakeTransport(nsISocketTransport **,
                            nsIAsyncInputStream **, nsIAsyncOutputStream **)
{
  MOZ_ASSERT(false, "TakeTransport of Http2Session");
  return NS_ERROR_UNEXPECTED;
}

nsHttpConnection *
Http2Session::TakeHttpConnection()
{
  MOZ_ASSERT(false, "TakeHttpConnection of Http2Session");
  return nullptr;
}

uint32_t
Http2Session::CancelPipeline(nsresult reason)
{
  
  return 0;
}

nsAHttpTransaction::Classifier
Http2Session::Classification()
{
  if (!mConnection)
    return nsAHttpTransaction::CLASS_GENERAL;
  return mConnection->Classification();
}

void
Http2Session::GetSecurityCallbacks(nsIInterfaceRequestor **aOut)
{
  *aOut = nullptr;
}








void
Http2Session::SetConnection(nsAHttpConnection *)
{
  
  MOZ_ASSERT(false, "Http2Session::SetConnection()");
}

void
Http2Session::SetProxyConnectFailed()
{
  MOZ_ASSERT(false, "Http2Session::SetProxyConnectFailed()");
}

bool
Http2Session::IsDone()
{
  return !mStreamTransactionHash.Count();
}

nsresult
Http2Session::Status()
{
  MOZ_ASSERT(false, "Http2Session::Status()");
  return NS_ERROR_UNEXPECTED;
}

uint32_t
Http2Session::Caps()
{
  MOZ_ASSERT(false, "Http2Session::Caps()");
  return 0;
}

void
Http2Session::SetDNSWasRefreshed()
{
  MOZ_ASSERT(false, "Http2Session::SetDNSWasRefreshed()");
}

uint64_t
Http2Session::Available()
{
  MOZ_ASSERT(false, "Http2Session::Available()");
  return 0;
}

nsHttpRequestHead *
Http2Session::RequestHead()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  MOZ_ASSERT(false,
             "Http2Session::RequestHead() "
             "should not be called after http/2 is setup");
  return NULL;
}

uint32_t
Http2Session::Http1xTransactionCount()
{
  return 0;
}


static PLDHashOperator
  TakeStream(nsAHttpTransaction *key,
             nsAutoPtr<Http2Stream> &stream,
             void *closure)
{
  nsTArray<nsRefPtr<nsAHttpTransaction> > *list =
    static_cast<nsTArray<nsRefPtr<nsAHttpTransaction> > *>(closure);

  list->AppendElement(key);

  
  
  return PL_DHASH_REMOVE;
}

nsresult
Http2Session::TakeSubTransactions(
  nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions)
{
  
  

  LOG3(("Http2Session::TakeSubTransactions %p\n", this));

  if (mConcurrentHighWater > 0)
    return NS_ERROR_ALREADY_OPENED;

  LOG3(("   taking %d\n", mStreamTransactionHash.Count()));

  mStreamTransactionHash.Enumerate(TakeStream, &outTransactions);
  return NS_OK;
}

nsresult
Http2Session::AddTransaction(nsAHttpTransaction *)
{
  
  

  MOZ_ASSERT(false,
             "Http2Session::AddTransaction() should not be called");

  return NS_ERROR_NOT_IMPLEMENTED;
}

uint32_t
Http2Session::PipelineDepth()
{
  return IsDone() ? 0 : 1;
}

nsresult
Http2Session::SetPipelinePosition(int32_t position)
{
  
  

  MOZ_ASSERT(false,
             "Http2Session::SetPipelinePosition() should not be called");

  return NS_ERROR_NOT_IMPLEMENTED;
}

int32_t
Http2Session::PipelinePosition()
{
  return 0;
}





nsAHttpConnection *
Http2Session::Connection()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  return mConnection;
}

nsresult
Http2Session::OnHeadersAvailable(nsAHttpTransaction *transaction,
                                 nsHttpRequestHead *requestHead,
                                 nsHttpResponseHead *responseHead, bool *reset)
{
  return mConnection->OnHeadersAvailable(transaction,
                                         requestHead,
                                         responseHead,
                                         reset);
}

bool
Http2Session::IsReused()
{
  return mConnection->IsReused();
}

nsresult
Http2Session::PushBack(const char *buf, uint32_t len)
{
  return mConnection->PushBack(buf, len);
}

} 
} 
