





































#include "nsDebug.h"
#include "nsOggCodecState.h"
#include "nsOggDecoder.h"
#include <string.h>
#include "nsTraceRefcnt.h"
#include "VideoUtils.h"







#define MAX_VIDEO_WIDTH  4000
#define MAX_VIDEO_HEIGHT 3000

nsOggCodecState*
nsOggCodecState::Create(ogg_page* aPage)
{
  nsAutoPtr<nsOggCodecState> codecState;
  if (aPage->body_len > 6 && memcmp(aPage->body+1, "theora", 6) == 0) {
    codecState = new nsTheoraState(aPage);
  } else if (aPage->body_len > 6 && memcmp(aPage->body+1, "vorbis", 6) == 0) {
    codecState = new nsVorbisState(aPage);
  } else if (aPage->body_len > 8 && memcmp(aPage->body, "fishead\0", 8) == 0) {
    codecState = new nsSkeletonState(aPage);
  } else {
    codecState = new nsOggCodecState(aPage);
  }
  return codecState->nsOggCodecState::Init() ? codecState.forget() : nsnull;
}

nsOggCodecState::nsOggCodecState(ogg_page* aBosPage) :
  mPacketCount(0),
  mSerial(ogg_page_serialno(aBosPage)),
  mActive(PR_FALSE),
  mDoneReadingHeaders(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsOggCodecState);
  memset(&mState, 0, sizeof(ogg_stream_state));
}

nsOggCodecState::~nsOggCodecState() {
  MOZ_COUNT_DTOR(nsOggCodecState);
  int ret = ogg_stream_clear(&mState);
  NS_ASSERTION(ret == 0, "ogg_stream_clear failed");
}

nsresult nsOggCodecState::Reset() {
  if (ogg_stream_reset(&mState) != 0) {
    return NS_ERROR_FAILURE;
  }
  mBuffer.Erase();
  return NS_OK;
}

PRBool nsOggCodecState::Init() {
  int ret = ogg_stream_init(&mState, mSerial);
  return ret == 0;
}

void nsPageQueue::Append(ogg_page* aPage) {
  ogg_page* p = new ogg_page();
  p->header_len = aPage->header_len;
  p->body_len = aPage->body_len;
  p->header = new unsigned char[p->header_len + p->body_len];
  p->body = p->header + p->header_len;
  memcpy(p->header, aPage->header, p->header_len);
  memcpy(p->body, aPage->body, p->body_len);
  nsDeque::Push(p);
}

PRBool nsOggCodecState::PageInFromBuffer() {
  if (mBuffer.IsEmpty())
    return PR_FALSE;
  ogg_page *p = mBuffer.PeekFront();
  int ret = ogg_stream_pagein(&mState, p);
  NS_ENSURE_TRUE(ret == 0, PR_FALSE);
  mBuffer.PopFront();
  delete p->header;
  delete p;
  return PR_TRUE;
}

nsTheoraState::nsTheoraState(ogg_page* aBosPage) :
  nsOggCodecState(aBosPage),
  mSetup(0),
  mCtx(0),
  mFrameDuration(0),
  mPixelAspectRatio(0)
{
  MOZ_COUNT_CTOR(nsTheoraState);
  th_info_init(&mInfo);
  th_comment_init(&mComment);
}

nsTheoraState::~nsTheoraState() {
  MOZ_COUNT_DTOR(nsTheoraState);
  th_setup_free(mSetup);
  th_decode_free(mCtx);
  th_comment_clear(&mComment);
  th_info_clear(&mInfo);
}

PRBool nsTheoraState::Init() {
  if (!mActive)
    return PR_FALSE;

  PRInt64 n = mInfo.fps_numerator;
  PRInt64 d = mInfo.fps_denominator;

  PRInt64 f;
  if (!MulOverflow(1000, d, f)) {
    return mActive = PR_FALSE;
  }
  f /= n;
  if (f > PR_UINT32_MAX) {
    return mActive = PR_FALSE;
  }
  mFrameDuration = static_cast<PRUint32>(f);

  n = mInfo.aspect_numerator;

  d = mInfo.aspect_denominator;
  mPixelAspectRatio = (n == 0 || d == 0) ?
    1.0f : static_cast<float>(n) / static_cast<float>(d);

  
  PRUint32 pixels;
  if (!MulOverflow32(mInfo.frame_width, mInfo.frame_height, pixels) ||
      pixels > MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT ||
      pixels == 0)
  {
    return mActive = PR_FALSE;
  }

  
  if (!MulOverflow32(mInfo.pic_width, mInfo.pic_height, pixels) ||
      pixels > MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT ||
      pixels == 0)
  {
    return mActive = PR_FALSE;
  }

  mCtx = th_decode_alloc(&mInfo, mSetup);
  if (mCtx == NULL) {
    return mActive = PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsTheoraState::DecodeHeader(ogg_packet* aPacket)
{
  mPacketCount++;
  int ret = th_decode_headerin(&mInfo,
                               &mComment,
                               &mSetup,
                               aPacket);
 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRBool isSetupHeader = aPacket->bytes > 0 && aPacket->packet[0] == 0x82;
  if (ret < 0 || mPacketCount > 3) {
    
    
    mDoneReadingHeaders = PR_TRUE;
  } else if (ret > 0 && isSetupHeader && mPacketCount == 3) {
    
    mDoneReadingHeaders = PR_TRUE;
    mActive = PR_TRUE;
  }
  return mDoneReadingHeaders;
}

PRInt64
nsTheoraState::Time(PRInt64 granulepos) {
  if (granulepos < 0 || !mActive || mInfo.fps_numerator == 0) {
    return -1;
  }
  PRInt64 t = 0;
  PRInt64 frameno = th_granule_frame(mCtx, granulepos);
  if (!AddOverflow(frameno, 1, t))
    return -1;
  if (!MulOverflow(t, 1000, t))
    return -1;
  if (!MulOverflow(t, mInfo.fps_denominator, t))
    return -1;
  return t / mInfo.fps_numerator;
}

PRInt64 nsTheoraState::StartTime(PRInt64 granulepos) {
  if (granulepos < 0 || !mActive || mInfo.fps_numerator == 0) {
    return -1;
  }
  PRInt64 t = 0;
  PRInt64 frameno = th_granule_frame(mCtx, granulepos);
  if (!MulOverflow(frameno, 1000, t))
    return -1;
  if (!MulOverflow(t, mInfo.fps_denominator, t))
    return -1;
  return t / mInfo.fps_numerator;
}

PRInt64
nsTheoraState::MaxKeyframeOffset()
{
  
  
  
  
  
  PRInt64 frameDuration;
  PRInt64 keyframeDiff;

  PRInt64 shift = mInfo.keyframe_granule_shift;

  
  keyframeDiff = (1 << shift) - 1;

  
  PRInt64 d = 0; 
  MulOverflow(1000, mInfo.fps_denominator, d);
  frameDuration = d / mInfo.fps_numerator;

  
  return frameDuration * keyframeDiff;
}

nsresult nsVorbisState::Reset()
{
  nsresult res = NS_OK;
  if (mActive && vorbis_synthesis_restart(&mDsp) != 0) {
    res = NS_ERROR_FAILURE;
  }
  if (NS_FAILED(nsOggCodecState::Reset())) {
    return NS_ERROR_FAILURE;
  }
  return res;
}

nsVorbisState::nsVorbisState(ogg_page* aBosPage) :
  nsOggCodecState(aBosPage)
{
  MOZ_COUNT_CTOR(nsVorbisState);
  vorbis_info_init(&mInfo);
  vorbis_comment_init(&mComment);
  memset(&mDsp, 0, sizeof(vorbis_dsp_state));
  memset(&mBlock, 0, sizeof(vorbis_block));
}

nsVorbisState::~nsVorbisState() {
  MOZ_COUNT_DTOR(nsVorbisState);
  vorbis_block_clear(&mBlock);
  vorbis_dsp_clear(&mDsp);
  vorbis_info_clear(&mInfo);
  vorbis_comment_clear(&mComment);
}

PRBool nsVorbisState::DecodeHeader(ogg_packet* aPacket) {
  mPacketCount++;
  int ret = vorbis_synthesis_headerin(&mInfo,
                                      &mComment,
                                      aPacket);
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  PRBool isSetupHeader = aPacket->bytes > 0 && aPacket->packet[0] == 0x5;

  if (ret < 0 || mPacketCount > 3) {
    
    
    mDoneReadingHeaders = PR_TRUE;
  } else if (ret == 0 && isSetupHeader && mPacketCount == 3) {
    
    mDoneReadingHeaders = PR_TRUE;
    mActive = PR_TRUE;
  }
  return mDoneReadingHeaders;
}

PRBool nsVorbisState::Init()
{
  if (!mActive)
    return PR_FALSE;

  int ret = vorbis_synthesis_init(&mDsp, &mInfo);
  if (ret != 0) {
    NS_WARNING("vorbis_synthesis_init() failed initializing vorbis bitstream");
    return mActive = PR_FALSE;
  }
  ret = vorbis_block_init(&mDsp, &mBlock);
  if (ret != 0) {
    NS_WARNING("vorbis_block_init() failed initializing vorbis bitstream");
    if (mActive) {
      vorbis_dsp_clear(&mDsp);
    }
    return mActive = PR_FALSE;
  }
  return PR_TRUE;
}

PRInt64 nsVorbisState::Time(PRInt64 granulepos) {
  if (granulepos == -1 || !mActive || mDsp.vi->rate == 0) {
    return -1;
  }
  PRInt64 t = 0;
  MulOverflow(1000, granulepos, t);
  return t / mDsp.vi->rate;
}

nsSkeletonState::nsSkeletonState(ogg_page* aBosPage)
  : nsOggCodecState(aBosPage)
{
  MOZ_COUNT_CTOR(nsSkeletonState);
}

nsSkeletonState::~nsSkeletonState()
{
  MOZ_COUNT_DTOR(nsSkeletonState);
}

PRBool nsSkeletonState::DecodeHeader(ogg_packet* aPacket)
{
  if (aPacket->e_o_s) {
    mActive = PR_TRUE;
    mDoneReadingHeaders = PR_TRUE;
  }
  return mDoneReadingHeaders;
}
