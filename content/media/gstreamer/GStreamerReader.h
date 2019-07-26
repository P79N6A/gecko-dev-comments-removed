



#if !defined(GStreamerReader_h_)
#define GStreamerReader_h_

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wreserved-user-defined-literal"
#include <gst/video/video.h>
#pragma GCC diagnostic pop
#include <map>
#include "MediaDecoderReader.h"
#include "nsRect.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}

namespace layers {
class PlanarYCbCrImage;
}

class AbstractMediaDecoder;

class GStreamerReader : public MediaDecoderReader
{
public:
  GStreamerReader(AbstractMediaDecoder* aDecoder);
  virtual ~GStreamerReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold);
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime);
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered, int64_t aStartTime);

  virtual bool HasAudio() {
    return mInfo.HasAudio();
  }

  virtual bool HasVideo() {
    return mInfo.HasVideo();
  }

private:

  void ReadAndPushData(guint aLength);
  int64_t QueryDuration();

  


  nsresult CheckSupportedFormats();

  

  static GstBusSyncReply ErrorCb(GstBus *aBus, GstMessage *aMessage, gpointer aUserData);
  GstBusSyncReply Error(GstBus *aBus, GstMessage *aMessage);

  


  static void PlayBinSourceSetupCb(GstElement* aPlayBin,
                                   GParamSpec* pspec,
                                   gpointer aUserData);
  void PlayBinSourceSetup(GstAppSrc* aSource);

  
  static void NeedDataCb(GstAppSrc* aSrc, guint aLength, gpointer aUserData);
  void NeedData(GstAppSrc* aSrc, guint aLength);

  
  static void EnoughDataCb(GstAppSrc* aSrc, gpointer aUserData);
  void EnoughData(GstAppSrc* aSrc);

  
  static gboolean SeekDataCb(GstAppSrc* aSrc,
                             guint64 aOffset,
                             gpointer aUserData);
  gboolean SeekData(GstAppSrc* aSrc, guint64 aOffset);

  
  static gboolean EventProbeCb(GstPad* aPad, GstEvent* aEvent, gpointer aUserData);
  gboolean EventProbe(GstPad* aPad, GstEvent* aEvent);

  




  static GstFlowReturn AllocateVideoBufferCb(GstPad* aPad, guint64 aOffset, guint aSize,
                                             GstCaps* aCaps, GstBuffer** aBuf);
  GstFlowReturn AllocateVideoBufferFull(GstPad* aPad, guint64 aOffset, guint aSize,
                                     GstCaps* aCaps, GstBuffer** aBuf, nsRefPtr<layers::PlanarYCbCrImage>& aImage);
  GstFlowReturn AllocateVideoBuffer(GstPad* aPad, guint64 aOffset, guint aSize,
                                     GstCaps* aCaps, GstBuffer** aBuf);

  


  static GstFlowReturn NewPrerollCb(GstAppSink* aSink, gpointer aUserData);
  void VideoPreroll();
  void AudioPreroll();

  
  static GstFlowReturn NewBufferCb(GstAppSink* aSink, gpointer aUserData);
  void NewVideoBuffer();
  void NewAudioBuffer();

  
  static void EosCb(GstAppSink* aSink, gpointer aUserData);
  void Eos();

  GstElement* mPlayBin;
  GstBus* mBus;
  GstAppSrc* mSource;
  
  GstElement* mVideoSink;
  
  GstAppSink* mVideoAppSink;
  
  GstElement* mAudioSink;
  
  GstAppSink* mAudioAppSink;
  GstVideoFormat mFormat;
  nsIntRect mPicture;
  int mVideoSinkBufferCount;
  int mAudioSinkBufferCount;
  GstAppSrcCallbacks mSrcCallbacks;
  GstAppSinkCallbacks mSinkCallbacks;
  

  ReentrantMonitor mGstThreadsMonitor;
  



  GstSegment mVideoSegment;
  GstSegment mAudioSegment;
  


  bool mReachedEos;
  int fpsNum;
  int fpsDen;
};

} 

#endif
