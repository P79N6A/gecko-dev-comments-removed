









#include "high_pass_filter_impl.h"

#include <cassert>

#include "critical_section_wrapper.h"
#include "typedefs.h"
#include "signal_processing_library.h"

#include "audio_processing_impl.h"
#include "audio_buffer.h"

namespace webrtc {
namespace {
const WebRtc_Word16 kFilterCoefficients8kHz[5] =
    {3798, -7596, 3798, 7807, -3733};

const WebRtc_Word16 kFilterCoefficients[5] =
    {4012, -8024, 4012, 8002, -3913};

struct FilterState {
  WebRtc_Word16 y[4];
  WebRtc_Word16 x[2];
  const WebRtc_Word16* ba;
};

int InitializeFilter(FilterState* hpf, int sample_rate_hz) {
  assert(hpf != NULL);

  if (sample_rate_hz == AudioProcessingImpl::kSampleRate8kHz) {
    hpf->ba = kFilterCoefficients8kHz;
  } else {
    hpf->ba = kFilterCoefficients;
  }

  WebRtcSpl_MemSetW16(hpf->x, 0, 2);
  WebRtcSpl_MemSetW16(hpf->y, 0, 4);

  return AudioProcessing::kNoError;
}

int Filter(FilterState* hpf, WebRtc_Word16* data, int length) {
  assert(hpf != NULL);

  WebRtc_Word32 tmp_int32 = 0;
  WebRtc_Word16* y = hpf->y;
  WebRtc_Word16* x = hpf->x;
  const WebRtc_Word16* ba = hpf->ba;

  for (int i = 0; i < length; i++) {
    
    

    tmp_int32 =
        WEBRTC_SPL_MUL_16_16(y[1], ba[3]); 
    tmp_int32 +=
        WEBRTC_SPL_MUL_16_16(y[3], ba[4]); 
    tmp_int32 = (tmp_int32 >> 15);
    tmp_int32 +=
        WEBRTC_SPL_MUL_16_16(y[0], ba[3]); 
    tmp_int32 +=
        WEBRTC_SPL_MUL_16_16(y[2], ba[4]); 
    tmp_int32 = (tmp_int32 << 1);

    tmp_int32 += WEBRTC_SPL_MUL_16_16(data[i], ba[0]); 
    tmp_int32 += WEBRTC_SPL_MUL_16_16(x[0], ba[1]);    
    tmp_int32 += WEBRTC_SPL_MUL_16_16(x[1], ba[2]);    

    
    x[1] = x[0];
    x[0] = data[i];

    
    y[2] = y[0];
    y[3] = y[1];
    y[0] = static_cast<WebRtc_Word16>(tmp_int32 >> 13);
    y[1] = static_cast<WebRtc_Word16>((tmp_int32 -
        WEBRTC_SPL_LSHIFT_W32(static_cast<WebRtc_Word32>(y[0]), 13)) << 2);

    
    tmp_int32 += 2048;

    
    tmp_int32 = WEBRTC_SPL_SAT(static_cast<WebRtc_Word32>(134217727),
                               tmp_int32,
                               static_cast<WebRtc_Word32>(-134217728));

    
    data[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp_int32, 12);

  }

  return AudioProcessing::kNoError;
}
}  

typedef FilterState Handle;

HighPassFilterImpl::HighPassFilterImpl(const AudioProcessingImpl* apm)
  : ProcessingComponent(apm),
    apm_(apm) {}

HighPassFilterImpl::~HighPassFilterImpl() {}

int HighPassFilterImpl::ProcessCaptureAudio(AudioBuffer* audio) {
  int err = apm_->kNoError;

  if (!is_component_enabled()) {
    return apm_->kNoError;
  }

  assert(audio->samples_per_split_channel() <= 160);

  for (int i = 0; i < num_handles(); i++) {
    Handle* my_handle = static_cast<Handle*>(handle(i));
    err = Filter(my_handle,
                 audio->low_pass_split_data(i),
                 audio->samples_per_split_channel());

    if (err != apm_->kNoError) {
      return GetHandleError(my_handle);
    }
  }

  return apm_->kNoError;
}

int HighPassFilterImpl::Enable(bool enable) {
  CriticalSectionScoped crit_scoped(apm_->crit());
  return EnableComponent(enable);
}

bool HighPassFilterImpl::is_enabled() const {
  return is_component_enabled();
}

void* HighPassFilterImpl::CreateHandle() const {
  return new FilterState;
}

int HighPassFilterImpl::DestroyHandle(void* handle) const {
  delete static_cast<Handle*>(handle);
  return apm_->kNoError;
}

int HighPassFilterImpl::InitializeHandle(void* handle) const {
  return InitializeFilter(static_cast<Handle*>(handle),
                          apm_->sample_rate_hz());
}

int HighPassFilterImpl::ConfigureHandle(void* ) const {
  return apm_->kNoError; 
}

int HighPassFilterImpl::num_handles_required() const {
  return apm_->num_output_channels();
}

int HighPassFilterImpl::GetHandleError(void* handle) const {
  
  assert(handle != NULL);
  return apm_->kUnspecifiedError;
}
}  
