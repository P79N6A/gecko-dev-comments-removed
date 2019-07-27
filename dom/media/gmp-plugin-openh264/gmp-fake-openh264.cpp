


































#include <stdint.h>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <memory>
#include <assert.h>
#include <limits.h>

#include "gmp-platform.h"
#include "gmp-video-host.h"
#include "gmp-video-encode.h"
#include "gmp-video-decode.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"

#if defined(GMP_FAKE_SUPPORT_DECRYPT)
#include "gmp-decryption.h"
#include "gmp-test-decryptor.h"
#include "gmp-test-storage.h"
#endif

#if defined(_MSC_VER)
#define PUBLIC_FUNC __declspec(dllexport)
#else
#define PUBLIC_FUNC
#endif

#define BIG_FRAME 10000

static int g_log_level = 0;

#define GMPLOG(l, x) do { \
        if (l <= g_log_level) { \
        const char *log_string = "unknown"; \
        if ((l >= 0) && (l <= 3)) {               \
        log_string = kLogStrings[l];            \
        } \
        std::cerr << log_string << ": " << x << std::endl; \
        } \
    } while(0)

#define GL_CRIT 0
#define GL_ERROR 1
#define GL_INFO  2
#define GL_DEBUG 3

const char* kLogStrings[] = {
  "Critical",
  "Error",
  "Info",
  "Debug"
};


GMPPlatformAPI* g_platform_api = NULL;

class FakeVideoEncoder;
class FakeVideoDecoder;

struct EncodedFrame {
  uint32_t length_;
  uint8_t h264_compat_;
  uint32_t magic_;
  uint32_t width_;
  uint32_t height_;
  uint8_t y_;
  uint8_t u_;
  uint8_t v_;
  uint32_t timestamp_;
};

#define ENCODED_FRAME_MAGIC 0x4652414d

class FakeEncoderTask : public GMPTask {
 public:
  FakeEncoderTask(FakeVideoEncoder* encoder,
                  GMPVideoi420Frame* frame,
                  GMPVideoFrameType type)
      : encoder_(encoder), frame_(frame), type_(type) {}

  virtual void Run();
  virtual void Destroy() { delete this; }

  FakeVideoEncoder* encoder_;
  GMPVideoi420Frame* frame_;
  GMPVideoFrameType type_;
};

class FakeVideoEncoder : public GMPVideoEncoder {
 public:
  explicit FakeVideoEncoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    callback_ (NULL) {}

  virtual void InitEncode (const GMPVideoCodec& codecSettings,
                             const uint8_t* aCodecSpecific,
                             uint32_t aCodecSpecificSize,
                             GMPVideoEncoderCallback* callback,
                             int32_t numberOfCores,
                             uint32_t maxPayloadSize) {
    callback_ = callback;

    GMPLOG (GL_INFO, "Initialized encoder");
  }

  virtual void Encode (GMPVideoi420Frame* inputImage,
                         const uint8_t* aCodecSpecificInfo,
                         uint32_t aCodecSpecificInfoLength,
                         const GMPVideoFrameType* aFrameTypes,
                         uint32_t aFrameTypesLength) {
    GMPLOG (GL_DEBUG,
            __FUNCTION__
            << " size="
            << inputImage->Width() << "x" << inputImage->Height());

    assert (aFrameTypesLength != 0);

    g_platform_api->runonmainthread(new FakeEncoderTask(this,
                                                        inputImage,
                                                        aFrameTypes[0]));
  }

  void Encode_m (GMPVideoi420Frame* inputImage,
                 GMPVideoFrameType frame_type) {
    if (frame_type  == kGMPKeyFrame) {
      if (!inputImage)
        return;
    }
    if (!inputImage) {
      GMPLOG (GL_ERROR, "no input image");
      return;
    }

    
    GMPVideoFrame* ftmp;
    GMPErr err = host_->CreateFrame(kGMPEncodedVideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error creating encoded frame");
      return;
    }

    GMPVideoEncodedFrame* f = static_cast<GMPVideoEncodedFrame*> (ftmp);

    
    
    
    EncodedFrame eframe;
    eframe.length_ = sizeof(eframe) - sizeof(uint32_t);
    eframe.h264_compat_ = 5; 
    eframe.magic_ = ENCODED_FRAME_MAGIC;
    eframe.width_ = inputImage->Width();
    eframe.height_ = inputImage->Height();
    eframe.y_ = AveragePlane(inputImage->Buffer(kGMPYPlane),
                             inputImage->AllocatedSize(kGMPYPlane));
    eframe.u_ = AveragePlane(inputImage->Buffer(kGMPUPlane),
                             inputImage->AllocatedSize(kGMPUPlane));
    eframe.v_ = AveragePlane(inputImage->Buffer(kGMPVPlane),
                             inputImage->AllocatedSize(kGMPVPlane));

    eframe.timestamp_ = inputImage->Timestamp();

    err = f->CreateEmptyFrame (sizeof(eframe) +
                               (frame_type  == kGMPKeyFrame ? sizeof(uint32_t) + BIG_FRAME : 0));
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error allocating frame data");
      f->Destroy();
      return;
    }
    memcpy(f->Buffer(), &eframe, sizeof(eframe));
    if (frame_type  == kGMPKeyFrame) {
      *((uint32_t*) f->Buffer() + sizeof(eframe)) = BIG_FRAME;
    }

    f->SetEncodedWidth (inputImage->Width());
    f->SetEncodedHeight (inputImage->Height());
    f->SetTimeStamp (inputImage->Timestamp());
    f->SetFrameType (frame_type);
    f->SetCompleteFrame (true);
    f->SetBufferType(GMP_BufferLength32);

    GMPLOG (GL_DEBUG, "Encoding complete. type= "
            << f->FrameType()
            << " length="
            << f->Size()
            << " timestamp="
            << f->TimeStamp());

    
    GMPCodecSpecificInfo info;
    memset (&info, 0, sizeof (info));
    info.mCodecType = kGMPVideoCodecH264;
    info.mBufferType = GMP_BufferLength32;
    info.mCodecSpecific.mH264.mSimulcastIdx = 0;
    GMPLOG (GL_DEBUG, "Calling callback");
    callback_->Encoded (f, reinterpret_cast<uint8_t*> (&info), sizeof(info));
    GMPLOG (GL_DEBUG, "Callback called");
  }

  virtual void SetChannelParameters (uint32_t aPacketLoss, uint32_t aRTT) {
  }

  virtual void SetRates (uint32_t aNewBitRate, uint32_t aFrameRate) {
  }

  virtual void SetPeriodicKeyFrames (bool aEnable) {
  }

  virtual void EncodingComplete() {
    delete this;
  }

 private:
  uint8_t AveragePlane(uint8_t* ptr, size_t len) {
    uint64_t val = 0;

    for (size_t i=0; i<len; ++i) {
      val += ptr[i];
    }

    return (val / len) % 0xff;
  }

  GMPVideoHost* host_;
  GMPVideoEncoderCallback* callback_;
};

void FakeEncoderTask::Run() {
  encoder_->Encode_m(frame_, type_);
  frame_->Destroy();
}

class FakeDecoderTask : public GMPTask {
 public:
  FakeDecoderTask(FakeVideoDecoder* decoder,
                  GMPVideoEncodedFrame* frame,
                  int64_t time)
      : decoder_(decoder), frame_(frame), time_(time) {}

  virtual void Run();
  virtual void Destroy() { delete this; }

  FakeVideoDecoder* decoder_;
  GMPVideoEncodedFrame* frame_;
  int64_t time_;
};

class FakeVideoDecoder : public GMPVideoDecoder {
 public:
  explicit FakeVideoDecoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    callback_ (NULL) {}

  virtual ~FakeVideoDecoder() {
  }

  virtual void InitDecode (const GMPVideoCodec& codecSettings,
                             const uint8_t* aCodecSpecific,
                             uint32_t aCodecSpecificSize,
                             GMPVideoDecoderCallback* callback,
                             int32_t coreCount) {
    GMPLOG (GL_INFO, "InitDecode");

    callback_ = callback;
  }

  virtual void Decode (GMPVideoEncodedFrame* inputFrame,
                         bool missingFrames,
                         const uint8_t* aCodecSpecificInfo,
                         uint32_t aCodecSpecificInfoLength,
                         int64_t renderTimeMs = -1) {
    GMPLOG (GL_DEBUG, __FUNCTION__
            << "Decoding frame size=" << inputFrame->Size()
            << " timestamp=" << inputFrame->TimeStamp());
    g_platform_api->runonmainthread(new FakeDecoderTask(this, inputFrame, renderTimeMs));
  }

  virtual void Reset() {
  }

  virtual void Drain() {
  }

  virtual void DecodingComplete() {
    delete this;
  }

  
  void Decode_m (GMPVideoEncodedFrame* inputFrame,
                 int64_t renderTimeMs) {
    EncodedFrame *eframe;
    if (inputFrame->Size() != (sizeof(*eframe))) {
      GMPLOG (GL_ERROR, "Couldn't decode frame. Size=" << inputFrame->Size());
      return;
    }
    eframe = reinterpret_cast<EncodedFrame*>(inputFrame->Buffer());

    if (eframe->magic_ != ENCODED_FRAME_MAGIC) {
      GMPLOG (GL_ERROR, "Couldn't decode frame. Magic=" << eframe->magic_);
      return;
    }

    int width = eframe->width_;
    int height = eframe->height_;
    int ystride = eframe->width_;
    int uvstride = eframe->width_/2;

    GMPLOG (GL_DEBUG, "Video frame ready for display "
            << width
            << "x"
            << height
            << " timestamp="
            << inputFrame->TimeStamp());

    GMPVideoFrame* ftmp = NULL;

    
    GMPErr err = host_->CreateFrame (kGMPI420VideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't allocate empty I420 frame");
      return;
    }

    GMPVideoi420Frame* frame = static_cast<GMPVideoi420Frame*> (ftmp);
    err = frame->CreateEmptyFrame (
        width, height,
        ystride, uvstride, uvstride);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't make decoded frame");
      return;
    }

    memset(frame->Buffer(kGMPYPlane),
           eframe->y_,
           frame->AllocatedSize(kGMPYPlane));
    memset(frame->Buffer(kGMPUPlane),
           eframe->u_,
           frame->AllocatedSize(kGMPUPlane));
    memset(frame->Buffer(kGMPVPlane),
           eframe->v_,
           frame->AllocatedSize(kGMPVPlane));

    GMPLOG (GL_DEBUG, "Allocated size = "
            << frame->AllocatedSize (kGMPYPlane));
    frame->SetTimestamp (inputFrame->TimeStamp());
    frame->SetDuration (inputFrame->Duration());
    callback_->Decoded (frame);

  }

  GMPVideoHost* host_;
  GMPVideoDecoderCallback* callback_;
};

void FakeDecoderTask::Run() {
  decoder_->Decode_m(frame_, time_);
  frame_->Destroy();
}

extern "C" {

  PUBLIC_FUNC GMPErr
  GMPInit (GMPPlatformAPI* aPlatformAPI) {
    g_platform_api = aPlatformAPI;
    return GMPNoErr;
  }

  PUBLIC_FUNC GMPErr
  GMPGetAPI (const char* aApiName, void* aHostAPI, void** aPluginApi) {
    if (!strcmp (aApiName, GMP_API_VIDEO_DECODER)) {
      *aPluginApi = new FakeVideoDecoder (static_cast<GMPVideoHost*> (aHostAPI));
      return GMPNoErr;
    } else if (!strcmp (aApiName, GMP_API_VIDEO_ENCODER)) {
      *aPluginApi = new FakeVideoEncoder (static_cast<GMPVideoHost*> (aHostAPI));
      return GMPNoErr;
#if defined(GMP_FAKE_SUPPORT_DECRYPT)
    } else if (!strcmp (aApiName, GMP_API_DECRYPTOR)) {
      *aPluginApi = new FakeDecryptor(static_cast<GMPDecryptorHost*> (aHostAPI));
      return GMPNoErr;
    } else if (!strcmp (aApiName, GMP_API_ASYNC_SHUTDOWN)) {
      *aPluginApi = new TestAsyncShutdown(static_cast<GMPAsyncShutdownHost*> (aHostAPI));
      return GMPNoErr;
#endif
    }
    return GMPGenericErr;
  }

  PUBLIC_FUNC void
  GMPShutdown (void) {
    g_platform_api = NULL;
  }

} 
