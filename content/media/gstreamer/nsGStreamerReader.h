



#if !defined(nsGStreamerReader_h_)
#define nsGStreamerReader_h_

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "nsBuiltinDecoderReader.h"

using namespace mozilla;

class nsMediaDecoder;
class nsTimeRanges;

class nsGStreamerReader : public nsBuiltinDecoderReader
{
public:
  nsGStreamerReader(nsBuiltinDecoder* aDecoder);
  virtual ~nsGStreamerReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                PRInt64 aTimeThreshold);
  virtual nsresult ReadMetadata(nsVideoInfo* aInfo,
                                nsHTMLMediaElement::MetadataTags** aTags);
  virtual nsresult Seek(PRInt64 aTime,
                        PRInt64 aStartTime,
                        PRInt64 aEndTime,
                        PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

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
  PRInt64 QueryDuration();

  

  


  static void PlayBinSourceSetupCb(GstElement *aPlayBin,
                                   GstElement *aSource,
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
  

  mozilla::ReentrantMonitor mGstThreadsMonitor;
  



  GstSegment mVideoSegment;
  GstSegment mAudioSegment;
  


  bool mReachedEos;
  
  gint64 mByteOffset;
  
  gint64 mLastReportedByteOffset;
  int fpsNum;
  int fpsDen;
};

#endif
