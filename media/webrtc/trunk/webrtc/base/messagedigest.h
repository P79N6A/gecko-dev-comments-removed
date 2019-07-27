









#ifndef WEBRTC_BASE_MESSAGEDIGEST_H_
#define WEBRTC_BASE_MESSAGEDIGEST_H_

#include <string>

namespace rtc {


extern const char DIGEST_MD5[];
extern const char DIGEST_SHA_1[];
extern const char DIGEST_SHA_224[];
extern const char DIGEST_SHA_256[];
extern const char DIGEST_SHA_384[];
extern const char DIGEST_SHA_512[];


class MessageDigest {
 public:
  enum { kMaxSize = 64 };  
  virtual ~MessageDigest() {}
  
  virtual size_t Size() const = 0;
  
  virtual void Update(const void* buf, size_t len) = 0;
  
  
  virtual size_t Finish(void* buf, size_t len) = 0;
};


class MessageDigestFactory {
 public:
  static MessageDigest* Create(const std::string& alg);
};


bool IsFips180DigestAlgorithm(const std::string& alg);







size_t ComputeDigest(MessageDigest* digest, const void* input, size_t in_len,
                     void* output, size_t out_len);



size_t ComputeDigest(const std::string& alg, const void* input, size_t in_len,
                     void* output, size_t out_len);


std::string ComputeDigest(MessageDigest* digest, const std::string& input);



std::string ComputeDigest(const std::string& alg, const std::string& input);

bool ComputeDigest(const std::string& alg, const std::string& input,
                   std::string* output);


inline std::string MD5(const std::string& input) {
  return ComputeDigest(DIGEST_MD5, input);
}








size_t ComputeHmac(MessageDigest* digest, const void* key, size_t key_len,
                   const void* input, size_t in_len,
                   void* output, size_t out_len);



size_t ComputeHmac(const std::string& alg, const void* key, size_t key_len,
                   const void* input, size_t in_len,
                   void* output, size_t out_len);


std::string ComputeHmac(MessageDigest* digest, const std::string& key,
                        const std::string& input);



std::string ComputeHmac(const std::string& alg, const std::string& key,
                        const std::string& input);

bool ComputeHmac(const std::string& alg, const std::string& key,
                 const std::string& input, std::string* output);

}  

#endif
