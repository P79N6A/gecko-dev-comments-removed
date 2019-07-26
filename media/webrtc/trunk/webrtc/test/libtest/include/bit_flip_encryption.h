









#ifndef SRC_VIDEO_ENGINE_TEST_AUTO_TEST_HELPERS_BIT_FLIP_ENCRYPTION_H_
#define SRC_VIDEO_ENGINE_TEST_AUTO_TEST_HELPERS_BIT_FLIP_ENCRYPTION_H_

#include "webrtc/common_types.h"



class BitFlipEncryption : public webrtc::Encryption {
 public:
  
  
  
  
  BitFlipEncryption(unsigned int rand_seed, float flip_probability);

  virtual void encrypt(int channel_no,
                       unsigned char* in_data,
                       unsigned char* out_data,
                       int bytes_in,
                       int* bytes_out) OVERRIDE;

  virtual void decrypt(int channel_no,
                       unsigned char* in_data,
                       unsigned char* out_data,
                       int bytes_in,
                       int* bytes_out) OVERRIDE;

  virtual void encrypt_rtcp(int channel_no,
                            unsigned char* in_data,
                            unsigned char* out_data,
                            int bytes_in,
                            int* bytes_out) OVERRIDE;

  virtual void decrypt_rtcp(int channel_no,
                            unsigned char* in_data,
                            unsigned char* out_data,
                            int bytes_in,
                            int* bytes_out) OVERRIDE;

  int64_t flip_count() const { return flip_count_; }

 private:
  
  float flip_probability_;
  
  int64_t flip_count_;

  
  void FlipSomeBitsInData(const unsigned char *in_data, unsigned char* out_data,
                          int bytes_in, int* bytes_out);
};

#endif  
