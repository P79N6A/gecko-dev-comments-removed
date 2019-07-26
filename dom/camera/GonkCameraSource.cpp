
















#include <base/basictypes.h>
#include "nsDebug.h"
#define DOM_CAMERA_LOG_LEVEL        3
#include "CameraCommon.h"








#define CS_LOGD(fmt, ...) DOM_CAMERA_LOGA("[%s:%d]" fmt,__FILE__,__LINE__, ## __VA_ARGS__)
#define CS_LOGV(fmt, ...) DOM_CAMERA_LOGI("[%s:%d]" fmt,__FILE__,__LINE__, ## __VA_ARGS__)
#define CS_LOGI(fmt, ...) DOM_CAMERA_LOGI("[%s:%d]" fmt,__FILE__,__LINE__, ## __VA_ARGS__)
#define CS_LOGW(fmt, ...) DOM_CAMERA_LOGW("[%s:%d]" fmt,__FILE__,__LINE__, ## __VA_ARGS__)
#define CS_LOGE(fmt, ...) DOM_CAMERA_LOGE("[%s:%d]" fmt,__FILE__,__LINE__, ## __VA_ARGS__)

#include <OMX_Component.h>
#include <binder/IPCThreadState.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <camera/CameraParameters.h>
#include <utils/String8.h>
#include <cutils/properties.h>

#include "GonkCameraSource.h"
#include "GonkCameraListener.h"
#include "GonkCameraHwMgr.h"

using namespace mozilla;

namespace android {

static const int64_t CAMERA_SOURCE_TIMEOUT_NS = 3000000000LL;

struct GonkCameraSourceListener : public GonkCameraListener {
    GonkCameraSourceListener(const sp<GonkCameraSource> &source);

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory> &dataPtr,
                          camera_frame_metadata_t *metadata);

    virtual void postDataTimestamp(
            nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

protected:
    virtual ~GonkCameraSourceListener();

private:
    wp<GonkCameraSource> mSource;

    GonkCameraSourceListener(const GonkCameraSourceListener &);
    GonkCameraSourceListener &operator=(const GonkCameraSourceListener &);
};

GonkCameraSourceListener::GonkCameraSourceListener(const sp<GonkCameraSource> &source)
    : mSource(source) {
}

GonkCameraSourceListener::~GonkCameraSourceListener() {
}

void GonkCameraSourceListener::notify(int32_t msgType, int32_t ext1, int32_t ext2) {
    CS_LOGV("notify(%d, %d, %d)", msgType, ext1, ext2);
}

void GonkCameraSourceListener::postData(int32_t msgType, const sp<IMemory> &dataPtr,
                                    camera_frame_metadata_t *metadata) {
    CS_LOGV("postData(%d, ptr:%p, size:%d)",
         msgType, dataPtr->pointer(), dataPtr->size());

    sp<GonkCameraSource> source = mSource.promote();
    if (source.get() != NULL) {
        source->dataCallback(msgType, dataPtr);
    }
}

void GonkCameraSourceListener::postDataTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) {

    sp<GonkCameraSource> source = mSource.promote();
    if (source.get() != NULL) {
        source->dataCallbackTimestamp(timestamp/1000, msgType, dataPtr);
    }
}

static int32_t getColorFormat(const char* colorFormat) {
    return OMX_COLOR_FormatYUV420SemiPlanar; 

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420P)) {
       return OMX_COLOR_FormatYUV420Planar;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422SP)) {
       return OMX_COLOR_FormatYUV422SemiPlanar;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420SP)) {
        return OMX_COLOR_FormatYUV420SemiPlanar;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422I)) {
        return OMX_COLOR_FormatYCbYCr;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_RGB565)) {
       return OMX_COLOR_Format16bitRGB565;
    }

    if (!strcmp(colorFormat, "OMX_TI_COLOR_FormatYUV420PackedSemiPlanar")) {
       return OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
    }
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_ANDROID_OPAQUE)) {
        return OMX_COLOR_FormatAndroidOpaque;
    }
#endif
    CS_LOGE("Uknown color format (%s), please add it to "
         "GonkCameraSource::getColorFormat", colorFormat);

    CHECK(!"Unknown color format");
}

GonkCameraSource *GonkCameraSource::Create(
    const sp<GonkCameraHardware>& aCameraHw,
    Size videoSize,
    int32_t frameRate,
    bool storeMetaDataInVideoBuffers) {

    GonkCameraSource *source = new GonkCameraSource(aCameraHw,
                    videoSize, frameRate,
                    storeMetaDataInVideoBuffers);
    return source;
}

GonkCameraSource::GonkCameraSource(
    const sp<GonkCameraHardware>& aCameraHw,
    Size videoSize,
    int32_t frameRate,
    bool storeMetaDataInVideoBuffers)
    : mCameraHw(aCameraHw),
      mCameraFlags(0),
      mNumInputBuffers(0),
      mVideoFrameRate(-1),
      mNumFramesReceived(0),
      mLastFrameTimestampUs(0),
      mStarted(false),
      mNumFramesEncoded(0),
      mTimeBetweenFrameCaptureUs(0),
      mFirstFrameTimeUs(0),
      mNumFramesDropped(0),
      mNumGlitches(0),
      mGlitchDurationThresholdUs(200000),
      mCollectStats(false) {
    mVideoSize.width  = -1;
    mVideoSize.height = -1;

    mInitCheck = init(
                    videoSize, frameRate,
                    storeMetaDataInVideoBuffers);
    if (mInitCheck != OK) releaseCamera();
}

status_t GonkCameraSource::initCheck() const {
    return mInitCheck;
}











static bool isVideoSizeSupported(
    int32_t width, int32_t height,
    const Vector<Size>& supportedSizes) {

    CS_LOGV("isVideoSizeSupported");
    for (size_t i = 0; i < supportedSizes.size(); ++i) {
        if (width  == supportedSizes[i].width &&
            height == supportedSizes[i].height) {
            return true;
        }
    }
    return false;
}




















static void getSupportedVideoSizes(
    const CameraParameters& params,
    bool *isSetVideoSizeSupported,
    Vector<Size>& sizes) {

    *isSetVideoSizeSupported = true;
    params.getSupportedVideoSizes(sizes);
    if (sizes.size() == 0) {
        CS_LOGD("Camera does not support setVideoSize()");
        params.getSupportedPreviewSizes(sizes);
        *isSetVideoSizeSupported = false;
    }
}






status_t GonkCameraSource::isCameraColorFormatSupported(
        const CameraParameters& params) {
    mColorFormat = getColorFormat(params.get(
            CameraParameters::KEY_VIDEO_FRAME_FORMAT));
    if (mColorFormat == -1) {
        return BAD_VALUE;
    }
    return OK;
}
















status_t GonkCameraSource::configureCamera(
        CameraParameters* params,
        int32_t width, int32_t height,
        int32_t frameRate) {
    CS_LOGV("configureCamera");
    Vector<Size> sizes;
    bool isSetVideoSizeSupportedByCamera = true;
    getSupportedVideoSizes(*params, &isSetVideoSizeSupportedByCamera, sizes);
    bool isCameraParamChanged = false;
    if (width != -1 && height != -1) {
        if (!isVideoSizeSupported(width, height, sizes)) {
            CS_LOGE("Video dimension (%dx%d) is unsupported", width, height);
            return BAD_VALUE;
        }
        if (isSetVideoSizeSupportedByCamera) {
            params->setVideoSize(width, height);
        } else {
            params->setPreviewSize(width, height);
        }
        isCameraParamChanged = true;
    } else if ((width == -1 && height != -1) ||
               (width != -1 && height == -1)) {
        
        
        CS_LOGE("Requested video size (%dx%d) is not supported", width, height);
        return BAD_VALUE;
    } else {  
        
        
    }

    if (frameRate != -1) {
        CHECK(frameRate > 0 && frameRate <= 120);
        const char* supportedFrameRates =
                params->get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES);
        CHECK(supportedFrameRates != NULL);
        CS_LOGV("Supported frame rates: %s", supportedFrameRates);
        char buf[4];
        snprintf(buf, 4, "%d", frameRate);
        if (strstr(supportedFrameRates, buf) == NULL) {
            CS_LOGE("Requested frame rate (%d) is not supported: %s",
                frameRate, supportedFrameRates);
            return BAD_VALUE;
        }

        
        params->setPreviewFrameRate(frameRate);
        isCameraParamChanged = true;
    } else {  
        
        
    }

    if (isCameraParamChanged) {
        
        if (OK != mCameraHw->PushParameters(*params)) {
            CS_LOGE("Could not change settings."
                 " Someone else is using camera?");
            return -EBUSY;
        }
    }
    return OK;
}












status_t GonkCameraSource::checkVideoSize(
        const CameraParameters& params,
        int32_t width, int32_t height) {

    CS_LOGV("checkVideoSize");
    
    
    
    
    int32_t frameWidthActual = -1;
    int32_t frameHeightActual = -1;
    Vector<Size> sizes;
    params.getSupportedVideoSizes(sizes);
    if (sizes.size() == 0) {
        
        params.getPreviewSize(&frameWidthActual, &frameHeightActual);
    } else {
        
        params.getVideoSize(&frameWidthActual, &frameHeightActual);
    }
    if (frameWidthActual < 0 || frameHeightActual < 0) {
        CS_LOGE("Failed to retrieve video frame size (%dx%d)",
                frameWidthActual, frameHeightActual);
        return UNKNOWN_ERROR;
    }

    
    
    if (width != -1 && height != -1) {
        if (frameWidthActual != width || frameHeightActual != height) {
            CS_LOGE("Failed to set video frame size to %dx%d. "
                    "The actual video size is %dx%d ", width, height,
                    frameWidthActual, frameHeightActual);
            return UNKNOWN_ERROR;
        }
    }

    
    mVideoSize.width = frameWidthActual;
    mVideoSize.height = frameHeightActual;
    return OK;
}










status_t GonkCameraSource::checkFrameRate(
        const CameraParameters& params,
        int32_t frameRate) {

    CS_LOGV("checkFrameRate");
    int32_t frameRateActual = params.getPreviewFrameRate();
    if (frameRateActual < 0) {
        CS_LOGE("Failed to retrieve preview frame rate (%d)", frameRateActual);
        return UNKNOWN_ERROR;
    }

    
    
    if (frameRate != -1 && (frameRateActual - frameRate) != 0) {
        CS_LOGE("Failed to set preview frame rate to %d fps. The actual "
                "frame rate is %d", frameRate, frameRateActual);
        return UNKNOWN_ERROR;
    }

    
    mVideoFrameRate = frameRateActual;
    return OK;
}



















status_t GonkCameraSource::init(
        Size videoSize,
        int32_t frameRate,
        bool storeMetaDataInVideoBuffers) {

    CS_LOGV("init");
    status_t err = OK;
    

    CameraParameters params;
    mCameraHw->PullParameters(params);
    if ((err = isCameraColorFormatSupported(params)) != OK) {
        return err;
    }

    
    
    if ((err = configureCamera(&params,
                    videoSize.width, videoSize.height,
                    frameRate))) {
        return err;
    }

    
    CameraParameters newCameraParams;
    mCameraHw->PullParameters(newCameraParams);
    if ((err = checkVideoSize(newCameraParams,
                videoSize.width, videoSize.height)) != OK) {
        return err;
    }
    if ((err = checkFrameRate(newCameraParams, frameRate)) != OK) {
        return err;
    }

    
    mIsMetaDataStoredInVideoBuffers = false;
    mCameraHw->StoreMetaDataInBuffers(false);
    if (storeMetaDataInVideoBuffers) {
        if (OK == mCameraHw->StoreMetaDataInBuffers(true)) {
            mIsMetaDataStoredInVideoBuffers = true;
        }
    }

    int64_t glitchDurationUs = (1000000LL / mVideoFrameRate);
    if (glitchDurationUs > mGlitchDurationThresholdUs) {
        mGlitchDurationThresholdUs = glitchDurationUs;
    }

    
    
    mMeta = new MetaData;
    mMeta->setCString(kKeyMIMEType,  MEDIA_MIMETYPE_VIDEO_RAW);
    mMeta->setInt32(kKeyColorFormat, mColorFormat);
    mMeta->setInt32(kKeyWidth,       mVideoSize.width);
    mMeta->setInt32(kKeyHeight,      mVideoSize.height);
    mMeta->setInt32(kKeyStride,      mVideoSize.width);
    mMeta->setInt32(kKeySliceHeight, mVideoSize.height);
    mMeta->setInt32(kKeyFrameRate,   mVideoFrameRate);
    return OK;
}

GonkCameraSource::~GonkCameraSource() {
    if (mStarted) {
        reset();
    } else if (mInitCheck == OK) {
        
        
        
        
        releaseCamera();
    }
}

int GonkCameraSource::startCameraRecording() {
    CS_LOGV("startCameraRecording");
    return mCameraHw->StartRecording();
}

status_t GonkCameraSource::start(MetaData *meta) {
    int rv;

    CS_LOGV("start");
    CHECK(!mStarted);
    if (mInitCheck != OK) {
        CS_LOGE("GonkCameraSource is not initialized yet");
        return mInitCheck;
    }

    char value[PROPERTY_VALUE_MAX];
    if (property_get("media.stagefright.record-stats", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) {
        mCollectStats = true;
    }

    mStartTimeUs = 0;
    mNumInputBuffers = 0;
    if (meta) {
        int64_t startTimeUs;
        if (meta->findInt64(kKeyTime, &startTimeUs)) {
            mStartTimeUs = startTimeUs;
        }
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
        int32_t nBuffers;
        if (meta->findInt32(kKeyNumBuffers, &nBuffers)) {
            CHECK_GT(nBuffers, 0);
            mNumInputBuffers = nBuffers;
        }
#endif
    }

    
    mCameraHw->SetListener(new GonkCameraSourceListener(this));

    rv = startCameraRecording();

    mStarted = (rv == OK);
    return rv;
}

void GonkCameraSource::stopCameraRecording() {
    CS_LOGV("stopCameraRecording");
    mCameraHw->StopRecording();
}

void GonkCameraSource::releaseCamera() {
    CS_LOGV("releaseCamera");
}

status_t GonkCameraSource::reset() {
    CS_LOGD("reset: E");
    Mutex::Autolock autoLock(mLock);
    mStarted = false;
    mFrameAvailableCondition.signal();

    releaseQueuedFrames();
    while (!mFramesBeingEncoded.empty()) {
        if (NO_ERROR !=
            mFrameCompleteCondition.waitRelative(mLock,
                    mTimeBetweenFrameCaptureUs * 1000LL + CAMERA_SOURCE_TIMEOUT_NS)) {
            CS_LOGW("Timed out waiting for outstanding frames being encoded: %d",
                mFramesBeingEncoded.size());
        }
    }
    stopCameraRecording();
    releaseCamera();

    if (mCollectStats) {
        CS_LOGI("Frames received/encoded/dropped: %d/%d/%d in %lld us",
                mNumFramesReceived, mNumFramesEncoded, mNumFramesDropped,
                mLastFrameTimestampUs - mFirstFrameTimeUs);
    }

    if (mNumGlitches > 0) {
        CS_LOGW("%d long delays between neighboring video frames", mNumGlitches);
    }

    CHECK_EQ(mNumFramesReceived, mNumFramesEncoded + mNumFramesDropped);
    CS_LOGD("reset: X");
    return OK;
}

void GonkCameraSource::releaseRecordingFrame(const sp<IMemory>& frame) {
    CS_LOGV("releaseRecordingFrame");
    mCameraHw->ReleaseRecordingFrame(frame);
}

void GonkCameraSource::releaseQueuedFrames() {
    List<sp<IMemory> >::iterator it;
    while (!mFramesReceived.empty()) {
        it = mFramesReceived.begin();
        releaseRecordingFrame(*it);
        mFramesReceived.erase(it);
        ++mNumFramesDropped;
    }
}

sp<MetaData> GonkCameraSource::getFormat() {
    return mMeta;
}

void GonkCameraSource::releaseOneRecordingFrame(const sp<IMemory>& frame) {
    releaseRecordingFrame(frame);
}

void GonkCameraSource::signalBufferReturned(MediaBuffer *buffer) {
    CS_LOGV("signalBufferReturned: %p", buffer->data());
    Mutex::Autolock autoLock(mLock);
    for (List<sp<IMemory> >::iterator it = mFramesBeingEncoded.begin();
         it != mFramesBeingEncoded.end(); ++it) {
        if ((*it)->pointer() ==  buffer->data()) {
            releaseOneRecordingFrame((*it));
            mFramesBeingEncoded.erase(it);
            ++mNumFramesEncoded;
            buffer->setObserver(0);
            buffer->release();
            mFrameCompleteCondition.signal();
            return;
        }
    }
    CHECK(!"signalBufferReturned: bogus buffer");
}

status_t GonkCameraSource::read(
        MediaBuffer **buffer, const ReadOptions *options) {
    CS_LOGV("read");

    *buffer = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        return ERROR_UNSUPPORTED;
    }

    sp<IMemory> frame;
    int64_t frameTime;

    {
        Mutex::Autolock autoLock(mLock);
        while (mStarted && mFramesReceived.empty()) {
            if (NO_ERROR !=
                mFrameAvailableCondition.waitRelative(mLock,
                    mTimeBetweenFrameCaptureUs * 1000LL + CAMERA_SOURCE_TIMEOUT_NS)) {
                
                CS_LOGW("Timed out waiting for incoming camera video frames: %lld us",
                    mLastFrameTimestampUs);
            }
        }
        if (!mStarted) {
            return OK;
        }
        frame = *mFramesReceived.begin();
        mFramesReceived.erase(mFramesReceived.begin());

        frameTime = *mFrameTimes.begin();
        mFrameTimes.erase(mFrameTimes.begin());
        mFramesBeingEncoded.push_back(frame);
        *buffer = new MediaBuffer(frame->pointer(), frame->size());
        (*buffer)->setObserver(this);
        (*buffer)->add_ref();
        (*buffer)->meta_data()->setInt64(kKeyTime, frameTime);
    }
    return OK;
}

void GonkCameraSource::dataCallbackTimestamp(int64_t timestampUs,
        int32_t msgType, const sp<IMemory> &data) {
    CS_LOGV("dataCallbackTimestamp: timestamp %lld us", timestampUs);
    Mutex::Autolock autoLock(mLock);
    if (!mStarted || (mNumFramesReceived == 0 && timestampUs < mStartTimeUs)) {
        CS_LOGV("Drop frame at %lld/%lld us", timestampUs, mStartTimeUs);
        releaseOneRecordingFrame(data);
        return;
    }

    if (mNumFramesReceived > 0) {
        CHECK(timestampUs > mLastFrameTimestampUs);
        if (timestampUs - mLastFrameTimestampUs > mGlitchDurationThresholdUs) {
            ++mNumGlitches;
        }
    }

    
    
    if (skipCurrentFrame(timestampUs)) {
        releaseOneRecordingFrame(data);
        return;
    }

    mLastFrameTimestampUs = timestampUs;
    if (mNumFramesReceived == 0) {
        mFirstFrameTimeUs = timestampUs;
        
        if (mStartTimeUs > 0) {
            if (timestampUs < mStartTimeUs) {
                
                
                releaseOneRecordingFrame(data);
                return;
            }
            mStartTimeUs = timestampUs - mStartTimeUs;
        }
    }
    ++mNumFramesReceived;

    CHECK(data != NULL && data->size() > 0);
    mFramesReceived.push_back(data);
    int64_t timeUs = mStartTimeUs + (timestampUs - mFirstFrameTimeUs);
    mFrameTimes.push_back(timeUs);
    CS_LOGV("initial delay: %lld, current time stamp: %lld",
        mStartTimeUs, timeUs);
    mFrameAvailableCondition.signal();
}

bool GonkCameraSource::isMetaDataStoredInVideoBuffers() const {
    CS_LOGV("isMetaDataStoredInVideoBuffers");
    return mIsMetaDataStoredInVideoBuffers;
}

}  
