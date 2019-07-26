















#ifndef GONK_CAMERA_SOURCE_H_

#define GONK_CAMERA_SOURCE_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>
#include <camera/CameraParameters.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include "GonkCameraHwMgr.h"

namespace android {

class IMemory;
class GonkCameraSourceListener;

class GonkCameraSource : public MediaSource, public MediaBufferObserver {
public:

    static GonkCameraSource *Create(const sp<GonkCameraHardware>& aCameraHw,
                                    Size videoSize,
                                    int32_t frameRate,
                                    bool storeMetaDataInVideoBuffers = false);

    virtual ~GonkCameraSource();

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

    




    virtual status_t initCheck() const;

    







    virtual sp<MetaData> getFormat();

    







    bool isMetaDataStoredInVideoBuffers() const;

    virtual void signalBufferReturned(MediaBuffer* buffer);

protected:

    enum CameraFlags {
        FLAGS_SET_CAMERA = 1L << 0,
        FLAGS_HOT_CAMERA = 1L << 1,
    };

    int32_t  mCameraFlags;
    Size     mVideoSize;
    int32_t  mVideoFrameRate;
    int32_t  mColorFormat;
    status_t mInitCheck;

    sp<MetaData> mMeta;

    int64_t mStartTimeUs;
    int32_t mNumFramesReceived;
    int64_t mLastFrameTimestampUs;
    bool mStarted;
    int32_t mNumFramesEncoded;

    
    int64_t mTimeBetweenFrameCaptureUs;

    GonkCameraSource(const sp<GonkCameraHardware>& aCameraHw,
                 Size videoSize, int32_t frameRate,
                 bool storeMetaDataInVideoBuffers = false);

    virtual int startCameraRecording();
    virtual void stopCameraRecording();
    virtual void releaseRecordingFrame(const sp<IMemory>& frame);

    
    
    virtual bool skipCurrentFrame(int64_t timestampUs) {return false;}

    friend class GonkCameraSourceListener;
    
    virtual void dataCallback(int32_t msgType, const sp<IMemory> &data) {}

    virtual void dataCallbackTimestamp(int64_t timestampUs, int32_t msgType,
            const sp<IMemory> &data);

private:

    Mutex mLock;
    Condition mFrameAvailableCondition;
    Condition mFrameCompleteCondition;
    List<sp<IMemory> > mFramesReceived;
    List<sp<IMemory> > mFramesBeingEncoded;
    List<int64_t> mFrameTimes;

    int64_t mFirstFrameTimeUs;
    int32_t mNumFramesDropped;
    int32_t mNumGlitches;
    int64_t mGlitchDurationThresholdUs;
    bool mCollectStats;
    bool mIsMetaDataStoredInVideoBuffers;
    sp<GonkCameraHardware> mCameraHw;

    void releaseQueuedFrames();
    void releaseOneRecordingFrame(const sp<IMemory>& frame);

    status_t init(Size videoSize, int32_t frameRate,
                  bool storeMetaDataInVideoBuffers);
    status_t isCameraColorFormatSupported(const CameraParameters& params);
    status_t configureCamera(CameraParameters* params,
                    int32_t width, int32_t height,
                    int32_t frameRate);

    status_t checkVideoSize(const CameraParameters& params,
                    int32_t width, int32_t height);

    status_t checkFrameRate(const CameraParameters& params,
                    int32_t frameRate);

    void releaseCamera();

    GonkCameraSource(const GonkCameraSource &);
    GonkCameraSource &operator=(const GonkCameraSource &);
};

}  

#endif
