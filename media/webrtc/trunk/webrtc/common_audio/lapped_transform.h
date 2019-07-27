









#ifndef WEBRTC_COMMON_AUDIO_LAPPED_TRANSFORM_H_
#define WEBRTC_COMMON_AUDIO_LAPPED_TRANSFORM_H_

#include <complex>

#include "webrtc/base/checks.h"
#include "webrtc/common_audio/blocker.h"
#include "webrtc/common_audio/real_fourier.h"
#include "webrtc/system_wrappers/interface/aligned_array.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {









class LappedTransform {
 public:
  class Callback {
   public:
    virtual ~Callback() {}

    virtual void ProcessAudioBlock(const std::complex<float>* const* in_block,
                                   int in_channels, int frames,
                                   int out_channels,
                                   std::complex<float>* const* out_block) = 0;
  };

  
  
  
  
  
  
  
  LappedTransform(int in_channels, int out_channels, int chunk_length,
                  const float* window, int block_length, int shift_amount,
                  Callback* callback);
  ~LappedTransform();

  
  
  
  
  void ProcessChunk(const float* const* in_chunk, float* const* out_chunk);

 private:
  
  
  friend class BlockThunk;
  class BlockThunk : public BlockerCallback {
   public:
    explicit BlockThunk(LappedTransform* parent) : parent_(parent) {}
    virtual ~BlockThunk() {}

    virtual void ProcessBlock(const float* const* input, int num_frames,
                              int num_input_channels, int num_output_channels,
                              float* const* output);

   private:
    LappedTransform* parent_;
  } blocker_callback_;

  int in_channels_;
  int out_channels_;

  const float* window_;
  bool own_window_;
  int window_shift_amount_;

  int block_length_;
  int chunk_length_;
  Callback* block_processor_;
  scoped_ptr<Blocker> blocker_;

  RealFourier fft_;
  int cplx_length_;
  AlignedArray<float> real_buf_;
  AlignedArray<std::complex<float> > cplx_pre_;
  AlignedArray<std::complex<float> > cplx_post_;
};

}  

#endif  

