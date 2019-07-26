









#ifndef WEBRTC_VIDEO_ENGINE_VIE_ENCRYPTION_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_ENCRYPTION_IMPL_H_

#include "typedefs.h"  
#include "video_engine/include/vie_encryption.h"
#include "video_engine/vie_ref_count.h"

namespace webrtc {

class ViESharedData;

class ViEEncryptionImpl
    : public ViEEncryption,
      public ViERefCount {
 public:
  virtual int Release();

  
  virtual int RegisterExternalEncryption(const int video_channel,
                                         Encryption& encryption);
  virtual int DeregisterExternalEncryption(const int video_channel);

 protected:
  explicit ViEEncryptionImpl(ViESharedData* shared_data);
  virtual ~ViEEncryptionImpl();

 private:
  ViESharedData* shared_data_;
};

}  

#endif  
