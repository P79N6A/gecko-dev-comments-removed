









#include "voice_engine/include/voe_encryption.h"
#include "voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"

class BasicBitInverseEncryption : public webrtc::Encryption {
  void encrypt(int channel_no, unsigned char* in_data,
               unsigned char* out_data, int bytes_in, int* bytes_out);
  void decrypt(int channel_no, unsigned char* in_data,
               unsigned char* out_data, int bytes_in, int* bytes_out);
  void encrypt_rtcp(int channel_no, unsigned char* in_data,
                    unsigned char* out_data, int bytes_in, int* bytes_out);
  void decrypt_rtcp(int channel_no, unsigned char* in_data,
                    unsigned char* out_data, int bytes_in, int* bytes_out);
};

void BasicBitInverseEncryption::encrypt(int, unsigned char* in_data,
                                        unsigned char* out_data,
                                        int bytes_in, int* bytes_out) {
  int i;
  for (i = 0; i < bytes_in; i++)
    out_data[i] = ~in_data[i];
  out_data[bytes_in] = 0;
  out_data[bytes_in + 1] = 0;
  *bytes_out = bytes_in + 2;
}

void BasicBitInverseEncryption::decrypt(int, unsigned char* in_data,
                                        unsigned char* out_data,
                                        int bytes_in, int* bytes_out) {
  int i;
  for (i = 0; i < bytes_in; i++)
    out_data[i] = ~in_data[i];
  *bytes_out = bytes_in - 2;
}

void BasicBitInverseEncryption::encrypt_rtcp(int, unsigned char* in_data,
                                             unsigned char* out_data,
                                             int bytes_in, int* bytes_out) {
  int i;
  for (i = 0; i < bytes_in; i++)
    out_data[i] = ~in_data[i];
  out_data[bytes_in] = 0;
  out_data[bytes_in + 1] = 0;
  *bytes_out = bytes_in + 2;
}

void BasicBitInverseEncryption::decrypt_rtcp(int, unsigned char* in_data,
                                             unsigned char* out_data,
                                             int bytes_in, int* bytes_out) {
  int i;
  for (i = 0; i < bytes_in; i++)
    out_data[i] = ~in_data[i];
  out_data[bytes_in] = 0;
  out_data[bytes_in + 1] = 0;
  *bytes_out = bytes_in + 2;
}


class EncryptionTest : public AfterStreamingFixture {
};

TEST_F(EncryptionTest, ManualBasicCorrectExternalEncryptionHasNoEffectOnVoice) {
  BasicBitInverseEncryption basic_encryption;

  voe_encrypt_->RegisterExternalEncryption(channel_, basic_encryption);

  TEST_LOG("Registered external encryption, should still hear good audio.\n");
  Sleep(3000);

  voe_encrypt_->DeRegisterExternalEncryption(channel_);
}
