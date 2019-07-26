









#ifndef SRC_VIDEO_ENGINE_TEST_AUTO_TEST_HELPERS_RANDOM_ENCRYPTION_H_
#define SRC_VIDEO_ENGINE_TEST_AUTO_TEST_HELPERS_RANDOM_ENCRYPTION_H_

#include "common_types.h"



class RandomEncryption : public webrtc::Encryption {
 public:
  explicit RandomEncryption(unsigned int rand_seed);

  virtual void encrypt(int channel_no, unsigned char* in_data,
                       unsigned char* out_data, int bytes_in, int* bytes_out) {
    GenerateRandomData(out_data, bytes_in, bytes_out);
  }

  virtual void decrypt(int channel_no, unsigned char* in_data,
                       unsigned char* out_data, int bytes_in, int* bytes_out) {
    GenerateRandomData(out_data, bytes_in, bytes_out);
  }

  virtual void encrypt_rtcp(int channel_no, unsigned char* in_data,
                            unsigned char* out_data, int bytes_in,
                            int* bytes_out) {
    GenerateRandomData(out_data, bytes_in, bytes_out);
  }

  virtual void decrypt_rtcp(int channel_no, unsigned char* in_data,
                            unsigned char* out_data, int bytes_in,
                            int* bytes_out) {
    GenerateRandomData(out_data, bytes_in, bytes_out);
  }

 private:
  
  void GenerateRandomData(unsigned char* out_data, int bytes_in,
                          int* bytes_out);

  
  
  int MakeUpSimilarLength(int original_length);
};

#endif  
