





#include <string.h>

#include "mozilla/DebugOnly.h"
#include "mozilla/Endian.h"
#include <stdint.h>

#include "nsDebug.h"
#include "MediaDecoderReader.h"
#include "OggCodecState.h"
#include "OggDecoder.h"
#include "nsISupportsImpl.h"
#include "VideoUtils.h"
#include <algorithm>




#ifdef MOZ_WIDGET_GONK
#ifdef version_major
#undef version_major
#endif
#ifdef version_minor
#undef version_minor
#endif
#endif

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif


OggCodecState*
OggCodecState::Create(ogg_page* aPage)
{
  NS_ASSERTION(ogg_page_bos(aPage), "Only call on BOS page!");
  nsAutoPtr<OggCodecState> codecState;
  if (aPage->body_len > 6 && memcmp(aPage->body+1, "theora", 6) == 0) {
    codecState = new TheoraState(aPage);
  } else if (aPage->body_len > 6 && memcmp(aPage->body+1, "vorbis", 6) == 0) {
    codecState = new VorbisState(aPage);
#ifdef MOZ_OPUS
  } else if (aPage->body_len > 8 && memcmp(aPage->body, "OpusHead", 8) == 0) {
    codecState = new OpusState(aPage);
#endif
  } else if (aPage->body_len > 8 && memcmp(aPage->body, "fishead\0", 8) == 0) {
    codecState = new SkeletonState(aPage);
  } else {
    codecState = new OggCodecState(aPage, false);
  }
  return codecState->OggCodecState::Init() ? codecState.forget() : nullptr;
}

OggCodecState::OggCodecState(ogg_page* aBosPage, bool aActive) :
  mPacketCount(0),
  mSerial(ogg_page_serialno(aBosPage)),
  mActive(aActive),
  mDoneReadingHeaders(!aActive)
{
  MOZ_COUNT_CTOR(OggCodecState);
  memset(&mState, 0, sizeof(ogg_stream_state));
}

OggCodecState::~OggCodecState() {
  MOZ_COUNT_DTOR(OggCodecState);
  Reset();
#ifdef DEBUG
  int ret =
#endif
  ogg_stream_clear(&mState);
  NS_ASSERTION(ret == 0, "ogg_stream_clear failed");
}

nsresult OggCodecState::Reset() {
  if (ogg_stream_reset(&mState) != 0) {
    return NS_ERROR_FAILURE;
  }
  mPackets.Erase();
  ClearUnstamped();
  return NS_OK;
}

void OggCodecState::ClearUnstamped()
{
  for (uint32_t i = 0; i < mUnstamped.Length(); ++i) {
    OggCodecState::ReleasePacket(mUnstamped[i]);
  }
  mUnstamped.Clear();
}

bool OggCodecState::Init() {
  int ret = ogg_stream_init(&mState, mSerial);
  return ret == 0;
}

bool OggCodecState::IsValidVorbisTagName(nsCString& aName)
{
  
  
  uint32_t length = aName.Length();
  const char* data = aName.Data();
  for (uint32_t i = 0; i < length; i++) {
    if (data[i] < 0x20 || data[i] > 0x7D || data[i] == '=') {
      return false;
    }
  }
  return true;
}

bool OggCodecState::AddVorbisComment(MetadataTags* aTags,
                                       const char* aComment,
                                       uint32_t aLength)
{
  const char* div = (const char*)memchr(aComment, '=', aLength);
  if (!div) {
    LOG(PR_LOG_DEBUG, ("Skipping comment: no separator"));
    return false;
  }
  nsCString key = nsCString(aComment, div-aComment);
  if (!IsValidVorbisTagName(key)) {
    LOG(PR_LOG_DEBUG, ("Skipping comment: invalid tag name"));
    return false;
  }
  uint32_t valueLength = aLength - (div-aComment);
  nsCString value = nsCString(div + 1, valueLength);
  if (!IsUTF8(value)) {
    LOG(PR_LOG_DEBUG, ("Skipping comment: invalid UTF-8 in value"));
    return false;
  }
  aTags->Put(key, value);
  return true;
}

void VorbisState::RecordVorbisPacketSamples(ogg_packet* aPacket,
                                              long aSamples)
{
#ifdef VALIDATE_VORBIS_SAMPLE_CALCULATION
  mVorbisPacketSamples[aPacket] = aSamples;
#endif
}

void VorbisState::ValidateVorbisPacketSamples(ogg_packet* aPacket,
                                                long aSamples)
{
#ifdef VALIDATE_VORBIS_SAMPLE_CALCULATION
  NS_ASSERTION(mVorbisPacketSamples[aPacket] == aSamples,
    "Decoded samples for Vorbis packet don't match expected!");
  mVorbisPacketSamples.erase(aPacket);
#endif
}

void VorbisState::AssertHasRecordedPacketSamples(ogg_packet* aPacket)
{
#ifdef VALIDATE_VORBIS_SAMPLE_CALCULATION
  NS_ASSERTION(mVorbisPacketSamples.count(aPacket) == 1,
    "Must have recorded packet samples");
#endif
}

static ogg_packet* Clone(ogg_packet* aPacket) {
  ogg_packet* p = new ogg_packet();
  memcpy(p, aPacket, sizeof(ogg_packet));
  p->packet = new unsigned char[p->bytes];
  memcpy(p->packet, aPacket->packet, p->bytes);
  return p;
}

void OggCodecState::ReleasePacket(ogg_packet* aPacket) {
  if (aPacket)
    delete [] aPacket->packet;
  delete aPacket;
}

void OggPacketQueue::Append(ogg_packet* aPacket) {
  nsDeque::Push(aPacket);
}

ogg_packet* OggCodecState::PacketOut() {
  if (mPackets.IsEmpty()) {
    return nullptr;
  }
  return mPackets.PopFront();
}

nsresult OggCodecState::PageIn(ogg_page* aPage) {
  if (!mActive)
    return NS_OK;
  NS_ASSERTION(static_cast<uint32_t>(ogg_page_serialno(aPage)) == mSerial,
               "Page must be for this stream!");
  if (ogg_stream_pagein(&mState, aPage) == -1)
    return NS_ERROR_FAILURE;
  int r;
  do {
    ogg_packet packet;
    r = ogg_stream_packetout(&mState, &packet);
    if (r == 1) {
      mPackets.Append(Clone(&packet));
    }
  } while (r != 0);
  if (ogg_stream_check(&mState)) {
    NS_WARNING("Unrecoverable error in ogg_stream_packetout");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult OggCodecState::PacketOutUntilGranulepos(bool& aFoundGranulepos) {
  int r;
  aFoundGranulepos = false;
  
  
  do {
    ogg_packet packet;
    r = ogg_stream_packetout(&mState, &packet);
    if (r == 1) {
      ogg_packet* clone = Clone(&packet);
      if (IsHeader(&packet)) {
        
        mPackets.Append(clone);
      } else {
        
        
        
        mUnstamped.AppendElement(clone);
        aFoundGranulepos = packet.granulepos > 0;
      }
    }
  } while (r != 0 && !aFoundGranulepos);
  if (ogg_stream_check(&mState)) {
    NS_WARNING("Unrecoverable error in ogg_stream_packetout");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

TheoraState::TheoraState(ogg_page* aBosPage) :
  OggCodecState(aBosPage, true),
  mSetup(0),
  mCtx(0),
  mPixelAspectRatio(0)
{
  MOZ_COUNT_CTOR(TheoraState);
  th_info_init(&mInfo);
  th_comment_init(&mComment);
}

TheoraState::~TheoraState() {
  MOZ_COUNT_DTOR(TheoraState);
  th_setup_free(mSetup);
  th_decode_free(mCtx);
  th_comment_clear(&mComment);
  th_info_clear(&mInfo);
}

bool TheoraState::Init() {
  if (!mActive)
    return false;

  int64_t n = mInfo.aspect_numerator;
  int64_t d = mInfo.aspect_denominator;

  mPixelAspectRatio = (n == 0 || d == 0) ?
    1.0f : static_cast<float>(n) / static_cast<float>(d);

  
  
  nsIntSize frame(mInfo.frame_width, mInfo.frame_height);
  nsIntRect picture(mInfo.pic_x, mInfo.pic_y, mInfo.pic_width, mInfo.pic_height);
  if (!IsValidVideoRegion(frame, picture, frame)) {
    return mActive = false;
  }

  mCtx = th_decode_alloc(&mInfo, mSetup);
  if (mCtx == nullptr) {
    return mActive = false;
  }

  return true;
}

bool
TheoraState::DecodeHeader(ogg_packet* aPacket)
{
  nsAutoRef<ogg_packet> autoRelease(aPacket);
  mPacketCount++;
  int ret = th_decode_headerin(&mInfo,
                               &mComment,
                               &mSetup,
                               aPacket);
 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool isSetupHeader = aPacket->bytes > 0 && aPacket->packet[0] == 0x82;
  if (ret < 0 || mPacketCount > 3) {
    
    
    
    return false;
  } else if (ret > 0 && isSetupHeader && mPacketCount == 3) {
    
    mDoneReadingHeaders = true;
  }
  return true;
}

int64_t
TheoraState::Time(int64_t granulepos) {
  if (!mActive) {
    return -1;
  }
  return TheoraState::Time(&mInfo, granulepos);
}

bool
TheoraState::IsHeader(ogg_packet* aPacket) {
  return th_packet_isheader(aPacket);
}

# define TH_VERSION_CHECK(_info,_maj,_min,_sub) \
 (((_info)->version_major>(_maj)||(_info)->version_major==(_maj))&& \
 (((_info)->version_minor>(_min)||(_info)->version_minor==(_min))&& \
 (_info)->version_subminor>=(_sub)))

int64_t TheoraState::Time(th_info* aInfo, int64_t aGranulepos)
{
  if (aGranulepos < 0 || aInfo->fps_numerator == 0) {
    return -1;
  }
  
  
  int shift = aInfo->keyframe_granule_shift; 
  ogg_int64_t iframe = aGranulepos >> shift;
  ogg_int64_t pframe = aGranulepos - (iframe << shift);
  int64_t frameno = iframe + pframe - TH_VERSION_CHECK(aInfo, 3, 2, 1);
  CheckedInt64 t = ((CheckedInt64(frameno) + 1) * USECS_PER_S) * aInfo->fps_denominator;
  if (!t.isValid())
    return -1;
  t /= aInfo->fps_numerator;
  return t.isValid() ? t.value() : -1;
}

int64_t TheoraState::StartTime(int64_t granulepos) {
  if (granulepos < 0 || !mActive || mInfo.fps_numerator == 0) {
    return -1;
  }
  CheckedInt64 t = (CheckedInt64(th_granule_frame(mCtx, granulepos)) * USECS_PER_S) * mInfo.fps_denominator;
  if (!t.isValid())
    return -1;
  return t.value() / mInfo.fps_numerator;
}

int64_t
TheoraState::MaxKeyframeOffset()
{
  
  
  
  
  
  int64_t frameDuration;
  
  
  int64_t keyframeDiff = (1 << mInfo.keyframe_granule_shift) - 1;

  
  frameDuration = (mInfo.fps_denominator * USECS_PER_S) / mInfo.fps_numerator;

  
  return frameDuration * keyframeDiff;
}

nsresult
TheoraState::PageIn(ogg_page* aPage)
{
  if (!mActive)
    return NS_OK;
  NS_ASSERTION(static_cast<uint32_t>(ogg_page_serialno(aPage)) == mSerial,
               "Page must be for this stream!");
  if (ogg_stream_pagein(&mState, aPage) == -1)
    return NS_ERROR_FAILURE;
  bool foundGp;
  nsresult res = PacketOutUntilGranulepos(foundGp);
  if (NS_FAILED(res))
    return res;
  if (foundGp && mDoneReadingHeaders) {
    
    
    ReconstructTheoraGranulepos();
    for (uint32_t i = 0; i < mUnstamped.Length(); ++i) {
      ogg_packet* packet = mUnstamped[i];
#ifdef DEBUG
      NS_ASSERTION(!IsHeader(packet), "Don't try to recover header packet gp");
      NS_ASSERTION(packet->granulepos != -1, "Packet must have gp by now");
#endif
      mPackets.Append(packet);
    }
    mUnstamped.Clear();
  }
  return NS_OK;
}



int
TheoraVersion(th_info* info,
              unsigned char maj,
              unsigned char min,
              unsigned char sub)
{
  ogg_uint32_t ver = (maj << 16) + (min << 8) + sub;
  ogg_uint32_t th_ver = (info->version_major << 16) +
                        (info->version_minor << 8) +
                        info->version_subminor;
  return (th_ver >= ver) ? 1 : 0;
}

void TheoraState::ReconstructTheoraGranulepos()
{
  if (mUnstamped.Length() == 0) {
    return;
  }
  ogg_int64_t lastGranulepos = mUnstamped[mUnstamped.Length() - 1]->granulepos;
  NS_ASSERTION(lastGranulepos != -1, "Must know last granulepos");

  
  
  
  
  ogg_int64_t shift = mInfo.keyframe_granule_shift;
  ogg_int64_t version_3_2_1 = TheoraVersion(&mInfo,3,2,1);
  ogg_int64_t lastFrame = th_granule_frame(mCtx,
                                           lastGranulepos) + version_3_2_1;
  ogg_int64_t firstFrame = lastFrame - mUnstamped.Length() + 1;

  
  
  
  
  
  
  
  ogg_int64_t keyframe = lastGranulepos >> shift;

  
  
  
  
  for (uint32_t i = 0; i < mUnstamped.Length() - 1; ++i) {
    ogg_int64_t frame = firstFrame + i;
    ogg_int64_t granulepos;
    ogg_packet* packet = mUnstamped[i];
    bool isKeyframe = th_packet_iskeyframe(packet) == 1;

    if (isKeyframe) {
      granulepos = frame << shift;
      keyframe = frame;
    } else if (frame >= keyframe &&
                frame - keyframe < ((ogg_int64_t)1 << shift))
    {
      
      
      granulepos = (keyframe << shift) + (frame - keyframe);
    } else {
      
      
      
      ogg_int64_t k = std::max(frame - (((ogg_int64_t)1 << shift) - 1), version_3_2_1);
      granulepos = (k << shift) + (frame - k);
    }
    
    
    
    
    NS_ASSERTION(granulepos >= version_3_2_1,
                  "Invalid granulepos for Theora version");

    
    
    NS_ASSERTION(i == 0 ||
                 th_granule_frame(mCtx, granulepos) ==
                 th_granule_frame(mCtx, mUnstamped[i-1]->granulepos) + 1,
                 "Granulepos calculation is incorrect!");

    packet->granulepos = granulepos;
  }

  
  
  
  NS_ASSERTION(mUnstamped.Length() < 2 ||
    th_granule_frame(mCtx, mUnstamped[mUnstamped.Length()-2]->granulepos) + 1 ==
    th_granule_frame(mCtx, lastGranulepos),
    "Granulepos recovery should catch up with packet->granulepos!");
}

nsresult VorbisState::Reset()
{
  nsresult res = NS_OK;
  if (mActive && vorbis_synthesis_restart(&mDsp) != 0) {
    res = NS_ERROR_FAILURE;
  }
  if (NS_FAILED(OggCodecState::Reset())) {
    return NS_ERROR_FAILURE;
  }

  mGranulepos = 0;
  mPrevVorbisBlockSize = 0;

  return res;
}

VorbisState::VorbisState(ogg_page* aBosPage) :
  OggCodecState(aBosPage, true),
  mPrevVorbisBlockSize(0),
  mGranulepos(0)
{
  MOZ_COUNT_CTOR(VorbisState);
  vorbis_info_init(&mInfo);
  vorbis_comment_init(&mComment);
  memset(&mDsp, 0, sizeof(vorbis_dsp_state));
  memset(&mBlock, 0, sizeof(vorbis_block));
}

VorbisState::~VorbisState() {
  MOZ_COUNT_DTOR(VorbisState);
  Reset();
  vorbis_block_clear(&mBlock);
  vorbis_dsp_clear(&mDsp);
  vorbis_info_clear(&mInfo);
  vorbis_comment_clear(&mComment);
}

bool VorbisState::DecodeHeader(ogg_packet* aPacket) {
  nsAutoRef<ogg_packet> autoRelease(aPacket);
  mPacketCount++;
  int ret = vorbis_synthesis_headerin(&mInfo,
                                      &mComment,
                                      aPacket);
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  bool isSetupHeader = aPacket->bytes > 0 && aPacket->packet[0] == 0x5;

  if (ret < 0 || mPacketCount > 3) {
    
    
    
    return false;
  } else if (ret == 0 && isSetupHeader && mPacketCount == 3) {
    
    
    mDoneReadingHeaders = true;
  }
  return true;
}

bool VorbisState::Init()
{
  if (!mActive)
    return false;

  int ret = vorbis_synthesis_init(&mDsp, &mInfo);
  if (ret != 0) {
    NS_WARNING("vorbis_synthesis_init() failed initializing vorbis bitstream");
    return mActive = false;
  }
  ret = vorbis_block_init(&mDsp, &mBlock);
  if (ret != 0) {
    NS_WARNING("vorbis_block_init() failed initializing vorbis bitstream");
    if (mActive) {
      vorbis_dsp_clear(&mDsp);
    }
    return mActive = false;
  }
  return true;
}

int64_t VorbisState::Time(int64_t granulepos)
{
  if (!mActive) {
    return -1;
  }

  return VorbisState::Time(&mInfo, granulepos);
}

int64_t VorbisState::Time(vorbis_info* aInfo, int64_t aGranulepos)
{
  if (aGranulepos == -1 || aInfo->rate == 0) {
    return -1;
  }
  CheckedInt64 t = CheckedInt64(aGranulepos) * USECS_PER_S;
  if (!t.isValid())
    t = 0;
  return t.value() / aInfo->rate;
}

bool
VorbisState::IsHeader(ogg_packet* aPacket)
{
  
  
  
  
  
  return aPacket->bytes > 0 ? (aPacket->packet[0] & 0x1) : false;
}

MetadataTags*
VorbisState::GetTags()
{
  MetadataTags* tags;
  NS_ASSERTION(mComment.user_comments, "no vorbis comment strings!");
  NS_ASSERTION(mComment.comment_lengths, "no vorbis comment lengths!");
  tags = new MetadataTags;
  for (int i = 0; i < mComment.comments; i++) {
    AddVorbisComment(tags, mComment.user_comments[i],
                     mComment.comment_lengths[i]);
  }
  return tags;
}

nsresult
VorbisState::PageIn(ogg_page* aPage)
{
  if (!mActive)
    return NS_OK;
  NS_ASSERTION(static_cast<uint32_t>(ogg_page_serialno(aPage)) == mSerial,
               "Page must be for this stream!");
  if (ogg_stream_pagein(&mState, aPage) == -1)
    return NS_ERROR_FAILURE;
  bool foundGp;
  nsresult res = PacketOutUntilGranulepos(foundGp);
  if (NS_FAILED(res))
    return res;
  if (foundGp && mDoneReadingHeaders) {
    
    
    ReconstructVorbisGranulepos();
    for (uint32_t i = 0; i < mUnstamped.Length(); ++i) {
      ogg_packet* packet = mUnstamped[i];
      AssertHasRecordedPacketSamples(packet);
      NS_ASSERTION(!IsHeader(packet), "Don't try to recover header packet gp");
      NS_ASSERTION(packet->granulepos != -1, "Packet must have gp by now");
      mPackets.Append(packet);
    }
    mUnstamped.Clear();
  }
  return NS_OK;
}

nsresult VorbisState::ReconstructVorbisGranulepos()
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_ASSERTION(mUnstamped.Length() > 0, "Length must be > 0");
  ogg_packet* last = mUnstamped[mUnstamped.Length()-1];
  NS_ASSERTION(last->e_o_s || last->granulepos >= 0,
    "Must know last granulepos!");
  if (mUnstamped.Length() == 1) {
    ogg_packet* packet = mUnstamped[0];
    long blockSize = vorbis_packet_blocksize(&mInfo, packet);
    if (blockSize < 0) {
      
      
      
      blockSize = 0;
      mPrevVorbisBlockSize = 0;
    }
    long samples = mPrevVorbisBlockSize / 4 + blockSize / 4;
    mPrevVorbisBlockSize = blockSize;
    if (packet->granulepos == -1) {
      packet->granulepos = mGranulepos + samples;
    }

    
    if (packet->e_o_s && packet->granulepos >= mGranulepos) {
       samples = packet->granulepos - mGranulepos;
    }
 
    mGranulepos = packet->granulepos;
    RecordVorbisPacketSamples(packet, samples);
    return NS_OK;
  }

  bool unknownGranulepos = last->granulepos == -1;
  int totalSamples = 0;
  for (int32_t i = mUnstamped.Length() - 1; i > 0; i--) {
    ogg_packet* packet = mUnstamped[i];
    ogg_packet* prev = mUnstamped[i-1];
    ogg_int64_t granulepos = packet->granulepos;
    NS_ASSERTION(granulepos != -1, "Must know granulepos!");
    long prevBlockSize = vorbis_packet_blocksize(&mInfo, prev);
    long blockSize = vorbis_packet_blocksize(&mInfo, packet);

    if (blockSize < 0 || prevBlockSize < 0) {
      
      
      
      blockSize = 0;
      prevBlockSize = 0;
    }

    long samples = prevBlockSize / 4 + blockSize / 4;
    totalSamples += samples;
    prev->granulepos = granulepos - samples;
    RecordVorbisPacketSamples(packet, samples);
  }

  if (unknownGranulepos) {
    for (uint32_t i = 0; i < mUnstamped.Length(); i++) {
      ogg_packet* packet = mUnstamped[i];
      packet->granulepos += mGranulepos + totalSamples + 1;
    }
  }

  ogg_packet* first = mUnstamped[0];
  long blockSize = vorbis_packet_blocksize(&mInfo, first);
  if (blockSize < 0) {
    mPrevVorbisBlockSize = 0;
    blockSize = 0;
  }

  long samples = (mPrevVorbisBlockSize == 0) ? 0 :
                  mPrevVorbisBlockSize / 4 + blockSize / 4;
  int64_t start = first->granulepos - samples;
  RecordVorbisPacketSamples(first, samples);

  if (last->e_o_s && start < mGranulepos) {
    
    
    
    
    
    int64_t pruned = mGranulepos - start;
    for (uint32_t i = 0; i < mUnstamped.Length() - 1; i++) {
      mUnstamped[i]->granulepos += pruned;
    }
#ifdef VALIDATE_VORBIS_SAMPLE_CALCULATION
    mVorbisPacketSamples[last] -= pruned;
#endif
  }

  mPrevVorbisBlockSize = vorbis_packet_blocksize(&mInfo, last);
  mPrevVorbisBlockSize = std::max(static_cast<long>(0), mPrevVorbisBlockSize);
  mGranulepos = last->granulepos;

  return NS_OK;
}

#ifdef MOZ_OPUS
OpusState::OpusState(ogg_page* aBosPage) :
  OggCodecState(aBosPage, true),
  mParser(nullptr),
  mDecoder(nullptr),
  mSkip(0),
  mPrevPacketGranulepos(0),
  mPrevPageGranulepos(0)
{
  MOZ_COUNT_CTOR(OpusState);
}

OpusState::~OpusState() {
  MOZ_COUNT_DTOR(OpusState);
  Reset();

  if (mDecoder) {
    opus_multistream_decoder_destroy(mDecoder);
    mDecoder = nullptr;
  }
}

nsresult OpusState::Reset()
{
  return Reset(false);
}

nsresult OpusState::Reset(bool aStart)
{
  nsresult res = NS_OK;

  if (mActive && mDecoder) {
    
    opus_multistream_decoder_ctl(mDecoder, OPUS_RESET_STATE);
    
    mSkip = aStart ? mParser->mPreSkip : 0;
    
    
    mPrevPageGranulepos = aStart ? 0 : -1;
    mPrevPacketGranulepos = aStart ? 0 : -1;
  }

  
  if (NS_FAILED(OggCodecState::Reset())) {
    return NS_ERROR_FAILURE;
  }

  LOG(PR_LOG_DEBUG, ("Opus decoder reset, to skip %d", mSkip));

  return res;
}

bool OpusState::Init(void)
{
  if (!mActive)
    return false;

  int error;

  NS_ASSERTION(mDecoder == nullptr, "leaking OpusDecoder");

  mDecoder = opus_multistream_decoder_create(mParser->mRate,
                                             mParser->mChannels,
                                             mParser->mStreams,
                                             mParser->mCoupledStreams,
                                             mParser->mMappingTable,
                                             &error);

  mSkip = mParser->mPreSkip;

  LOG(PR_LOG_DEBUG, ("Opus decoder init, to skip %d", mSkip));

  return error == OPUS_OK;
}

bool OpusState::DecodeHeader(ogg_packet* aPacket)
{
  nsAutoRef<ogg_packet> autoRelease(aPacket);
  switch(mPacketCount++) {
    
    case 0: {
        mParser = new OpusParser;
        if(!mParser->DecodeHeader(aPacket->packet, aPacket->bytes)) {
          return false;
        }
        mRate = mParser->mRate;
        mChannels = mParser->mChannels;
        mPreSkip = mParser->mPreSkip;
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
        mGain = mParser->mGain;
#else
        mGain_Q16 = mParser->mGain_Q16;
#endif
    }
    break;

    
    case 1: {
        if(!mParser->DecodeTags(aPacket->packet, aPacket->bytes)) {
          return false;
        }
    }
    break;

    
    
    default: {
      mDoneReadingHeaders = true;
      
      mPackets.PushFront(autoRelease.disown());
    }
    break;
  }
  return true;
}


MetadataTags* OpusState::GetTags()
{
  MetadataTags* tags;

  tags = new MetadataTags;
  for (uint32_t i = 0; i < mParser->mTags.Length(); i++) {
    AddVorbisComment(tags, mParser->mTags[i].Data(), mParser->mTags[i].Length());
  }

  return tags;
}


int64_t OpusState::Time(int64_t aGranulepos)
{
  if (!mActive)
    return -1;

  return Time(mParser->mPreSkip, aGranulepos);
}

int64_t OpusState::Time(int aPreSkip, int64_t aGranulepos)
{
  if (aGranulepos < 0)
    return -1;

  
  CheckedInt64 t = CheckedInt64(aGranulepos - aPreSkip) * USECS_PER_S;
  return t.isValid() ? t.value() / 48000 : -1;
}

bool OpusState::IsHeader(ogg_packet* aPacket)
{
  return aPacket->bytes >= 16 &&
         (!memcmp(aPacket->packet, "OpusHead", 8) ||
          !memcmp(aPacket->packet, "OpusTags", 8));
}

nsresult OpusState::PageIn(ogg_page* aPage)
{
  if (!mActive)
    return NS_OK;
  NS_ASSERTION(static_cast<uint32_t>(ogg_page_serialno(aPage)) == mSerial,
               "Page must be for this stream!");
  if (ogg_stream_pagein(&mState, aPage) == -1)
    return NS_ERROR_FAILURE;

  bool haveGranulepos;
  nsresult rv = PacketOutUntilGranulepos(haveGranulepos);
  if (NS_FAILED(rv) || !haveGranulepos || mPacketCount < 2)
    return rv;
  if(!ReconstructOpusGranulepos())
    return NS_ERROR_FAILURE;
  for (uint32_t i = 0; i < mUnstamped.Length(); i++) {
    ogg_packet* packet = mUnstamped[i];
    NS_ASSERTION(!IsHeader(packet), "Don't try to play a header packet");
    NS_ASSERTION(packet->granulepos != -1, "Packet should have a granulepos");
    mPackets.Append(packet);
  }
  mUnstamped.Clear();
  return NS_OK;
}






static int GetOpusDeltaGP(ogg_packet* packet)
{
  int nframes;
  nframes = opus_packet_get_nb_frames(packet->packet, packet->bytes);
  if (nframes > 0) {
    return nframes*opus_packet_get_samples_per_frame(packet->packet, 48000);
  }
  NS_WARNING("Invalid Opus packet.");
  return nframes;
}

bool OpusState::ReconstructOpusGranulepos(void)
{
  NS_ASSERTION(mUnstamped.Length() > 0, "Must have unstamped packets");
  ogg_packet* last = mUnstamped[mUnstamped.Length()-1];
  NS_ASSERTION(last->e_o_s || last->granulepos > 0,
      "Must know last granulepos!");
  int64_t gp;
  
  
  if (last->e_o_s) {
    if (mPrevPageGranulepos != -1) {
      
      
      if (!mDoneReadingHeaders && last->granulepos < mPreSkip)
        return false;
      int64_t last_gp = last->granulepos;
      gp = mPrevPageGranulepos;
      
      
      
      for (uint32_t i = 0; i < mUnstamped.Length() - 1; ++i) {
        ogg_packet* packet = mUnstamped[i];
        int offset = GetOpusDeltaGP(packet);
        
        if (offset >= 0 && gp <= INT64_MAX - offset) {
          gp += offset;
          if (gp >= last_gp) {
            NS_WARNING("Opus end trimming removed more than a full packet.");
            
            
            
            gp = last_gp;
            for (uint32_t j = i+1; j < mUnstamped.Length(); ++j) {
              OggCodecState::ReleasePacket(mUnstamped[j]);
            }
            mUnstamped.RemoveElementsAt(i+1, mUnstamped.Length() - (i+1));
            last = packet;
            last->e_o_s = 1;
          }
        }
        packet->granulepos = gp;
      }
      mPrevPageGranulepos = last_gp;
      return true;
    } else {
      NS_WARNING("No previous granule position to use for Opus end trimming.");
      
      
      
    }
  }

  gp = last->granulepos;
  
  
  
  for (uint32_t i = mUnstamped.Length() - 1; i > 0; i--) {
    int offset = GetOpusDeltaGP(mUnstamped[i]);
    
    if (offset >= 0) {
      if (offset <= gp) {
        gp -= offset;
      } else {
        
        
        
        if (!mDoneReadingHeaders)
          return false;
        
        
        
        NS_WARNING("Clamping negative Opus granulepos to zero.");
        gp = 0;
      }
    }
    mUnstamped[i - 1]->granulepos = gp;
  }

  
  
  
  
  if (!mDoneReadingHeaders && GetOpusDeltaGP(mUnstamped[0]) > gp)
    return false;
  mPrevPageGranulepos = last->granulepos;
  return true;
}
#endif 

SkeletonState::SkeletonState(ogg_page* aBosPage) :
  OggCodecState(aBosPage, true),
  mVersion(0),
  mPresentationTime(0),
  mLength(0)
{
  MOZ_COUNT_CTOR(SkeletonState);
}
 
SkeletonState::~SkeletonState()
{
  MOZ_COUNT_DTOR(SkeletonState);
}





static const long SKELETON_MIN_HEADER_LEN = 28;
static const long SKELETON_4_0_MIN_HEADER_LEN = 80;


static const long SKELETON_4_0_MIN_INDEX_LEN = 42;


static const long SKELETON_MIN_FISBONE_LEN = 52;


static const size_t MIN_KEY_POINT_SIZE = 2;



static const size_t SKELETON_VERSION_MAJOR_OFFSET = 8;
static const size_t SKELETON_VERSION_MINOR_OFFSET = 10;


static const size_t SKELETON_PRESENTATION_TIME_NUMERATOR_OFFSET = 12;
static const size_t SKELETON_PRESENTATION_TIME_DENOMINATOR_OFFSET = 20;


static const size_t SKELETON_FILE_LENGTH_OFFSET = 64;


static const size_t INDEX_SERIALNO_OFFSET = 6;
static const size_t INDEX_NUM_KEYPOINTS_OFFSET = 10;
static const size_t INDEX_TIME_DENOM_OFFSET = 18;
static const size_t INDEX_FIRST_NUMER_OFFSET = 26;
static const size_t INDEX_LAST_NUMER_OFFSET = 34;
static const size_t INDEX_KEYPOINT_OFFSET = 42;


static const size_t FISBONE_MSG_FIELDS_OFFSET = 8;
static const size_t FISBONE_SERIALNO_OFFSET = 12;

static bool IsSkeletonBOS(ogg_packet* aPacket)
{
  static_assert(SKELETON_MIN_HEADER_LEN >= 8,
                "Minimum length of skeleton BOS header incorrect");
  return aPacket->bytes >= SKELETON_MIN_HEADER_LEN &&
         memcmp(reinterpret_cast<char*>(aPacket->packet), "fishead", 8) == 0;
}

static bool IsSkeletonIndex(ogg_packet* aPacket)
{
  static_assert(SKELETON_4_0_MIN_INDEX_LEN >= 5,
                "Minimum length of skeleton index header incorrect");
  return aPacket->bytes >= SKELETON_4_0_MIN_INDEX_LEN &&
         memcmp(reinterpret_cast<char*>(aPacket->packet), "index", 5) == 0;
}

static bool IsSkeletonFisbone(ogg_packet* aPacket)
{
  static_assert(SKELETON_MIN_FISBONE_LEN >= 8,
                "Minimum length of skeleton fisbone header incorrect");
  return aPacket->bytes >= SKELETON_MIN_FISBONE_LEN &&
         memcmp(reinterpret_cast<char*>(aPacket->packet), "fisbone", 8) == 0;
}



static const unsigned char* ReadVariableLengthInt(const unsigned char* p,
                                                  const unsigned char* aLimit,
                                                  int64_t& n)
{
  int shift = 0;
  int64_t byte = 0;
  n = 0;
  while (p < aLimit &&
         (byte & 0x80) != 0x80 &&
         shift < 57)
  {
    byte = static_cast<int64_t>(*p);
    n |= ((byte & 0x7f) << shift);
    shift += 7;
    p++;
  }
  return p;
}

bool SkeletonState::DecodeIndex(ogg_packet* aPacket)
{
  NS_ASSERTION(aPacket->bytes >= SKELETON_4_0_MIN_INDEX_LEN,
               "Index must be at least minimum size");
  if (!mActive) {
    return false;
  }

  uint32_t serialno = LittleEndian::readUint32(aPacket->packet + INDEX_SERIALNO_OFFSET);
  int64_t numKeyPoints = LittleEndian::readInt64(aPacket->packet + INDEX_NUM_KEYPOINTS_OFFSET);

  int64_t endTime = 0, startTime = 0;
  const unsigned char* p = aPacket->packet;

  int64_t timeDenom = LittleEndian::readInt64(aPacket->packet + INDEX_TIME_DENOM_OFFSET);
  if (timeDenom == 0) {
    LOG(PR_LOG_DEBUG, ("Ogg Skeleton Index packet for stream %u has 0 "
                       "timestamp denominator.", serialno));
    return (mActive = false);
  }

  
  CheckedInt64 t = CheckedInt64(LittleEndian::readInt64(p + INDEX_FIRST_NUMER_OFFSET)) * USECS_PER_S;
  if (!t.isValid()) {
    return (mActive = false);
  } else {
    startTime = t.value() / timeDenom;
  }

  
  t = LittleEndian::readInt64(p + INDEX_LAST_NUMER_OFFSET) * USECS_PER_S;
  if (!t.isValid()) {
    return (mActive = false);
  } else {
    endTime = t.value() / timeDenom;
  }

  
  
  CheckedInt64 minPacketSize = (CheckedInt64(numKeyPoints) * MIN_KEY_POINT_SIZE) + INDEX_KEYPOINT_OFFSET;
  if (!minPacketSize.isValid())
  {
    return (mActive = false);
  }
  
  int64_t sizeofIndex = aPacket->bytes - INDEX_KEYPOINT_OFFSET;
  int64_t maxNumKeyPoints = sizeofIndex / MIN_KEY_POINT_SIZE;
  if (aPacket->bytes < minPacketSize.value() ||
      numKeyPoints > maxNumKeyPoints || 
      numKeyPoints < 0)
  {
    
    
    
    
    
    
    LOG(PR_LOG_DEBUG, ("Possibly malicious number of key points reported "
                       "(%lld) in index packet for stream %u.",
                       numKeyPoints,
                       serialno));
    return (mActive = false);
  }

  nsAutoPtr<nsKeyFrameIndex> keyPoints(new nsKeyFrameIndex(startTime, endTime));
  
  p = aPacket->packet + INDEX_KEYPOINT_OFFSET;
  const unsigned char* limit = aPacket->packet + aPacket->bytes;
  int64_t numKeyPointsRead = 0;
  CheckedInt64 offset = 0;
  CheckedInt64 time = 0;
  while (p < limit &&
         numKeyPointsRead < numKeyPoints)
  {
    int64_t delta = 0;
    p = ReadVariableLengthInt(p, limit, delta);
    offset += delta;
    if (p == limit ||
        !offset.isValid() ||
        offset.value() > mLength ||
        offset.value() < 0)
    {
      return (mActive = false);
    }
    p = ReadVariableLengthInt(p, limit, delta);
    time += delta;
    if (!time.isValid() ||
        time.value() > endTime ||
        time.value() < startTime)
    {
      return (mActive = false);
    }
    CheckedInt64 timeUsecs = time * USECS_PER_S;
    if (!timeUsecs.isValid())
      return mActive = false;
    timeUsecs /= timeDenom;
    keyPoints->Add(offset.value(), timeUsecs.value());
    numKeyPointsRead++;
  }

  int32_t keyPointsRead = keyPoints->Length();
  if (keyPointsRead > 0) {
    mIndex.Put(serialno, keyPoints.forget());
  }

  LOG(PR_LOG_DEBUG, ("Loaded %d keypoints for Skeleton on stream %u",
                     keyPointsRead, serialno));
  return true;
}

nsresult SkeletonState::IndexedSeekTargetForTrack(uint32_t aSerialno,
                                                    int64_t aTarget,
                                                    nsKeyPoint& aResult)
{
  nsKeyFrameIndex* index = nullptr;
  mIndex.Get(aSerialno, &index);

  if (!index ||
      index->Length() == 0 ||
      aTarget < index->mStartTime ||
      aTarget > index->mEndTime)
  {
    return NS_ERROR_FAILURE;
  }

  
  int start = 0;
  int end = index->Length() - 1;
  while (end > start) {
    int mid = start + ((end - start + 1) >> 1);
    if (index->Get(mid).mTime == aTarget) {
       start = mid;
       break;
    } else if (index->Get(mid).mTime < aTarget) {
      start = mid;
    } else {
      end = mid - 1;
    }
  }

  aResult = index->Get(start);
  NS_ASSERTION(aResult.mTime <= aTarget, "Result should have time <= target");
  return NS_OK;
}

nsresult SkeletonState::IndexedSeekTarget(int64_t aTarget,
                                            nsTArray<uint32_t>& aTracks,
                                            nsSeekTarget& aResult)
{
  if (!mActive || mVersion < SKELETON_VERSION(4,0)) {
    return NS_ERROR_FAILURE;
  }
  
  
  
  
  nsSeekTarget r;
  for (uint32_t i=0; i<aTracks.Length(); i++) {
    nsKeyPoint k;
    if (NS_SUCCEEDED(IndexedSeekTargetForTrack(aTracks[i], aTarget, k)) &&
        k.mOffset < r.mKeyPoint.mOffset)
    {
      r.mKeyPoint = k;
      r.mSerial = aTracks[i];
    }
  }
  if (r.IsNull()) {
    return NS_ERROR_FAILURE;
  }
  LOG(PR_LOG_DEBUG, ("Indexed seek target for time %lld is offset %lld",
                     aTarget, r.mKeyPoint.mOffset));
  aResult = r;
  return NS_OK;
}

nsresult SkeletonState::GetDuration(const nsTArray<uint32_t>& aTracks,
                                      int64_t& aDuration)
{
  if (!mActive ||
      mVersion < SKELETON_VERSION(4,0) ||
      !HasIndex() ||
      aTracks.Length() == 0)
  {
    return NS_ERROR_FAILURE;
  }
  int64_t endTime = INT64_MIN;
  int64_t startTime = INT64_MAX;
  for (uint32_t i=0; i<aTracks.Length(); i++) {
    nsKeyFrameIndex* index = nullptr;
    mIndex.Get(aTracks[i], &index);
    if (!index) {
      
      return NS_ERROR_FAILURE;
    }
    if (index->mEndTime > endTime) {
      endTime = index->mEndTime;
    }
    if (index->mStartTime < startTime) {
      startTime = index->mStartTime;
    }
  }
  NS_ASSERTION(endTime > startTime, "Duration must be positive");
  CheckedInt64 duration = CheckedInt64(endTime) - startTime;
  aDuration = duration.isValid() ? duration.value() : 0;
  return duration.isValid() ? NS_OK : NS_ERROR_FAILURE;
}

bool SkeletonState::DecodeFisbone(ogg_packet* aPacket)
{
  if (aPacket->bytes < static_cast<long>(FISBONE_MSG_FIELDS_OFFSET + 4)) {
    return false;
  }
  uint32_t offsetMsgField = LittleEndian::readUint32(aPacket->packet + FISBONE_MSG_FIELDS_OFFSET);

  if (aPacket->bytes < static_cast<long>(FISBONE_SERIALNO_OFFSET + 4)) {
      return false;
  }
  uint32_t serialno = LittleEndian::readUint32(aPacket->packet + FISBONE_SERIALNO_OFFSET);

  CheckedUint32 checked_fields_pos = CheckedUint32(FISBONE_MSG_FIELDS_OFFSET) + offsetMsgField;
  if (!checked_fields_pos.isValid() ||
      aPacket->bytes < static_cast<int64_t>(checked_fields_pos.value())) {
    return false;
  }
  int64_t msgLength = aPacket->bytes - checked_fields_pos.value();
  char* msgProbe = (char*)aPacket->packet + checked_fields_pos.value();
  char* msgHead = msgProbe;
  nsAutoPtr<MessageField> field(new MessageField());

  const static FieldPatternType kFieldTypeMaps[] = {
      {"Content-Type:", eContentType},
      {"Role:", eRole},
      {"Name:", eName},
      {"Language:", eLanguage},
      {"Title:", eTitle},
      {"Display-hint:", eDisplayHint},
      {"Altitude:", eAltitude},
      {"TrackOrder:", eTrackOrder},
      {"Track dependencies:", eTrackDependencies}
  };

  bool isContentTypeParsed = false;
  while (msgLength > 1) {
    if (*msgProbe == '\r' && *(msgProbe+1) == '\n') {
      nsAutoCString strMsg(msgHead, msgProbe-msgHead);
      for (size_t i = 0; i < ArrayLength(kFieldTypeMaps); i++) {
        if (strMsg.Find(kFieldTypeMaps[i].mPatternToRecognize) != -1) {
          
          
          
          
          
          if (i != 0 && !isContentTypeParsed) {
            return false;
          }

          if ((i == 0 && IsASCII(strMsg)) || (i != 0 && IsUTF8(strMsg))) {
            EMsgHeaderType eHeaderType = kFieldTypeMaps[i].mMsgHeaderType;
            if (!field->mValuesStore.Contains(eHeaderType)) {
              uint32_t nameLen = strlen(kFieldTypeMaps[i].mPatternToRecognize);
              field->mValuesStore.Put(eHeaderType, new nsCString(msgHead+nameLen,
                                                                 msgProbe-msgHead-nameLen));
            }
            isContentTypeParsed = i==0 ? true : isContentTypeParsed;
          }
          break;
        }
      }
      msgProbe += 2;
      msgLength -= 2;
      msgHead = msgProbe;
      continue;
    }
    msgLength--;
    msgProbe++;
  };

  if (!mMsgFieldStore.Contains(serialno)) {
    mMsgFieldStore.Put(serialno, field.forget());
  } else {
    return false;
  }

  return true;
}

bool SkeletonState::DecodeHeader(ogg_packet* aPacket)
{
  nsAutoRef<ogg_packet> autoRelease(aPacket);
  if (IsSkeletonBOS(aPacket)) {
    uint16_t verMajor = LittleEndian::readUint16(aPacket->packet + SKELETON_VERSION_MAJOR_OFFSET);
    uint16_t verMinor = LittleEndian::readUint16(aPacket->packet + SKELETON_VERSION_MINOR_OFFSET);

    
    
    int64_t n = LittleEndian::readInt64(aPacket->packet + SKELETON_PRESENTATION_TIME_NUMERATOR_OFFSET);
    int64_t d = LittleEndian::readInt64(aPacket->packet + SKELETON_PRESENTATION_TIME_DENOMINATOR_OFFSET);
    mPresentationTime = d == 0 ? 0 : (static_cast<float>(n) / static_cast<float>(d)) * USECS_PER_S;

    mVersion = SKELETON_VERSION(verMajor, verMinor);
    
    if (mVersion < SKELETON_VERSION(4,0) ||
        mVersion >= SKELETON_VERSION(5,0) ||
        aPacket->bytes < SKELETON_4_0_MIN_HEADER_LEN)
      return false;

    
    mLength = LittleEndian::readInt64(aPacket->packet + SKELETON_FILE_LENGTH_OFFSET);

    LOG(PR_LOG_DEBUG, ("Skeleton segment length: %lld", mLength));

    
    return true;
  } else if (IsSkeletonIndex(aPacket) && mVersion >= SKELETON_VERSION(4,0)) {
    return DecodeIndex(aPacket);
  } else if (IsSkeletonFisbone(aPacket)) {
    return DecodeFisbone(aPacket);
  } else if (aPacket->e_o_s) {
    mDoneReadingHeaders = true;
    return true;
  }
  return true;
}


} 

