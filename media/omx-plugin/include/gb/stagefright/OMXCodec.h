















#ifndef OMX_CODEC_H_

#define OMX_CODEC_H_

#if defined(MOZ_ANDROID_HC)
#include <android/native_window.h>
#endif
#include <media/IOMX.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>
#include <utils/threads.h>

namespace android {

class MemoryDealer;
struct OMXCodecObserver;
struct CodecProfileLevel;

struct OMXCodec : public MediaSource,
                  public MediaBufferObserver {
    enum CreationFlags {
        kPreferSoftwareCodecs    = 1,
        kIgnoreCodecSpecificData = 2,

        
        
        kClientNeedsFramebuffer  = 4,
    };
    static sp<MediaSource> Create(
            const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName = NULL,
            uint32_t flags = 0
#if defined(MOZ_ANDROID_HC)
            ,const sp<ANativeWindow> &nativeWindow = NULL
#endif
	    );


    static void setComponentRole(
            const sp<IOMX> &omx, IOMX::node_id node, bool isEncoder,
            const char *mime);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

    virtual status_t pause();

    
    virtual void signalBufferReturned(MediaBuffer *buffer);

protected:
    virtual ~OMXCodec();

private:

    
    friend class OMXCodecObserver;

    
    void on_message(const omx_message &msg);

    enum State {
        DEAD,
        LOADED,
        LOADED_TO_IDLE,
        IDLE_TO_EXECUTING,
        EXECUTING,
        EXECUTING_TO_IDLE,
        IDLE_TO_LOADED,
        RECONFIGURING,
        ERROR
    };

    enum {
        kPortIndexInput  = 0,
        kPortIndexOutput = 1
    };

    enum PortStatus {
        ENABLED,
        DISABLING,
        DISABLED,
        ENABLING,
        SHUTTING_DOWN,
    };

    enum Quirks {
        kNeedsFlushBeforeDisable              = 1,
        kWantsNALFragments                    = 2,
        kRequiresLoadedToIdleAfterAllocation  = 4,
        kRequiresAllocateBufferOnInputPorts   = 8,
        kRequiresFlushCompleteEmulation       = 16,
        kRequiresAllocateBufferOnOutputPorts  = 32,
        kRequiresFlushBeforeShutdown          = 64,
        kDefersOutputBufferAllocation         = 128,
        kDecoderLiesAboutNumberOfChannels     = 256,
        kInputBufferSizesAreBogus             = 512,
        kSupportsMultipleFramesPerInputBuffer = 1024,
        kAvoidMemcopyInputRecordingFrames     = 2048,
        kRequiresLargerEncoderOutputBuffer    = 4096,
        kOutputBuffersAreUnreadable           = 8192,
        kStoreMetaDataInInputVideoBuffers     = 16384,
    };

    struct BufferInfo {
        IOMX::buffer_id mBuffer;
        bool mOwnedByComponent;
        sp<IMemory> mMem;
        size_t mSize;
        void *mData;
        MediaBuffer *mMediaBuffer;
    };

    struct CodecSpecificData {
        size_t mSize;
        uint8_t mData[1];
    };

    sp<IOMX> mOMX;
    bool mOMXLivesLocally;
    IOMX::node_id mNode;
    uint32_t mQuirks;
    bool mIsEncoder;
    char *mMIME;
    char *mComponentName;
    sp<MetaData> mOutputFormat;
    sp<MediaSource> mSource;
    Vector<CodecSpecificData *> mCodecSpecificData;
    size_t mCodecSpecificDataIndex;

    sp<MemoryDealer> mDealer[2];

    State mState;
    Vector<BufferInfo> mPortBuffers[2];
    PortStatus mPortStatus[2];
    bool mInitialBufferSubmit;
    bool mSignalledEOS;
    status_t mFinalStatus;
    bool mNoMoreOutputData;
    bool mOutputPortSettingsHaveChanged;
    int64_t mSeekTimeUs;
    ReadOptions::SeekMode mSeekMode;
    int64_t mTargetTimeUs;
    int64_t mSkipTimeUs;

    MediaBuffer *mLeftOverBuffer;

    Mutex mLock;
    Condition mAsyncCompletion;

    bool mPaused;

    
    List<size_t> mFilledBuffers;
    Condition mBufferFilled;

    OMXCodec(const sp<IOMX> &omx, IOMX::node_id node, uint32_t quirks,
             bool isEncoder, const char *mime, const char *componentName,
             const sp<MediaSource> &source);

    void addCodecSpecificData(const void *data, size_t size);
    void clearCodecSpecificData();

    void setComponentRole();

    void setAMRFormat(bool isWAMR, int32_t bitRate);
    void setAACFormat(int32_t numChannels, int32_t sampleRate, int32_t bitRate);

    status_t setVideoPortFormatType(
            OMX_U32 portIndex,
            OMX_VIDEO_CODINGTYPE compressionFormat,
            OMX_COLOR_FORMATTYPE colorFormat);

    void setVideoInputFormat(
            const char *mime, const sp<MetaData>& meta);

    status_t setupBitRate(int32_t bitRate);
    status_t setupErrorCorrectionParameters();
    status_t setupH263EncoderParameters(const sp<MetaData>& meta);
    status_t setupMPEG4EncoderParameters(const sp<MetaData>& meta);
    status_t setupAVCEncoderParameters(const sp<MetaData>& meta);
    status_t findTargetColorFormat(
            const sp<MetaData>& meta, OMX_COLOR_FORMATTYPE *colorFormat);

    status_t isColorFormatSupported(
            OMX_COLOR_FORMATTYPE colorFormat, int portIndex);

    
    
    status_t getVideoProfileLevel(const sp<MetaData>& meta,
            const CodecProfileLevel& defaultProfileLevel,
            CodecProfileLevel& profileLevel);

    status_t setVideoOutputFormat(
            const char *mime, OMX_U32 width, OMX_U32 height);

    void setImageOutputFormat(
            OMX_COLOR_FORMATTYPE format, OMX_U32 width, OMX_U32 height);

    void setJPEGInputFormat(
            OMX_U32 width, OMX_U32 height, OMX_U32 compressedSize);

    void setMinBufferSize(OMX_U32 portIndex, OMX_U32 size);

    void setRawAudioFormat(
            OMX_U32 portIndex, int32_t sampleRate, int32_t numChannels);

    status_t allocateBuffers();
    status_t allocateBuffersOnPort(OMX_U32 portIndex);

    status_t freeBuffersOnPort(
            OMX_U32 portIndex, bool onlyThoseWeOwn = false);

    void drainInputBuffer(IOMX::buffer_id buffer);
    void fillOutputBuffer(IOMX::buffer_id buffer);
    void drainInputBuffer(BufferInfo *info);
    void fillOutputBuffer(BufferInfo *info);

    void drainInputBuffers();
    void fillOutputBuffers();

    
    
    
    
    
    bool flushPortAsync(OMX_U32 portIndex);

    void disablePortAsync(OMX_U32 portIndex);
    void enablePortAsync(OMX_U32 portIndex);

    static size_t countBuffersWeOwn(const Vector<BufferInfo> &buffers);
    static bool isIntermediateState(State state);

    void onEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);
    void onCmdComplete(OMX_COMMANDTYPE cmd, OMX_U32 data);
    void onStateChange(OMX_STATETYPE newState);
    void onPortSettingsChanged(OMX_U32 portIndex);

    void setState(State newState);

    status_t init();
    void initOutputFormat(const sp<MetaData> &inputFormat);

    void dumpPortStatus(OMX_U32 portIndex);

    status_t configureCodec(const sp<MetaData> &meta, uint32_t flags);

    static uint32_t getComponentQuirks(
            const char *componentName, bool isEncoder);

    static void findMatchingCodecs(
            const char *mime,
            bool createEncoder, const char *matchComponentName,
            uint32_t flags,
            Vector<String8> *matchingCodecs);

    OMXCodec(const OMXCodec &);
    OMXCodec &operator=(const OMXCodec &);
};

struct CodecProfileLevel {
    OMX_U32 mProfile;
    OMX_U32 mLevel;
};

struct CodecCapabilities {
    String8 mComponentName;
    Vector<CodecProfileLevel> mProfileLevels;
};












status_t QueryCodecs(
        const sp<IOMX> &omx,
        const char *mimeType, bool queryDecoders,
        Vector<CodecCapabilities> *results);

}  

#endif  
