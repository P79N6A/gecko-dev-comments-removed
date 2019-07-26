



#if !defined(GStreamerReader_h_)
#define GStreamerReader_h_

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "MediaDecoderReader.h"

class TimeRanges;

namespace mozilla {

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
  virtual nsresult ReadMetadata(VideoInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime);
  virtual nsresult GetBuffered(TimeRanges* aBuffered, int64_t aStartTime);

  virtual bool HasAudio() {
    return mInfo.mHasAudio;
  }

  virtual bool HasVideo() {
    return mInfo.mHasVideo;
  }

private:

  void ReadAndPushData(guint aLength);
  bool WaitForDecodedData(int *counter);
  void NotifyBytesConsumed();
  int64_t QueryDuration();

  

  


  static void PlayBinSourceSetupCb(GstElement *aPlayBin,
                                   GParamSpec *pspec,
                                   gpointer aUserData);
  void PlayBinSourceSetup(GstAppSrc *aSource);

  
  static void NeedDataCb(GstAppSrc *aSrc, guint aLength, gpointer aUserData);
  void NeedData(GstAppSrc *aSrc, guint aLength);

  
  static void EnoughDataCb(GstAppSrc *aSrc, gpointer aUserData);
  void EnoughData(GstAppSrc *aSrc);

  
  static gboolean SeekDataCb(GstAppSrc *aSrc,
                             guint64 aOffset,
                             gpointer aUserData);
  gboolean SeekData(GstAppSrc *aSrc, guint64 aOffset);

  
  static gboolean EventProbeCb(GstPad *aPad, GstEvent *aEvent, gpointer aUserData);
  gboolean EventProbe(GstPad *aPad, GstEvent *aEvent);

  


  static GstFlowReturn NewPrerollCb(GstAppSink *aSink, gpointer aUserData);
  void VideoPreroll();
  void AudioPreroll();

  
  static GstFlowReturn NewBufferCb(GstAppSink *aSink, gpointer aUserData);
  void NewVideoBuffer();
  void NewAudioBuffer();

  
  static void EosCb(GstAppSink *aSink, gpointer aUserData);
  void Eos(GstAppSink *aSink);

  GstElement *mPlayBin;
  GstBus *mBus;
  GstAppSrc *mSource;
  
  GstElement *mVideoSink;
  
  GstAppSink *mVideoAppSink;
  
  GstElement *mAudioSink;
  
  GstAppSink *mAudioAppSink;
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
  
  gint64 mByteOffset;
  
  gint64 mLastReportedByteOffset;
  int fpsNum;
  int fpsDen;
};

} 

#endif
