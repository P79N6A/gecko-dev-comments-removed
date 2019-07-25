















#ifndef ANDROID_AUDIOTRACK_H
#define ANDROID_AUDIOTRACK_H

#include <stdint.h>
#include <sys/types.h>

#include "IAudioFlinger.h"
#include "IAudioTrack.h"
#include "AudioSystem.h"

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <utils/threads.h>


namespace android {



class audio_track_cblk_t;



class AudioTrack
{
public:
    enum channel_index {
        MONO   = 0,
        LEFT   = 0,
        RIGHT  = 1
    };

    

    enum event_type {
        EVENT_MORE_DATA = 0,        
        EVENT_UNDERRUN = 1,         
        EVENT_LOOP_END = 2,         
        EVENT_MARKER = 3,           
        EVENT_NEW_POS = 4,          
        EVENT_BUFFER_END = 5        
    };

    



    class Buffer
    {
    public:
        enum {
            MUTE    = 0x00000001
        };
        uint32_t    flags;
        int         channelCount;
        int         format;
        size_t      frameCount;
        size_t      size;
        union {
            void*       raw;
            short*      i16;
            int8_t*     i8;
        };
    };


    

















    typedef void (*callback_t)(int event, void* user, void *info);

    






     static status_t getMinFrameCount(int* frameCount,
                                      int streamType      =-1,
                                      uint32_t sampleRate = 0);

    


                        AudioTrack();

    






















                        AudioTrack( int streamType,
                                    uint32_t sampleRate  = 0,
                                    int format           = 0,
                                    int channels         = 0,
                                    int frameCount       = 0,
                                    uint32_t flags       = 0,
                                    callback_t cbf       = 0,
                                    void* user           = 0,
                                    int notificationFrames = 0,
                                    int sessionId = 0);

    








                        AudioTrack( int streamType,
                                    uint32_t sampleRate = 0,
                                    int format          = 0,
                                    int channels        = 0,
                                    const sp<IMemory>& sharedBuffer = 0,
                                    uint32_t flags      = 0,
                                    callback_t cbf      = 0,
                                    void* user          = 0,
                                    int notificationFrames = 0,
                                    int sessionId = 0);

    


                        ~AudioTrack();


    






            status_t    set(int streamType      =-1,
                            uint32_t sampleRate = 0,
                            int format          = 0,
                            int channels        = 0,
                            int frameCount      = 0,
                            uint32_t flags      = 0,
                            callback_t cbf      = 0,
                            void* user          = 0,
                            int notificationFrames = 0,
                            const sp<IMemory>& sharedBuffer = 0,
                            bool threadCanCallJava = false,
                            int sessionId = 0);


    




            status_t    initCheck() const;

    



            uint32_t     latency() const;

    

            int         streamType() const;
            int         format() const;
            int         channelCount() const;
            uint32_t    frameCount() const;
            int         frameSize() const;
            sp<IMemory>& sharedBuffer();


    


            void        start();

    



            void        stop();
            bool        stopped() const;

    


            void        flush();

    



            void        pause();

    


            void        mute(bool);
            bool        muted() const;


    


            status_t    setVolume(float left, float right);
            void        getVolume(float* left, float* right);

    


            status_t    setAuxEffectSendLevel(float level);
            void        getAuxEffectSendLevel(float* level);

    

            status_t    setSampleRate(int sampleRate);
            uint32_t    getSampleRate();

    











            status_t    setLoop(uint32_t loopStart, uint32_t loopEnd, int loopCount);
            status_t    getLoop(uint32_t *loopStart, uint32_t *loopEnd, int *loopCount);


    












            status_t    setMarkerPosition(uint32_t marker);
            status_t    getMarkerPosition(uint32_t *marker);


    













            status_t    setPositionUpdatePeriod(uint32_t updatePeriod);
            status_t    getPositionUpdatePeriod(uint32_t *updatePeriod);


    

















            status_t    setPosition(uint32_t position);
            status_t    getPosition(uint32_t *position);

    







            status_t    reload();

    







            audio_io_handle_t    getOutput();

    







            int    getSessionId();


    











            status_t    attachAuxEffect(int effectId);

    









        enum {
            NO_MORE_BUFFERS = 0x80000001,
            STOPPED = 1
        };

            status_t    obtainBuffer(Buffer* audioBuffer, int32_t waitCount);
            void        releaseBuffer(Buffer* audioBuffer);


    




            ssize_t     write(const void* buffer, size_t size);

    


            status_t dump(int fd, const Vector<String16>& args) const;

private:
    
                        AudioTrack(const AudioTrack& other);
            AudioTrack& operator = (const AudioTrack& other);

    
    class AudioTrackThread : public Thread
    {
    public:
        AudioTrackThread(AudioTrack& receiver, bool bCanCallJava = false);
    private:
        friend class AudioTrack;
        virtual bool        threadLoop();
        virtual status_t    readyToRun();
        virtual void        onFirstRef();
        AudioTrack& mReceiver;
        Mutex       mLock;
    };

            bool processAudioBuffer(const sp<AudioTrackThread>& thread);
            status_t createTrack(int streamType,
                                 uint32_t sampleRate,
                                 int format,
                                 int channelCount,
                                 int frameCount,
                                 uint32_t flags,
                                 const sp<IMemory>& sharedBuffer,
                                 audio_io_handle_t output,
                                 bool enforceFrameCount);

    sp<IAudioTrack>         mAudioTrack;
    sp<IMemory>             mCblkMemory;
    sp<AudioTrackThread>    mAudioTrackThread;

    float                   mVolume[2];
    float                   mSendLevel;
    uint32_t                mFrameCount;

    audio_track_cblk_t*     mCblk;
    uint8_t                 mStreamType;
    uint8_t                 mFormat;
    uint8_t                 mChannelCount;
    uint8_t                 mMuted;
    uint32_t                mChannels;
    status_t                mStatus;
    uint32_t                mLatency;

    volatile int32_t        mActive;

    callback_t              mCbf;
    void*                   mUserData;
    uint32_t                mNotificationFramesReq; 
    uint32_t                mNotificationFramesAct; 
    sp<IMemory>             mSharedBuffer;
    int                     mLoopCount;
    uint32_t                mRemainingFrames;
    uint32_t                mMarkerPosition;
    bool                    mMarkerReached;
    uint32_t                mNewPosition;
    uint32_t                mUpdatePeriod;
    uint32_t                mFlags;
    int                     mSessionId;
    int                     mAuxEffectId;
    uint32_t                mPadding[8];
};


}; 

#endif
