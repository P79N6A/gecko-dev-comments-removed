





#if !defined(MediaSystemResourceTypes_h_)
#define MediaSystemResourceTypes_h_

namespace mozilla {

enum class MediaSystemResourceType : uint32_t {
  VIDEO_DECODER = 0,
  AUDIO_DECODER,  
  VIDEO_ENCODER,
  AUDIO_ENCODER,  
  CAMERA,          
  INVALID_RESOURCE,
};

} 

#endif
