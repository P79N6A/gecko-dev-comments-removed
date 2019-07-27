



#if !defined(GStreamerReader_h_)
#define GStreamerReader_h_

#include <map>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wreserved-user-defined-literal"
#include <gst/video/video.h>
#pragma GCC diagnostic pop

#include "MediaDecoderReader.h"
#include "MP3FrameParser.h"
#include "ImageContainer.h"
#include "nsRect.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}

class AbstractMediaDecoder;

class GStreamerReader : public MediaDecoderReader
{
  typedef gfx::IntRect IntRect;

public:
  explicit GStreamerReader(AbstractMediaDecoder* aDecoder);
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

  virtual void NotifyDataArrived(const char *aBuffer,
                                 uint32_t aLength,
                                 int64_t aOffset) MOZ_OVERRIDE;

  virtual bool HasAudio() {
    return mInfo.HasAudio();
  }

  virtual bool HasVideo() {
    return mInfo.HasVideo();
  }

  layers::ImageContainer* GetImageContainer() { return mDecoder->GetImageContainer(); }

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

private:

  void ReadAndPushData(guint aLength);
  nsRefPtr<layers::PlanarYCbCrImage> GetImageFromBuffer(GstBuffer* aBuffer);
  void CopyIntoImageBuffer(GstBuffer *aBuffer, GstBuffer** aOutBuffer, nsRefPtr<layers::PlanarYCbCrImage> &image);
  GstCaps* BuildAudioSinkCaps();
  void InstallPadCallbacks();

#if GST_VERSION_MAJOR >= 1
  void ImageDataFromVideoFrame(GstVideoFrame *aFrame, layers::PlanarYCbCrImage::Data *aData);
#endif

  


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

  
#if GST_VERSION_MAJOR == 1
  static GstPadProbeReturn EventProbeCb(GstPad *aPad, GstPadProbeInfo *aInfo, gpointer aUserData);
  GstPadProbeReturn EventProbe(GstPad *aPad, GstEvent *aEvent);
#else
  static gboolean EventProbeCb(GstPad* aPad, GstEvent* aEvent, gpointer aUserData);
  gboolean EventProbe(GstPad* aPad, GstEvent* aEvent);
#endif

  




#if GST_VERSION_MAJOR == 1
  static GstPadProbeReturn QueryProbeCb(GstPad *aPad, GstPadProbeInfo *aInfo, gpointer aUserData);
  GstPadProbeReturn QueryProbe(GstPad *aPad, GstPadProbeInfo *aInfo, gpointer aUserData);
#else
  static GstFlowReturn AllocateVideoBufferCb(GstPad* aPad, guint64 aOffset, guint aSize,
                                             GstCaps* aCaps, GstBuffer** aBuf);
  GstFlowReturn AllocateVideoBufferFull(GstPad* aPad, guint64 aOffset, guint aSize,
                                     GstCaps* aCaps, GstBuffer** aBuf, nsRefPtr<layers::PlanarYCbCrImage>& aImage);
  GstFlowReturn AllocateVideoBuffer(GstPad* aPad, guint64 aOffset, guint aSize,
                                     GstCaps* aCaps, GstBuffer** aBuf);
#endif


  


  static GstFlowReturn NewPrerollCb(GstAppSink* aSink, gpointer aUserData);
  void VideoPreroll();
  void AudioPreroll();

  
  static GstFlowReturn NewBufferCb(GstAppSink* aSink, gpointer aUserData);
  void NewVideoBuffer();
  void NewAudioBuffer();

  
  static void EosCb(GstAppSink* aSink, gpointer aUserData);
  


  void Eos(GstAppSink* aSink = nullptr);

  


  static void PlayElementAddedCb(GstBin *aBin, GstElement *aElement,
                                 gpointer *aUserData);

  

  static bool ShouldAutoplugFactory(GstElementFactory* aFactory, GstCaps* aCaps);

  


  static GValueArray* AutoplugSortCb(GstElement* aElement,
                                     GstPad* aPad, GstCaps* aCaps,
                                     GValueArray* aFactories);

  
  nsresult ParseMP3Headers();

  
  
  int64_t GetDataLength();

  
  MP3FrameParser mMP3FrameParser;

  
  
  uint64_t mDataOffset;

  
  
  
  
  
  bool mUseParserDuration;
  int64_t mLastParserDuration;

#if GST_VERSION_MAJOR >= 1
  GstAllocator *mAllocator;
  GstBufferPool *mBufferPool;
  GstVideoInfo mVideoInfo;
#endif
  GstElement* mPlayBin;
  GstBus* mBus;
  GstAppSrc* mSource;
  
  GstElement* mVideoSink;
  
  GstAppSink* mVideoAppSink;
  
  GstElement* mAudioSink;
  
  GstAppSink* mAudioAppSink;
  GstVideoFormat mFormat;
  IntRect mPicture;
  int mVideoSinkBufferCount;
  int mAudioSinkBufferCount;
  GstAppSrcCallbacks mSrcCallbacks;
  GstAppSinkCallbacks mSinkCallbacks;
  

  ReentrantMonitor mGstThreadsMonitor;
  



  GstSegment mVideoSegment;
  GstSegment mAudioSegment;
  


  bool mReachedAudioEos;
  bool mReachedVideoEos;
#if GST_VERSION_MAJOR >= 1
  bool mConfigureAlignment;
#endif
  int fpsNum;
  int fpsDen;
};

} 

#endif
